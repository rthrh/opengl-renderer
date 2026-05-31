#include "include/pcss.glsl"


// Dir Shadows //
float ShadowDirectionalLight(vec3 fragPos, vec3 normal, mat4 lightSpaceMatrix, float biasMin, float biasMax)
{
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w; // perform perspective divide

    // keep the shadow at 0.0 when outside the farPlane region of the light's frustum.
    if(projCoords.z > 1.0)
        return 0.0;

    projCoords = projCoords * 0.5 + 0.5; // transform to [0,1] range

    // calculate bias
    vec3 lightDir = normalize(-dirLight.direction.xyz);
    float bias = max(biasMax * (1.0 - dot(normal, lightDir)), biasMin);

    // PCF
    vec3 biasedCoords = vec3(projCoords.xy, projCoords.z - bias);
    float visibility = PCSS_Dir(biasedCoords);
    return 1.0 - visibility;
}

// Point Shadows //
// Array of offset direction for sampling
const vec3 poissonSphere[20] = vec3[](
    vec3( 0.7569, -0.0739, -0.6494),
    vec3(-0.4393, -0.7565,  0.4838),
    vec3( 0.1654,  0.9355,  0.3127),
    vec3(-0.6841,  0.5193, -0.5128),
    vec3( 0.3829, -0.4986,  0.7768),
    vec3( 0.8927,  0.4486, -0.0480),
    vec3(-0.9018, -0.0913,  0.4226),
    vec3( 0.0533,  0.6195, -0.7831),
    vec3(-0.2645, -0.3489, -0.8995),
    vec3( 0.5828,  0.7888,  0.1971),
    vec3( 0.4534, -0.8902, -0.0467),
    vec3(-0.7186,  0.6824,  0.1334),
    vec3( 0.1284,  0.1782,  0.9755),
    vec3(-0.5497, -0.3984,  0.7344),
    vec3( 0.9456, -0.2783,  0.1689),
    vec3(-0.1923,  0.7392, -0.6456),
    vec3( 0.6238,  0.0867,  0.7768),
    vec3(-0.8421,  0.4831, -0.2447),
    vec3( 0.2156, -0.7234, -0.6552),
    vec3(-0.4127,  0.1645,  0.8956)
);

float random(vec3 seed) {
    return fract(sin(dot(seed, vec3(12.9898, 78.233, 45.164))) * 43758.5453);
}

float SamplePointShadowDepth(vec3 dir, int lightIdx) {
    if (lightIdx == 0) return texture(shadowPointMap0, dir).r;
    if (lightIdx == 1) return texture(shadowPointMap1, dir).r;
    if (lightIdx == 2) return texture(shadowPointMap2, dir).r;
    return texture(shadowPointMap3, dir).r;
}

float ShadowPointLight(vec3 fragPos, vec3 lightPos, float farPlane, float bias, int lightIndex)
{
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);

    // PCF kernel
    float shadow = 0.0;
    int samples = 20;
    float viewDistance = currentDepth;
    float diskRadius = (1.0 + (viewDistance / farPlane)) / 25.0;

    // Per-fragment rotation matrix to turn banding into noise
    float angle = random(fragPos) * 6.2831853;
    float cosA = cos(angle);
    float sinA = sin(angle);
    mat3 rot = mat3(
        cosA, 0.0, sinA,
        0.0,  1.0, 0.0,
       -sinA, 0.0, cosA
    );

    for(int i = 0; i < samples; ++i)
    {
        vec3 offset = rot * poissonSphere[i] * diskRadius;
        float closestDepth = SamplePointShadowDepth(fragToLight + offset, lightIndex);
        closestDepth *= farPlane; // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);

    return shadow;
}

// Spot Shadows //
float ShadowSpotLight(vec3 fragPos, vec3 normal, mat4 lightSpaceMatrix, float biasMin, float biasMax, int lightIndex)
{
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w; // perform perspective divide

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        return 0.0;

    projCoords = projCoords * 0.5 + 0.5; // transform to [0,1] range

    // calculate bias (based on depth map resolution and slope)
    vec3 lightDir = normalize(-spotLights.lights[lightIndex].direction.xyz);
    float bias = max(biasMax * (1.0 - dot(normal, lightDir)), biasMin);

    // PCSS
    float intensity = spotLights.lights[lightIndex].colorAndIntensity.w;
    float cosOuter = spotLights.lights[lightIndex].outerCone;
    float lightSizeUV = ComputeLightSizeUV_Spot(intensity, cosOuter);

    vec3 biasedCoords = vec3(projCoords.xy, projCoords.z - bias);
    float visibility = PCSS_Spot(biasedCoords, lightSizeUV, lightIndex);
    return 1.0 - visibility;
}
