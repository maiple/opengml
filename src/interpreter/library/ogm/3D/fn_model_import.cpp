#include "assimp.h"

#include "interpreter/library/libpre.h"
    #include "fn_model_import.h"
#include "interpreter/library/libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"
#include "ogm/interpreter/Executor.hpp"
#include "ogm/interpreter/execute.hpp"
#include "ogm/interpreter/display/Display.hpp"

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

#define frame staticExecutor.m_frame

#ifdef ASSIMP
using namespace Assimp;
#endif

#ifdef ASSIMP
namespace ogm::interpreter
{
    Importer g_import{ };
    const aiScene* g_scene{ nullptr };
}
#endif

void ogm::interpreter::fn::ogm_model_import_available(VO out)
{
    #ifdef ASSIMP
    out = true;
    #else
    out = false;
    #endif
}

void ogm::interpreter::fn::ogm_model_import(VO out, V vpath)
{
    #ifdef ASSIMP
    std::string path = frame.m_fs.resolve_file_path(
        vpath.castCoerce<std::string>()
    );
    
    g_scene = g_import.ReadFile(
        path.c_str(),
        aiProcess_Triangulate
    );
    
    if(!g_scene || g_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !g_scene->mRootNode) 
    {
        std::cout << "ERROR::ASSIMP::" << g_import.GetErrorString() << std::endl;
        out = -1.0;
        return;
    }
    
    out = 0.0;
    #else
    out = -1.0;
    #endif
}

void ogm::interpreter::fn::ogm_model_import_mesh_count(VO out)
{
    #ifdef ASSIMP
    out = static_cast<real_t>(g_scene->mNumMeshes);
    #endif
}

void ogm::interpreter::fn::ogm_model_import_mesh_vertex_count(VO out, V vmesh_index)
{
    #ifdef ASSIMP
    size_t mesh_index = vmesh_index.castCoerce<size_t>();
    if (mesh_index >= g_scene->mNumMeshes)
    {
        out = 0.0;
    }
    else
    {
        aiMesh* mesh = g_scene->mMeshes[mesh_index];
        out = static_cast<real_t>(mesh->mNumVertices);
    }
    #endif
}

void ogm::interpreter::fn::ogm_model_import_mesh_name(VO out, V vmesh_index)
{
    #ifdef ASSIMP
    size_t mesh_index = vmesh_index.castCoerce<size_t>();
    if (mesh_index >= g_scene->mNumMeshes)
    {
        out = "";
    }
    else
    {
        aiMesh* mesh = g_scene->mMeshes[mesh_index];
        out = static_cast<std::string>(mesh->mName.C_Str());
    }
    #endif
}

void ogm::interpreter::fn::ogm_model_import_mesh_vertex_x(VO out, V vmesh_index, V vvertex_index)
{
    #ifdef ASSIMP
    const size_t mesh_index = vmesh_index.castCoerce<size_t>();
    const size_t vertex_index = vvertex_index.castCoerce<size_t>();
    if (mesh_index >= g_scene->mNumMeshes)
    {
        out = 0.0f;
    }
    else
    {
        aiMesh* mesh = g_scene->mMeshes[mesh_index];
        if (vertex_index >= mesh->mNumVertices)
        {
            out = 0.0f;
        }
        
        aiVector3D& vec = mesh->mVertices[vertex_index];
        out = static_cast<real_t>(vec.x);
    }
    #endif
}

void ogm::interpreter::fn::ogm_model_import_mesh_vertex_y(VO out, V vmesh_index, V vvertex_index)
{
    #ifdef ASSIMP
    const size_t mesh_index = vmesh_index.castCoerce<size_t>();
    const size_t vertex_index = vvertex_index.castCoerce<size_t>();
    if (mesh_index >= g_scene->mNumMeshes)
    {
        out = 0.0f;
    }
    else
    {
        aiMesh* mesh = g_scene->mMeshes[mesh_index];
        if (vertex_index >= mesh->mNumVertices)
        {
            out = 0.0f;
        }
        
        aiVector3D& vec = mesh->mVertices[vertex_index];
        out = static_cast<real_t>(vec.y);
    }
    #endif
}

void ogm::interpreter::fn::ogm_model_import_mesh_vertex_z(VO out, V vmesh_index, V vvertex_index)
{
    #ifdef ASSIMP
    const size_t mesh_index = vmesh_index.castCoerce<size_t>();
    const size_t vertex_index = vvertex_index.castCoerce<size_t>();
    if (mesh_index >= g_scene->mNumMeshes)
    {
        out = 0.0f;
    }
    else
    {
        aiMesh* mesh = g_scene->mMeshes[mesh_index];
        if (vertex_index >= mesh->mNumVertices)
        {
            out = 0.0f;
        }
        
        aiVector3D& vec = mesh->mVertices[vertex_index];
        out = static_cast<real_t>(vec.z);
    }
    #endif
}

void ogm::interpreter::fn::ogm_model_import_mesh_vertex_nx(VO out, V vmesh_index, V vvertex_index)
{
    #ifdef ASSIMP
    const size_t mesh_index = vmesh_index.castCoerce<size_t>();
    const size_t vertex_index = vvertex_index.castCoerce<size_t>();
    if (mesh_index >= g_scene->mNumMeshes)
    {
        out = 0.0f;
    }
    else
    {
        aiMesh* mesh = g_scene->mMeshes[mesh_index];
        if (vertex_index >= mesh->mNumVertices)
        {
            out = 0.0f;
        }
        
        aiVector3D& vec = mesh->mNormals[vertex_index];
        out = static_cast<real_t>(vec.x);
    }
    #endif
}

