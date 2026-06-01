#pragma once

#include "renderer/scene.h"
#include "renderer/model_loader.h"
#include <memory>
#include <filesystem>



std::vector<Transform> randomTransforms(int num, unsigned int seed = 888) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> posDist(-10.f, 10.f);
    std::uniform_real_distribution<float> rotDist(-180.f, 180.f);
    std::uniform_real_distribution<float> scaleDist(0.5f, 2.f);
    std::vector<Transform> transforms;
    for (int i = 0; i < num; i++) {
        transforms.push_back({
            .translation = { posDist(rng), posDist(rng), posDist(rng) },
            .eulerAngles = { rotDist(rng), rotDist(rng), rotDist(rng) },
            //.scale       = { scaleDist(rng), scaleDist(rng), scaleDist(rng) }
        });
    }
    return transforms;
}


Scene setupScene(const std::shared_ptr<AssetCache>& assetCache, ModelLoader& modelLoader) {
    Scene scene(assetCache);
    std::filesystem::path root = std::filesystem::path(PROJECT_SOURCE_DIR) / ".." / "glTF-Sample-Models/2.0";

    auto loadModel = [&modelLoader, &root](const char* filename, glm::vec3 translation) {
        auto path = root / filename;
        auto testModel = *modelLoader.Load(path);
        testModel.SetTranslation(translation);
        return testModel;
    };

    std::filesystem::path modelPath = root / ".." / "glTF-Sample-Models/2.0" / "EnvironmentTest/glTF-IBL/EnvironmentTest.gltf";
    std::filesystem::path modelPath2 = root / ".." / "glTF-Sample-Models/2.0" / "DamagedHelmet/glTF/DamagedHelmet.gltf";

    scene.AddModel(loadModel("EnvironmentTest/glTF/EnvironmentTest.gltf", {0, 0, 0}));
    scene.AddModel(loadModel("DamagedHelmet/glTF/DamagedHelmet.gltf", {0, -2, 0}));

    // floor model
    uint32_t defaultMatIndex = assetCache->AddMaterial(assetCache->GetDefaultMaterial());
    Mesh floorMesh(floor_vertices, floor_indices, defaultMatIndex);
    auto floorModel = modelLoader.Load((std::move(floorMesh)));

    floorModel->SetTranslation({0.0f, -2.0f, 0.0f});
    floorModel->SetScale({50.0f, 1.0f, 50.0f});

    scene.AddModel(std::move(*floorModel));

    DirectionalLightUBO dirLight({-1.0, -1.0, 0.0});
    auto light1 = PointLightBlockGPU({10,10,0}).SetColor(0, 125, 255).SetRange(50);
    auto light2 = PointLightBlockGPU({0,10,-10}).SetColor(0, 255, 125).SetRange(50);
    auto light3 = PointLightBlockGPU({0,10,10}).SetColor(255, 125, 0).SetRange(50);
    auto light4 = PointLightBlockGPU({-10,10,0}).SetColor(0, 125, 0).SetRange(50);

    auto spotLight1 = SpotLightBlockGPU({0, 3, 6}, {0, -0.5, -1}).SetColor(0, 0, 255).SetRange(25.0).SetIntensity(10);
    auto spotLight2 = SpotLightBlockGPU({0, 10, 0}, {0, -1.0, 0}).SetColor(125, 0, 0).SetRange(25.0).SetIntensity(10);

    scene.AddDirectionalLight(std::move(dirLight));
    scene.AddPointLight(std::move(light1));
    scene.AddPointLight(std::move(light2));
    scene.AddPointLight(std::move(light3));
    scene.AddPointLight(std::move(light4));
    scene.AddSpotLight(std::move(spotLight1));
    scene.AddSpotLight(std::move(spotLight2));

    return scene;
}


Scene setupTestModels(const std::shared_ptr<AssetCache>& assetCache, ModelLoader& modelLoader) {
    Scene scene(assetCache);
    std::filesystem::path root = std::filesystem::path(PROJECT_SOURCE_DIR) / ".." / "glTF-Sample-Models/2.0";

    auto loadModel = [&modelLoader, &root](const char* filename, glm::vec3 translation) {
        auto path = root / filename;
        auto testModel = *modelLoader.Load(path);
        testModel.SetTranslation(translation);
        return testModel;
    };

    scene.AddModel(loadModel("MetalRoughSpheres/glTF/MetalRoughSpheres.gltf", {0, 0, 0}));
    scene.AddModel(loadModel("AlphaBlendModeTest/glTF/AlphaBlendModeTest.gltf", {10, 0, 0}));
    scene.AddModel(loadModel("TextureCoordinateTest/glTF/TextureCoordinateTest.gltf", {10, -3, 0}));
    scene.AddModel(loadModel("NormalTangentTest/glTF/NormalTangentTest.gltf", {8, 5, 0}));
    scene.AddModel(loadModel("NormalTangentMirrorTest/glTF/NormalTangentMirrorTest.gltf", {11, 5, 0}));

    //scene.AddModel(loadModel("TextureEncodingTest/glTF/TextureEncodingTest.gltf", {25, 0, 0}));
    scene.AddModel(loadModel("TextureLinearInterpolationTest/glTF/TextureLinearInterpolationTest.gltf", {20, 0, 0}));
    scene.AddModel(loadModel("TextureSettingsTest/glTF/TextureSettingsTest.gltf", {30, 0, 0}));
    scene.AddModel(loadModel("NegativeScaleTest/glTF/NegativeScaleTest.gltf", {42, 0, 0}));

    return scene;
}

