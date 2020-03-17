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

layout(location = 4) rayPayloadNV Payload indirectData;

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
    vec3 lightPos = vec3(0, 5, 6);
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
    vec3 color = vec3(0);

    vec3 N = normal;
    vec3 V = normalize(tracedData.cameraPos - forigin);
    vec3 L = normalize(lightPos - forigin);

    float dist    = length(lightPos - forigin);
    float attenuation = 100.0 / (dist * dist);
    vec3 radiance     = vec3(1) * attenuation;
    color = objMat.albedo.rgb * dot(N, L) * radiance; 

    isShadowed = true;
    traceNV(Scene, rayFlags, 0xFF, 0, 0, 0, forigin, 0, fdir, 1000.0f, 3);
    if(isShadowed)
        color = vec3(0);

    tracedData.newOrigin = forigin;
    tracedData.newDir = normalize(reflect(gl_WorldRayDirectionNV, normal));
    tracedData.color = color;
    tracedData.normal = normal;
    tracedData.baseAlbedo = objMat.albedo.rgb;
    tracedData.hasHit = true;

}




    /*float roughness = objMat.roughness;
    if(roughness < 0.00001)
        roughness = 0.00001;

    vec3 H = normalize(V + L);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, objMat.albedo.rgb, objMat.metallic);
	           

    // calculate per-light radiance
    float dist    = length(lightPos - forigin);
    float attenuation = 80.0 / (dist * dist);
    vec3 radiance     = vec3(1) * attenuation;        
    
    // cook-torrance brdf
    float NDF = DistributionGGX(N, H, roughness);        
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - objMat.metallic;	  
    
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular     = numerator / max(denominator, 0.001);  
        
    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);                
    vec3 color = (kD * objMat.albedo.rgb / PI + specular) * radiance * NdotL; */