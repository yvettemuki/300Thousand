#ifndef __LOADTEXTURE_H__
#define __LOADTEXTURE_H__

#include <string>
#include <windows.h>
#include "GL/glew.h"
#include "GL/gl.h"
#include "FreeImage.h"

GLuint LoadTexture(const std::string& fname);

GLuint createTexture(FIBITMAP* img, GLint format, int bits = 32, bool withMipMap = true);

#endif