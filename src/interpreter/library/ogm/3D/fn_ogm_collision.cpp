#include "fcl.h"
#include "assimp.h"

#include "interpreter/library/libpre.h"
    #include "fn_ogm_collision.h"
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

namespace ogm::interpreter
{
    #ifdef OGM_FCL
    std::vector<shared_ptr<Model>> g_fcl_models{ };
    std::vector<shared_ptr<CollisionObject>> g_fcl_instances{ };
    
    template<typename T, typename... Arguments>
    shared_ptr<T> vec_add(
        std::vector<shared_ptr<T>>& io_vector,
        size_t& out_id,
        Arguments... args
    )
    {
        for (size_t i = 0; i < io_vector.size(); ++i)
        {
            shared_ptr<T>& ptr = io_vector[i];
            if (!ptr)
            {
                out_id = i;
                ptr = shared_ptr<T>(new T(args...));
                return ptr;
            }
        }
        
        out_id = io_vector.size();
        return io_vector.emplace_back(new T(args...));
    }
    
    template<typename T>
    shared_ptr<T> vec_get(
        const std::vector<shared_ptr<T>>& io_vector,
        size_t id
    )
    {
        if (id >= io_vector.size())
        {
            return nullptr;
        }
        return io_vector.at(id);
    }
    
    template<typename T>
    void vec_replace(
        std::vector<shared_ptr<T>>& io_vector,
        size_t id,
        shared_ptr<T> value
    )
    {
        if (id < io_vector.size())
        {
            io_vector[id] = value;
        }
    }
    #endif
}

void ogm::interpreter::fn::ogm_fcl_available(VO out)
{
    #ifdef OGM_FCL
    out = true;
    #else
    out = false;
    #endif
}

void ogm::interpreter::fn::ogm_fcl_mesh_from_model_mesh(VO out, V vmesh_index)
{
    const size_t mesh_index = vmesh_index.castCoerce<size_t>();
    
    #if defined(OGM_FCL) && defined(ASSIMP)    
    if (mesh_index >= g_scene->mNumMeshes)
    {
        out = -1.0f;
    }
    else
    {
        using namespace Assimp;
        
        std::vector<fcl::Vec3f> verts;
        std::vector<fcl::Triangle> triangles;
        size_t id;
        shared_ptr<Model> geom = vec_add(g_fcl_models, id);        
        
        aiMesh* mesh = g_scene->mMeshes[mesh_index];
        for (size_t i = 0; i < mesh->mNumVertices; ++i)
        {
            aiVector3D& vec = mesh->mVertices[i];
            float x = vec.x, y = vec.y, z = vec.z;
            verts.emplace_back(x, y, z);
        }
        for (size_t i = 0; i < mesh->mNumFaces; ++i)
        {
            aiFace& face = mesh->mFaces[i];
            ogm_assert(face.mNumIndices == 3);
            triangles.emplace_back(
                face.mIndices[0],
                face.mIndices[1],
                face.mIndices[2]
            );
        }
        
        geom->beginModel();
        geom->addSubModel(verts, triangles);
        geom->endModel();
        out = static_cast<float>(id);
    }
    #else
    out = -1.0;
    #endif
}

void ogm::interpreter::fn::ogm_fcl_mesh_instance_create(VO out, V vmesh_index, V x, V y, V z)
{
    #ifdef OGM_FCL
    shared_ptr<Model> geom = vec_get(g_fcl_models, vmesh_index.castCoerce<size_t>());
    fcl::Vec3f translation{
        x.castCoerce<real_t>(),
        y.castCoerce<real_t>(),
        z.castCoerce<real_t>()
    };
    
    fcl::Transform3f pose{ };
    assert(pose.isIdentity());
    pose.setTranslation(translation);
    
    size_t index;
    shared_ptr<CollisionObject> ptr = vec_add(
        g_fcl_instances, index,
        geom, pose
    );
    
    out = static_cast<float>(index);
    
    #else
    out = -1.0;
    #endif
}

void ogm::interpreter::fn::ogm_fcl_mesh_instance_collision(VO out, V vindex1, V vindex2)
{
    #ifdef OGM_FCL
    shared_ptr<CollisionObject> co1 = vec_get(g_fcl_instances, vindex1.castCoerce<size_t>());
    shared_ptr<CollisionObject> co2 = vec_get(g_fcl_instances, vindex2.castCoerce<size_t>());
    
    fcl::CollisionRequest request;
    fcl::CollisionResult result;
    out = true;
    
    if (!co1.get() || !co2.get())
    {
        throw MiscError("collision instances do not exist.");
    }
    
    fcl::collide(co1.get(), co2.get(), request, result);
    
    out = result.isCollision();
    #else
    out = false;
    #endif
}

void ogm::interpreter::fn::ogm_fcl_mesh_instance_destroy(VO out, V viid)
{
    #ifdef OGM_FCL
    size_t iid = viid.castCoerce<size_t>();
    vec_replace(g_fcl_instances, iid, shared_ptr<CollisionObject>());
    #endif
}