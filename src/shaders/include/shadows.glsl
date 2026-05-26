#include "include/pcss.glsl"

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

    // PCSS
    vec3 biasedCoords = vec3(projCoords.xy, projCoords.z - bias);
    float visibility = PCSS_Dir(biasedCoords);
    return 1.0 - visibility;
}


// array of offset direction for sampling
vec3 gridSamplingDisk[20] = vec3[]
(
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1),
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);


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
    float viewDistance = currentDepth;//length(viewPos - fragPos); TODO
    float diskRadius = (1.0 + (viewDistance / farPlane)) / 25.0;
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = SamplePointShadowDepth(fragToLight + gridSamplingDisk[i] * diskRadius, lightIndex);
        closestDepth *= farPlane; // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);

    return shadow;
}


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
    vec3 biasedCoords = vec3(projCoords.xy, projCoords.z - bias);
    float visibility = PCSS_Spot(biasedCoords, lightIndex);
    return 1.0 - visibility;
}
