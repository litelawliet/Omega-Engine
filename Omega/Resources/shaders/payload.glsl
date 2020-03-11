#extension GL_GOOGLE_include_directive : enable

struct Payload
{
    vec3 color;
    uint seed; 
    vec3 newOrigin;
    vec3 newDir;
    vec3 cameraPos;
    float reflectanceFactor;
    bool hasHit;
};