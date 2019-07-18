/// gigInit()
// loads gig dll

var dllName = "gig.dll";
var callType = dll_cdecl;

global.dll_gigGenerate              = external_define(dllName, "gig_generate",                callType, ty_real,   1, ty_string);
global.dll_gigError                 = external_define(dllName, "gig_error",                   callType, ty_real,   1, ty_real);
global.dll_gigWhatError             = external_define(dllName, "gig_what_error",              callType, ty_string, 1, ty_real);
global.dll_gigVariableInstanceCount = external_define(dllName, "gig_variable_instance_count", callType, ty_real,   1, ty_real);
global.dll_gigVariableInstanceName  = external_define(dllName, "gig_variable_instance_name",  callType, ty_string, 2, ty_real,   ty_real);
global.dll_gigVariableGlobalCount   = external_define(dllName, "gig_variable_global_count",   callType, ty_real,   1, ty_real);
global.dll_gigVariableGlobalName    = external_define(dllName, "gig_variable_global_name",    callType, ty_string, 2, ty_real,   ty_real);
global.dll_gigInstructionCount      = external_define(dllName, "gig_instruction_count",       callType, ty_real,   1, ty_real);
global.dll_gigInstructionOpcode     = external_define(dllName, "gig_instruction_opcode",      callType, ty_string, 2, ty_real,   ty_real);
global.dll_gigInstructionImmediate  = external_define(dllName, "gig_instruction_immediate",   callType, ty_string, 2, ty_real,   ty_real);
global.dll_gigInstructionAddress    = external_define(dllName, "gig_instruction_address",     callType, ty_real,   2, ty_real,   ty_real);
global.dll_gigFree                  = external_define(dllName, "gig_free",                    callType, ty_real,   1, ty_real);
global.dll_gigReturnValue           = 0
