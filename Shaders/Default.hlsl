#include "Common.hlsl"

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
	float4 _shadowPosHCS : POSITION0;
	float3 _posWorld : POSITION1;
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
	vOut._normalWorld = mul(vIn._normalLocal, (float3x3)gWorld);
	vOut._shadowPosHCS = mul(posWorld, gShadowTransform);
	return vOut;
}

float4 DefaultPixelShader(VertexOut pIn) : SV_TARGET
{
	float4 diffuseAlbedo = gDiffuseAlbedo * gDiffuseMap[gDiffuseMapIndex].Sample(gsamAnisotropicWrap, pIn._textureCoord);

//#ifdef ALPHA_TEST
	clip(diffuseAlbedo.a - 0.1f);
//#endif

	pIn._normalWorld = normalize(pIn._normalWorld);

	float3 toEye = gCameraPos - pIn._posWorld;
	float distanceToEye = length(toEye);
	toEye /= distanceToEye;

	float4 ambient = gAmbientLight * diffuseAlbedo;

	const float shininess = 1.f - gRoughness;
	Material material = { diffuseAlbedo, gFresnelR0, shininess };
	float3 shadowFactor = { 1.f, 1.f, 1.f };
	shadowFactor[0] = CalcShadowFactor(pIn._shadowPosHCS);

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