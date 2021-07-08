// this file simply tests that the build system works correctly
// it includes and links against all the libraries required for the
// interpreter to run.
//
// a dummy function from each library is run to ensure
// that the library has been linked.

#ifdef OGM_LINK_TEST

#include <cstdio>
#include <vector>
#include <string>
#include <cstring>

#ifdef ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif

#ifdef OGM_FCL
#include <fcl/broadphase/broadphase_dynamic_AABB_tree.h>
#endif

#ifdef NATIVE_FILE_DIALOG
#include <nfd.h>
#endif

#ifdef OGM_CURL
#include <curl/curl.h>
#endif

#ifdef GFX_AVAILABLE
#include <glm/glm.hpp>
  #ifndef EMSCRIPTEN
    #include <GL/glew.h>
    #include <SDL2/SDL.h>
  #else
    #include <SDL.h>
  #endif
#endif

int main(int argc, char** argv)
{
  std::vector<std::string> verified;
  std::vector<std::string> unverified;

  #define _STR(s) #s
  #define STR(s) _STR(s)
  #define VLIST(macro, name) (strcmp(#macro, STR(macro)) ? verified : unverified).emplace_back(name)

  #ifdef ASSIMP
  {
    volatile Assimp::Importer imp;
  }
  #endif
  VLIST(ASSIMP, "assimp (Open Asset Import Library)");

  #ifdef OGM_FCL
  {
    fcl::DynamicAABBTreeCollisionManager m;
    m.setup();
  }
  #endif
  VLIST(OGM_FCL, "fcl (Flexible Collision Library)");

  #ifdef NATIVE_FILE_DIALOG
  {
    NFD_GetError();
  }
  #endif
  VLIST(OGM_FCL, "nfd (Native File Dialog)");


  #ifdef OGM_CURL
  {
    curl_easy_strerror(CURLE_FAILED_INIT);
  }
  #endif
  VLIST(OGM_FCL, "curl");

  #ifdef GFX_AVAILABLE
  {
    volatile glm::mat4 m(1.0);
    VLIST(GFX_AVAILABLE, "glm (OpenGL Mathematics)");

    SDL_Init(0);
    #ifdef SDL_MAJOR_VERSION
      VLIST(GFX_AVAILABLE, "SDL" + std::to_string(SDL_MAJOR_VERSION));
    #else
      VLIST(GFX_AVAILABLE, "SDL");
    #endif

    #ifndef EMSCRIPTEN
      glewInit();
      VLIST(GFX_AVAILABLE, "glew");
    #endif

    SDL_Quit();
  }
  #else
  VLIST(GFX_AVAILABLE, "glm (OpenGL Mathematics)");
  VLIST(GFX_AVAILABLE, "SDL");
  VLIST(GFX_AVAILABLE, "glew");
  #endif

  if (!verified.empty())
  {
    printf("The following libraries have been verified:\n");
    for (std::string& name : verified)
    {
      printf("  %s\n", name.c_str());
    }
  }

  if (!unverified.empty())
  {
    printf("The following libraries were not verified:\n");
    for (std::string& name : unverified)
    {
      printf("  %s\n", name.c_str());
    }
  }

  // fail if nothing was verified
  // (this generally indicates an error)
  return verified.empty();
}

#endif