void ogm::interpreter::fn::ogm_model_import_mesh_vertex_ny(VO out, V vmesh_index, V vvertex_index)
{
    #ifdef ASSIMP
    const size_t mesh_index = vmesh_index.castCoerce<size_t>();
    const size_t vertex_index = vvertex_index.castCoerce<size_t>();
    if (mesh_index >= g_scene->mNumMeshes)
    {
        out = 0.0f;
    }
    else
    {
        aiMesh* mesh = g_scene->mMeshes[mesh_index];
        if (vertex_index >= mesh->mNumVertices)
        {
            out = 0.0f;
        }
        
        aiVector3D& vec = mesh->mNormals[vertex_index];
        out = static_cast<real_t>(vec.y);
    }
    #endif
}

void ogm::interpreter::fn::ogm_model_import_mesh_vertex_nz(VO out, V vmesh_index, V vvertex_index)
{
    #ifdef ASSIMP
    const size_t mesh_index = vmesh_index.castCoerce<size_t>();
    const size_t vertex_index = vvertex_index.castCoerce<size_t>();
    if (mesh_index >= g_scene->mNumMeshes)
    {
        out = 0.0f;
    }
    else
    {
        aiMesh* mesh = g_scene->mMeshes[mesh_index];
        if (vertex_index >= mesh->mNumVertices)
        {
            out = 0.0f;
        }
        
        aiVector3D& vec = mesh->mNormals[vertex_index];
        out = static_cast<real_t>(vec.z);
    }
    #endif
}

void ogm::interpreter::fn::ogm_model_import_mesh_vertex_u(VO out, V vmesh_index, V vvertex_index)
{
    #ifdef ASSIMP
    const size_t mesh_index = vmesh_index.castCoerce<size_t>();
    const size_t vertex_index = vvertex_index.castCoerce<size_t>();
    if (mesh_index >= g_scene->mNumMeshes)
    {
        out = 0.0f;
    }
    else
    {
        aiMesh* mesh = g_scene->mMeshes[mesh_index];
        if (vertex_index >= mesh->mNumVertices)
        {
            out = 0.0f;
        }
        
        aiVector3D& vec = mesh->mTextureCoords[0][vertex_index];
        out = static_cast<real_t>(vec.x);
    }
    #endif
}

void ogm::interpreter::fn::ogm_model_import_mesh_vertex_v(VO out, V vmesh_index, V vvertex_index)
{
    #ifdef ASSIMP
    const size_t mesh_index = vmesh_index.castCoerce<size_t>();
    const size_t vertex_index = vvertex_index.castCoerce<size_t>();
    if (mesh_index >= g_scene->mNumMeshes)
    {
        out = 0.0f;
    }
    else
    {
        aiMesh* mesh = g_scene->mMeshes[mesh_index];
        if (vertex_index >= mesh->mNumVertices)
        {
            out = 0.0f;
        }
        
        aiVector3D& vec = mesh->mTextureCoords[0][vertex_index];
        out = static_cast<real_t>(vec.y);
    }
    #endif
}

void ogm::interpreter::fn::ogm_model_import_mesh_face_count(VO out, V vmesh_index)
{
    #ifdef ASSIMP
    size_t mesh_index = vmesh_index.castCoerce<size_t>();
    if (mesh_index >= g_scene->mNumMeshes)
    {
        out = 0.0;
    }
    else
    {
        aiMesh* mesh = g_scene->mMeshes[mesh_index];
        out = static_cast<real_t>(mesh->mNumFaces);
    }
    #endif
}

void ogm::interpreter::fn::ogm_model_import_mesh_face_vertex_count(VO out, V vmesh_index, V vface_index)
{
    #ifdef ASSIMP
    size_t mesh_index = vmesh_index.castCoerce<size_t>();
    size_t face_index = vface_index.castCoerce<size_t>();
    if (mesh_index >= g_scene->mNumMeshes)
    {
        out = -1.0;
    }
    else
    {
        aiMesh* mesh = g_scene->mMeshes[mesh_index];
        if (face_index >= mesh->mNumFaces)
        {
            out = -1.0;
        }
        aiFace& face = mesh->mFaces[face_index];
        out = static_cast<real_t>(face.mNumIndices);
    }
    #endif
}

void ogm::interpreter::fn::ogm_model_import_mesh_face_vertex_index(VO out, V vmesh_index, V vface_index, V vfvindex)
{
    #ifdef ASSIMP
    size_t mesh_index = vmesh_index.castCoerce<size_t>();
    size_t face_index = vface_index.castCoerce<size_t>();
    size_t fvindex = vfvindex.castCoerce<size_t>();
    if (mesh_index >= g_scene->mNumMeshes)
    {
        out = -1.0;
    }
    else
    {
        aiMesh* mesh = g_scene->mMeshes[mesh_index];
        if (face_index >= mesh->mNumFaces)
        {
            out = -1.0;
        }
        aiFace& face = mesh->mFaces[face_index];
        if (fvindex >= face.mNumIndices)
        {
            out = -1.0;
        }
        out = static_cast<real_t>(face.mIndices[fvindex]);
    }
    #endif
}