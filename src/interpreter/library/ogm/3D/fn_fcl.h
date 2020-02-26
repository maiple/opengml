// OpenGML 3D collision extensions

// takes current model_import model and 
//FNDEF0(ogm_fcl_mesh_create, id)
FNDEF0(ogm_fcl_available)

// meshes
FNDEF1(ogm_fcl_mesh_from_model_mesh, meshid)

// instances
FNDEF6(ogm_fcl_box_instance_create, width, height, breadth, cx, cy, cz)
FNDEF4(ogm_fcl_mesh_instance_create, meshid, x, y, z)
FNDEF1(ogm_fcl_instance_destroy, id)

// broadphase (many-object) collision
FNDEF0(ogm_fcl_broadphase_create)
FNDEF2(ogm_fcl_broadphase_add, id, instance_id)
FNDEF2(ogm_fcl_broadphase_remove, id, instance_id)
FNDEF2(ogm_fcl_broadphase_update, id, instance_id)
FNDEF1(ogm_fcl_broadphase_destroy, id)

// collision
FNDEF2(ogm_fcl_collision_instance_instance, id1, id2)
FNDEF2(ogm_fcl_collision_instance_broadphase, id, bv)
FNDEF2(ogm_fcl_collision_broadphase_instance, bv, id)