#pragma once

#include <memory>

#include "shader_cache.h"
#include "shader.h"

// Shaders used by Renderer class
struct ShaderSet {
    explicit ShaderSet(ShaderCache& cache) :
        shadowDir(cache.Build("shadow_directional", "shadow_directional.vert", "depth.frag")),
        shadowPoint(cache.Build("shadow_point", "shadow_point.vert", "shadow_point.frag")),
        shadowSpot(cache.Build("shadow_spot", "shadow_spot.vert", "depth.frag")),

        deferredLight(cache.Build("deferred", "quad.vert", "deferred_pbr.frag")),
        gBuffer(cache.Build("gBuffer", "gBuffer.vert", "gBuffer.frag")),
        forward(cache.Build("forward", "forward.vert", "forward_pbr.frag")),

        equirect(cache.Build("equirect", "equirect_to_cubemap.vert", "equirect_to_cubemap.frag")),
        skybox(cache.Build("skybox", "skybox.vert", "skybox.frag")),
        irradiance(cache.Build("irradiance", "irradiance.vert", "irradiance.frag")),
        prefilter(cache.Build("prefilter", "irradiance.vert", "prefilter.frag")),
        brdf(cache.Build("brdf", "brdf.vert", "brdf.frag")),

        bloomDownsample(cache.Build("downsample", "quad.vert", "downsample.frag")),
        bloomUpsample(cache.Build("upsample", "quad.vert", "upsample.frag")),
        bloomFinal(cache.Build("bloomFinal", "quad.vert", "bloom_final.frag")),

        ssao(cache.Build("ssao", "quad.vert", "ssao.frag")),
        ssaoBlur(cache.Build("ssao_blur", "quad.vert", "ssao_blur.frag")),

        fxaa(cache.Build("fxaa", "quad.vert", "fxaa.frag")),

        unlit(cache.Build("unlit", "unlit.vert", "unlit.frag"))
    {}

    std::shared_ptr<Shader> shadowDir;
    std::shared_ptr<Shader> shadowPoint;
    std::shared_ptr<Shader> shadowSpot;

    std::shared_ptr<Shader> deferredLight;
    std::shared_ptr<Shader> gBuffer;
    std::shared_ptr<Shader> forward;

    std::shared_ptr<Shader> equirect;
    std::shared_ptr<Shader> skybox;
    std::shared_ptr<Shader> irradiance;
    std::shared_ptr<Shader> prefilter;
    std::shared_ptr<Shader> brdf;

    std::shared_ptr<Shader> bloomDownsample;
    std::shared_ptr<Shader> bloomUpsample;
    std::shared_ptr<Shader> bloomFinal;

    std::shared_ptr<Shader> ssao;
    std::shared_ptr<Shader> ssaoBlur;

    std::shared_ptr<Shader> fxaa;
    std::shared_ptr<Shader> unlit;
};
