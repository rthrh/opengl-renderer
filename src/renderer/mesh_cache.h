#pragma once

#include <gl/headers.h>

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <unordered_map>

#include "mesh.h"
#include "asset_cache.h"

enum RenderQueueType {
    Opaque, // Opaque
    Masked, // Alpha masked + alpha cut off
    Blend, // Blend rendered, sorted TODO
    NoShadow // Debug unlit
};


struct MeshHandle {
    RenderQueueType queue;
    uint32_t id;
    bool operator==(const MeshHandle&) const = default;
};

struct MeshHandleHash {
        uint64_t operator()(const MeshHandle& h) const noexcept {
        return std::hash<uint64_t>{}(
            (static_cast<uint64_t>(h.queue) << 32) | h.id
        );
    }
};

using MeshQueue = std::unordered_map<MeshHandle, Mesh, MeshHandleHash>;

class MeshCache {
public:
    MeshCache(std::shared_ptr<AssetCache> assetCache) :
        _assetCache(std::move(assetCache))
    {
    }

    ~MeshCache() = default;

    MeshCache(const MeshCache&) = delete;
    MeshCache& operator=(const MeshCache&) = delete;
    MeshCache(MeshCache&&) noexcept = default;
    MeshCache& operator=(MeshCache&&) noexcept = default;

    std::vector<MeshHandle> AddMeshes(std::vector<Mesh> meshes) {
        std::vector<MeshHandle> handles;
        handles.reserve(meshes.size());
        for (auto& mesh : meshes) {
            auto& material = _assetCache->GetMaterial(mesh.GetMaterialIndex());
            auto queueType = this->getQueueType(material.alphaMode);
            auto& meshQueue = this->GetQueue(queueType);

            MeshHandle meshHandle = {.queue = queueType, .id = _nextId++};
            meshQueue.insert({meshHandle, std::move(mesh)});
            handles.emplace_back(meshHandle);
        }

        return handles;
    }

    MeshQueue& GetQueue(RenderQueueType queueType) {
        switch(queueType) {
            case RenderQueueType::Opaque: return _opaqueQueue;
            case RenderQueueType::Masked: return _maskedQueue;
            case RenderQueueType::Blend: return _blendQueue;
            case RenderQueueType::NoShadow: return _debugQueue;
        }
        std::unreachable();
    }

    Mesh* FindMesh(MeshHandle handle) {
        auto& queue = this->GetQueue(handle.queue);
        auto it = queue.find(handle);
        return (it == queue.end()) ? nullptr : &it->second;
    }
private:
    RenderQueueType getQueueType(AlphaMode alphaMode) {
        switch(alphaMode) {
            case AlphaMode::Opaque: return RenderQueueType::Opaque;
            case AlphaMode::Mask: return RenderQueueType::Masked;
            case AlphaMode::Blend: return RenderQueueType::Blend;
        }
        std::unreachable();
    }

    uint32_t _nextId = 0;
    std::shared_ptr<AssetCache> _assetCache;
    MeshQueue _opaqueQueue;
    MeshQueue _maskedQueue;
    MeshQueue _blendQueue;
    MeshQueue _debugQueue;
};
