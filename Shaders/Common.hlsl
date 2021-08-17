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
Texture2D gDiffuseMap : register(t0);
Texture2D gShadowMap : register(t1);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gTextureTransform;
};

#ifdef SKINNED
cbuffer cbSkinned : register(b1)
{
	float4x4 gBoneTransforms[96];
};
#endif

cbuffer cbPassConstants : register(b2)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float4x4 gShadowTransform;
	float3 gCameraPos;
	float pad1;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;

	float4 gFogColor;
	float gFogStart;
	float gFogEnd;
	float2 pad2;
	float4 gAmbientLight;
	Light gLights[MAX_LIGHT_COUNT];
};

cbuffer cbMaterial : register(b3)
{
	float4 gDiffuseAlbedo;
	float3 gFresnelR0;
	float gRoughness;
	float4x4 gMaterialTransform;
};