#include "blur.hpp"
#include "blur/math.hpp"

#include <cstddef>
#include <span>
#include <utility>

void CpuApply(std::span<const uint8_t> input,
              std::span<uint8_t>       output,
              int widthVal, int heightVal,
              float radiusVal, int downscaleVal, int typeVal);

Blur::Blur(Hardware backendIn) noexcept : Backend(backendIn) {}

Blur::~Blur() {
    Release();
}

void Blur::Process(int posX, int posY, int widthVal, int heightVal, float radiusVal, int downscaleVal) {
    x         = posX;
    y         = posY;
    w         = widthVal;
    h         = heightVal;
    radius    = radiusVal;
    downscale = downscaleVal;
    Apply();
}

void Blur::Process(const Rect& rectVal) {
    Process(rectVal.X, rectVal.Y, rectVal.W, rectVal.H, rectVal.Radius, rectVal.Downscale);
}

#if defined(IMGUI_VERSION) && !defined(BLUR_NO_IMGUI)
void Blur::Process(ImDrawList* draw, float cornerRadius,
                   float radiusVal, int downscaleVal, Hardware hw) {
    if (!draw) return;
    Backend     = hw;
    ImVec2 pos  = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    Process(static_cast<int>(pos.x),  static_cast<int>(pos.y),
            static_cast<int>(size.x), static_cast<int>(size.y),
            radiusVal, downscaleVal);
    draw->AddImageRounded(
        reinterpret_cast<ImTextureID>(static_cast<intptr_t>(Texture)),
        pos,
        ImVec2(pos.x + size.x, pos.y + size.y),
        ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f),
        IM_COL32(255, 255, 255, 255),
        cornerRadius,
        ImDrawFlags_RoundCornersAll
    );
}

void Blur::Process(const ImRect& rectVal, float radiusVal, int downscaleVal) {
    Process(static_cast<int>(rectVal.Min.x), static_cast<int>(rectVal.Min.y),
            static_cast<int>(rectVal.Max.x - rectVal.Min.x),
            static_cast<int>(rectVal.Max.y - rectVal.Min.y),
            radiusVal, downscaleVal);
}

void Blur::Process(const ImVec2& posVal, const ImVec2& sizeVal, float radiusVal, int downscaleVal) {
    Process(static_cast<int>(posVal.x),  static_cast<int>(posVal.y),
            static_cast<int>(sizeVal.x), static_cast<int>(sizeVal.y),
            radiusVal, downscaleVal);
}
#endif

void Blur::Release() {
    GpuRelease(*this);
    if (Texture) { glDeleteTextures(1, &Texture); Texture = 0; }
    tex = 0;
    cpuInput.reset();
    cpuOutput.reset();
    cpuSize  = 0;
    cpuReady = false;
}

void Blur::Reset() {
    Release();
    x = y = w = h = 0;
    width = height = blurW = blurH = 0;
}

void Blur::Ensure() {
    if (Backend == Hardware::GPU) {
        GpuEnsure(*this);
    }
    if (Backend == Hardware::CPU || !cpuReady) {
        if (Texture == 0) glGenTextures(1, &Texture);
        tex = Texture;
        glBindTexture(GL_TEXTURE_2D, Texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        if (width > 0 && height > 0) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                         width, height, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }
        cpuReady = true;
    }
}

void Blur::Apply() {
    if (w <= 0 || h <= 0) [[unlikely]] return;

    if (width != w || height != h) {
        width  = w;
        height = h;
    }
    Ensure();

    if (Backend == Hardware::CPU) {
        const std::size_t required =
            static_cast<std::size_t>(w) *
            static_cast<std::size_t>(h) * 4u;

        if (cpuSize != required) {
            cpuInput  = std::make_unique<uint8_t[]>(required);
            cpuOutput = std::make_unique<uint8_t[]>(required);
            cpuSize   = required;
        }

        glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, cpuInput.get());
        CpuApply(
            std::span<const uint8_t>(cpuInput.get(),  cpuSize),
            std::span<uint8_t>      (cpuOutput.get(), cpuSize),
            w, h, radius, downscale,
            static_cast<int>(std::to_underlying(BlurType))
        );
        glBindTexture(GL_TEXTURE_2D, Texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h,
                        GL_RGBA, GL_UNSIGNED_BYTE, cpuOutput.get());
        return;
    }

    GpuApply(*this);
}
