// SPDX-License-Identifier: MIT
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
void                mem_recycle         ();
void                mem_clear           ();
size_t              mem_get_usage       ();
size_t              mem_get_footprint   (const MEM *);
MEM *               mem_get_metadata    (const void *data, size_t alignment);

CLIP *              mem_new_clip        ();
void                mem_free_clip       (CLIP *);
USER *              mem_new_user        ();
void                mem_free_user       (USER *);
SERVER *            mem_new_server      ();
void                mem_free_server     (SERVER *);
CLIENT *            mem_new_client      ();
void                mem_free_client     (CLIENT *);
TERMINAL *          mem_new_terminal    ();
void                mem_free_terminal   (TERMINAL *);

#endif
