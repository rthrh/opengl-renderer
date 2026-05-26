#version 420 core

out float FragColor;
in vec2 TexCoords;

// ssaoMap = texNoise
uniform vec3 samples[64];

// tile noise texture over screen based on screen dimensions divided by noise size
uniform vec2 noiseScale;

#include "include/samplers.glsl"
#include "include/ubo.glsl"


// get view space position from depth map
vec3 ReconstructViewPos(vec2 uv, float depth) {
    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view = inverse(camera.projection) * clip;
    view /= view.w;
    return view.xyz; // stay in view space
}

void main()
{
    if(!Config.ssaoEnabled) {
        FragColor = 1.0;
        return;
    }

    // Config
    float radius = Config.ssaoRadius;
    float bias = Config.ssaoBias;
    int kernelSize = Config.ssaoKernel;

    // Reconstruct view space position from depth map
    float fragDepth = texture(depthMap, TexCoords).r;
    vec3 fragPos = ReconstructViewPos(TexCoords, fragDepth);

    //vec3 normal = normalize(texture(normalMap, TexCoords).rgb);
    vec3 normal = normalize(mat3(camera.view) * texture(normalMap, TexCoords).rgb); // normal to view space

    vec3 randomVec = normalize(texture(ssaoMap, TexCoords * noiseScale).xyz);
    // create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    // iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for(int i = 0; i < kernelSize; ++i)
    {
        // get sample position
        vec3 samplePos = TBN * samples[i]; // from tangent to view-space
        samplePos = fragPos + samplePos * radius;

        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = camera.projection * offset;
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

        // get sample depth
        float sampleDepth = ReconstructViewPos(offset.xy, texture(depthMap, offset.xy).r).z;

        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / kernelSize);

    FragColor = occlusion;
}
