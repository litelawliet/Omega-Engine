#version 450

#define MAX_LIGHTS 16

#define PI 3.141592653589793238462643383279502884

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

layout(binding = 1) uniform sampler2D texSampler;

layout(set = 0, binding = 2) buffer LightsInfo
{
    float lightsData[];
} Lights;

layout(binding = 3) uniform inLightNumber
{
    uint nbLights;
} numberOfLight;
 
layout(binding = 4) uniform MaterialInfo
{
    vec4 Color; // Diffuse color for dielectrics, f0 for metallic
    float Rough; // Roughness
    bool Metal; // Metallic (true) or dielectric (false)
} Material;

layout(location = 0) out vec4 FragColor;

vec3 lightPosition = vec3(1.0, 1.0, 1.0);

void main()
{
    vec4 texColor = texture(texSampler, inTexCoord);

    vec3 N = normalize(inNormal);
    vec3 L = normalize(lightPosition - inPosition);
    vec3 V = normalize(inCameraPosition.xyz);
    vec3 R = reflect(-L, N);

    float ambiant = 0.2;
    vec3 diffuse = max(dot(N, L), 0.0) * vec3(1.0);

    float shininess = 16.0;
    float specular = pow(max(dot(R,V), 0.0), shininess) * texColor.a;

    FragColor = vec4(diffuse * texColor.rgb + specular, 1.0);
}       