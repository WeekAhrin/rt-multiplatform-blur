**Advanced Multiplatform Real-Time Blur & FX Base**

_High-performance post-processing blur and visual effects base for Android (OpenGL ES 3.0) and Desktop (OpenGL 3.3 Core) with seamless ImGui integration._

```<---                    --->```

_Q: What is the advantage of this base?_

**A: Let's try to highlight the main ones:**
1) **Multiplatform Core.** Native OpenGL ES 3.0 (Android) and OpenGL 3.3 Core (Desktop Windows/Linux/macOS) rendering backends.
2) **Rich Visual FX Suite.** Supports 9 real-time shader algorithms:
   - **Gaussian Blur** — Classic separable Gaussian blur
   - **Kawase Blur** — High-speed dual-filter bloom/blur
   - **Frosted Glass** — Jittered noise frosted blur
   - **Bokeh Effect** — Circular lens light spread
   - **Motion Blur** — Directional motion smear
   - **Radial Zoom Blur** — Center-focused radial zoom blur
   - **Pixelate Effect** — Retro block-averaging pixelation
   - **Box Blur** — Fast 5-tap box filter
3) **Dual Hardware Execution.** Choose between `Hardware::GPU` (ping-pong framebuffers) and `Hardware::CPU` (software fallback).
4) **ImGui Ready.** Direct overloads for `ImDrawList`, `ImRect`, and `ImVec2` for window overlays & UI elements.
5) **Zero Mandatory Dependencies.** Drop-in headers and sources into any project setup without forced vendor libraries.

```<---                    --->```

_Q: How to add a Blur or FX to your project?_

**A: Ah, it's simple:**
1) Add `blur.cpp`, `blur/cpu/cpu.cpp`, and `blur/gpu/gpu.cpp` to your build.
2) Include `blur.hpp` and instantiate:

```cpp
#include "blur.hpp"

static Blur* blur = nullptr;
if (!blur) {
    blur = new Blur(Hardware::GPU);
    blur->type = Blur::Type::Kawase; // Or Bokeh, Pixelate, Frosted, Zoom, Motion...
}

blur->process(x, y, width, height, 12.0f, 4);
// Use blur->tex in your OpenGL or ImGui renderer
```

```<---                    --->```

_Q: How to use with ImGui?_

**A: Pass your draw list or window rect directly:**

```cpp
blur->process(ImGui::GetWindowDrawList(), 14.0f, 12.0f, 4, Hardware::GPU);
```

```<---                    --->```

_Q: How to configure Desktop OpenGL loader?_

**A: Use your preferred GL loader header:**

```cpp
// By default <glad/glad.h> is included on desktop.
// You can supply your custom loader header via compiler flag:
// -DBLUR_GL_LOADER_HEADER="my_gl_loader.h"
```
