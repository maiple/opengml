#ifdef PELOADER

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/unistd.h>
#include <asm/unistd.h>
#include <fcntl.h>
#include <iconv.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <err.h>
#include <winnt_types.h>
#include <pe_linker.h>
#include <ntoskernel.h>

#include "fn_external_c.h"

bool _c_load_library(const char* name)
{
    struct pe_image pe =
    {
        .entry  = NULL
    };

    for (size_t i = 0; i < 127 && i < strlen(name); ++i)
    {
        pe.name[i] = name[i];
        pe.name[i + 1] = 0;
    }

    if (!pe_load_library(pe.name, &pe.image, &pe.size))
    {
        return true;
    }

    link_pe_images(&pe, 1);

    pe.entry((PVOID) 'MPEN', DLL_PROCESS_ATTACH, NULL);

    return false;
}

int _c_get_export(const char *name, void *func)
{
    return get_export(name, func);
}

#endif
