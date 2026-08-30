#include "ParticleGraph.h"
#include "ParticleNodes.h"   // pulls in the node registrations (NodeFactory)

#include "assets/AssetManager.h"
#include "DekiLogSystem.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <vector>

// Runtime loader for the ParticleGraph effect asset. Mirrors the plain
// data-asset loaders: the editor compiles the ".asset" JSON to a MessagePack
// cache via the generic path, and here we parse that cache with the generic
// NodeGraphData loader (which creates the node instances via NodeFactory).
// Same path on desktop and device. A malformed graph loads as nullptr, loudly.

namespace
{
    ParticleGraph* LoadGraphFromMemory(const uint8_t* data, size_t size)
    {
        NodeGraphData* graphData = NodeGraphData::LoadFromMemory(data, size);
        if (!graphData)
        {
            DEKI_LOG_ERROR("ParticleGraph: failed to load particle effect asset");
            return nullptr;
        }

        auto* graph = new ParticleGraph();
        graph->data = graphData;
        return graph;
    }

    struct _ParticleGraphLoaderReg
    {
        _ParticleGraphLoaderReg()
        {
            auto pathLoader = [](const char* p) -> void*
            {
                std::ifstream f(p, std::ios::binary | std::ios::ate);
                if (!f.is_open())
                    return nullptr;
                std::streamsize n = f.tellg();
                f.seekg(0, std::ios::beg);
                std::vector<uint8_t> buf(static_cast<size_t>(n));
                if (!f.read(reinterpret_cast<char*>(buf.data()), n))
                    return nullptr;
                return LoadGraphFromMemory(buf.data(), static_cast<size_t>(n));
            };
            auto unloader  = [](void* a) { delete static_cast<ParticleGraph*>(a); };
            auto memLoader = [](const uint8_t* d, size_t n) -> void* { return LoadGraphFromMemory(d, n); };

            Deki::AssetManager::RegisterLoader("ParticleGraph", pathLoader, unloader, memLoader);
        }
    };
    static _ParticleGraphLoaderReg s_particleGraphLoaderReg;
}
