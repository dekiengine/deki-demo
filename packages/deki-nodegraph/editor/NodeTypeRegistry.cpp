#ifdef DEKI_EDITOR

#include "deki-nodegraph/NodeTypeRegistry.h"
#include "deki-nodegraph/DekiNode.h"   // full DekiNodeMeta

#include "DekiLogSystem.h"

#include <cstdio>

NodeTypeRegistry& NodeTypeRegistry::Instance() {
    static NodeTypeRegistry instance;
    return instance;
}

void NodeTypeRegistry::Register(const DekiNodeMeta* meta, size_t metaSize) {
    if (!meta) return;
    if (metaSize != sizeof(DekiNodeMeta)) {
        // Deliberately does NOT read meta->name: the whole point is that this
        // struct's layout is not the one we compiled against, so every field
        // past the first divergence is garbage.
        if (!m_LayoutMismatch) {
            m_LayoutMismatch = true;
            char buf[320];
            std::snprintf(buf, sizeof(buf),
                          "A loaded package was built against a different DekiNodeMeta "
                          "layout (%zu bytes there, %zu here), so its node types are "
                          "missing from this editor session.\n"
                          "Restart the editor: a DLL's struct layout is fixed once it is "
                          "loaded, so rebuilding again will not help.",
                          metaSize, sizeof(DekiNodeMeta));
            m_MismatchDetail = buf;
            DEKI_LOG_ERROR("NodeTypeRegistry: %s", buf);
        }
        return;
    }
    // Dedup by type id (re-registration on hot-reload, or a duplicate name).
    if (m_ByType.count(meta->typeId)) return;
    m_Nodes.push_back(meta);
    m_ByType[meta->typeId] = meta;
    if (meta->name) m_ByName[meta->name] = meta;
}

const DekiNodeMeta* NodeTypeRegistry::GetMeta(uint32_t typeId) const {
    auto it = m_ByType.find(typeId);
    return it != m_ByType.end() ? it->second : nullptr;
}

const DekiNodeMeta* NodeTypeRegistry::GetMeta(const std::string& name) const {
    auto it = m_ByName.find(name);
    return it != m_ByName.end() ? it->second : nullptr;
}

void NodeTypeRegistry::Clear() {
    m_Nodes.clear();
    m_ByType.clear();
    m_ByName.clear();
}

#endif // DEKI_EDITOR
