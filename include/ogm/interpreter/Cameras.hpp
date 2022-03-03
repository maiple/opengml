#pragma once

#include "ogm/common/types.hpp"
#include "ogm/geometry/Vector.hpp"
#include "ogm/geometry/aabb.hpp"

#include <set>
#include <string>
#include <map>
#include <tuple>

namespace ogm::interpreter
{
    typedef id_t camera_id_t;
    constexpr camera_id_t k_no_camera = -1;
}

#ifdef OGM_CAMERAS

namespace ogm::interpreter
{
    using namespace ogm::geometry;
    
    struct Camera
    {
        bool m_manual = false; // if false, then matrices will be overwritten each frame.
        
        matrix_t m_matrix_view;
        matrix_t m_matrix_projection;
        
        Vector<coord_t> m_position;
        Vector<coord_t> m_dimensions;
        real_t m_angle = 0;
        
        Vector<coord_t> m_velocity;
        Vector<coord_t> m_margins; // (used during instance following)
        
        // for some reason, -1 (k_self) is used instead of -4 (k_noone) ...?
        // confirm this!
        instance_id_t m_instance_target = -1;
        
        asset_index_t m_script_update = k_no_asset;
        asset_index_t m_script_begin = k_no_asset;
        asset_index_t m_script_end = k_no_asset;
    };
    
    class CameraManager
    {
        std::map<camera_id_t, Camera> m_map;
        camera_id_t m_next_id = 0;
            
    public:
        camera_id_t new_camera()
        {
            m_map[m_next_id];
            return m_next_id++;
        }
        
        void free_camera(camera_id_t id)
        {
            m_map.erase(id);
        }
        
        bool camera_exists(camera_id_t id)
        {
            return m_map.find(id) != m_map.end();
        }
        
        Camera& get_camera(camera_id_t id)
        {
            return m_map.at(id);
        }
    };
}

#endif