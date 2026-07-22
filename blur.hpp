#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <span>

#include "blur/platform.hpp"

#if defined(IMGUI_VERSION) && !defined(BLUR_NO_IMGUI)
#  include "imgui.h"
#endif

enum class Hardware : uint8_t {
    GPU,
    CPU
};

class Blur {
public:
    enum class Type : uint8_t {
        Default   = 0,
        Gaussian  = 1,
        Box       = 2,
        Frosted   = 3,
        Kawase    = 4,
        Bokeh     = 5,
        Motion    = 6,
        Zoom      = 7,
        Pixelate  = 8
    };

    struct Rect {
        int   X         = 0;
        int   Y         = 0;
        int   W         = 0;
        int   H         = 0;
        float Radius    = 12.0f;
        int   Downscale = 4;
    };

    explicit Blur(Hardware backend = Hardware::GPU) noexcept;
    ~Blur();

    Blur(const Blur&)            = delete;
    Blur& operator=(const Blur&) = delete;
    Blur(Blur&&)                 = delete;
    Blur& operator=(Blur&&)      = delete;

    void Process(int posX, int posY, int widthVal, int heightVal, float radiusVal, int downscaleVal);
    void Process(const Rect& rectVal);

    void process(int posX, int posY, int widthVal, int heightVal, float radiusVal, int downscaleVal) {
        Process(posX, posY, widthVal, heightVal, radiusVal, downscaleVal);
    }
    void process(const Rect& rectVal) { Process(rectVal); }

#if defined(IMGUI_VERSION) && !defined(BLUR_NO_IMGUI)
    void Process(ImDrawList* draw, float cornerRadius, float radiusVal, int downscaleVal, Hardware hw);
    void Process(const ImRect& rectVal, float radiusVal, int downscaleVal);
    void Process(const ImVec2& posVal, const ImVec2& sizeVal, float radiusVal, int downscaleVal);

    void process(ImDrawList* draw, float cornerRadius, float radiusVal, int downscaleVal, Hardware hw) {
        Process(draw, cornerRadius, radiusVal, downscaleVal, hw);
    }
    void process(const ImRect& rectVal, float radiusVal, int downscaleVal) {
        Process(rectVal, radiusVal, downscaleVal);
    }
    void process(const ImVec2& posVal, const ImVec2& sizeVal, float radiusVal, int downscaleVal) {
        Process(posVal, sizeVal, radiusVal, downscaleVal);
    }
#endif

    void Release();
    void Reset();

    void release() { Release(); }
    void reset() { Reset(); }

    GLuint   Texture  = 0;
    GLuint   tex      = 0;
    Hardware Backend  = Hardware::GPU;
    Type     BlurType = Type::Default;

private:
    void Ensure();
    void Apply();

    friend void GpuEnsure (Blur& t);
    friend void GpuApply  (Blur& t);
    friend void GpuRelease(Blur& t);

    friend void CpuApply(std::span<const uint8_t> input,
                         std::span<uint8_t>       output,
                         int widthVal, int heightVal,
                         float radiusVal, int downscaleVal, int typeVal);

    int   x         = 0;
    int   y         = 0;
    int   w         = 0;
    int   h         = 0;
    float radius    = 12.0f;
    int   downscale = 4;

    int   width     = 0;
    int   height    = 0;

    std::unique_ptr<uint8_t[]> cpuInput;
    std::unique_ptr<uint8_t[]> cpuOutput;
    std::size_t                cpuSize   = 0;

    GLuint srcTex         = 0;
    GLuint pingTex        = 0;
    GLuint pongTex        = 0;
    GLuint fbo            = 0;
    GLuint vao            = 0;
    GLuint vbo            = 0;
    GLuint blurProgram    = 0;
    GLuint blitProgram    = 0;
    GLint  locTex         = -1;
    GLint  locDirection   = -1;
    GLint  locTexel       = -1;
    GLint  locBlitTex     = -1;
    GLint  locStrength    = -1;
    int    blurW          = 0;
    int    blurH          = 0;
    bool   gpuReady       = false;
    bool   cpuReady       = false;
    Type   gpuProgramType = Type::Default;
};
