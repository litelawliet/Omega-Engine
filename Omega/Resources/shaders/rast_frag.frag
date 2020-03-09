#version 460

#define MAX_LIGHTS 16

#define PI 3.141592653589793238462643383279502884

/*struct LightInfoData
{
    vec4 Position; // Light position in camera coordinates
    vec4 L; // Intensity
};*/


struct LightInfoData
{
    vec4 Position; // Light position in camera coordinates
    vec4 ambient; // Intensity
    vec4 diffuse;
    vec4 specular;
};

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inPosition; 
layout(location = 4) in vec4 inCameraPosition;

layout(set = 0, binding = 1) uniform sampler2D texSampler;

layout(set = 0, binding = 2) buffer LightsInfo
{
    float lightsData[];
} Lights;

layout(set = 0, binding = 3) uniform inLightNumber
{
    uint nbLights;
} numberOfLight;
 
layout(set = 0, binding = 4) uniform MaterialInfo
{
    vec3 Color; // Diffuse color for dielectrics, f0 for metallic
    float Rough; // Roughness
    bool Metal; // Metallic (true) or dielectric (false)
} Material;

layout(location = 0) out vec4 FragColor;

//vec3 BlinnPhong(vec3 position, vec3 normals);
vec3 SchlickFresnel(float lDotH);
float GeomSmith(float dotProd);
float GgxDistribution(float nDotH);
vec3 MicroFacetModel(int lightIdx, vec3 position, vec3 n);
LightInfoData light;
void main()
{
  /* light.Position.x = Lights.lightsData[0];
   light.Position.y = Lights.lightsData[1];
   light.Position.z = Lights.lightsData[2];
   light.Position.w = Lights.lightsData[3];

   light.L.x = Lights.lightsData[4];
   light.L.y = Lights.lightsData[5];
   light.L.z = Lights.lightsData[6];
   light.L.w = Lights.lightsData[7];

    if(light.L.x == 1.0 && light.L.y == 1.0 && light.L.z == 1.0 && light.L.w == 1.0)
    {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    //  return;
    }

    vec3 sum = vec3(0);
    vec3 n = (inNormal);
    for (int i = 0; i < numberOfLight.nbLights; i++)
    {
        sum += MicroFacetModel(i, inCameraPosition.xyz, n);
    }

    // Gamma
    sum = pow(sum, vec3(1.0/2.2));

    FragColor = vec4(sum, 1.0);*/

    vec3 N = normalize(inNormal);

    vec4 finalColor = vec4(0.0, 0.0, 0.0, 0.0);

    light.Position.x = Lights.lightsData[0];
   light.Position.y = Lights.lightsData[1];
   light.Position.z = Lights.lightsData[2];
   light.Position.w = Lights.lightsData[3];

   light.ambient.x = Lights.lightsData[4];
   light.ambient.y = Lights.lightsData[5];
   light.ambient.z = Lights.lightsData[6];
   light.ambient.w = Lights.lightsData[7];

      light.diffuse.x = Lights.lightsData[8];
   light.diffuse.y = Lights.lightsData[9];
   light.diffuse.z = Lights.lightsData[10];
   light.diffuse.w = Lights.lightsData[11];

      light.specular.x = Lights.lightsData[12];
   light.specular.y = Lights.lightsData[13];
   light.specular.z = Lights.lightsData[14];
   light.specular.w = Lights.lightsData[15];

    for (int i = 0; i < numberOfLight.nbLights; i++)
    {
        vec3 L = normalize(light.Position.xyz - inPosition.xyz);
        vec3 E = normalize(-inPosition.xyz);

        vec3 R = normalize(-reflect(L, N));
        vec3 H = normalize(E + L);
        vec4 Iamb = light.ambient;

        vec4 Idiff = light.diffuse * max(dot(N, L), 0.0);
        Idiff = clamp(Idiff, 0.0, 1.0);

        float shininess = 16.0;
        vec4 Ispec = light.specular * pow(max(dot(H, N), 0.0), shininess);
        Ispec = clamp(Ispec, 0.0, 1.0);
 
        finalColor += Iamb + Idiff + Ispec;
    }

    FragColor = vec4(finalColor);
} 

