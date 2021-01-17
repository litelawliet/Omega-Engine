#version 460
#extension GL_NV_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#include "payload.glsl"

layout(location = 0) rayPayloadInNV Payload tracedData;

void main() 
{
    tracedData.hasHit = false;
    
}