Scene setupSponza(const std::shared_ptr<AssetCache>& assetCache, ModelLoader& modelLoader) {
    Scene scene(assetCache);
    std::filesystem::path root = std::filesystem::path(PROJECT_SOURCE_DIR) / ".." / "glTF-Sample-Models/2.0";

    auto loadModel = [&modelLoader, &root](const char* filename, glm::vec3 translation) {
        auto path = root / filename;
        auto testModel = *modelLoader.Load(path);
        testModel.SetTranslation(translation);
        return testModel;
    };

    scene.AddModel(loadModel("Sponza/glTF/Sponza.gltf", {0, 0, 0}));

    DirectionalLightUBO dirLight({0.0, -1.0, 0.0});
    dirLight.SetIntensity(2.0f).SetColor(255, 181, 110); // golden hour
    //dirLight.SetIntensity(10.0f).SetColor(255, 248, 242); // high noon
    //dirLight.SetIntensity(10.0f).SetColor(255, 133, 43); // deep sunset
    //dirLight.SetIntensity(10.0f).SetColor(255, 89, 10); // dusk
    scene.AddDirectionalLight(std::move(dirLight));

    auto light1 = PointLightBlockGPU({0,10,0}).SetColor(0, 125, 255).SetRange(50);
    scene.AddPointLight(std::move(light1));

    return scene;
}

Scene setupLocal(const std::shared_ptr<AssetCache>& assetCache, ModelLoader& modelLoader, Camera& camera) {
    Scene scene(assetCache);
    std::filesystem::path root = std::filesystem::path(PROJECT_SOURCE_DIR) / "resources";

    auto loadModel = [&modelLoader, &root](const char* filename, glm::vec3 translation) {
        auto path = root / filename;
        auto testModel = *modelLoader.Load(path);
        testModel.SetTranslation(translation);
        return testModel;
    };

    //scene.AddModel(loadModel("DamagedHelmet/glTF/DamagedHelmet.gltf", {0, 0, 0}));
    scene.AddModel(loadModel("Sponza-downscaled/glTF/Sponza.gltf", {0, 0, 0}));

    DirectionalLightUBO dirLight({0.0, -0.9, 0.0});
    //dirLight.SetIntensity(1.0f).SetColor(255, 181, 110); // golden hour
    //dirLight.SetIntensity(1.0f).SetColor(255, 248, 242); // high noon
    //dirLight.SetIntensity(1.0f).SetColor(255, 133, 43); // deep sunset
    //dirLight.SetIntensity(1.0f).SetColor(255, 89, 10); // dusk
    dirLight.SetIntensity(1.0f).SetColor(80, 150, 255); // moon
    scene.AddDirectionalLight(std::move(dirLight));


    //unsigned r1 = 255, g1 = 100, b1 = 30;
    unsigned r1 = 80, g1 = 150, b1 = 255;
    unsigned r2 = 255, g2 = 100, b2 = 30;
    //unsigned r2 = 80, g2 = 150, b2 = 255;
    float intensity = 5;
    auto light1 = PointLightBlockGPU({3.9, 1.2, 1.15}).SetColor(r1, g1, b1).SetRange(10).SetIntensity(intensity);
    auto light2 = PointLightBlockGPU({3.9, 1.2, -1.75}).SetColor(r1, g1, b1).SetRange(10).SetIntensity(intensity);

    auto light3 = PointLightBlockGPU({-4.95, 1.2, 1.15}).SetColor(r2, g2, b2).SetRange(10).SetIntensity(intensity);
    auto light4 = PointLightBlockGPU({-4.95, 1.2, -1.75}).SetColor(r2, g2, b2).SetRange(10).SetIntensity(intensity);

    scene.AddPointLight(std::move(light1));
    scene.AddPointLight(std::move(light2));
    scene.AddPointLight(std::move(light3));
    scene.AddPointLight(std::move(light4));

    camera.SetPosition({-10.0, 2.0, 0.0});
    camera.SetYawPitch(0.0, 0.0);
    return scene;
}
