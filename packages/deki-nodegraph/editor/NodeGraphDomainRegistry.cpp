#ifdef DEKI_EDITOR

#include "deki-nodegraph/NodeGraphDomainRegistry.h"

NodeGraphDomainRegistry& NodeGraphDomainRegistry::Instance() {
    static NodeGraphDomainRegistry instance;
    return instance;
}

void NodeGraphDomainRegistry::Register(const DekiNodeGraphDomain* domain) {
    if (!domain || !domain->assetTypeName) return;
    // Dedup by asset type (re-registration on hot-reload).
    if (m_ByAssetType.count(domain->assetTypeName)) return;
    m_Domains.push_back(domain);
    m_ByAssetType[domain->assetTypeName] = domain;
}

const DekiNodeGraphDomain* NodeGraphDomainRegistry::Get(const std::string& assetTypeName) const {
    auto it = m_ByAssetType.find(assetTypeName);
    return it != m_ByAssetType.end() ? it->second : nullptr;
}

void NodeGraphDomainRegistry::Clear() {
    m_Domains.clear();
    m_ByAssetType.clear();
}

#endif // DEKI_EDITOR
