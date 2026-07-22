# RT Multiplatform Blur

_Fast, lightweight, real-time blur library written in C++23 for Android and Desktop platforms._

```<---                    --->```

_Q: What is the advantage of this blur library?_

**A: Let's try to highlight the main ones:**
1) **Multiplatform API.** Native support for Android (OpenGL ES 3.0) and Desktop Windows/Linux/macOS (OpenGL 3.3 Core).
2) **Zero Heavy Dependencies.** Standalone C++23 implementation with bundled GLAD loader for desktop.
3) **Dual Hardware Backend.** Supports `Hardware::GPU` (ping-pong FBOs) and `Hardware::CPU` (multithreading / software fallback).
4) **Multiple Blur Algorithms.** Built-in Gaussian, Box, Frosted, Kawase, Bokeh, Motion, Zoom, and Pixelate shaders.
5) **ImGui Integration.** Easy `ImDrawList` rounded image blur rendering for overlays.

```<---                    --->```

_Q: How to add to your project?_

**A: Ah, it's simple:**
1) Include `blur.hpp` in your project.
2) Add `blur.cpp`, `blur/cpu/cpu.cpp`, and `blur/gpu/gpu.cpp` to your build targets.
3) Initialize and use in your render loop:

```cpp
#include "blur.hpp"

static Blur* blur = nullptr;
if (!blur) {
    blur = new Blur(Hardware::GPU);
    blur->BlurType = Blur::Type::Kawase;
}

blur->Process(x, y, width, height, 12.0f, 4);
// Use blur->Texture in your OpenGL / ImGui renderer
```

```<---                    --->```

_Q: How to use with ImGui?_

**A: Pass your draw list or window rect directly:**

```cpp
blur->Process(ImGui::GetWindowDrawList(), 10.0f, 15.0f, 4, Hardware::GPU);
```
