#include <iostream>
#include <vector>
#include <cmath>

#if defined(_WIN32)
#  include <windows.h>
#endif

#include "../blur.hpp"
#include "demo_blur.h"
#include "imgui.h"

// GLFW & GLAD Desktop Headers
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// ── Background GLSL Shader (Vibrant Anime / Cyberpunk Aesthetic) ──────────────
static const char* kBackgroundVS = R"(#version 330 core
layout(location=0) in vec2 aPos;
out vec2 vUV;
void main() {
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* kBackgroundFS = R"(#version 330 core
in vec2 vUV;
uniform float uTime;
uniform vec2 uResolution;
out vec4 FragColor;

void main() {
    vec2 st = (gl_FragCoord.xy - 0.5 * uResolution.xy) / uResolution.y;
    
    // Anime aesthetic gradient background (purple/pink/cyan)
    vec3 col1 = vec3(0.12, 0.08, 0.22); // Deep Indigo
    vec3 col2 = vec3(0.85, 0.35, 0.65); // Vibrant Pink
    vec3 col3 = vec3(0.40, 0.20, 0.75); // Electric Violet
    vec3 col4 = vec3(0.20, 0.80, 0.90); // Neon Cyan

    float t = uTime * 0.4;
    float dist = length(st);
    float angle = atan(st.y, st.x);
    
    // Animated glowing geometric patterns & radial waves
    float wave1 = sin(dist * 12.0 - t * 2.0 + sin(angle * 4.0)) * 0.5 + 0.5;
    float wave2 = cos(st.x * 8.0 + sin(st.y * 6.0 + t)) * 0.5 + 0.5;
    
    vec3 finalColor = mix(col1, col3, st.y + 0.5);
    finalColor = mix(finalColor, col2, wave1 * 0.6);
    finalColor += col4 * (0.15 / (dist + 0.2)) * wave2;
    
    // Floating glowing orb particles
    for (int i = 0; i < 6; i++) {
        float fi = float(i);
        vec2 p = vec2(
            sin(t * 0.5 + fi * 1.5) * 0.6,
            cos(t * 0.7 + fi * 2.1) * 0.4
        );
        float d = length(st - p);
        finalColor += vec3(0.9, 0.5 + 0.1 * fi, 0.8) * (0.015 / (d * d + 0.005));
    }

    FragColor = vec4(finalColor, 1.0);
}
)";

static GLuint CompileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}

static GLuint CreateProgram(const char* vs, const char* fs) {
    GLuint v = CompileShader(GL_VERTEX_SHADER, vs);
    GLuint f = CompileShader(GL_FRAGMENT_SHADER, fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

int main(int argc, char** argv) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "RT Multiplatform Blur — Desktop Demo", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // VSync

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // Custom aesthetic styling
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 10.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.08f, 0.15f, 0.85f);
    style.Colors[ImGuiCol_Header]   = ImVec4(0.45f, 0.20f, 0.60f, 0.65f);

    // Setup Quad for Background Shader
    GLuint bgProgram = CreateProgram(kBackgroundVS, kBackgroundFS);
    GLuint bgVao = 0, bgVbo = 0;
    glGenVertexArrays(1, &bgVao);
    glGenBuffers(1, &bgVbo);
    glBindVertexArray(bgVao);
    glBindBuffer(GL_ARRAY_BUFFER, bgVbo);
    const float quad[] = { -1,-1, 1,-1, -1,1, 1,1 };
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    // Create Main Blur Instance
    Blur blur(Hardware::GPU);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int displayW = 0, displayH = 0;
        glfwGetFramebufferSize(window, &displayW, &displayH);

        // 1. Render OpenGL Background
        glViewport(0, 0, displayW, displayH);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(bgProgram);
        glUniform1f(glGetUniformLocation(bgProgram, "uTime"), static_cast<float>(glfwGetTime()));
        glUniform2f(glGetUniformLocation(bgProgram, "uResolution"), static_cast<float>(displayW), static_cast<float>(displayH));
        glBindVertexArray(bgVao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // 2. Render ImGui & Blur Panels Over Background
        ImGui::NewFrame();
        
        demo_blur(&blur, displayW, displayH);

        ImGui::Render();

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &bgVao);
    glDeleteBuffers(1, &bgVbo);
    glDeleteProgram(bgProgram);

    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
