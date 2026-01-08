// SPDX-License-Identifier: MIT
#ifndef CLIP_H_06_01_2026
#define CLIP_H_06_01_2026
////////////////////////////////////////////////////////////////////////////////
#include "global.h"
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <unitypes.h>
////////////////////////////////////////////////////////////////////////////////


typedef enum : unsigned char {
    CLIP_NONE = 0,
    ////////////////////////////////////////////////////////////////////////////
    CLIP_BYTE, CLIP_CHAR, CLIP_LONG, CLIP_VOIDPTR, CLIP_UCS4,
    ////////////////////////////////////////////////////////////////////////////
    MAX_CLIP_TYPE
} CLIP_TYPE;

typedef struct CLIP CLIP;

struct CLIP {
    MEM *memory;
    size_t size;
    size_t capacity;
    CLIP_TYPE type;
};

CLIP *      clip_create_byte_array      ();
CLIP *      clip_create_char_array      ();
CLIP *      clip_create_long_array      ();
CLIP *      clip_create_voidptr_array   ();
CLIP *      clip_create_ucs4_array      ();
void        clip_destroy                (CLIP *);
bool        clip_reserve                (CLIP *, size_t);
void        clip_swap                   (CLIP *, CLIP *);
void        clip_clear                  (CLIP *clip);
size_t      clip_type_get_alignment     (CLIP_TYPE);
size_t      clip_type_get_size          (CLIP_TYPE);
uint8_t *   clip_get_byte_array         (const CLIP *);
char *      clip_get_char_array         (const CLIP *);
long *      clip_get_long_array         (const CLIP *);
void **     clip_get_voidptr_array      (const CLIP *);
ucs4_t *    clip_get_ucs4_array         (const CLIP *);
uint8_t     clip_get_byte_at            (const CLIP *, size_t index);
char        clip_get_char_at            (const CLIP *, size_t index);
long        clip_get_long_at            (const CLIP *, size_t index);
void *      clip_get_voidptr_at         (const CLIP *, size_t index);
ucs4_t      clip_get_ucs4_at            (const CLIP *, size_t index);
void        clip_set_byte_at            (const CLIP *, size_t index, uint8_t);
void        clip_set_char_at            (const CLIP *, size_t index, char);
void        clip_set_long_at            (const CLIP *, size_t index, long);
void        clip_set_voidptr_at         (const CLIP *, size_t index, void *);
void        clip_set_ucs4_at            (const CLIP *, size_t index, ucs4_t);
bool        clip_set_byte_array         (CLIP *, const uint8_t *, size_t);
bool        clip_set_char_array         (CLIP *, const char *, size_t);
bool        clip_set_long_array         (CLIP *, const long *, size_t);
bool        clip_set_voidptr_array      (CLIP *, const void **, size_t);
bool        clip_set_ucs4_array         (CLIP *, const ucs4_t *, size_t);
bool        clip_append_byte_array      (CLIP *, const uint8_t *, size_t);
bool        clip_append_char_array      (CLIP *, const char *, size_t);
bool        clip_append_long_array      (CLIP *, const long *, size_t);
bool        clip_append_voidptr_array   (CLIP *, const void **, size_t);
bool        clip_append_ucs4_array      (CLIP *, const ucs4_t *, size_t);
bool        clip_push_byte              (CLIP *, uint8_t);
bool        clip_push_char              (CLIP *, char);
bool        clip_push_long              (CLIP *, long);
bool        clip_push_voidptr           (CLIP *, const void *);
bool        clip_push_ucs4              (CLIP *, ucs4_t);
bool        clip_pop_byte               (CLIP *);
bool        clip_pop_char               (CLIP *);
bool        clip_pop_long               (CLIP *);
bool        clip_pop_voidptr            (CLIP *);
bool        clip_pop_ucs4               (CLIP *);
bool        clip_append_clip            (CLIP *, const CLIP *);
size_t      clip_get_size               (const CLIP *);
size_t      clip_get_capacity           (const CLIP *);
bool        clip_is_empty               (const CLIP *);

[[nodiscard]] CLIP *clip_shift          (CLIP *, size_t);

#endif
