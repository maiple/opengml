#pragma once

#ifdef OGM_FCL

#ifdef OGM_ALT_FCL_AABB_DIR
    #include <fcl/math/bv/AABB.h>
    #include <fcl/narrowphase/collision.h>
    #include <fcl/geometry/shape/shape_base.h>
    #include <fcl/geometry/bvh/BVH_utility.h>
#else
    #include <fcl/BV/AABB.h>
    #include <fcl/collision.h>
    #include <fcl/shape/geometric_shapes.h>
    #include <fcl/BVH/BVH_utility.h>
#endif
#include <fcl/broadphase/broadphase_dynamic_AABB_tree.h>

#ifndef OGM_FCL_V2
#else
#include <memory>
#endif

namespace ogm::interpreter
{
    namespace ogmfcl
    {
        template<typename T>
        using shared_ptr = std::shared_ptr<T>;

        #ifndef OGM_FCL_V2
        typedef fcl::BVHModel<fcl::OBB> Model;
        typedef fcl::Box Box;
        typedef fcl::DynamicAABBTreeCollisionManager BroadPhaseManager;
        typedef fcl::CollisionObject CollisionObject;
        typedef fcl::Vec3f Vector3f;
        #else
        typedef fcl::BVHModel<fcl::OBB> Model;
        typedef fcl::Box Box;
        typedef fcl::DynamicAABBTreeCollisionManager BroadPhaseManager;
        typedef fcl::CollisionObjectf CollisionObject;
        typedef fcl::Vector3f Vector3f;
        #endif
    }
    
    // ----------- defined in fn_fcl.cpp -----------------
    
    // currently-loaded models.
    extern std::vector<ogmfcl::shared_ptr<ogmfcl::Model>> g_fcl_models;
    
    // currently-loaded collision objects
    extern std::vector<ogmfcl::shared_ptr<ogmfcl::CollisionObject>> g_fcl_instances;
    
    // broadphase managers
    extern std::vector<ogmfcl::shared_ptr<ogmfcl::BroadPhaseManager>> g_fcl_broadphases;
}

#endif