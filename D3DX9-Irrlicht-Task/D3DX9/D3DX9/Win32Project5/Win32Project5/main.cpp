

#include <windows.h>
#include "d3dUtility.h"
#include <d3dx9.h>
// Globals

IDirect3DDevice9*     Device = 0;

const int Width = 500;
const int Height = 500;

IDirect3DVertexBuffer9* Quad = 0;//存储四边形顶点缓存，
IDirect3DTexture9*      Tex = 0;//映射到四边形的纹理

ID3DXMesh* Objects;//cylinder
ID3DXMesh* Objects2;//sphere
D3DXMATRIX ObjWorldMatrices;
D3DXMATRIX ObjWorldMatrices2;

// spot light 
D3DLIGHT9 Spot;

static D3DMATERIAL9 Mtrls = d3d::WHITE_MTRL;
static D3DMATERIAL9 MtrlsCylinder = d3d::WHITE_MTRL;

static IDirect3DBaseTexture9* tempTexture = NULL;
D3DXMATRIX Worlds;

static float x = 1.0f;
static float y = 2.0f;
static float z = -10.0f;
static float angle = (3.0f * D3DX_PI) / 2.0f;
static int count = 0;
static float time = 0.0f;
//本程序使用的是D3DXCreateTextureFromFile创建的多级渐进纹理

//  Structures：构造物体的顶点，顶点法线坐标，并为其制定纹理坐标
struct Vertex
{
	Vertex(){}
	Vertex(
		float x, float y, float z,
		float nx, float ny, float nz,
		float u, float v)
	{
		_x = x;  _y = y;  _z = z;
		_nx = nx; _ny = ny; _nz = nz;
		_u = u;  _v = v;
	}
	float _x, _y, _z;
	float _nx, _ny, _nz;
	float _u, _v; // texture coordinates

	static const DWORD FVF; //not sure what's this?
};
//增加了标记D3DFVF_TEX1：标识顶点结构中包含了一对纹理坐标
const DWORD Vertex::FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;

