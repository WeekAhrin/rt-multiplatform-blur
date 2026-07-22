#include "../../blur.hpp"
#include "../math.hpp"
#include "shaders/shaders.hpp"

#include <algorithm>
#include <utility>

namespace {

[[nodiscard]] static constexpr const char* GetFragmentForType(Blur::Type t) noexcept {
    using T = Blur::Type;
    switch (t) {
        case T::Gaussian:  return BlurFS;
        case T::Box:       return BoxFS;
        case T::Frosted:   return FrostedFS;
        case T::Kawase:    return KawaseFS;
        case T::Bokeh:     return BokehFS;
        case T::Motion:    return MotionFS;
        case T::Zoom:      return ZoomFS;
        case T::Pixelate:  return PixelateFS;
        case T::Default:
        default:           return BlurFS;
    }
}

[[nodiscard]] static constexpr int GetPassesForType(Blur::Type t, float radius) noexcept {
    const int r = std::max(1, static_cast<int>(radius));
    using T = Blur::Type;
    switch (t) {
        case T::Gaussian:  return std::max(2, r / 3);
        case T::Box:       return std::max(1, r / 4);
        case T::Frosted:   return std::max(2, r / 5);
        case T::Kawase:    return std::max(2, std::min(8, r / 4 + 1));
        case T::Bokeh:     return std::max(2, r / 4);
        case T::Motion:    return std::max(1, r / 6);
        case T::Zoom:      return std::max(2, r / 5);
        case T::Pixelate:  return 1;
        case T::Default:
        default:           return 1;
    }
}

static void SetupTexture(GLuint tex, int w, int h) noexcept {
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

[[nodiscard]] static GLuint CompileShader(GLenum type, const char* src) noexcept {
    if (!src) return 0u;
    const GLuint s = glCreateShader(type);
    if (!s) return 0u;
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = GL_FALSE;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) { glDeleteShader(s); return 0u; }
    return s;
}

[[nodiscard]] static GLuint CreateProgram(const char* vs_src, const char* fs_src) noexcept {
    const GLuint vs = CompileShader(GL_VERTEX_SHADER,   vs_src);
    const GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fs_src);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return 0u;
    }
    const GLuint prog = glCreateProgram();
    if (!prog) { glDeleteShader(vs); glDeleteShader(fs); return 0u; }
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (ok != GL_TRUE) { glDeleteProgram(prog); return 0u; }
    return prog;
}

} // namespace

void GpuEnsure(Blur& t) {
    if (!t.gpuReady) {
        t.blurProgram = CreateProgram(BlurVS, GetFragmentForType(t.BlurType));
        t.blitProgram = CreateProgram(BlitVS, BlitFS);
        if (!t.blurProgram || !t.blitProgram) [[unlikely]] return;

        t.locTex       = glGetUniformLocation(t.blurProgram, "uTex");
        t.locDirection = glGetUniformLocation(t.blurProgram, "uDirection");
        t.locTexel     = glGetUniformLocation(t.blurProgram, "uTexel");
        t.locStrength  = glGetUniformLocation(t.blurProgram, "uStrength");
        t.locBlitTex   = glGetUniformLocation(t.blitProgram, "uTex");

        glGenFramebuffers(1, &t.fbo);
        glGenTextures(1, &t.srcTex);
        glGenTextures(1, &t.pingTex);
        glGenTextures(1, &t.pongTex);
        if (t.Texture == 0) glGenTextures(1, &t.Texture);
        t.tex = t.Texture;

        static constexpr float Quad[] = {
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f,  1.0f,  1.0f, 1.0f,
        };
        glGenVertexArrays(1, &t.vao);
        glGenBuffers(1, &t.vbo);
        glBindVertexArray(t.vao);
        glBindBuffer(GL_ARRAY_BUFFER, t.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Quad), Quad, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        t.gpuReady       = true;
        t.gpuProgramType = t.BlurType;

    } else if (t.gpuProgramType != t.BlurType) {
        if (t.blurProgram) { glDeleteProgram(t.blurProgram); t.blurProgram = 0; }
        t.blurProgram = CreateProgram(BlurVS, GetFragmentForType(t.BlurType));
        if (!t.blurProgram) [[unlikely]] { t.gpuReady = false; return; }
        t.locTex       = glGetUniformLocation(t.blurProgram, "uTex");
        t.locDirection = glGetUniformLocation(t.blurProgram, "uDirection");
        t.locTexel     = glGetUniformLocation(t.blurProgram, "uTexel");
        t.locStrength  = glGetUniformLocation(t.blurProgram, "uStrength");
        t.gpuProgramType = t.BlurType;
    }

    if (t.width > 0 && t.height > 0) {
        const int ds = std::max(1, t.downscale);
        t.blurW   = std::max(1, t.width  / ds);
        t.blurH   = std::max(1, t.height / ds);
        SetupTexture(t.srcTex,  t.width,  t.height);
        SetupTexture(t.Texture, t.width,  t.height);
        t.tex = t.Texture;
        SetupTexture(t.pingTex, t.blurW, t.blurH);
        SetupTexture(t.pongTex, t.blurW, t.blurH);
    }
}

