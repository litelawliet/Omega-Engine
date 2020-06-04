#extension GL_GOOGLE_include_directive : enable

struct Payload
{
    vec3 color;
    vec3 emissive;
    vec3 lightReceived;
    vec3 newOrigin;
    vec3 normal;
    vec3 newDir;
    uint seed;
    bool hasHit;
    bool isDirect;
    int lightSize;
};