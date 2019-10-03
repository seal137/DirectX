#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <xnamath.h>
#include <d3dcompiler.h>  //dynamically adding shaders

//Structures
struct SimpleVertex
{
	XMFLOAT3 Pos;
};


//Global variables

HINSTANCE				g_hInst = NULL; //application id
HWND					g_hWnd = NULL;  //window id
D3D_DRIVER_TYPE			g_driverType = D3D_DRIVER_TYPE_NULL; //driver type
D3D_FEATURE_LEVEL		g_featureLevel = D3D_FEATURE_LEVEL_11_0; //version support level
ID3D11Device* g_pd3dDevice = NULL; //resource creator
ID3D11DeviceContext* g_pImmediateContext = NULL;  //graphic information output
IDXGISwapChain* g_pSwapChain = NULL;  //conection chain (buffer and screen)   
ID3D11RenderTargetView* g_pRenderTargetView = NULL; //back buffer object
ID3D11VertexShader* g_pVertexShader = NULL; //Vertex shader
ID3D11PixelShader* g_pPixelShader = NULL;  //Pixel shader
ID3D11InputLayout* g_pVertexLayout = NULL; // Vertex format description
ID3D11Buffer* g_pVertexBuffer = NULL; //Vertex buffer


//Function declarations

//nitialization of input pattern and vertex buffer
HRESULT InitGeometry();

//Create window
HRESULT InitWindow(HINSTANCE hInstance, int  nCmdShow);

//Initializing DirectX devices
HRESULT InitDevice();

//Delete DirectX devices
void CleanupDevice();

//Window function
//HWND - window handle
//UNIT - message to OS
//WPARAM - options of messages
//LPARAM - options of messages
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//Render function
void Render();


//Program entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);	 //variables isn't used
	UNREFERENCED_PARAMETER(lpCmdLine);		 //variables isn't used

	//creating window 
	if (FAILED(InitWindow(hInstance, nCmdShow)))
	{
		return 0;
	}

	//creating directX object
	if (FAILED(InitDevice()))
	{
		CleanupDevice();
		return 0;
	}

	//creating shaders and vertex buffer
	if (FAILED(InitGeometry()))
	{
		CleanupDevice();
		return 0;
	}

	//Main message cycle
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Render();
		}
	}
	CleanupDevice();
	return (int)msg.wParam;
}


//Class regestration and creating window
HRESULT InitWindow(HINSTANCE hInstance, int  nCmdShow)
{
	WNDCLASSEX wcex; //window class
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW; //redraw if width or/and height change
	wcex.lpfnWndProc = WndProc; //window procedure pointer
	wcex.cbClsExtra = 0; //sets the number of extra bytes that follow the window class structure.
	wcex.cbWndExtra = 0; //sets the number of extra bytes that are placed after the window instance.
	wcex.hInstance = hInstance; //instance handle
	wcex.hIcon = NULL; //class icon handle
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW); //class cursor handle
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); //class background brush handle
	wcex.lpszMenuName = NULL; //pointer to a character string with a line break character
	wcex.lpszClassName = L"DX_1";
	wcex.hIconSm = NULL;
	if (!RegisterClassEx(&wcex))
	{
		return E_FAIL;
	}

	//create window
	g_hInst = hInstance;
	RECT rc = { 0, 0, 640, 480 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindow(L"DX_1", L"Lab 1", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance, NULL);
	if (!g_hWnd)
	{
		return E_FAIL;
	}
	ShowWindow(g_hWnd, nCmdShow);
	return S_OK;
}


//Recive message
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps; //paint workspace
	HDC hdc; //grafic object pointer

	switch (message)
	{
	case WM_PAINT: //window coloring request
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


//compile shaders from file
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK; //error condition
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS; //shader flag
	ID3DBlob* pErrorBlob; //return data of arbitrary length
	hr = D3DX11CompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel, dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL); //Compile a shader or an effect from a file
	if (FAILED(hr))
	{
		if (pErrorBlob != NULL)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		if (pErrorBlob) pErrorBlob->Release();
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();
	return S_OK;
}


