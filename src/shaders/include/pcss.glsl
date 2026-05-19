/*
Source: https://developer.download.nvidia.com/whitepapers/2008/PCSS_Integration.pdf
*/

#define BLOCKER_SEARCH_NUM_SAMPLES 16
#define PCF_NUM_SAMPLES 16
#define NEAR_PLANE 3.1//9.5
#define LIGHT_WORLD_SIZE .5 //TODO
#define LIGHT_FRUSTUM_WIDTH 3.75 //TODO should come from the light
// Assuming that LIGHT_FRUSTUM_WIDTH == LIGHT_FRUSTUM_HEIGHT
#define LIGHT_SIZE_UV_SPOT (LIGHT_WORLD_SIZE / LIGHT_FRUSTUM_WIDTH) // TODO range 0.05 – 0.15 / 0.15 – 0.3
#define LIGHT_SIZE_UV_DIR 0.005 // should be tuned to something TODO range 0.001 – 0.005 / 0.005 – 0.01
//TODO those constants


const vec2 poissonDisk[16] = vec2[](
    vec2( -0.94201624, -0.39906216 ),
    vec2(  0.94558609, -0.76890725 ),
    vec2( -0.094184101,-0.92938870 ),
    vec2(  0.34495938,  0.29387760 ),
    vec2( -0.91588581,  0.45771432 ),
    vec2( -0.81544232, -0.87912464 ),
    vec2( -0.38277543,  0.27676845 ),
    vec2(  0.97484398,  0.75648379 ),
    vec2(  0.44323325, -0.97511554 ),
    vec2(  0.53742981, -0.47373420 ),
    vec2( -0.26496911, -0.41893023 ),
    vec2(  0.79197514,  0.19090188 ),
    vec2( -0.24188840,  0.99706507 ),
    vec2( -0.81409955,  0.91437590 ),
    vec2(  0.19984126,  0.78641367 ),
    vec2(  0.14383161, -0.14100790 )
);

// Parallel plane estimation
float PenumbraSize(float zReceiver, float zBlocker) {
    return (zReceiver - zBlocker) / zBlocker;
}

// Spot lights
void FindBlocker_Spot(out float avgBlockerDepth, out float numBlockers, vec2 uv, float zReceiver, int lightIndex) {
    //This uses similar triangles to compute what
    //area of the shadow map we should search
    float searchWidth = LIGHT_SIZE_UV_SPOT * (zReceiver - NEAR_PLANE) / zReceiver;
    float blockerSum = 0;
    numBlockers = 0;
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
    return sum / PCF_NUM_SAMPLES;
}

float PCSS_Spot(vec3 coords, int lightIndex)
{
    vec2 uv = coords.xy;
    float zReceiver = coords.z; // Assumed to be eye-space z in this code

    // STEP 1: blocker search
    float avgBlockerDepth = 0.0;
    float numBlockers = 0.0;
    FindBlocker_Spot(avgBlockerDepth, numBlockers, uv, zReceiver, lightIndex);
    if( numBlockers < 1.0 )
        //There are no occluders so early out (this saves filtering)
        return 1.0;

    // STEP 2: penumbra size
    float penumbraRatio = PenumbraSize(zReceiver, avgBlockerDepth);
    float filterRadiusUV = penumbraRatio * LIGHT_SIZE_UV_SPOT * NEAR_PLANE / coords.z;

    // STEP 3: filtering
    return PCF_Filter_Spot(uv, zReceiver, filterRadiusUV, lightIndex);
}


// Dir lights
void FindBlocker_Dir(out float avgBlockerDepth, out float numBlockers, vec2 uv, float zReceiver) {
    //This uses similar triangles to compute what
    //area of the shadow map we should search
    //float searchWidth = LIGHT_SIZE_UV_DIR * (zReceiver - NEAR_PLANE) / zReceiver;
    float searchWidth = LIGHT_SIZE_UV_DIR;
    float blockerSum = 0;
    numBlockers = 0;
    for( int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; ++i )
    {
        vec2 offset = poissonDisk[i] * searchWidth;
        float shadowMapDepth = texture(shadowDirMap, uv + offset).r;
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

float PCF_Filter_Dir(vec2 uv, float zReceiver, float filterRadiusUV)
{
    float sum = 0.0;
    for (int i = 0; i < PCF_NUM_SAMPLES; ++i) {
        vec2 offset = poissonDisk[i] * filterRadiusUV;
        float depth = texture(shadowDirMap, uv + offset).r;
        sum += zReceiver > depth ? 0.0 : 1.0;
    }
    return sum / PCF_NUM_SAMPLES;
}

float PCSS_Dir(vec3 coords)
{
    vec2 uv = coords.xy;
    float zReceiver = coords.z; // Assumed to be eye-space z in this code

    // STEP 1: blocker search
    float avgBlockerDepth = 0.0;
    float numBlockers = 0.0;
    FindBlocker_Dir(avgBlockerDepth, numBlockers, uv, zReceiver);
    if( numBlockers < 1.0 )
        //There are no occluders so early out (this saves filtering)
        return 1.0;

    // STEP 2: penumbra size
    float penumbraRatio = PenumbraSize(zReceiver, avgBlockerDepth);
    //float filterRadiusUV = penumbraRatio * LIGHT_SIZE_UV_DIR * NEAR_PLANE / coords.z;
    float filterRadiusUV = penumbraRatio * LIGHT_SIZE_UV_DIR;

    // STEP 3: filtering
    return PCF_Filter_Dir(uv, zReceiver, filterRadiusUV);
}
