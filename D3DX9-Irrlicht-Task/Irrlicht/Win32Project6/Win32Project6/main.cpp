#include <irrlicht.h>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

#ifdef _IRR_WINDOWS_
#pragma comment(lib, "Irrlicht.lib")
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif



int ResX = 800;
int ResY = 600;
ICameraSceneNode *camera = 0;
ICameraSceneNode *camera2 = 0;

static bool UseHighLevelShaders = true;
static bool UseCgShaders = true;

IrrlichtDevice* device = 0;
static core::position2d<s32> pRed(10, 10);
static core::position2d<s32> pGreen(10, 160);
static core::position2d<s32> pBlue(160, 10);
static core::position2d<s32> pPink(160, 160);


class CRightSceneNode : public scene::ISceneNode {
	core::aabbox3d<f32> Box;
	video::SMaterial Material;
	video::S3DVertex GrassV[4];
public:
	CRightSceneNode(scene::ISceneNode* parent, scene::ISceneManager* mgr, s32 id)
		: scene::ISceneNode(parent, mgr, id) {
			Material.Wireframe = false;
			Material.Lighting = false;
			GrassV[0] = video::S3DVertex(-12, 35, 0, 0, 0, 1,
				video::SColor(255, 255, 255, 255), 1, 0);
			GrassV[1] = video::S3DVertex(-12, 55, 0, 0, 0, 1,
				video::SColor(255, 255, 255, 255), 1, 1);
			GrassV[2] = video::S3DVertex(16, 35, 0, 0, 0, 1,
				video::SColor(255, 255, 255, 255), 0, 1);
			GrassV[3] = video::S3DVertex(16, 55, 0, 0, 0, 1,
				video::SColor(255, 255, 255, 255), 0, 0);
			Box.reset(GrassV[0].Pos);
			for (s32 i = 1; i<4; ++i)
				Box.addInternalPoint(GrassV[i].Pos);
		}

	virtual void OnRegisterSceneNode()
	{
		if (IsVisible)
			SceneManager->registerNodeForRendering(this);
		ISceneNode::OnRegisterSceneNode();
	}

	/*
	In the render() method most of the interesting stuff happens: The
	Scene node renders itself. We override this method and draw the
	tetraeder.
	*/
	virtual void render()
	{
		u16 indices[] = { 0, 2, 3, 0, 1, 3, 0, 3, 2, 3, 1, 0 };
		video::IVideoDriver* driver = SceneManager->getVideoDriver();

		driver->setMaterial(Material);
		driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
		driver->drawVertexPrimitiveList(&GrassV[0], 4, &indices[0], 4, video::EVT_STANDARD, scene::EPT_TRIANGLES, video::EIT_16BIT);
	}

	virtual const core::aabbox3d<f32>& getBoundingBox() const
	{
		return Box;
	}

	virtual u32 getMaterialCount() const
	{
		return 1;
	}

	virtual video::SMaterial& getMaterial(u32 i)
	{
		return Material;
	}
};

class MyShaderCallBack : public video::IShaderConstantSetCallBack
{
public:

	virtual void OnSetConstants(video::IMaterialRendererServices* services,
		s32 userData)
	{
		video::IVideoDriver* driver = services->getVideoDriver();

		// set inverted world matrix
		// if we are using highlevel shaders (the user can select this when
		// starting the program), we must set the constants by name.

		core::matrix4 invWorld = driver->getTransform(video::ETS_WORLD);
		invWorld.makeInverse();

		if (UseHighLevelShaders)
			services->setVertexShaderConstant("mInvWorld", invWorld.pointer(), 16);
		else
			services->setVertexShaderConstant(invWorld.pointer(), 0, 4);

		// set clip matrix

		core::matrix4 worldViewProj;
		worldViewProj = driver->getTransform(video::ETS_PROJECTION);
		worldViewProj *= driver->getTransform(video::ETS_VIEW);
		worldViewProj *= driver->getTransform(video::ETS_WORLD);

		if (UseHighLevelShaders)
			services->setVertexShaderConstant("mWorldViewProj", worldViewProj.pointer(), 16);
		else
			services->setVertexShaderConstant(worldViewProj.pointer(), 4, 4);

		// set camera position

		core::vector3df pos = device->getSceneManager()->
			getActiveCamera()->getAbsolutePosition();

		if (UseHighLevelShaders)
			services->setVertexShaderConstant("mLightPos", reinterpret_cast<f32*>(&pos), 3);
		else
			services->setVertexShaderConstant(reinterpret_cast<f32*>(&pos), 8, 1);

		// set light color

		video::SColorf col(0.0f, 1.0f, 1.0f, 0.0f);

		if (UseHighLevelShaders)
			services->setVertexShaderConstant("mLightColor",
			reinterpret_cast<f32*>(&col), 4);
		else
			services->setVertexShaderConstant(reinterpret_cast<f32*>(&col), 9, 1);

		// set transposed world matrix

		core::matrix4 world = driver->getTransform(video::ETS_WORLD);
		world = world.getTransposed();

		if (UseHighLevelShaders)
		{
			services->setVertexShaderConstant("mTransWorld", world.pointer(), 16);

			// set texture, for textures you can use both an int and a float setPixelShaderConstant interfaces (You need it only for an OpenGL driver).
			s32 TextureLayerID = 0;
			if (UseHighLevelShaders)
				services->setPixelShaderConstant("myTexture", &TextureLayerID, 1);
		}
		else
			services->setVertexShaderConstant(world.pointer(), 10, 4);
	}
};

