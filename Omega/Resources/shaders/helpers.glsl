#include "random.glsl"
#define PI 3.141592653589793

struct Vertex
{
    vec3 pos;
    vec3 normal;
    vec3 tangent;
    vec2 uv;
    int dummy;
};

struct Material 
{
    
    vec4 albedo;     // Albedo color
    vec4 specular;   // Specular color
    float roughness; // Roughness or Phong exponent
    float ior;       // Index of refraction
    vec4 emissive;   // Emissive color
    int type;
};

struct MaterialData
{    
    vec4 albedo;     // Albedo color
    vec4 specular;   // Specular color
    vec4 data;
    vec4 emissive;   // Emissive color
};

struct LightData
{
    vec4 pos;
    vec4 color;
    vec4 dir;
};


vec3 calculateNormal(Vertex v0, Vertex v1, Vertex v2, sampler2D normalMap, vec3 aNormal, vec2 pointUV, inout mat3 TBN)
{
    vec3 edge1 = v1.pos - v0.pos;
    vec3 edge2 = v2.pos - v0.pos;
    vec2 deltaUV1 = v1.uv - v0.uv;
    vec2 deltaUV2 = v2.uv - v0.uv;  

    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
    vec3 tangent1;
    tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
    tangent1 = normalize(tangent1);

    vec3 bitangent1;
    bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
    bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
    bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
    bitangent1 = normalize(bitangent1);

    vec3 T = normalize(vec3(gl_ObjectToWorldNV * vec4(tangent1,   0.0)));
    vec3 B = normalize(vec3(gl_ObjectToWorldNV * vec4(bitangent1, 0.0)));
    vec3 N = normalize(vec3(gl_ObjectToWorldNV * vec4(aNormal,    0.0)));
    TBN = mat3(T, B, N);

    vec3 finalNormal = texture(normalMap, pointUV).rgb;
    finalNormal = finalNormal * 2.0 - 1.0;
    finalNormal = normalize(TBN * finalNormal);
    return finalNormal;
}

// BRDF math
vec3 ggx(vec3 n, vec3 v, vec3 l, float roughness, vec3 F0) 
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    
    float dotNL = clamp(dot(n, l), 0., 1.);
    float dotNV = clamp(dot(n, v), 0., 1.);
    
    vec3 h = normalize(v + l);
    float dotNH = clamp(dot(n, h), 0., 1.);
    float dotLH = clamp(dot(l, h), 0., 1.);
    
    // GGX microfacet distribution function
    float den = (alpha2 - 1.) * dotNH * dotNH + 1.;
    float D = alpha2 / (M_PI * den * den);
    
    // Fresnel with Schlick approximation
    vec3 F = F0 + (1.0 - F0) * pow(1. - dotLH, 5.);
    
    // Smith joint masking-shadowing function
    float k = .5 * alpha;
    float G = 1. / ((dotNL * (1.0 - k) + k) * (dotNV * (1. - k) + k));
    
    return D * F * G;
}

mat3 matrixFromAxisAngle(vec3 axis, float angle) 
{

    float c = cos(angle);
    float s = sin(angle);
    float t = 1.0 - c;
	//  if axis is not already normalised then uncomment this
	// double magnitude = Math.sqrt(axis.x*axis.x + axis.y*axis.y + axis.z*axis.z);
	// if (magnitude==0) throw error;
	// axis.x /= magnitude;
	// axis.y /= magnitude;
	// axis.z /= magnitude;
    mat3 m;
    m[0][0] = c + axis.x*axis.x*t;
    m[1][1] = c + axis.y*axis.y*t;
    m[2][2] = c + axis.z*axis.z*t;


    float tmp1 = axis.x*axis.y*t;
    float tmp2 = axis.z*s;
    m[1][0] = tmp1 + tmp2;
    m[0][1] = tmp1 - tmp2;
    tmp1 = axis.x*axis.z*t;
    tmp2 = axis.y*s;
    m[2][0] = tmp1 - tmp2;
    m[0][2] = tmp1 + tmp2;    
    tmp1 = axis.y*axis.z*t;
    tmp2 = axis.x*s;
    m[2][1] = tmp1 + tmp2;
    m[1][2] = tmp1 - tmp2;

    return m;
}

