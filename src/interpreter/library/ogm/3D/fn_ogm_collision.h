// OpenGML 3D collision extensions

// takes current model_import model and 
//FNDEF0(ogm_fcl_mesh_create, id)
FNDEF0(ogm_fcl_available)
FNDEF1(ogm_fcl_mesh_from_model_mesh, meshid)
FNDEF4(ogm_fcl_mesh_instance_create, meshid, x, y, z)
FNDEF2(ogm_fcl_mesh_instance_collision, id1, id2)
FNDEF1(ogm_fcl_mesh_instance_destroy, id)
