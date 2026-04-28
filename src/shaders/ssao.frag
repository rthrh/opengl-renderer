#version 420 core
out float FragColor;

in vec2 TexCoords;

layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 2) uniform sampler2D gNormal;
layout(binding = 13) uniform sampler2D texNoise;

uniform vec3 samples[64];

// parameters (you'd probably want to use them as uniforms to more easily tweak the effect)
int kernelSize = 64;
float radius = 0.5;
float bias = 0.025;

// tile noise texture over screen based on screen dimensions divided by noise size
uniform vec2 noiseScale;

#include "include/ubo.glsl"

void main()
{
    // get input for SSAO algorithm
    vec3 fragPosWorld = texture(gPosition, TexCoords).xyz;
    vec3 fragPos = vec3(camera.view * vec4(fragPosWorld, 1.0)); // from world to view space

    // OLD TODO
    //vec3 fragPos = texture(gPosition, TexCoords).xyz; // from world to view space

    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
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
        vec3 sampleDepthWorld = texture(gPosition, offset.xy).xyz; // get depth value of kernel sample
        float sampleDepth = vec3(camera.view * vec4(sampleDepthWorld, 1.0)).z; // to view space

        // original TODO we can use depth texture instead?
        //float sampleDepth = texture(gPosition, offset.xy).x; // get depth value of kernel sample
        
        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;           
    }
    occlusion = 1.0 - (occlusion / kernelSize);
    
    FragColor = occlusion;
}
