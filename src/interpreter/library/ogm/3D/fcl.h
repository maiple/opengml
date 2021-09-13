#pragma once

#ifdef OGM_FCL

#if OGM_FCL == 500
    #include <fcl/BV/AABB.h>
    #include <fcl/collision.h>
    #include <fcl/shape/geometric_shapes.h>
    #include <fcl/BVH/BVH_utility.h>
    #include <fcl/broadphase/broadphase_dynamic_AABB_tree.h>
#elif OGM_FCL == 600
    #include <fcl/math/bv/AABB.h>
    #include <fcl/narrowphase/collision.h>
    #include <fcl/geometry/shape/shape_base.h>
    #include <fcl/geometry/bvh/BVH_utility.h>
    #include <fcl/broadphase/broadphase_dynamic_AABB_tree.h>
#endif


namespace ogm::interpreter
{
    namespace ogmfcl
    {
        template<typename T>
        using shared_ptr = std::shared_ptr<T>;
                

        #if OGM_FCL == 500
            typedef fcl::BVHModel<fcl::OBB> Model;
            typedef fcl::Vec3f Vec3f;
            #define FCL_TEMPLATE
        #elif OGM_FCL == 600
            typedef fcl::BVHModel<fcl::OBBRSSf> Model;
            typedef fcl::Vector3f Vec3f;
            #define FCL_TEMPLATE <float>
        #endif
        
        typedef fcl::Box FCL_TEMPLATE Box;
        typedef fcl::CollisionObject FCL_TEMPLATE CollisionObject;
        typedef fcl::DynamicAABBTreeCollisionManager FCL_TEMPLATE BroadPhaseManager;
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