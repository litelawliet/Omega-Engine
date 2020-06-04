#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "payload.glsl"
#include "helpers.glsl"

#define PI 3.141592653589793
#define GGX_SAMPLING
// Materials
#define MAT_LAMBERT     0
#define MAT_BLINNPHONG  1
#define MAT_SPECULAR    2
#define MAT_REFRACTION  3
#define MAT_LOMMEL      4
#define MAT_GGX         5

//#define SOFT_SHADOWS
#define SHADOW_RES 1

layout(location = 0) rayPayloadInEXT Payload tracedData;
layout(location = 2) rayPayloadEXT bool isShadowed;

layout(location = 4) rayPayloadEXT Payload indirectData;

layout(set = 0, binding = 0) uniform accelerationStructureEXT Scene;

layout(binding = 3, set = 0, scalar) buffer buf1 
{ 
    Vertex vertex[]; 
} sceneVerts[];

layout(binding = 4, set = 0) buffer buf2
{
    uint indices[];
}sceneIndices[];

layout(binding = 5, set = 0) uniform sampler2D textures[];
layout(binding = 11, set = 0) uniform sampler2D normalMaps[];

layout(binding = 6, set = 0) buffer buf3
{
    uint id[];
}sceneObjTexture;

layout(binding = 7, set = 0) buffer buf4
{
    MaterialData mat;

}sceneMaterials[];

layout(binding = 8, set = 0) buffer buf5
{
    uint id[];
}sceneObjID;

layout(binding = 9, set = 0) buffer buf6
{
    uint id[];
}sceneObjBLAS;

layout(binding = 10, set = 0) buffer buf7
{
    uint id[];
}sceneObjNormalMaps;

layout(binding = 12, set = 0) buffer buf8
{
    LightData light;
}sceneLights[];

hitAttributeEXT vec2 HitAttribs;

Vertex getVertex(uint index)
{
    Vertex v;
    v.pos = sceneVerts[sceneObjBLAS.id[gl_InstanceID]].vertex[index].pos;
    v.normal = sceneVerts[sceneObjBLAS.id[gl_InstanceID]].vertex[index].normal;
    v.uv = sceneVerts[sceneObjBLAS.id[gl_InstanceID]].vertex[index].uv;

    return v;
}

