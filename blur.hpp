#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <span>

#include "blur/platform.hpp"

#if defined(IMGUI_VERSION) && !defined(BLUR_NO_IMGUI)
#  include "imgui.h"
#endif

enum class Hardware {
    GPU,
    CPU
};

class Blur {
public:
    enum class Type {
        Default = 0,
        Gaussian = 1,
        Box = 2,
        Frosted = 3,
        Kawase = 4,
        Bokeh = 5,
        Motion = 6,
        Zoom = 7,
        Pixelate = 8
    };

    explicit Blur(Hardware backend_in = Hardware::GPU);
    ~Blur();

    void process(int x, int y, int w, int h, float radius, int downscale);
    struct Rect {
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        float radius = 12.0f;
        int downscale = 4;
    };
    void process(const Rect& r);

#if defined(IMGUI_VERSION) && !defined(BLUR_NO_IMGUI)
    void process(ImDrawList* draw, float corner_radius, float radius, int downscale, Hardware hw);
    void process(const ImRect& rect, float radius, int downscale);
    void process(const ImVec2& pos, const ImVec2& size, float radius, int downscale);
#endif

    void process(int x, int y, int w, int h, float radius, int downscale, Type t) {
        type = t;
        process(x, y, w, h, radius, downscale);
    }

    void Process(int x, int y, int w, int h, float radius, int downscale) { process(x, y, w, h, radius, downscale); }
    void Process(const Rect& r) { process(r); }

    void release();
    void reset();

    void Release() { release(); }
    void Reset() { reset(); }

    GLuint tex = 0;
    Hardware backend = Hardware::GPU;
    Type type = Type::Default;

private:
    void ensure();
    void apply();

    friend void gpuEnsure(Blur& t);
    friend void gpuApply(Blur& t);
    friend void gpuRelease(Blur& t);

    friend void cpuApply(std::span<const uint8_t> input,
                         std::span<uint8_t> output,
                         int w, int h, float radius, int downscale, int type);

    int x_ = 0;
    int y_ = 0;
    int w_ = 0;
    int h_ = 0;
    float radius_ = 12.0f;
    int downscale_ = 4;

    int width_ = 0;
    int height_ = 0;

    std::unique_ptr<uint8_t[]> cpu_input_;
    std::unique_ptr<uint8_t[]> cpu_output_;
    size_t cpu_size_ = 0;

    GLuint src_tex_ = 0;
    GLuint ping_tex_ = 0;
    GLuint pong_tex_ = 0;
    GLuint fbo_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint blur_program_ = 0;
    GLuint blit_program_ = 0;
    GLint loc_tex_ = -1;
    GLint loc_direction_ = -1;
    GLint loc_texel_ = -1;
    GLint loc_blit_tex_ = -1;
    GLint loc_strength_ = -1;
    int blur_w_ = 0;
    int blur_h_ = 0;
    bool gpu_ready_ = false;
    bool cpu_ready_ = false;
    Type gpu_program_type_ = Type::Default;
};
