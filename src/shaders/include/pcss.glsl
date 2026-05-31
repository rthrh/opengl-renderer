/*
Source: https://developer.download.nvidia.com/whitepapers/2008/PCSS_Integration.pdf
*/

#define NEAR_PLANE 0.1

// To modify these values, poissonDisk must be also regenerated
#define BLOCKER_SEARCH_NUM_SAMPLES 16
#define PCF_NUM_SAMPLES 16

const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2( 0.94558609, -0.76890725),
    vec2(-0.094184101,-0.92938870),
    vec2( 0.34495938,  0.29387760),
    vec2(-0.91588581,  0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543,  0.27676845),
    vec2( 0.97484398,  0.75648379),
    vec2( 0.44323325, -0.97511554),
    vec2( 0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2( 0.79197514,  0.19090188),
    vec2(-0.24188840,  0.99706507),
    vec2(-0.81409955,  0.91437590),
    vec2( 0.19984126,  0.78641367),
    vec2( 0.14383161, -0.14100790)
);

// Parallel plane estimation
float PenumbraSize(float zReceiver, float zBlocker) {
    return (zReceiver - zBlocker) / zBlocker;
}

// Spot lights
float ComputeLightSizeUV_Spot(float intensity, float cosOuter) {
    float worldSize = intensity * 0.02;
    float tanOuter = sqrt(max(0.0, 1.0 - cosOuter * cosOuter)) / max(cosOuter, 1e-4);
    float frustumWidth = 2.0 * NEAR_PLANE * tanOuter;
    return worldSize / frustumWidth;
}

void FindBlocker_Spot(out float avgBlockerDepth, out float numBlockers, vec2 uv, float zReceiver, float lightSizeUV, int lightIndex) {
    //This uses similar triangles to compute what
    //area of the shadow map we should search
    float searchWidth = lightSizeUV * (zReceiver - NEAR_PLANE) / zReceiver;
    float blockerSum = 0.0;
    numBlockers = 0.0;
    for( int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; ++i )
    {
        vec2 offset = poissonDisk[i] * searchWidth;
        float shadowMapDepth = texture(shadowSpotMap, vec3(uv + offset, float(lightIndex))).r;
        if (shadowMapDepth < zReceiver) {
            blockerSum += shadowMapDepth;
            numBlockers += 1.0;
        }
    }

    if (numBlockers > 0.0)
        avgBlockerDepth = blockerSum / numBlockers;
    else
        avgBlockerDepth = 0.0;
}

float PCF_Filter_Spot(vec2 uv, float zReceiver, float filterRadiusUV, int lightIndex)
{
    float sum = 0.0;
    for (int i = 0; i < PCF_NUM_SAMPLES; ++i) {
        vec2 offset = poissonDisk[i] * filterRadiusUV;
        float depth = texture(shadowSpotMap, vec3(uv + offset, float(lightIndex))).r;
        sum += zReceiver > depth ? 0.0 : 1.0;
    }
    return sum / float(PCF_NUM_SAMPLES);
}

float PCSS_Spot(vec3 coords, float lightSizeUV, int lightIndex)
{
    vec2 uv = coords.xy;
    float zReceiver = coords.z; // Assumed to be eye-space z in this code

    // STEP 1: blocker search
    float avgBlockerDepth = 0.0;
    float numBlockers = 0.0;
    FindBlocker_Spot(avgBlockerDepth, numBlockers, uv, zReceiver, lightSizeUV, lightIndex);
    if( numBlockers < 1.0 )
        //There are no occluders so early out (this saves filtering)
        return 1.0;

    // STEP 2: penumbra size
    float penumbraRatio = PenumbraSize(zReceiver, avgBlockerDepth);
    float filterRadiusUV = penumbraRatio * lightSizeUV * NEAR_PLANE / coords.z;

    // STEP 3: filtering
    return PCF_Filter_Spot(uv, zReceiver, filterRadiusUV, lightIndex);
}

float PCF_Filter_Dir(vec2 uv, float zReceiver, float filterRadiusUV)
{
    float sum = 0.0;
    for (int i = 0; i < PCF_NUM_SAMPLES; ++i) {
        vec2 offset = poissonDisk[i] * filterRadiusUV;
        float depth = texture(shadowDirMap, uv + offset).r;
        sum += zReceiver > depth ? 0.0 : 1.0;
    }
    return sum / float(PCF_NUM_SAMPLES);
}
float PCSS_Dir(vec3 coords) {
    float texelSize = 1.0 / float(textureSize(shadowDirMap, 0).x);
    return PCF_Filter_Dir(coords.xy, coords.z, texelSize * 1.5);
}
