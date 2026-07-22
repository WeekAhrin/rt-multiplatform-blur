# RT Multiplatform Blur

![Android](https://img.shields.io/badge/Android-3DDC84?logo=android&logoColor=white)
![Desktop](https://img.shields.io/badge/Desktop-Windows%20%7C%20Linux%20%7C%20macOS-0078D4?logo=windows&logoColor=white)
![OpenGL%20ES](https://img.shields.io/badge/OpenGL%20ES-3.0-5586A4?logo=opengl&logoColor=white)
![OpenGL](https://img.shields.io/badge/OpenGL-3.3%20Core-5586A4?logo=opengl&logoColor=white)

Multiplatform OpenGL ES 3.0 / OpenGL 3.3 Core real-time blur library for Android and Desktop (C++23).

```<---                    --->```

_Q: What is the advantage of this library?_

**A: Key features:**
1) **Multiplatform API.** Support for Android (GLES 3.0) and Desktop Windows/Linux/macOS (OpenGL 3.3 Core).
2) **Zero Heavy Dependencies.** Standalone C++23 implementation.
3) **Dual Hardware Backend.** Supports `Hardware::GPU` (ping-pong FBOs) and `Hardware::CPU` (readback + software fallback).
4) **Multiple Blur Algorithms.** Built-in Gaussian, Box, Frosted, Kawase, Bokeh, Motion, Zoom, and Pixelate shaders.
5) **ImGui Overloads.** Direct rendering for `ImDrawList`, `ImRect`, and `ImVec2`.

```<---                    --->```

_Q: How to add to your project?_

**A: Ah, it's simple:**
1) Add sources to your build:
   - `blur.cpp`
   - `blur/cpu/cpu.cpp`
   - `blur/gpu/gpu.cpp`
2) Include `blur.hpp`:

```cpp
#include "blur.hpp"

static Blur* blur = nullptr;
if (!blur) {
    blur = new Blur(Hardware::GPU);
}

blur->process(x, y, w, h, 12.0f, 4);
```

```<---                    --->```

_Q: How to use with ImGui?_

**A: Pass your draw list or window rect directly:**

```cpp
blur->process(ImGui::GetWindowDrawList(), 14.0f, 12.0f, 4, Hardware::GPU);
```
