#ifndef MEM_H_06_01_2026
#define MEM_H_06_01_2026
////////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
////////////////////////////////////////////////////////////////////////////////


struct MEM {
    MEM     *next;
    MEM     *prev;
    void    *data;
    size_t  capacity;
    size_t  alignment;
};

MEM *               mem_new             (size_t alignment, size_t size);
void                mem_free            (MEM *);
void                mem_clear           ();
size_t              mem_get_usage       ();
size_t              mem_get_footprint   (const MEM *);
MEM *               mem_get_metadata    (const void *data, size_t alignment);

#endif
