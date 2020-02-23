#pragma once

#ifdef OGM_FCL
#include <fcl/BVH/BVH_utility.h>
#include <fcl/collision.h>
#include <memory>

namespace ogm::interpreter
{    
    #ifndef OGM_FCL_V2
    typedef fcl::BVHModel<fcl::OBBRSS> Model;
    typedef fcl::CollisionObject CollisionObject;
    typedef fcl::Vec3f Vector3f;
    
    template<typename T>
    using shared_ptr = boost::shared_ptr<T>;
    
    #else
    typedef fcl::BVHModel<fcl::OBBRSSf> Model;
    typedef fcl::CollisionObjectf CollisionObject;
    typedef fcl::Vector3f Vector3f;
    
    template<typename T>
    using shared_ptr = std::shared_ptr<T>;
    #endif
    
    // ----------- defined in fn_ogm_collision.cpp -----------------
    
    // currently-loaded models.
    extern std::vector<shared_ptr<Model>> g_fcl_models;
    
    // currently-loaded collision objects
    extern std::vector<shared_ptr<CollisionObject>> g_fcl_instances;
}

#endif