void GpuRelease(Blur& t) {
    if (t.blurProgram) glDeleteProgram(t.blurProgram);
    if (t.blitProgram) glDeleteProgram(t.blitProgram);
    t.blurProgram = t.blitProgram = 0;
    if (t.fbo)     glDeleteFramebuffers(1, &t.fbo);
    if (t.srcTex)  glDeleteTextures(1, &t.srcTex);
    if (t.pingTex) glDeleteTextures(1, &t.pingTex);
    if (t.pongTex) glDeleteTextures(1, &t.pongTex);
    if (t.vao)     glDeleteVertexArrays(1, &t.vao);
    if (t.vbo)     glDeleteBuffers(1, &t.vbo);
    t.fbo = t.srcTex = t.pingTex = t.pongTex = 0;
    t.vao = t.vbo = 0;
    t.gpuReady       = false;
    t.gpuProgramType = Blur::Type::Default;
}

void GpuApply(Blur& t) {
    if (!t.gpuReady) [[unlikely]] return;

    GLint  prev_program        = 0;
    GLint  prev_fbo            = 0;
    GLint  prev_read_fbo       = 0;
    GLint  prev_draw_fbo       = 0;
    GLint  prev_viewport[4]    = {};
    GLint  prev_scissor[4]     = {};
    GLint  prev_active         = 0;
    GLint  prev_tex_active     = 0;
    GLint  prev_tex0           = 0;
    GLint  prev_vao            = 0;
    GLint  prev_vbo            = 0;
    GLint  prev_ebo            = 0;
    GLint  prev_bsrc_rgb       = 0, prev_bdst_rgb       = 0;
    GLint  prev_bsrc_a         = 0, prev_bdst_a         = 0;
    GLint  prev_beq_rgb        = 0, prev_beq_a          = 0;
    GLfloat prev_blend_col[4]  = {};
    const GLboolean prev_blend   = glIsEnabled(GL_BLEND);
    const GLboolean prev_depth   = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean prev_cull    = glIsEnabled(GL_CULL_FACE);
    const GLboolean prev_scissor_en = glIsEnabled(GL_SCISSOR_TEST);
    const GLboolean prev_stencil = glIsEnabled(GL_STENCIL_TEST);
    GLboolean prev_color_mask[4] = {1,1,1,1};

    glGetIntegerv(GL_CURRENT_PROGRAM,           &prev_program);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,        &prev_fbo);
#ifdef GL_READ_FRAMEBUFFER_BINDING
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,   &prev_read_fbo);
#endif
#ifdef GL_DRAW_FRAMEBUFFER_BINDING
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,   &prev_draw_fbo);
#endif
    glGetIntegerv(GL_VIEWPORT,                   prev_viewport);
    glGetIntegerv(GL_SCISSOR_BOX,                prev_scissor);
    glGetIntegerv(GL_ACTIVE_TEXTURE,             &prev_active);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,         &prev_tex_active);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,         &prev_tex0);
    glActiveTexture(static_cast<GLenum>(prev_active));
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING,       &prev_vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING,       &prev_vbo);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prev_ebo);
    glGetIntegerv(GL_BLEND_SRC_RGB,              &prev_bsrc_rgb);
    glGetIntegerv(GL_BLEND_DST_RGB,              &prev_bdst_rgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA,            &prev_bsrc_a);
    glGetIntegerv(GL_BLEND_DST_ALPHA,            &prev_bdst_a);
    glGetIntegerv(GL_BLEND_EQUATION_RGB,         &prev_beq_rgb);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA,       &prev_beq_a);
    if (glGetFloatv)   glGetFloatv  (GL_BLEND_COLOR,   prev_blend_col);
    if (glGetBooleanv) glGetBooleanv(GL_COLOR_WRITEMASK, prev_color_mask);

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    if (glColorMask) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    const int src_y =
#if defined(BLUR_FLIP_Y)
        t.height - t.y - t.h;
#else
        t.y;