vec3 getConeSample(vec3 direction, float coneAngle, inout uint seed) 
{
    float cosAngle = cos(coneAngle);

    // Generate points on the spherical cap around the north pole [1].
    // [1] See https://math.stackexchange.com/a/205589/81266
    float z = RandomFloat(seed) * (1.0f - cosAngle) + cosAngle;
    float phi = RandomFloat(seed) * 2.0f * PI;

    float x = sqrt(1.0f - z * z) * cos(phi);
    float y = sqrt(1.0f - z * z) * sin(phi);
    vec3 north = vec3(0.f, 0.f, 1.f);

    // Find the rotation axis `u` and rotation angle `rot` [1]
    vec3 axis = normalize(cross(north, normalize(direction)));
    float angle = acos(dot(normalize(direction), north));

    // Convert rotation axis and angle to 3x3 rotation matrix [2]
    mat3 R = matrixFromAxisAngle(axis, angle);

    return R * vec3(x, y, z);
}

float getConeAngle(vec3 lightPos, vec3 lightDir, vec3 origin, float radius)
{
    // Calculates the angle of a cone that starts at position forigin and perfectly
    // encapsulates a sphere at position lightPos with radius light.radius
    // Calculate a vector perpendicular to L
    vec3 perpL = cross(lightDir, vec3(0.f, 1.0f, 0.f));
    // Handle case where L = up -> perpL should then be (1,0,0)
    if (length(perpL) == 0) 
        perpL.x = 1.0;

    // Use perpL to get a vector from forigin to the edge of the light sphere
    vec3 lightDirEdge = normalize((lightPos + perpL * radius) - origin);
    // Angle between L and lightDirEdge. Used as the cone angle when sampling shadow rays
    return acos(dot(lightDir, lightDirEdge)) * 2.0f;
}

vec3 getHemisphereUniformSample(vec3 n, inout uint seed) {
    float cosTheta = RandomFloat(seed);
    float sinTheta = sqrt(1. - cosTheta * cosTheta);
    
    float phi = 2. * M_PI * RandomFloat(seed);
    
    // Spherical to cartesian
    vec3 t = normalize(cross(n.yzx, n));
    vec3 b = cross(n, t);
    
	return (t * cos(phi) + b * sin(phi)) * sinTheta + n * cosTheta;
}

vec3 getHemisphereCosineSample(vec3 n, out float weight, inout uint seed) {
    float cosTheta2 = RandomFloat(seed);
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1. - cosTheta2);
    
    float phi = 2. * M_PI * RandomFloat(seed);
    
    // Spherical to cartesian
    vec3 t = normalize(cross(n.yzx, n));
    vec3 b = cross(n, t);
    
	vec3 l = (t * cos(phi) + b * sin(phi)) * sinTheta + n * cosTheta;
    
    // Sample weight
    float pdf = (1. / M_PI) * cosTheta;
    weight = (.5 / M_PI) / (pdf + 1e-6);
    
    return l;
}

vec3 getHemisphereGGXSample(vec3 n, vec3 v, float roughness, out float weight, inout uint seed) {
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    
    float epsilon = clamp(RandomFloat(seed), 0.001, 1.);
    float cosTheta2 = (1. - epsilon) / (epsilon * (alpha2 - 1.) + 1.);
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1. - cosTheta2);
    
    float phi = 2. * M_PI * RandomFloat(seed);
    
    // Spherical to cartesian
    vec3 t = normalize(cross(n.yzx, n));
    vec3 b = cross(n, t);
    
	vec3 microNormal = (t * cos(phi) + b * sin(phi)) * sinTheta + n * cosTheta;
    
    vec3 l = reflect(-v, microNormal);
    
    // Sample weight
    float den = (alpha2 - 1.) * cosTheta2 + 1.;
    float D = alpha2 / (M_PI * den * den);
    float pdf = D * cosTheta / (4. * dot(microNormal, v));
    weight = (.5 / M_PI) / (pdf + 1e-6);
    
    if (dot(l, n) < 0.)
        weight = 0.;
    
    return l;
}