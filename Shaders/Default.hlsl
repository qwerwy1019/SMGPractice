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
	bool	 gIsSkinned;
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

struct VertexIn
{
	float3 _posLocal : POSITION;
	float3 _normalLocal : NORMAL;
	float2 _textureCoord : TEXCOORD;
#ifdef SKINNED
	float3 _boneWeights : WEIGHTS;
	uint4 _boneIndices : BONEINDICES;
#endif
};

struct VertexOut
{
	float4 _posHCS : SV_POSITION;
	float3 _posWorld : POSITION;
	float3 _normalWorld : NORMAL;
	float2 _textureCoord : TEXTURE;
};

VertexOut DefaultVertexShader(VertexIn vIn)
{
#ifdef SKINNED
	float weights[4] = { 0.f, 0.f, 0.f, 0.f };
	weights[0] = vIn._boneWeights.x;
	weights[1] = vIn._boneWeights.y;
	weights[2] = vIn._boneWeights.z;
	weights[3] = 1.f - weights[0] - weights[1] - weights[2];

	float3 posLocal = float3(0.f, 0.f, 0.f);
	float3 normalLocal = float3(0.f, 0.f, 0.f);
	float3 tangentLocal = float3(0.f, 0.f, 0.f);

	for (int i = 0; i < 4; ++i)
	{
		if (vIn._boneIndices[i] != 255)
		{
			posLocal += weights[i] * mul(float4(vIn._posLocal, 1.f), gBoneTransforms[vIn._boneIndices[i]]).xyz;
			normalLocal += weights[i] * mul(vIn._normalLocal, (float3x3)gBoneTransforms[vIn._boneIndices[i]]);
		}
		//tangent
	}
	vIn._posLocal = posLocal;
	vIn._normalLocal = normalLocal;
	//tangent
#endif
	
	VertexOut vOut;

	float4 posWorld = mul(float4(vIn._posLocal, 1.0f), gWorld);
	vOut._posWorld = posWorld.xyz;
	vOut._posHCS = mul(posWorld, gViewProj);
	float4 textureCoord = mul(float4(vIn._textureCoord, 0.f, 1.f), gTextureTransform);
	vOut._textureCoord = mul(textureCoord, gMaterialTransform).xy;
	// 물체들에 비균등비례나 이동이 들어가지 않았으므로 역전치행렬이 월드행렬과 같아서 그대로 사용한다.
	vOut._normalWorld = mul(vIn._normalLocal, gWorld);
	return vOut;
}

float4 DefaultPixelShader(VertexOut pIn) : SV_TARGET
{
	float4 diffuseAlbedo = gDiffuseAlbedo * gDiffuseMap.Sample(gsamAnisotropicWrap, pIn._textureCoord);

#ifdef ALPHA_TEST
	clip(diffuseAlbedo.a - 0.1f);
#endif

	pIn._normalWorld = normalize(pIn._normalWorld);

	float3 toEye = gCameraPos - pIn._posWorld;
	float distanceToEye = length(toEye);
	toEye /= distanceToEye;

	float4 ambient = gAmbientLight * diffuseAlbedo;

	const float shininess = 1.f - gRoughness;
	Material material = { diffuseAlbedo, gFresnelR0, shininess };
	float3 shadowFactor = { 1.f, 1.f, 1.f };

	float4 directLight = ComputeLighting(gLights, material, pIn._posWorld, pIn._normalWorld, toEye, shadowFactor);

	float4 color = ambient + directLight;
#ifdef FOG
	float fogAmount = saturate((distanceToEye - gFogStart) / gFogEnd);
	color = lerp(color, gFogColor, fogAmount);
#endif
	// 통상적으로 diffuseAlbedo 알파값에 알파값을 넣는다고함.
	color.a = diffuseAlbedo.a;

	return color;
}