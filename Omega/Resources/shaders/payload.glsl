#extension GL_GOOGLE_include_directive : enable

struct Payload
{
    vec3 color;
    vec3 albedo;
    vec3 newOrigin;
    vec3 newDir;
    uint seed;
    bool hasHit;
    int lightSize;
};