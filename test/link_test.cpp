// this file simply tests that the build system works correctly
// it includes and links against all the libraries required for the
// interpreter to run.
//
// a dummy function from each library is run to ensure
// that the library has been linked.

#ifdef OGM_LINK_TEST

#include <cstdio>

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
  printf("The following libraries have been verified...\n");

  #ifdef ASSIMP
  {
    volatile Assimp::Importer imp;
    printf("Assimp\n");
  }
  #endif

  #ifdef OGM_FCL
  {
    fcl::DynamicAABBTreeCollisionManager m;
    m.setup();
    printf("fcl\n");
  }
  #endif

  #ifdef NATIVE_FILE_DIALOG
  {
    NFD_GetError();
    printf("nfd\n");
  }
  #endif

  #ifdef OGM_CURL
  {
    curl_easy_strerror(CURLE_FAILED_INIT);
    printf("curl\n");
  }
  #endif

  #ifdef GFX_AVAILABLE
  {
    volatile glm::mat4 m(1.0);
    printf("glm\n");

    SDL_Init(0);
    #ifdef SDL_MAJOR_VERSION
      printf("SDL%d\n", SDL_MAJOR_VERSION);
    #else
      printf("SDL\n")
    #endif

    #ifndef EMSCRIPTEN
      glewInit();
      printf("Glew\n");
    #endif

    SDL_Quit();
  }
  #endif

  return 0;
}

#endif