#version 450

layout(std140, set = 0,binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
//layout(location = 3) in vec3 inBitangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 outFragColor;
layout(location = 1) out vec2 outFragTexCoord;
layout(location = 2) out vec3 outFragNormal;
layout(location = 3) out vec3 outFragPosition;
layout(location = 4) out vec4 outCameraPosition;

vec3 inColor = vec3(1.0, 1.0, 1.0);

void main()
{
    // World position
    vec3 pos = vec3(ubo.model * vec4(inPosition, 1.0));
    
    gl_Position = ubo.projection * ubo.view * vec4(pos.xyz, 1.0);

    // pos in view space (world space)
    outFragPosition = vec3(pos);

    // color to next stage
    outFragColor = vec4(inColor, 1.0);

    // texture coordinates
    outFragTexCoord = inTexCoord;

    // normal to fragment shader
    outFragNormal = mat3(ubo.model) * inNormal;

    // V for frag shader
    vec3 viewPos = vec3(ubo.view[3][0], ubo.view[3][1], ubo.view[3][3]);
    outCameraPosition = vec4(viewPos - pos.xyz, 1.0);
}