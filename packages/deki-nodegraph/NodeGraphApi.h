#pragma once

// DLL export macro, in its own header so intra-package headers (NodeFactory,
// NodeTypeRegistry, NodeGraphDomainRegistry, NodeGraphData) can use it without
// pulling in the DekiNode.h aggregator, which includes them.
//
// The node registries are shared singletons: node types self-register from
// whichever package or project DLL owns them (deki-fsm's states and actions, a
// game's own nodes), so every consumer must reach the ONE instance living in
// deki-nodegraph.dll. On embedded / runtime static-link builds there are no
// DLLs and this collapses to nothing.
#ifdef DEKI_EDITOR
    #ifdef _WIN32
        #ifdef DEKI_NODEGRAPH_EXPORTS
            #define DEKI_NODEGRAPH_API __declspec(dllexport)
        #else
            #define DEKI_NODEGRAPH_API __declspec(dllimport)
        #endif
    #else
        #define DEKI_NODEGRAPH_API
    #endif
#else
    #define DEKI_NODEGRAPH_API
#endif
