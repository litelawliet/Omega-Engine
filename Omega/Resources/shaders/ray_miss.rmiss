#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#include "payload.glsl"

layout(location = 0) rayPayloadInEXT Payload tracedData;

void main() 
{
    tracedData.hasHit = false;
    //tracedData.color = vec3(0.6);
    tracedData.emissive = vec3(0.1);
    
}