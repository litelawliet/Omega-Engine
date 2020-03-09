#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) rayPayloadInNV vec3 ResultColor;

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

hitAttributeNV vec2 HitAttribs;


Vertex getVertex(uint index)
{
    Vertex v;
    v.pos = sceneVerts[gl_InstanceID].vertex[index].pos;
    v.normal = sceneVerts[gl_InstanceID].vertex[index].normal;
    v.uv = sceneVerts[gl_InstanceID].vertex[index].uv;

    return v;
}

void main() 
{
    const vec3 barycentricCoords = vec3(1.0 - HitAttribs.x - HitAttribs.y, HitAttribs.x, HitAttribs.y);
    vec3 hitPos = gl_WorldRayOriginNV + (gl_WorldRayDirectionNV * gl_HitTNV);
    vec3 lightPos = vec3(0, 4, 2);
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

    vec3  lightColor  = vec3(23.47, 21.31, 20.79);
    vec3  wi          = normalize(lightPos - hitPos);
    float cosTheta    = max(dot(normal, wi), 0.0);
    float attenuation = length(lightPos - hitPos) * length(lightPos - hitPos); 
    vec3  radiance    = (lightColor *  cosTheta) / attenuation;

    vec3 finalColor = radiance * vec3(1);
    finalColor *= texture(textures[texID], pointUV).xyz;

    /*vec3 finalColor;
    if(gl_InstanceID == 0)
        finalColor = vec3(1, 0, 0);

    if(gl_InstanceID == 1)
        finalColor = vec3(0, 1, 0);

    if(gl_InstanceID == 2)
        finalColor = vec3(0, 0, 1);

    if(gl_InstanceID == 3)
        finalColor = vec3(1, 0, 1);  */ 
    ResultColor = finalColor;

}