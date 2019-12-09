#ifdef PELOADER
#ifndef _FN_EXTERNAL_C_H_
#define _FN_EXTERNAL_C_H_

// allow C++ access to the C PELOADER library

// returns true on failure
bool _c_load_library(const char* name);
int _c_get_export(const char *name, void *func);



#endif
#endif
