#pragma once

#ifdef GFX_AVAILABLE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#ifndef EMSCRIPTEN
    #include <GL/glew.h>
    #include <SDL2/SDL.h>
    #ifdef GFX_TEXT_AVAILABLE
        #include <SDL2/SDL_ttf.h>
        extern unsigned char _binary_Default_Font_ttf[];
        extern unsigned int _binary_Default_Font_ttf_len;
    #endif
#else
    #define GL_GLEXT_PROTOTYPES 1
    #include <SDL.h>
    #include <SDL_image.h>
    #include <SDL_opengles2.h>
    
    #define glGenVertexArrays glGenVertexArraysOES
    #define glDeleteVertexArrays glDeleteVertexArraysOES
    #define glBindVertexArray glBindVertexArrayOES
#endif

#endif