//creating Direct3D device
HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc); //retrieves the coordinates of the window workspace
	UINT width = rc.right - rc.left; //get width
	UINT height = rc.bottom - rc.top; // get height
	UINT createDeviceFlags = 0;

	//list of supported drivers
	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	//list of supported DirectX versions
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};

	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	//creating DirctX devices
	DXGI_SWAP_CHAIN_DESC sd; //Link chain structure (Swap Chain)
	ZeroMemory(&sd, sizeof(sd)); //clean structure
	sd.BufferCount = 1; //number of buffers
	sd.BufferDesc.Width = width; //buffer width
	sd.BufferDesc.Height = height; //buffer height
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //pixel format in buffer
	sd.BufferDesc.RefreshRate.Numerator = 75; //screen update frequency
	sd.BufferDesc.RefreshRate.Denominator = 1; //bottom of the rational number (frequency)
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; //buffer assignment - back buffer
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;  //smoothness
	sd.SampleDesc.Quality = 0; //quality of smoothness
	sd.Windowed = TRUE; //not windowed

	//creating devices
	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr)) return hr;

	//creating back buffer
	ID3D11Texture2D* pBackBuffer = NULL; //creating texture object
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)& pBackBuffer); //load from object g_pSwapChain buffer characteristics
	if (FAILED(hr)) return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr)) return hr;

	//connect the back buffer object to the device context
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);


	//viewport setup
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;

	// conect viewport to device context
	g_pImmediateContext->RSSetViewports(1, &vp);

	return S_OK;
}


//Creating vertex buffer, shaders, description input layot
HRESULT InitGeometry()
{
	HRESULT hr = S_OK;
	// Compiling vertex shader from file
	ID3DBlob* pVSBlob = NULL; // space in RAM
	hr = CompileShaderFromFile(L"shader.fx", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL, L"Unable to compile FX file. Please run this program from the folder containing the FX file.", L"Error", MB_OK);
		return hr;

	}
	//creating vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}
	// Vertex Pattern Definition
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		/*semantic name, semantic index, size, incoming slot(0 - 15), address of the beginning of data in the vertex buffer, class of the incoming slot(not important), InstanceDataStepRate(not important) */
	};

	UINT numElements = ARRAYSIZE(layout);

	// Creating vertex pattern
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
	{
		return hr;
	}

	//Vertex pattern conection
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	// Pixel shader compile from file
	ID3DBlob* pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL, L"Unable to compile FX file. Please run this program from the folder containing the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Creating pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
	{
		return hr;
	}

	// Create vertex buffer
	SimpleVertex vertices[3];
	vertices[0].Pos.x = 0.0f;  vertices[0].Pos.y = 0.5f;  vertices[0].Pos.z = 0.5f;
	vertices[1].Pos.x = 0.5f;  vertices[1].Pos.y = -0.5f;  vertices[1].Pos.z = 0.5f;
	vertices[2].Pos.x = -0.5f;  vertices[2].Pos.y = -0.5f;  vertices[2].Pos.z = 0.5f;

	D3D11_BUFFER_DESC bd;  // The structure that describes the buffer being created.
	ZeroMemory(&bd, sizeof(bd));  // clean it
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 3; // buffer size = size of 1 vertx * 3
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER; // buffer type is vertex buffer
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData; //Structure containing buffer data
	ZeroMemory(&InitData, sizeof(InitData)); //clean it
	InitData.pSysMem = vertices; // vertices pointer

	//Calling the g_pd3dDevice method will create an ID3D11Buffer vertex buffer object
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
	if (FAILED(hr))
	{
		return hr;
	}

	// Set vertex buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// Setting the way to draw vertices in the buffer
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return S_OK;
}


void CleanupDevice()
{
	// disable the device context, then release the objects
	// delete in the reverse order of creating
	if (g_pImmediateContext) g_pImmediateContext->ClearState();
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pVertexLayout) g_pVertexLayout->Release();
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pPixelShader) g_pPixelShader->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}

void Render()
{
	float ClearColor[4] = { 0.5f, 0.0f, 4.0f, 1.0f }; // RGBA color
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor); //clean back buffer

	 //Connect shaders to drew object
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);

	g_pImmediateContext->Draw(3, 0); //draw vertices
	g_pSwapChain->Present(0, 0); //back buffer to screen

}