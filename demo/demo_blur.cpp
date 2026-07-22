#include "demo_blur.h"

#if defined(IMGUI_VERSION) && !defined(BLUR_NO_IMGUI)

namespace DemoBlur {

void RenderDemoWindow(Blur* blurInstance, bool* open) {
    if (!blurInstance) return;

    if (open && !*open) return;

    ImGui::SetNextWindowSize(ImVec2(400, 320), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Blur Configuration", open)) {
        ImGui::End();
        return;
    }

    static int currentBackend = static_cast<int>(blurInstance->Backend);
    const char* backends[] = { "GPU (OpenGL)", "CPU (Software)" };
    if (ImGui::Combo("Backend", &currentBackend, backends, IM_ARRAYSIZE(backends))) {
        blurInstance->Backend = static_cast<Hardware>(currentBackend);
    }

    static int currentType = static_cast<int>(blurInstance->BlurType);
    const char* types[] = {
        "Default", "Gaussian", "Box", "Frosted",
        "Kawase", "Bokeh", "Motion", "Zoom", "Pixelate"
    };
    if (ImGui::Combo("Algorithm", &currentType, types, IM_ARRAYSIZE(types))) {
        blurInstance->BlurType = static_cast<Blur::Type>(currentType);
    }

    static float radius = 12.0f;
    if (ImGui::SliderFloat("Blur Radius", &radius, 1.0f, 64.0f, "%.1f")) {
    }

    static int downscale = 4;
    if (ImGui::SliderInt("Downscale Factor", &downscale, 1, 8)) {
    }

    ImGui::Separator();
    ImGui::Text("Status: %s | Type: %s",
                blurInstance->Backend == Hardware::GPU ? "GPU (Fast)" : "CPU (Fallback)",
                types[static_cast<int>(blurInstance->BlurType)]);

    ImGui::Separator();
    ImVec2 previewPos = ImGui::GetCursorScreenPos();
    ImVec2 previewSize(ImGui::GetContentRegionAvail().x, 100);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(previewPos, ImVec2(previewPos.x + previewSize.x, previewPos.y + previewSize.y), IM_COL32(40, 40, 45, 255));

    blurInstance->Process(drawList, 8.0f, radius, downscale, blurInstance->Backend);

    ImGui::Dummy(previewSize);
    ImGui::End();
}

} // namespace DemoBlur

#endif
