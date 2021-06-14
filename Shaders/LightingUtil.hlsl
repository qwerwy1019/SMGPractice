// MAX_LIGHT_COUNT 값 변경 시 TypeData.h의 값 수정해야함.
#define MAX_LIGHT_COUNT 16
struct Light
{
	float3 _strength;
	float _falloffStart;
	float3 _direction;
	float _falloffEnd;
	float3 _position;
	float _spotPower;
};

struct Material
{
	float4 _diffuseAlbedo;
	float3 _fresnelR0;
	float _shininess;
};

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
	return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

float3 ShlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
	float cos = saturate(dot(normal, lightVec));
	float f0 = 1.f - cos;
	float3 reflectPercent = R0 + (1.f - R0) * (f0 * f0 * f0 * f0 * f0);

	return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material material)
{
	const float m = material._shininess * 256.f;
	float3 halfVec = normalize(toEye + lightVec);

	float roughnessFactor = (m + 8.f) * pow(max(dot(halfVec, normal), 0.f), m) / 8.f;
	float3 fresnelFactor = ShlickFresnel(material._fresnelR0, halfVec, lightVec);

	float3 specAlbedo = fresnelFactor * roughnessFactor;

	specAlbedo = (specAlbedo) / (specAlbedo + 1.f);
	float3 diffuseAlbedo = material._diffuseAlbedo.rgb;
	
	return (material._diffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

float3 ComputeDirectionalLight(Light light, Material material, float3 normal, float3 toEye)
{
	float3 lightVec = -light._direction;

	float lightDotNormal = max(dot(lightVec, normal), 0.f);
	float3 lightStrength = lightDotNormal * light._strength;

	return BlinnPhong(lightStrength, lightVec, normal, toEye, material);
}

float3 ComputePointLight(Light light, Material material, float3 position, float3 normal, float3 toEye)
{
	float3 lightVec = light._position - position;
	float d = length(lightVec);
	
	if (d > light._falloffEnd)
	{
		return 0.0f;
	}

	lightVec /= d;
	float lightDotNormal = max(dot(lightVec, normal), 0.f);

	float3 lightStrength = light._strength * lightDotNormal;

	float att = CalcAttenuation(d, light._falloffStart, light._falloffEnd);
	lightStrength *= att;
	return  BlinnPhong(lightStrength, lightVec, normal, toEye, material);
}

float3 ComputeSpotLight(Light light, Material material, float3 position, float3 normal, float3 toEye)
{
	float3 lightVec = light._position - position;
	float d = length(lightVec);

	if (d > light._falloffEnd)
	{
		return 0.0f;
	}

	lightVec /= d;
	float lightDotNormal = max(dot(lightVec, normal), 0.f);
	float3 lightStrength = light._strength * lightDotNormal;

	float att = CalcAttenuation(d, light._falloffStart, light._falloffEnd);
	lightStrength *= att;
	float spotFactor = pow(max(dot(-lightVec, light._direction), 0), light._spotPower);
	lightStrength *= spotFactor;

	return BlinnPhong(lightStrength, lightVec, normal, toEye, material);
}

float4 ComputeLighting(Light gLights[MAX_LIGHT_COUNT], Material material, float3 pos, float3 normal, float3 toEye, float3 shadowFactor)
{
	float3 result = { 0.f, 0.f, 0.f };
	int i = 0;

#if (NUM_DIR_LIGHTS > 0)
	for (i = 0; i < NUM_DIR_LIGHTS; ++i)
	{
		result += shadowFactor[i] * ComputeDirectionalLight(gLights[i], material, normal, toEye);
	}
#endif

#if (NUM_POINT_LIGHTS > 0)
	for(i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; ++i)
	{
		result += ComputePointLight(gLights[i], material, pos, normal, toEye);
	}
#endif

#if (NUM_SPOT_LIGHTS > 0)
	for (i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
	{
		result += ComputeSpotLight(gLights[i], material, pos, normal, toEye);
	}
#endif

	return float4(result, 0.f);
}