#pragma once
#define NOMINMAX
#include <winerror.h>
#include <comdef.h>
#include <wrl.h>

#include <stdint.h>

#include <fstream>
#include <iostream>

#include <array>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>

#include <dxgi1_5.h>
#include <d3dcompiler.h>
#include <d3d12.h>
#include <d3d11_4.h>
#include <d3d11on12.h>
#include <d2d1_3.h>
#include <DirectXMath.h>
//#include <d3dcommon.h>
//#include <dxgiformat.h>
//#include <dxgi.h>
#include "DirectX/d3dx12.h"
#include <dwrite_3.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")
//#pragma comment(lib, "d3d11on12.lib")
#pragma comment(lib, "DWrite.lib")
#pragma comment(lib, "WindowsCodecs.lib")
#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "DSound.lib")
//#pragma comment(lib, "DShow.lib")
#pragma comment(lib, "DInput8.lib")
#pragma comment(lib, "DXGuid.lib")

#include "PreDefines.h"