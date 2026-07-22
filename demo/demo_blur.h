#pragma once

#include "../blur.hpp"

#if defined(IMGUI_VERSION) && !defined(BLUR_NO_IMGUI)

namespace DemoBlur {
    void RenderDemoWindow(Blur* blurInstance, bool* open = nullptr);
}

#endif