int main()
{
	device =
		createDevice(video::EDT_SOFTWARE, dimension2d<u32>(800, 600), 16, false, false, false, 0);
	/* 16 means Amount of color bits per pixel. This should be 16 or 32. The
	parameter is often ignored when running in windowed mode. */

	if (!device)	return 1;
	// windows name
	device->setWindowCaption(L"Irricht Task");

	// left windows
	// get a pointer to the VideoDriver, the SceneManager and the graphical user interface environment
	IVideoDriver* driver = device->getVideoDriver();
	ISceneManager* smgr = device->getSceneManager();
	IGUIEnvironment* gui = device->getGUIEnvironment();
	
	if (UseCgShaders && !driver->queryFeature(video::EVDF_PIXEL_SHADER_3_0))
	{
		printf("Warning: No Cg support, disabling.\n");
		UseCgShaders = false;
	}
	io::path vsFileName;
	io::path psFileName;
	if (UseHighLevelShaders) {
		psFileName = "d3d9.hlsl";
		vsFileName = psFileName;
	}
	else {
		psFileName = "d3d9.hlsl";
		vsFileName = "d3d9.vsh";
	}

	// check if the hardware and selected renderer is able to execute the shaders
	if (!driver->queryFeature(video::EVDF_PIXEL_SHADER_1_1) &&
		!driver->queryFeature(video::EVDF_ARB_FRAGMENT_PROGRAM_1))
	{
		device->getLogger()->log("WARNING: Pixel shaders disabled "\
			"because of missing driver/hardware support.");
		psFileName = "";
	}

	if (!driver->queryFeature(video::EVDF_VERTEX_SHADER_1_1) &&
		!driver->queryFeature(video::EVDF_ARB_VERTEX_PROGRAM_1))
	{
		device->getLogger()->log("WARNING: Vertex shaders disabled "\
			"because of missing driver/hardware support.");
		vsFileName = "";
	}

	video::IGPUProgrammingServices* gpu = driver->getGPUProgrammingServices();
	s32 newMaterial1 = 0;
	if (gpu) {
		MyShaderCallBack* mc = new MyShaderCallBack();
		if (UseHighLevelShaders)
		{
			// Choose the desired shader type. Default is the native
			// shader type for the driver, for Cg pass the special
			// enum value EGSL_CG
			const video::E_GPU_SHADING_LANGUAGE shadingLanguage =
				UseCgShaders ? video::EGSL_CG : video::EGSL_DEFAULT;

			// create material from high level shaders (hlsl, glsl or cg)

			newMaterial1 = gpu->addHighLevelShaderMaterialFromFiles(
				vsFileName, "vertexMain", video::EVST_VS_1_1,
				psFileName, "pixelMain", video::EPST_PS_1_1,
				mc, video::EMT_SOLID, 0, shadingLanguage);
		}
		mc->drop();
	}

	camera = smgr->addCameraSceneNode(0, core::vector3df(0, 0, 20.0f), core::vector3df(0, 0, 0));
	camera2 = smgr->addCameraSceneNode(0, core::vector3df(2.0f, 45, 22), core::vector3df(2, 45.0f, 0));
	
	CRightSceneNode *rightNode = new CRightSceneNode(smgr->getRootSceneNode(), smgr, 666);

	core::position2d<s32> p1(10, 10);
	core::position2d<s32> p2(10, 160);
	core::position2d<s32> p3(160, 10);
	core::position2d<s32> p4(160, 160);


	//right windows
	// add texture
	video::ITexture* tex = driver->getTexture("grass.PNG");
	rightNode->setMaterialTexture(0, tex);
	video::ITexture* red = driver->getTexture("red.bmp");
	video::ITexture* blue = driver->getTexture("blue.bmp");
	video::ITexture* green = driver->getTexture("green.bmp");
	video::ITexture* pink = driver->getTexture("pink.bmp");
	video::ITexture* tex2 = 0;

	// add a cylinder
	const IGeometryCreator* igCreator = smgr->getGeometryCreator();
	IMesh* cylinder = igCreator->createCylinderMesh(1.0f, 5.0f, 30, video::SColor(255, 255, 255, 255), false);
	IMeshSceneNode* cylinderNode = smgr->addMeshSceneNode(cylinder);
	cylinderNode->setPosition(core::vector3df(2, 45, 0));
	// shader
	cylinderNode->setMaterialFlag(video::EMF_LIGHTING, true);
	cylinderNode->setMaterialFlag(video::EMF_BLEND_OPERATION, true);
	cylinderNode->setMaterialType((video::E_MATERIAL_TYPE)newMaterial1);
	cylinderNode->setRotation(irr::core::vector3df(-90,0,0));

	// add a sphere
	scene::ISceneNode * sphereNode = smgr->addSphereSceneNode(0.2);
	if (sphereNode) {
		sphereNode->setPosition(core::vector3df(0, 45, 10));
		sphereNode->setMaterialFlag(video::EMF_LIGHTING, false);
		// 自转
		sphereNode->setRotation(core::vector3df(0, 45, 5));
		// 绕柱体转
		scene::ISceneNodeAnimator* anim2 =
			smgr->createFlyCircleAnimator(core::vector3df(0, 45, 5), 5.0f, 0.001f, core::vector3df(0, 0, 1));
		if (anim2)
		{
			sphereNode->addAnimator(anim2);
			anim2->drop();
		}
	}
	
	// add a light with shader
	// create materials


	//cylinderNode->setMaterialFlag(EMF_LIGHTING, true); // 接受光照
	//cylinderNode->setMaterialFlag(EMF_ANISOTROPIC_FILTER, true); // 使用各向异性过滤
	//cylinderNode->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL_REF);// 材质开启Alpha通道
	//// 设置全局光照
	//smgr->setAmbientLight(SColor(255,255, 255, 255));
	//ILightSceneNode* lightNode = smgr->addLightSceneNode(cylinderNode);
	//SLight blueFireLight;
	//blueFireLight.DiffuseColor = SColor(255, 80, 80, 225);
	//blueFireLight.SpecularColor = SColor(255, 230, 230, 210);
	//// 设置光源属性
	//lightNode->setLightData(blueFireLight);
	//lightNode->setPosition(vector3df(0, 45.0f, 100.0f));

	u32 frames = 0;
	while (device->run() && driver) {
		if (device->isWindowActive()) {
			u32 time = device->getTimer()->getTime();
			driver->beginScene(true, true, video::SColor(0, 100, 100, 100));
			// LEFT PART
			smgr->setActiveCamera(camera);
			driver->setViewPort(rect<s32>(0, 0, ResX / 2, ResY));
			driver->draw2DImage(red, pRed,
				core::rect<s32>(0, 0, 150, 150), 0,
				video::SColor(255, 255, 255, 255), true);
			driver->draw2DImage(green, pGreen,
				core::rect<s32>(0, 0, 150, 150), 0,
				video::SColor(255, 255, 255, 255), true);
			driver->draw2DImage(blue, pBlue,
				core::rect<s32>(0, 0, 150, 150), 0,
				video::SColor(255, 255, 255, 255), true);
			driver->draw2DImage(pink, pPink,
				core::rect<s32>(0, 0, 150, 150), 0,
				video::SColor(255, 255, 255, 255), true);
			if (time / 500 % 4 == 0) {
				pRed = p1;		pGreen = p3;	pBlue = p4;		pPink = p2;
			}
			else if (time / 500 % 4 == 1) {
				pRed = p3;		pGreen = p4;	pBlue = p2;		pPink = p1;
			}
			else if (time / 500 % 4 == 2) {
				pRed = p4;		pGreen = p2;	pBlue = p1;		pPink = p3;
			}
			else {
				pRed = p2;		pGreen = p1;	pBlue = p3;		pPink = p4;
			}
			smgr->drawAll();

			//RIGHT PART
			smgr->setActiveCamera(camera2);
			driver->setViewPort(rect<s32>(ResX / 2, 0, ResX, ResY));
			/*driver->draw2DImage(tex, core::position2d<s32>(500, 150),
				core::rect<s32>(0, 0, 700, 500), 0,
				video::SColor(255, 255, 255, 255), true);*/
			smgr->drawAll();

			driver->endScene();
		}
	}

	device->drop();
	return 0;

}