void main() 
{
    const vec3 barycentricCoords = vec3(1.0 - HitAttribs.x - HitAttribs.y, HitAttribs.x, HitAttribs.y);
    vec3 hitPos = gl_WorldRayOriginEXT + (gl_WorldRayDirectionEXT * gl_HitTEXT);
    
    uint IDx = sceneIndices[sceneObjBLAS.id[gl_InstanceID]].indices[3 * gl_PrimitiveID];
    uint IDy = sceneIndices[sceneObjBLAS.id[gl_InstanceID]].indices[3 * gl_PrimitiveID + 1];
    uint IDz = sceneIndices[sceneObjBLAS.id[gl_InstanceID]].indices[3 * gl_PrimitiveID + 2];
    uint texID = sceneObjTexture.id[gl_InstanceID];
    uint normalMapID = sceneObjNormalMaps.id[gl_InstanceID];

    Vertex v0 = getVertex(IDx);
    Vertex v1 = getVertex(IDy);
    Vertex v2 = getVertex(IDz);
    
    vec3 normal = vec3(1);
    vec2 pointUV = v0.uv * barycentricCoords.x + v1.uv * barycentricCoords.y + v2.uv * barycentricCoords.z;
    
    vec3 geometryNormal = v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z;
    mat3 TBN;
    if(normalMapID == 12345)
    {
        normal = geometryNormal;
        normal = vec3(normalize(gl_ObjectToWorldEXT * vec4(normal, 0)));
    }
    else
    {
        normal = calculateNormal(v0, v1, v2, normalMaps[normalMapID], geometryNormal, pointUV, TBN);
    }
    
    vec3 forigin = (gl_WorldRayOriginEXT + (gl_WorldRayDirectionEXT * gl_HitTEXT)) + geometryNormal * 0.001;
    MaterialData objMat = sceneMaterials[gl_InstanceID].mat;
    
    Material material = Material(objMat.albedo, objMat.specular, objMat.data.x, objMat.data.y, objMat.emissive, int(objMat.data.z));
    

    vec3 viewVector = gl_WorldRayDirectionEXT;
    vec3 p = forigin;

    // Orient normal towards ray direction
    float cosThetaI = dot(gl_WorldRayDirectionEXT, normal);
    
    vec3 facingNormal = (cosThetaI < 0.0) ? normal : -normal;
    //vec3 facingNormal = normal;

    //float seed = float(tracedData.seed);

    vec3 acc = vec3(0);
    vec3 att = vec3(1.); 
    if (material.type == MAT_LAMBERT) 
    {
        float weight;
        vec3 reflected = getHemisphereUniformSample(facingNormal, tracedData.seed);
        
        att *= weight;
        att *= material.albedo.rgb * dot(facingNormal, reflected);
        
        tracedData.newOrigin = p;
        tracedData.newDir = reflected;
    }
    // Blinn-Phong material
    else if (material.type == MAT_BLINNPHONG) 
    {
        float weight;
        vec3 reflected = getHemisphereUniformSample(facingNormal, tracedData.seed);
        weight = dot(facingNormal, reflected);

        vec3 h = normalize(-gl_WorldRayDirectionEXT + reflected);
        
        //att *= weight;
        att *= material.albedo.rgb * dot(facingNormal, reflected) +
            material.specular.rgb * pow(max(dot(facingNormal, h), 0.), material.roughness);
        
        tracedData.newOrigin = p;
        tracedData.newDir = reflected;
    }
    // Specular material
    else if (material.type == MAT_SPECULAR) 
    {
        vec3 reflected = reflect(gl_WorldRayDirectionEXT, facingNormal);
        
        att *= material.specular.rgb;
        
        tracedData.newOrigin = p;
        tracedData.newDir = reflected;
    }
    // Refraction material
    else if (material.type == MAT_REFRACTION) 
    {
        float eta = (cosThetaI < 0.) ? (1. / material.ior) : material.ior;
            
            vec3 refracted = refract(gl_WorldRayDirectionEXT, facingNormal, eta);
            
            if (all(equal(refracted, vec3(0.)))) {
                // Total internal reflection
                vec3 reflected = reflect(gl_WorldRayDirectionEXT, facingNormal);
                
	            tracedData.newOrigin = p;
                tracedData.newDir = reflected;
            } else {
                // Fresnel F0
                float F0_ = (material.ior - 1.) / (1. + material.ior);
                float F0 = F0_ * F0_;
                
                // Fresnel with Schlick approximation
                float cosTheta = (cosThetaI < 0.) ? -cosThetaI : dot(refracted, normal);
                float F = F0 + (1.0 - F0) * pow(1. - cosTheta, 5.);
                
                if (RandomFloat(tracedData.seed) < F) {
                    vec3 reflected = reflect(gl_WorldRayDirectionEXT, facingNormal);
                    
                    tracedData.newOrigin = p + reflected * 0.01;
                    tracedData.newDir = reflected;
                } else {
                    att *= material.albedo.rgb;
                    
                    tracedData.newOrigin = p + refracted * 0.01;
                    tracedData.newDir = refracted;
                }
            }
    }
    // Lommel-Seeliger material
    else if (material.type == MAT_LOMMEL) 
    {
        vec3 reflected = getHemisphereUniformSample(facingNormal, tracedData.seed);
        
        att *= material.albedo.rgb * 10;
        
        tracedData.newOrigin = p;
        tracedData.newDir = reflected;
    }
    // GGX material
    else 
    {
        float weight;
        vec3 reflected;
        
        // Sample diffuse and specular.rgb at random
        if ((length(material.albedo.rgb) > 0.04) && (RandomFloat(tracedData.seed) < 0.5)) 
        {
            reflected = getHemisphereCosineSample(facingNormal, weight, tracedData.seed);
            weight = dot(facingNormal, reflected);
            att *= weight;
            att *= material.albedo.rgb * dot(facingNormal, reflected);
        }
        else 
        {
        #ifdef GGX_SAMPLING
            reflected = getHemisphereGGXSample(facingNormal, -gl_WorldRayDirectionEXT,
                                                material.roughness, weight, tracedData.seed);
        #else
            reflected = getHemisphereCosineSample(facingNormal, weight, tracedData.seed);
            weight = dot(facingNormal, reflected);
        #endif
            att *= weight;
            att *= ggx(facingNormal, -gl_WorldRayDirectionEXT, reflected, material.roughness, material.specular.rgb);
        }
        
        tracedData.newOrigin = p;
        tracedData.newDir = reflected;
    }

    vec3 lightPos = vec3(1000, -500, 0);
    float lightIntensity = 1;
    vec3 toLight = normalize(lightPos - forigin);
    float coneAngle = getConeAngle(lightPos, toLight, forigin, 10);

    vec3 lightReceived = vec3(0);
    int lightNb = tracedData.lightSize;
    for(int i = 0; i < SHADOW_RES; ++i)
    {
        if(lightNb > 1)
        {
            for(int i = 1; i < lightNb; ++i)
            {
                LightData light = sceneLights[i].light;
                vec3 dir = vec3(0);
                float maxDistance = distance(light.pos.xyz, forigin);
                float falloff = max(0.01, maxDistance);

                if(light.dir.w == 0)
                    dir = normalize(light.pos.xyz - forigin);
                else if(light.dir.w == 1)
                {
                    dir = light.dir.xyz;
                    falloff = 1;
                }

                /*if(SHADOW_RES == 1)
                    dir = -toLight;
                else
                    dir = getConeSample(toLight, coneAngle, tracedData.seed);*/
                float cosTheta = max(0.0, dot(normal, dir));
                isShadowed = true;  
                
                // Offset indices to match shadow hit/miss index
                traceRayEXT(Scene, gl_RayFlagsOpaqueNV | gl_RayFlagsSkipClosestHitShaderNV, 0xFF, 1, 0, 1, forigin, 0.01, dir, maxDistance, 2);
                if (!isShadowed) 
                {
                    lightReceived += (cosTheta / (falloff * falloff)) * light.color.rgb * sceneLights[i].light.color.w;
                }
            }
        }

    }
    lightReceived /= SHADOW_RES;
    lightReceived *= lightIntensity;

    tracedData.color = att * texture(textures[texID], pointUV).rgb;
    tracedData.lightReceived = lightReceived;
    tracedData.emissive = material.emissive.rgb;
    tracedData.normal = normal;
    tracedData.hasHit = true;

}