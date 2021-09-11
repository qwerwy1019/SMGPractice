#pragma once
#include "stdafx.h"
#define D3D_DEBUG_INFO
template<typename T>
using WComPtr = Microsoft::WRL::ComPtr<T>;

// 기본 정보
const std::wstring WINDOW_CAPTION = L"d3d App";
constexpr D3D_FEATURE_LEVEL D3D_BASE_LEVEL = D3D_FEATURE_LEVEL_11_0;
constexpr DXGI_FORMAT BACK_BUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
constexpr DXGI_FORMAT DEPTH_STENCIL_FORMAT = DXGI_FORMAT_D24_UNORM_S8_UINT;
constexpr int SWAP_CHAIN_BUFFER_COUNT = 2;
constexpr int FRAME_RESOURCE_COUNT = 3;

constexpr int NUM_DIR_LIGHTS = 3;
constexpr int NUM_POINT_LIGHTS = 0;
constexpr int NUM_SPOT_LIGHTS = 0;
constexpr const char* NUM_DIR_LIGHTS_LPCSTR = "3";
constexpr const char* NUM_POINT_LIGHTS_LPCSTR = "0";
constexpr const char* NUM_SPOT_LIGHTS_LPCSTR = "0";

constexpr UINT OBJECT_MAX = 200;
constexpr UINT MATERIAL_MAX = 200;
constexpr UINT SKINNED_INSTANCE_MAX = 50;
constexpr UINT EFFECT_INSTANCE_MAX = 200;
constexpr UINT TEXTURE_MAX = 200;
constexpr const char* TEXTURE_MAX_LPCSTR = "200";

// 셰이더 디파인
const D3D_SHADER_MACRO definesForShader[] =
{
	"NUM_DIR_LIGHTS", NUM_DIR_LIGHTS_LPCSTR,
//	"NUM_POINT_LIGHTS", NUM_POINT_LIGHTS_LPCSTR,
//	"NUM_SPOT_LIGHTS", NUM_SPOT_LIGHTS_LPCSTR,
//	"FOG", "1",
	"TEXTURE_MAX", TEXTURE_MAX_LPCSTR,
	NULL, NULL
};
const D3D_SHADER_MACRO definesForAlphaTestShader[] =
{
	"NUM_DIR_LIGHTS", NUM_DIR_LIGHTS_LPCSTR,
	"ALPHA_TEST", "1",
	"TEXTURE_MAX", TEXTURE_MAX_LPCSTR,
	NULL, NULL
};
const D3D_SHADER_MACRO definesForSkinnedVertexShader[] =
{
	"NUM_DIR_LIGHTS", NUM_DIR_LIGHTS_LPCSTR,
	"SKINNED", "1",
	"TEXTURE_MAX", TEXTURE_MAX_LPCSTR,
	NULL, NULL
};
const D3D_SHADER_MACRO definesForBackgroundShader[] =
{
	"NUM_DIR_LIGHTS", NUM_DIR_LIGHTS_LPCSTR,
	"TEXTURE_MAX", TEXTURE_MAX_LPCSTR,
	"BACKGROUND", "1",
	NULL, NULL
};

constexpr int FPS = 60;
constexpr float FPS_f = static_cast<float>(FPS);

enum class RenderLayer : uint8_t
{
	Opaque,
	OpaqueSkinned,
	AlphaTested,
	Shadow,
	Transparent,
	Background,
	GameObjectDev,
	Count,
};
constexpr RenderLayer RenderLayers[] =
{
	RenderLayer::Opaque,
	RenderLayer::OpaqueSkinned,
	RenderLayer::AlphaTested,
	RenderLayer::Shadow,
	RenderLayer::Transparent,
	RenderLayer::Background,
	RenderLayer::GameObjectDev,
};
static_assert(sizeof(RenderLayers) == static_cast<int>(RenderLayer::Count), "RenderLayer 추가 시 수정해주세요.");
constexpr RenderLayer HasShadowLayers[] =
{
	RenderLayer::Opaque,
	RenderLayer::OpaqueSkinned,
	RenderLayer::AlphaTested,
};
static_assert(static_cast<int>(RenderLayer::Count) == 7, "그림자가 생겨야 하는 레이어라면 추가해주세요.");
constexpr uint16_t SKINNED_UNDEFINED = std::numeric_limits<uint16_t>::max();

static constexpr float FOV_ANGLE = 0.25f * DirectX::XM_PI;
static constexpr float NEAR_Z = 10.f;
static constexpr float FAR_Z = 50000.f;