#define INSTANCING
#include "Common.hlsl"

// 이펙트 중심 점 한개
struct VertexIn
{
	float3 _posLocal : POSITION;
	float3 _normalLocal : NORMAL;
	float2 _textureCoord : TEXCOORD;
};

struct VertexOut
{
	float3 _posWorld : POSITION;
	uint _instanceID : INSTANCEID;
};

struct GeoOut
{
	float4 _posHCS : SV_POSITION;
	float3 _posWorld : POSITION;
	float3 _normalWorld : NORMAL;
	float2 _textureCoord    : TEXCOORD;
	uint _diffuseMapIndex : DIFFUSEINDEX;
	float _alpha : ALPHA;
};

VertexOut EffectVertexShader(VertexIn vIn, uint instanceID : SV_InstanceID)
{
	VertexOut vOut;

	vOut._posWorld = vIn._posLocal;
	vOut._instanceID = instanceID;

	return vOut;
}

[maxvertexcount(4)]
void EffectGeoShader(point VertexOut gIn[1],
	inout TriangleStream<GeoOut> triStream)
{
	InstanceData instanceData = gInstanceData[gIn[0]._instanceID];
	gIn[0]._posWorld += instanceData._position;
	float3 up = gCameraUpVector;
	float3 right = gCameraRight;

	float halfWidth = 0.5f * instanceData._size.x;
	float halfHeight = 0.5f * instanceData._size.y;

	float4 v[4];
	v[0] = float4(gIn[0]._posWorld + halfWidth * right - halfHeight * up, 1.0f);
	v[1] = float4(gIn[0]._posWorld + halfWidth * right + halfHeight * up, 1.0f);
	v[2] = float4(gIn[0]._posWorld - halfWidth * right - halfHeight * up, 1.0f);
	v[3] = float4(gIn[0]._posWorld - halfWidth * right + halfHeight * up, 1.0f);

	float texXUnit;
	float texYUnit;
	if (instanceData._totalFrame > 8)
	{
		texXUnit = 1.f / 8;
		texYUnit = 1.f / ((instanceData._totalFrame / 8) + 1);
	}
	else
	{
		texXUnit = 1.f / instanceData._totalFrame;
		texYUnit = 1.f;
	}

	float texXOffset = texXUnit * (instanceData._frame % 8);
	float texYOffset = texYUnit * (instanceData._frame / 8);
	float2 texCoord[4] =
	{
		float2(texXOffset + texXUnit,	texYOffset + texYUnit),
		float2(texXOffset + texXUnit,	texYOffset),
		float2(texXOffset,				texYOffset + texYUnit),
		float2(texXOffset,				texYOffset)
	};

	GeoOut gOut;
	[unroll]
	for (int i = 0; i < 4; ++i)
	{
		gOut._posHCS = mul(v[i], gViewProj);
		gOut._posWorld = v[i].xyz;
		gOut._normalWorld = cross(up, right);
		gOut._textureCoord = texCoord[i];
		gOut._diffuseMapIndex = instanceData._diffuseMapIndex;
		gOut._alpha = instanceData._alpha;
		triStream.Append(gOut);
	}
}

float4 EffectPixelShader(GeoOut pIn) : SV_TARGET
{
	float4 diffuseAlbedo = gDiffuseAlbedo * gDiffuseMap[pIn._diffuseMapIndex].Sample(gsamAnisotropicWrap, pIn._textureCoord);
	diffuseAlbedo.a *= pIn._alpha;

	pIn._normalWorld = normalize(pIn._normalWorld);

	float3 toEye = gCameraPos - pIn._posWorld;
	float distanceToEye = length(toEye);
	toEye /= distanceToEye;

	float4 color = diffuseAlbedo;
#ifdef FOG
	float fogAmount = 0.3 * saturate((distanceToEye - gFogStart) / gFogEnd);
	color = lerp(color, gFogColor, fogAmount);
#endif
	color.a = diffuseAlbedo.a;

	return color;
}