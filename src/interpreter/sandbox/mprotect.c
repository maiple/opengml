// https://github.com/18446744073709551615/reDroid/blob/master/jni/re_mprot.h

#ifdef __linux__

#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
//#define PAGE_SIZE 4096
#include "dlog.h"
#include "stdlib.h"
#include "re_mprot.h"

struct buffer {
    int pos;
    int size;
    char* mem;
};

char* _buf_reset(struct buffer*b) {
    b->mem[b->pos] = 0;
    b->pos = 0;
    return b->mem;
}

struct buffer* _new_buffer(int length) {
    struct buffer* res = malloc(sizeof(struct buffer)+length+4);
    res->pos = 0;
    res->size = length;
    res->mem = (void*)(res+1);
    return res;
}

int _buf_putchar(struct buffer*b, int c) {
    b->mem[b->pos++] = c;
    return b->pos >= b->size;
}

void show_mappings(void)
{
    DLOG("-----------------------------------------------\n");
    int a;
    FILE *f = fopen("/proc/self/maps", "r");
    struct buffer* b = _new_buffer(1024);
    while ((a = fgetc(f)) >= 0) {
        if (_buf_putchar(b,a) || a == '\n') {
            DLOG("/proc/self/maps: %s",_buf_reset(b));
        }
    }
    if (b->pos) {
        DLOG("/proc/self/maps: %s",_buf_reset(b));
    }
    free(b);
    fclose(f);
    DLOG("-----------------------------------------------\n");
}

unsigned int read_mprotection(void* addr) {
    int a;
    unsigned int res = MPROT_0;
    FILE *f = fopen("/proc/self/maps", "r");
    struct buffer* b = _new_buffer(1024);
    while ((a = fgetc(f)) >= 0) {
        if (_buf_putchar(b,a) || a == '\n') {
            char*end0 = (void*)0;
            unsigned long addr0 = strtoul(b->mem, &end0, 0x10);
            char*end1 = (void*)0;
            unsigned long addr1 = strtoul(end0+1, &end1, 0x10);
            if ((void*)addr0 < addr && addr < (void*)addr1) {
                res |= (end1+1)[0] == 'r' ? MPROT_R : 0;
                res |= (end1+1)[1] == 'w' ? MPROT_W : 0;
                res |= (end1+1)[2] == 'x' ? MPROT_X : 0;
                res |= (end1+1)[3] == 'p' ? MPROT_P
                     : (end1+1)[3] == 's' ? MPROT_S : 0;
                break;
            }
            _buf_reset(b);
        }
    }
    free(b);
    fclose(f);
    return res;
}

int has_mprotection(void* addr, unsigned int prot, unsigned int prot_mask) {
    unsigned prot1 = read_mprotection(addr);
    return (prot1 & prot_mask) == prot;
}

char* _mprot_tostring_(char*buf, unsigned int prot) {
    buf[0] = prot & MPROT_R ? 'r' : '-';
    buf[1] = prot & MPROT_W ? 'w' : '-';
    buf[2] = prot & MPROT_X ? 'x' : '-';
    buf[3] = prot & MPROT_S ? 's' : prot & MPROT_P ? 'p' :  '-';
    buf[4] = 0;
    return buf;
}


/*
int main(void)
{
    void *mapping;

    /* Show initial mappings. * /
    show_mappings();

    /* Map in some pages. * /
    mapping = mmap(NULL, 16 * PAGE_SIZE, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    printf("*** Returned mapping: %p\n", mapping);
    show_mappings();

    /* Change the mapping. * /
    mprotect(mapping, PAGE_SIZE, PROT_READ | PROT_WRITE);
    show_mappings();

    return 0;
}
*/

#endif
