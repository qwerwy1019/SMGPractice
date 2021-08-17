#include "Common.hlsl"

struct VertexIn
{
	float3 _posLocal : POSITION;
	float2 _textureCoord : TEXCOORD;
#ifdef SKINNED
	float3 _boneWeights : WEIGHTS;
	uint4 _boneIndices : BONEINDICES;
#endif
};

struct VertexOut
{
	float4 _posHCS : SV_POSITION;
	float2 _textureCoord : TEXTURE;
};
// Shadow Buffer에 깊이값만 쓰면 되기 때문에 vs, ps에서 최소한의 작업만 한다.
VertexOut ShadowVertexShader(VertexIn vIn)
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
	VertexOut vOut = (VertexOut)0.0f;;

	float4 posWorld = mul(float4(vIn._posLocal, 1.0f), gWorld);
	
	vOut._posHCS = mul(posWorld, gViewProj);
	float4 textureCoord = mul(float4(vIn._textureCoord, 0.f, 1.f), gTextureTransform);
	vOut._textureCoord = mul(textureCoord, gMaterialTransform).xy;
	return vOut;
}

void ShadowPixelShader(VertexOut pIn)
{
	float4 diffuseAlbedo = gDiffuseAlbedo * gDiffuseMap.Sample(gsamAnisotropicWrap, pIn._textureCoord);

#ifdef ALPHA_TEST
	clip(diffuseAlbedo.a - 0.1f);
#endif
}