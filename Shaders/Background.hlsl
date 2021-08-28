#include "Common.hlsl"

struct VertexIn
{
	float3 _posLocal : POSITION;
	float3 _normalLocal : NORMAL;
	float2 _textureCoord : TEXCOORD;
};

struct VertexOut
{
	float4 _posHCS : SV_POSITION;
	//float4 _shadowPosHCS : POSITION0;
	//float3 _posWorld : POSITION1;
	//float3 _normalWorld : NORMAL;
	float2 _textureCoord : TEXTURE;
};

VertexOut BackgroundVertexShader(VertexIn vIn)
{
	VertexOut vOut;

	float4 posWorld = mul(float4(vIn._posLocal, 1.0f), gWorld);
	//vOut._posWorld = posWorld.xyz;
	vOut._posHCS = mul(posWorld, gViewProj);
	float4 textureCoord = mul(float4(vIn._textureCoord, 0.f, 1.f), gTextureTransform);
	vOut._textureCoord = mul(textureCoord, gMaterialTransform).xy;
	// 물체들에 비균등비례나 이동이 들어가지 않았으므로 역전치행렬이 월드행렬과 같아서 그대로 사용한다.
	//vOut._normalWorld = mul(vIn._normalLocal, (float3x3)gWorld);
	//vOut._shadowPosHCS = mul(posWorld, gShadowTransform);
	return vOut;
}

float4 BackgroundPixelShader(VertexOut pIn) : SV_TARGET
{
	float4 diffuseAlbedo = gDiffuseAlbedo * gDiffuseMap[gDiffuseMapIndex].Sample(gsamAnisotropicWrap, pIn._textureCoord);
	return diffuseAlbedo;
}