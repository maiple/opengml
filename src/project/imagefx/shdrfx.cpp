#ifdef SHDRFX_SUPPORT
#include "shdrfx.hpp"

#include "ogm/common/types.hpp"
#include "common/sdl.hpp"

#include <iostream>

namespace ogm::project::fx
{
    static SDL_Window* g_window;
    static SDL_Renderer* g_renderer;
    static SDL_GLContext g_context;
    static unsigned int VBO;
    static unsigned int VAO;

    bool shdrfx_begin()
    {
        if (
            SDL_Init(
                SDL_INIT_VIDEO
            ) != 0
        )
        {
            return 1;
        }
        
        #ifdef EMSCRIPTEN
            SDL_CreateWindowAndRenderer(1, 1, SDL_WINDOW_HIDDEN, &g_window, &g_renderer);
        #else
            g_window = SDL_CreateWindow(
                "opengml compiler", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        		1, 1, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        #endif

        if (!g_window)
        {
            printf("Unable to create SDL window.\n");
            return true;
        }
        
        #ifdef EMSCRIPTEN
        if (!g_renderer)
        {
            // TODO: do we care about the renderer?
            printf("Unable to create SDL renderer.\n");
            return true;
        }
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        #else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        g_context = SDL_GL_CreateContext(g_window);
        
        // init GLEW
        glewExperimental = GL_TRUE;
        GLenum result = glewInit();
        if (result != GLEW_OK)
        {
            std::cerr << "Error (glew): " << glewGetErrorString(result) << std::endl;
            std::cerr << "could not initialize glew.\n";
            return false;
        }


        if (!GLEW_VERSION_3_0)
        {
            std::cerr << "Error (glew): OpenGL 3.0 not supported.\n";
            return true;
        }
        #endif

        // square
        static const float vertices[] = {
            -1.0f, -1.0f, 0.0f,
             1.0f, -1.0f, 0.0f,
             1.0f,  1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f
        }; 
        
        // VBO
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        // VAO
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        return 0;
    }
    
    void render(const char * vshader, const char* fshader, const uint8_t* data, uint16_t iw, uint16_t ih, uint16_t ow, uint16_t oh)
    {
        //glUseProgram(shader_program);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 4);
    }
    
    void shdrfx_end()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        SDL_Quit();
    }
}

#endif