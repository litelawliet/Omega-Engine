#version 460
#extension GL_NV_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#include "payload.glsl"

layout(location = 0) rayPayloadInNV Payload tracedData;

void main() 
{
    tracedData.hasHit = false;
    tracedData.color = vec3(1);
    tracedData.baseAlbedo = vec3(1);
}