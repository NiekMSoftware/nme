#include "nme/renderer/renderer.h"

namespace nme {

SubsystemError Renderer::startup() {
    return to_subsystem_error(init());
}

void Renderer::shutdown() {

}
RendererError Renderer::init() {
    return RendererError::None;
}

}  // namespace nme