// Framework Functions
bool Setup()
{
	// Create the quad vertex buffer and fill it with the
	// quad geoemtry.

	Device->CreateVertexBuffer(
		6 * sizeof(Vertex),
		D3DUSAGE_WRITEONLY,
		Vertex::FVF,
		D3DPOOL_MANAGED,
		&Quad,
		0);

	Vertex* v;
	Quad->Lock(0, 0, (void**)&v, 0);

	// quad built from two triangles, note texture coordinates:
	v[0] = Vertex(-25.0f, -25.0f, 1.25f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[1] = Vertex(-25.0f, 25.0f, 1.25f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[2] = Vertex(25.0f, 25.0f, 1.25f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);

	v[3] = Vertex(-25.0f, -25.0f, 1.25f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[4] = Vertex(25.0f, 25.0f, 1.25f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[5] = Vertex(25.0f, -25.0f, 1.25f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	Quad->Unlock();

	// Create the texture and set filters.
	D3DXCreateTextureFromFile(
		Device,
		"grass.png",
		&Tex);
	D3DXMatrixTranslation(&Worlds, 0.0f, 0.0f, 5.0f);
	//set filters 为了让纹理适应屏幕三角形大小，放大或被缩小，都会发生畸变，所以使用了过滤
	// point is by default
	// 对最近线性采样的使用了放大和缩小过滤器，以及点采样使用多级渐进纹理过滤器
	Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);

	//cylinder
	D3DXCreateCylinder(
		Device,
		1.0f, // radius at negative z end
		1.0f, // radius at positive z end
		5.0f, // length of cylinder
		20,   // slices
		20,   // stacks
		&Objects,
		0);
	// build the ObjWorldMatrices which translates by (0.0f, 0.0f, 1.25f)
	D3DXMatrixTranslation(&ObjWorldMatrices, 0.0f, 0.0f, 1.25f);

	//sphere 
	D3DXCreateSphere(
		Device,
		0.4f, // radius
		10,   // slices
		10,   // stacks
		&Objects2,
		0);
	

	//设置投影矩阵
	D3DXMATRIX proj;
	D3DXMatrixPerspectiveFovLH(
		&proj, //output
		D3DX_PI * 0.5f, // 90 - degree
		(float)Width / (float)Height,//aspect
		1.0f,//近裁剪面位置Z
		1000.0f);//远裁剪面位置Z
	Device->SetTransform(D3DTS_PROJECTION, &proj);//projection transformation
	
	// use lighting
	Device->SetRenderState(D3DRS_LIGHTING, true);
	Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);//Gouraud

	return true;
}

void Cleanup()
{
	d3d::Release<IDirect3DVertexBuffer9*>(Quad);
	d3d::Release<IDirect3DTexture9*>(Tex);
	d3d::Release<ID3DXMesh*>(Objects); 
	d3d::Release<ID3DXMesh*>(Objects2);
}

bool Display(float timeDelta)
{
	if (Device)
	{
		D3DXVECTOR3 position(x, y, z);
		// the camera is targetted at the origin of the world
		D3DXVECTOR3 target(0.0f, 0.0f, 1.25f);
		// the worlds up vector
		D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
		D3DXMATRIX V;
		//  up - position 就是相机的朝向
		D3DXMatrixLookAtLH(&V, &position, &target, &up);
		Device->SetTransform(D3DTS_VIEW, &V);

		//rotate with itself
		D3DXMATRIX Rx, Ry, Rz;
		static float xx, yy, zz;
		D3DXMatrixRotationX(&Rx, xx);
		D3DXMatrixRotationY(&Ry, yy);
		D3DXMatrixRotationZ(&Rz, zz);

		// rotate around the cylinder
		D3DXMATRIX p2;
		p2 = Rx * Rz;
		D3DXMatrixTranslation(&ObjWorldMatrices2, cosf(angle) *5.0f, sinf(angle)*5.0f, 5.0f);
		ObjWorldMatrices2 = p2*ObjWorldMatrices2;

		//set up spot light
		/*D3DXVECTOR3 pos(0.0f, 0.0f, -100.0f);*/
		D3DXVECTOR3 pos(cosf(angle) *10.0f, sinf(angle)*10.0f, -100.0f);
		time += timeDelta;
		/*if (time > 10)
			return true;*/
		angle += timeDelta;
		if (angle >= 6.28f)
			angle = 0.0f;
		xx += timeDelta;
		yy += timeDelta;
		zz += timeDelta;

		D3DXVECTOR3 dir(0.0f, 0.0f,		1.0f);
		D3DXCOLOR   c = d3d::WHITE;
		Spot = d3d::InitSpotLight(&pos, &dir, &c);
		//enable a spot light
		Device->SetLight(0, &Spot);
		Device->LightEnable(0, true);
		// Set light related render states.
		Device->SetRenderState(D3DRS_NORMALIZENORMALS, true);
		Device->SetRenderState(D3DRS_SPECULARENABLE, true);
	
		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
		Device->BeginScene();
		
		// texture
		Device->SetTexture(0,Tex);
		Device->SetStreamSource(0, Quad, 0, sizeof(Vertex));
		Device->SetFVF(Vertex::FVF);
		Device->SetTransform(D3DTS_WORLD, &Worlds);
		Device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);
		
		// cylinder
		Device->SetTexture(0, tempTexture);
		Device->SetMaterial(&MtrlsCylinder);
		Device->SetTransform(D3DTS_WORLD, &ObjWorldMatrices);
		Objects->DrawSubset(0);

		//sphere  
		Device->SetTexture(0, NULL);
		Device->SetMaterial(&Mtrls);
		Device->SetTransform(D3DTS_WORLD, &ObjWorldMatrices2);
		Objects2->DrawSubset(0);


		Device->EndScene();
		Device->Present(0, 0, 0, 0);
	}
	return true;
}


//
// WndProc
//
LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		::PostQuitMessage(0);
		break;
	case WM_CHAR:
		switch (wParam) {
			case('R'):
				MtrlsCylinder = d3d::RED_MTRL;
				break;
			case('r') :
				MtrlsCylinder = d3d::RED_MTRL;
				break;
			case('G') :
				MtrlsCylinder = d3d::GREEN_MTRL;
				break;
			case('g') :
				MtrlsCylinder = d3d::GREEN_MTRL;
				break;
			case('B'):
				MtrlsCylinder = d3d::BLUE_MTRL;
				break;
			case('b') :
				MtrlsCylinder = d3d::BLUE_MTRL;
				break;
			case('W') :
				MtrlsCylinder = d3d::WHITE_MTRL;
				break;
			case('w') :
				MtrlsCylinder = d3d::WHITE_MTRL;
				break;
		case ('X'):
				z += 1.0f;
			break;
		case ('Z'):
				z -= 1.0f;
			break;
		case ('C') :
			x += 1.0f;
			break;
		case ('V') :
			x -= 1.0f;
			break;
		case ('T') :
			tempTexture = Tex;
			break;
		case ('t') :
			tempTexture = Tex;
			break;
		case ('S') : {
			 count++;
			 int temp = count % 3;
			 if (temp == 0) {
				 x = 1.0f; y = 2.0f; z = -10.0f;
			 }
			 else if (temp == 1) {
				 x = cosf(angle) *5.0f;
				 y = sinf(angle)*5.0f;
				 z = 5.0f;
			 }
			 else {
				 x = cosf(angle) *10.0f;
				 y = sinf(angle)*10.0f;
				 z = -100.0f;
			 }
			 break;
		}
		}
	}
	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

//
// WinMain
//
int WINAPI WinMain(HINSTANCE hinstance,
	HINSTANCE prevInstance,
	PSTR cmdLine,
	int showCmd)
{
	if (!d3d::InitD3D(hinstance,
		Width, Height, true, D3DDEVTYPE_HAL, &Device))
	{
		::MessageBox(0, "InitD3D() - FAILED", 0, 0);
		return 0;
	}

	if (!Setup())
	{
		::MessageBox(0, "Setup() - FAILED", 0, 0);
		return 0;
	}

	d3d::EnterMsgLoop(Display);

	Cleanup();

	Device->Release();

	return 0;
}