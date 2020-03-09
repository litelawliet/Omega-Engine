#version 460

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
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

vec3 inColor = vec3(1,1,1);

void main()
{
    mat4 mvp = ubo.proj * ubo.view * ubo.model;

    gl_Position = mvp * vec4(inPosition.xyz, 1);

    // pos in view space
    outFragPosition = vec3(vec4(inPosition.xyz, 1.0) * ubo.model * ubo.view);

    // color to next stage
    outFragColor = vec4(inColor, 1.0);

    // texture coordinates
    outFragTexCoord = inTexCoord;

    // normal to fragment shader
    outFragNormal = mat3(transpose(inverse(ubo.model))) * inNormal;
}