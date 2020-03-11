#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "payload.glsl"
#include "random.glsl"
#define PI 3.141592653589793

layout(location = 0) rayPayloadInNV Payload tracedData;
layout(location = 3) rayPayloadNV bool isShadowed;

layout(set = 0, binding = 0) uniform accelerationStructureNV Scene;

struct Material
{
    vec4 albedo;
    float metallic;
    float roughness;
    float reflectance;
    float ambientOcc;
    int materialType;
};

struct Vertex
{
    vec3 pos;
    vec3 normal;
    vec3 tangent;
    vec2 uv;
    int dummy;
};

layout(binding = 3, set = 0, scalar) buffer buf1 
{ 
    Vertex vertex[]; 
} sceneVerts[];

layout(binding = 4, set = 0) buffer buf2
{
    uint indices[];
}sceneIndices[];

layout(binding = 5, set = 0) uniform sampler2D textures[];

layout(binding = 6, set = 0) buffer buf3
{
    uint id[];
}sceneObjTexture;

layout(binding = 7, set = 0) buffer buf4
{
    Material mat;

}sceneMaterials[];

hitAttributeNV vec2 HitAttribs;


Vertex getVertex(uint index)
{
    Vertex v;
    v.pos = sceneVerts[gl_InstanceID].vertex[index].pos;
    v.normal = sceneVerts[gl_InstanceID].vertex[index].normal;
    v.uv = sceneVerts[gl_InstanceID].vertex[index].uv;

    return v;
}

float DistributionGGX(vec3 N, vec3 H, float a)
{
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float nom    = a2;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom        = PI * denom * denom;
	
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float k)
{
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return nom / denom;
}
  
float GeometrySmith(vec3 N, vec3 V, vec3 L, float k)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);
	
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() 
{
    const vec3 barycentricCoords = vec3(1.0 - HitAttribs.x - HitAttribs.y, HitAttribs.x, HitAttribs.y);
    vec3 hitPos = gl_WorldRayOriginNV + (gl_WorldRayDirectionNV * gl_HitTNV);
    vec3 lightPos = vec3(-20, 20, 20);
    vec3 direction = normalize(lightPos - hitPos);
    
    uint IDx = sceneIndices[gl_InstanceID].indices[3 * gl_PrimitiveID];
    uint IDy = sceneIndices[gl_InstanceID].indices[3 * gl_PrimitiveID + 1];
    uint IDz = sceneIndices[gl_InstanceID].indices[3 * gl_PrimitiveID + 2];
    uint texID = sceneObjTexture.id[gl_InstanceID];

    Vertex v0 = getVertex(IDx);
    Vertex v1 = getVertex(IDy);
    Vertex v2 = getVertex(IDz);
    
    vec3 normal = v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z;
    vec2 pointUV = v0.uv * barycentricCoords.x + v1.uv * barycentricCoords.y + v2.uv * barycentricCoords.z;
    normal = vec3(normalize(gl_ObjectToWorldNV * vec4(normal, 0)));

    const uint rayFlags = gl_RayFlagsTerminateOnFirstHitNV | gl_RayFlagsOpaqueNV| gl_RayFlagsSkipClosestHitShaderNV;
    const vec3 forigin = gl_WorldRayOriginNV + (gl_WorldRayDirectionNV * gl_HitTNV) + normal * 0.001f;
    const vec3 fdir = normalize(lightPos - forigin);


    Material objMat = sceneMaterials[gl_InstanceID].mat;
    vec3 V = normalize(tracedData.cameraPos - forigin);
    vec3 L = normalize(lightPos - tracedData.cameraPos);
    vec3 H = normalize(V + L);

    float cosTheta = max(0.0, dot(normalize(lightPos - forigin), normal));
    float cosView = max(0.0, dot(V, normal));

    vec3 lambertDiffuse = objMat.albedo.rgb * cosTheta;
    vec3 F0 = vec3(0.04); // Plastic Reflectance
    F0 = mix(F0, objMat.albedo.rgb, objMat.metallic);

    float finalRoughness = objMat.roughness * objMat.roughness;
    float k = ((finalRoughness + 1) * (finalRoughness + 1)) / 8;
    float D = DistributionGGX(normal, H, finalRoughness);
    vec3 F = fresnelSchlick(cosView, F0);
    float G = GeometrySmith(normal, V, L, k);


    vec3 cookTorrance = D * F * G;
    cookTorrance /= 4 * dot(V, normal) * dot(L, normal);


    vec3 Ks = F;
    vec3 Kd = vec3(1.0) - Ks;
    Kd *= 1.0 - objMat.metallic;

    vec3 color = Kd * lambertDiffuse + Ks * cookTorrance;

    /*isShadowed = true;
    traceNV(Scene, rayFlags, 0xFF, 0, 0, 0, forigin, 0, fdir, 1000.0f, 3);
    if(isShadowed)
        color = vec3(0);*/

    tracedData.newOrigin = forigin;
    tracedData.newDir = normalize(reflect(gl_WorldRayDirectionNV, normal) + (RandomInUnitSphere(tracedData.seed) * D));
    tracedData.color = color;
    tracedData.reflectanceFactor = D;
    tracedData.hasHit = true;

}