#endif
    glBindTexture(GL_TEXTURE_2D, t.srcTex);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, t.x, src_y, t.w, t.h);

    glUseProgram(t.blurProgram);
    glBindVertexArray(t.vao);
    glViewport(0, 0, t.blurW, t.blurH);
    glBindFramebuffer(GL_FRAMEBUFFER, t.fbo);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(t.locTex, 0);
    if (t.locStrength >= 0 && glUniform1f)
        glUniform1f(t.locStrength, std::max(1.0f, t.radius * 0.35f));

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.pingTex, 0);
    glBindTexture(GL_TEXTURE_2D, t.srcTex);
    glUniform2f(t.locTexel, 1.0f / static_cast<float>(t.width), 1.0f / static_cast<float>(t.height));
    glUniform2f(t.locDirection, 1.0f, 0.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    const int n_passes = GetPassesForType(t.BlurType, t.radius);
    GLuint in_tex  = t.pingTex;
    GLuint out_tex = t.pongTex;

    for (int i = 0; i < n_passes; ++i) {
        float o = 1.0f;
        if      (t.BlurType == Blur::Type::Kawase)
            o = 1.0f + (n_passes > 1 ? static_cast<float>(i) / static_cast<float>(n_passes - 1) : 0.0f) * std::max(1.0f, t.radius * 0.22f);
        else if (t.BlurType == Blur::Type::Motion)   o = std::max(1.0f, t.radius * 0.20f);
        else if (t.BlurType == Blur::Type::Pixelate) o = std::max(1.0f, t.radius * 0.50f);
        else if (t.BlurType == Blur::Type::Bokeh || t.BlurType == Blur::Type::Zoom)
            o = std::max(1.0f, t.radius * 0.16f);

        const float tx = 1.0f / static_cast<float>(t.blurW);
        const float ty = 1.0f / static_cast<float>(t.blurH);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, out_tex, 0);
        glBindTexture(GL_TEXTURE_2D, in_tex);
        glUniform2f(t.locTexel, tx, ty);
        if (t.BlurType == Blur::Type::Motion)
            glUniform2f(t.locDirection, 1.0f, 0.35f);
        else
            glUniform2f(t.locDirection, o, 0.0f);
        if (t.locStrength >= 0 && glUniform1f)
            glUniform1f(t.locStrength, std::max(1.0f, o));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        std::swap(in_tex, out_tex);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, out_tex, 0);
        glBindTexture(GL_TEXTURE_2D, in_tex);
        if (t.BlurType == Blur::Type::Motion)
            glUniform2f(t.locDirection, 1.0f, 0.35f);
        else if (t.BlurType == Blur::Type::Pixelate)
            glUniform2f(t.locDirection, 1.0f, 0.0f);
        else
            glUniform2f(t.locDirection, 0.0f, o);
        if (t.locStrength >= 0 && glUniform1f)
            glUniform1f(t.locStrength, std::max(1.0f, o));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        std::swap(in_tex, out_tex);
    }

    glUseProgram(t.blitProgram);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.Texture, 0);
    glViewport(0, 0, t.width, t.height);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, in_tex);
    glUniform1i(t.locBlitTex, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
#ifdef GL_READ_FRAMEBUFFER
    glBindFramebuffer(GL_READ_FRAMEBUFFER, prev_read_fbo);
#endif
#ifdef GL_DRAW_FRAMEBUFFER
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_draw_fbo);
#endif
    glUseProgram(prev_program);
    glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);
    if (glScissor) glScissor(prev_scissor[0], prev_scissor[1], prev_scissor[2], prev_scissor[3]);
    if (glColorMask) glColorMask(prev_color_mask[0], prev_color_mask[1], prev_color_mask[2], prev_color_mask[3]);
    glActiveTexture(static_cast<GLenum>(prev_active));
    glBindTexture(GL_TEXTURE_2D, prev_tex_active);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, prev_tex0);
    glActiveTexture(static_cast<GLenum>(prev_active));
    glBindVertexArray(prev_vao);
    glBindBuffer(GL_ARRAY_BUFFER, prev_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prev_ebo);
    if (glBlendFuncSeparate)
        glBlendFuncSeparate((GLenum)prev_bsrc_rgb, (GLenum)prev_bdst_rgb, (GLenum)prev_bsrc_a, (GLenum)prev_bdst_a);
    else if (glBlendFunc)
        glBlendFunc((GLenum)prev_bsrc_rgb, (GLenum)prev_bdst_rgb);
    if (glBlendEquationSeparate)
        glBlendEquationSeparate((GLenum)prev_beq_rgb, (GLenum)prev_beq_a);
    else if (glBlendEquation)
        glBlendEquation((GLenum)prev_beq_rgb);
    if (glBlendColor)
        glBlendColor(prev_blend_col[0], prev_blend_col[1], prev_blend_col[2], prev_blend_col[3]);
    prev_blend      ? glEnable(GL_BLEND)        : glDisable(GL_BLEND);
    prev_depth      ? glEnable(GL_DEPTH_TEST)   : glDisable(GL_DEPTH_TEST);
    prev_cull       ? glEnable(GL_CULL_FACE)    : glDisable(GL_CULL_FACE);
    prev_scissor_en ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);
    prev_stencil    ? glEnable(GL_STENCIL_TEST)  : glDisable(GL_STENCIL_TEST);
}
