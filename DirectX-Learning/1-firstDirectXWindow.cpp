#include <windows.h>    // include the basic windows header file
#include <windowsx.h>
#include <d3d9.h>

// include the library
#pragma comment (lib, "d3d9.lib")

// some global value
LPDIRECT3D9 d3d;	// direct3D interface
LPDIRECT3DDEVICE9 d3ddev;	//the pointer to the device
// functions
void initD3D(HWND hWnd);
void render_frame();
void cleanD3D();

void initD3D(HWND hWnd) {
	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	D3DPRESENT_PARAMETERS d3dpp;	// hold various device info
	ZeroMemory(&d3dpp, sizeof(d3dpp));    // clear out the struct for use

	d3dpp.Windowed = TRUE;	//not fullscreen
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;	// set the window to be used by d3d

	// create a device class using this information and information from the d3dpp stuct
	d3d->CreateDevice(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
		&d3ddev);
}

void render_frame() {
	// clear the window to a deep blue
	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 40, 100), 1.0f, 0);
	d3ddev->BeginScene();	// begin 3D Scene
	// usually here you can draw everything you need
	d3ddev->EndScene();	// end 3D Scene
	d3ddev->Present(NULL, NULL, NULL, NULL);
}

void cleanD3D() {
	d3ddev->Release();
	d3d->Release();
}

// the WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	HWND hWnd;
	WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = L"WindowClass";

	RegisterClassEx(&wc);

	hWnd = CreateWindowEx(NULL,
		L"WindowClass",
		L"Our First Direct3D Program",
		WS_OVERLAPPEDWINDOW,
		300, 300,
		800, 600,
		NULL,
		NULL,
		hInstance,
		NULL);

	ShowWindow(hWnd, nCmdShow);

	// set up and initialize Direct3D
	initD3D(hWnd);

	// enter the main loop:

	MSG msg;

	while (TRUE)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
			break;

		render_frame();
	}

	// clean up DirectX and COM
	cleanD3D();

	return msg.wParam;
}

// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
	{
					   PostQuitMessage(0);
					   return 0;
	} break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

Direct3D uses a technology called a Flexible Vertex Format (or FVF). A vertex format is the layout of the data containing the location and properties of a vertex. A flexible vertex format would be a format of data that you can modify and set according to your needs. Let's take a look at how this works exactly.