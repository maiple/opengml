#pragma once

#ifdef ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace ogm::interpreter
{    
    using namespace Assimp;
    
    // defined in fn_model_import.cpp
    extern Importer g_import;
    extern const aiScene* g_scene;
}

#endif