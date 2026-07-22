#pragma once

#if defined(__ANDROID__) || defined(BLUR_FORCE_GLES)
#  define BLUR_PLATFORM_ANDROID 1
#  if defined(BLUR_RENDERER_NO_GLES)
#    include "../good_luck/glegl/glegl.h"
#  else
#    include <GLES3/gl3.h>
#    include <GLES3/gl3ext.h>
#  endif
#else
#  define BLUR_PLATFORM_DESKTOP 1
#  if defined(BLUR_GL_LOADER_HEADER)
#    include BLUR_GL_LOADER_HEADER
#  else
#    include <glad/glad.h>
#  endif
#endif
