#ifndef NUM_DIR_LIGHTS
	#define NUM_DIR_LIGHTS 0
#endif

#ifndef NUM_POINT_LIGHTS
	#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
	#define NUM_SPOT_LIGHTS 0
#endif

#include "LightingUtil.hlsl"
Texture2D gShadowMap : register(t0);
Texture2D gDiffuseMap[TEXTURE_MAX] : register(t1);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

#ifdef INSTANCING
struct InstanceData
{
	float4x4 _world;
	float4x4 _textureTransform;
	uint _diffuseMapIndex;
	uint pad0;
	uint pad1;
	uint pad2;
};
StructuredBuffer<InstanceData> gInstanceData : register(t1, space1);
#else
cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gTextureTransform;
};
#endif

#ifdef SKINNED
cbuffer cbSkinned : register(b1)
{
	float4x4 gBoneTransforms[96];
};
#endif

cbuffer cbMaterial : register(b2)
{
	float4 gDiffuseAlbedo;
	float3 gFresnelR0;
	float gRoughness;
	float4x4 gMaterialTransform;
	uint gDiffuseMapIndex;
	uint materialPad0;
	uint materialPad1;
	uint materialPad2;
};

cbuffer cbPassConstants : register(b3)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float4x4 gShadowTransform;
	float3 gCameraPos;
	float passPad0;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;

	float4 gFogColor;
	float gFogStart;
	float gFogEnd;
	float2 passPad1;
	float4 gAmbientLight;
	Light gLights[MAX_LIGHT_COUNT];
};
//struct MaterialData
//{
//	float4 _diffuseAlbedo;
//	float3 _fresnelR0;
//	float _roughness;
//	float4x4 _materialTransform;
//	uint _diffuseMapIndex;
//	uint pad0;
//	uint pad1;
//	uint pad2;
//};
//StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

float CalcShadowFactor(float4 shadowPosH)
{
	// Complete projection by doing division by w.
	shadowPosH.xyz /= shadowPosH.w;

	// Depth in NDC space.
	float depth = shadowPosH.z;

	uint width, height, numMips;
	gShadowMap.GetDimensions(0, width, height, numMips);

	// Texel size.
	float dx = 1.0f / (float)width;

	float percentLit = 0.0f;
	const float2 offsets[9] =
	{
		float2(-dx,  -dx), float2(0.0f,  -dx), float2(dx,  -dx),
		float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
		float2(-dx,  +dx), float2(0.0f,  +dx), float2(dx,  +dx)
	};

	[unroll]
	for (int i = 0; i < 9; ++i)
	{
		percentLit += gShadowMap.SampleCmpLevelZero(gsamShadow,
			shadowPosH.xy + offsets[i], depth).r;
	}

	return percentLit / 9.0f;
}