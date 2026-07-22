#include "../../blur.hpp"

#if defined(IMGUI_VERSION) && !defined(BLUR_NO_IMGUI)

void RenderImGuiBlurOverlay(Blur* blurInstance) {
    if (!blurInstance) return;

    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);

    ImGui::Begin("Blurred Panel Example");

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float cornerRadius = 12.0f;
    float blurRadius = 16.0f;
    int downscaleFactor = 4;

    blurInstance->Process(drawList, cornerRadius, blurRadius, downscaleFactor, Hardware::GPU);

    ImGui::Text("This window has a blurred background!");
    ImGui::Text("Rendered via ImDrawList callback.");
    if (ImGui::Button("Click Me")) {
    }

    ImGui::End();
}

#endif
