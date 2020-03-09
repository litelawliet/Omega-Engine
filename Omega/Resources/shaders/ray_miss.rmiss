#version 460
#extension GL_NV_ray_tracing : require

layout(location = 0) rayPayloadInNV vec3 ResultColor;

void main() 
{
    ResultColor = vec3(0.412f, 0.796f, 1.0f);
}