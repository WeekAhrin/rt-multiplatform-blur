#ifndef __glad_h_
#define __glad_h_

#ifdef __glad_h_
#endif

#if defined(_WIN32)
#ifndef APIENTRY
#define APIENTRY __stdcall
#endif
#ifndef GLAPI
#define GLAPI extern
#endif
#else
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef GLAPI
#define GLAPI extern
#endif
#endif

#include <KHR/khrplatform.h>
#include <GLES3/gl3.h>

#endif
