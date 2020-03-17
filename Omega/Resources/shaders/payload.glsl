#extension GL_GOOGLE_include_directive : enable

struct Payload
{
    vec3 color;
    vec3 baseAlbedo;
    vec3 newOrigin;
    vec3 normal;
    vec3 newDir;
    vec3 cameraPos;

    uint seed; 
    
    bool hasHit;
    bool isDirect;
};