#pragma once
#include "stdafx.h"
#define D3D_DEBUG_INFO
template<typename T>
using WComPtr = Microsoft::WRL::ComPtr<T>;

// �⺻ ����
const std::wstring WINDOW_CAPTION = L"d3d App";
constexpr D3D_FEATURE_LEVEL D3D_BASE_LEVEL = D3D_FEATURE_LEVEL_11_0;
constexpr DXGI_FORMAT BACK_BUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
constexpr DXGI_FORMAT DEPTH_STENCIL_FORMAT = DXGI_FORMAT_D24_UNORM_S8_UINT;
constexpr int SWAP_CHAIN_BUFFER_COUNT = 2;
constexpr int FRAME_RESOURCE_COUNT = 3;

constexpr int NUM_DIR_LIGHTS = 1;
constexpr int NUM_POINT_LIGHTS = 0;
constexpr int NUM_SPOT_LIGHTS = 0;
constexpr const char* NUM_DIR_LIGHTS_LPCSTR = "1";
constexpr const char* NUM_POINT_LIGHTS_LPCSTR = "0";
constexpr const char* NUM_SPOT_LIGHTS_LPCSTR = "0";

// ���̴� ������
const D3D_SHADER_MACRO definesForShader[] =
{
	"NUM_DIR_LIGHTS", NUM_DIR_LIGHTS_LPCSTR,
//	"NUM_POINT_LIGHTS", NUM_POINT_LIGHTS_LPCSTR,
//	"NUM_SPOT_LIGHTS", NUM_SPOT_LIGHTS_LPCSTR,
//	"FOG", "1",
	"ALPHA_TEST", "1",
//	"CARTOON_RENDER", "1",
	NULL, NULL
};


using Index16 = uint16_t;
constexpr Index16 UNDEFINED_COMMON_INDEX = static_cast<Index16>(-1);

constexpr int FPS = 60;
constexpr float FPS_f = static_cast<float>(FPS);

using CharacterKey = Index16;