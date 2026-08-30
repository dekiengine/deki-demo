#include "deki-nodegraph/NodeFactory.h"

namespace SceneFormat {

NodeFactory& NodeFactory::Instance() {
    static NodeFactory instance;
    return instance;
}

} // namespace SceneFormat