/*vec3 Gaussian()
{
    vec3 lightDir = vec3(0.0);
    float atten = CalcAttenuation(cameraSpacePosition, lightDir);
    vec4 attenIntensity = atten * lightIntensity;

    vec3 surfaceNormal = normalize(vertexNormal);
    float cosAngIncidence = dot(surfaceNormal, lightDir);
    cosAngIncidence = clamp(cosAngIncidence, 0, 1);

    vec3 viewDirection = normalize(-cameraSpacePosition);

    vec3 halfAngle = normalize(lightDir + viewDirection);
    float angleNormalHalf = acos(dot(halfAngle, surfaceNormal));
    float exponent = angleNormalHalf / shininessFactor;
    exponent = -(exponent * exponent);
    float gaussianTerm = exp(exponent);

    gaussianTerm = cosAngIncidence != 0.0 ? gaussianTerm : 0.0;

    outputColor = (diffuseColor * attenIntensity * cosAngIncidence) +
        (specularColor * attenIntensity * gaussianTerm) +
        (diffuseColor * ambientIntensity);
}

float CalcAttenuation(vec3 objPosition, vec3 lightPosition)
{
    float distance  = abs(objPosition - lightPosition);
    return 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
}

vec3 BlinnPhong()
{
    vec3 lightPosition = vec3(0.0);
    float atten = CalcAttenuation(inPosition, lightPosition);
    vec4 attenIntensity = atten * lightIntensity;
        
    vec3 surfaceNormal = normalize(vertexNormal);
    float cosAngIncidence = dot(surfaceNormal, lightPosition);
    cosAngIncidence = clamp(cosAngIncidence, 0, 1);
        
    vec3 viewDirection = normalize(-cameraSpacePosition);
        
    vec3 halfAngle = normalize(lightPosition + viewDirection);
    float blinnTerm = dot(surfaceNormal, halfAngle);
    blinnTerm = clamp(blinnTerm, 0, 1);
    blinnTerm = cosAngIncidence != 0.0 ? blinnTerm : 0.0;
    blinnTerm = pow(blinnTerm, shininessFactor);

    return vec3((diffuseColor * attenIntensity * cosAngIncidence) +
        (specularColor * attenIntensity * blinnTerm) +
        (diffuseColor * ambientIntensity));
}
*/
/*vec3 SchlickFresnel(float lDotH)
{
    vec3 f0 = vec3(0.04); // Dielectrics
    if (Material.Metal)
    {
        f0 = Material.Color;
    }

    return f0 + (1 - f0) * pow(1.0 - lDotH, 5);
}

float GeomSmith(float dotProd)
{
    float k = (Material.Rough + 1.0) * (Material.Rough + 1.0) / 8.0;
    float denom = dotProd * (1 - k) + k;
    return 1.0 / denom;
}

float GgxDistribution(float nDotH)
{
    float alpha2 = Material.Rough * Material.Rough * Material.Rough * Material.Rough;
    float d = (nDotH * nDotH) * (alpha2 - 1.0) + 1.0;

    return alpha2 / (PI * d * d);
}

vec3 MicroFacetModel(int lightIdx, vec3 position, vec3 n)
{
    vec3 diffuseBrdf = vec3(0.0); // Metallic
    if (!Material.Metal)
    {
        diffuseBrdf = Material.Color;
    }
    
    vec3 l = vec3(0.0);
    vec3 lightI = light.L.xyz;

    if (light.Position.w == 0.0) // Directional light
    {
        l = normalize(light.Position.xyz);
    }
    else // Positional light
    {
        l = light.Position.xyz - position;
        float dist = length(l);
        l = normalize(l);
        lightI /= (dist * dist);
    }

    vec3 v = normalize(-position);
    vec3 h = normalize(v + l);
    float nDotH = dot(n, h);
    float lDotH = dot(l, h);
    float nDotL = max(dot(n, l), 0.0);
    float nDotV = dot(n, v);

    vec3 specularBrdf = 0.25 * GgxDistribution(nDotH) * SchlickFresnel(lDotH) * GeomSmith(nDotL) * GeomSmith(nDotV);

    return (diffuseBrdf + PI * specularBrdf) * lightI * nDotL;
}*/