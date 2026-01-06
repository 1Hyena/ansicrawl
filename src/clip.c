// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////
#include "all.h"
////////////////////////////////////////////////////////////////////////////////
#include <string.h>
////////////////////////////////////////////////////////////////////////////////


static CLIP *clip_create(CLIP_TYPE type) {
    CLIP *clip = mem_new_clip();

    if (clip) {
        clip->type = type;
    }
    else FUSE();

    return clip;
}

CLIP * clip_create_byte_array() {
    return clip_create(CLIP_BYTE);
}

CLIP * clip_create_char_array() {
    return clip_create(CLIP_CHAR);
}

CLIP * clip_create_long_array() {
    return clip_create(CLIP_LONG);
}

CLIP * clip_create_voidptr_array() {
    return clip_create(CLIP_VOIDPTR);
}

CLIP * clip_create_ucs4_array() {
    return clip_create(CLIP_UCS4);
}

void clip_destroy(CLIP *clip) {
    if (!clip) {
        return;
    }

    mem_free_clip(clip);
}

size_t clip_type_get_alignment(CLIP_TYPE type) {
    switch (type) {
        case CLIP_BYTE: return alignof(uint8_t);
        case CLIP_CHAR: return alignof(char);
        case CLIP_LONG: return alignof(long);
        case CLIP_VOIDPTR: return alignof(void *);
        case CLIP_UCS4: return alignof(ucs4_t);
        case CLIP_NONE:
        case MAX_CLIP_TYPE: {
            break;
        }
    }

    return 0;
}

size_t clip_type_get_size(CLIP_TYPE type) {
    switch (type) {
        case CLIP_BYTE: return sizeof(uint8_t);
        case CLIP_CHAR: return sizeof(char);
        case CLIP_LONG: return sizeof(long);
        case CLIP_VOIDPTR: return sizeof(void *);
        case CLIP_UCS4: return sizeof(ucs4_t);
        case CLIP_NONE:
        case MAX_CLIP_TYPE: {
            break;
        }
    }

    return 0;
}

bool clip_reserve(CLIP *clip, size_t count) {
    if (clip->capacity >= count) {
        return true;
    }

    const size_t el_align = clip_type_get_alignment(clip->type);
    const size_t el_size = clip_type_get_size(clip->type);

    if (!el_align || !el_size) {
        FUSE();
        return false;
    }

    MEM *new_mem = mem_new(el_align, el_size * count);

    if (!new_mem) {
        return false;
    }

    if (clip->memory) {
        memcpy(new_mem->data, clip->memory->data, clip->size * el_size);
        mem_free(clip->memory);
    }

    clip->memory = new_mem;
    clip->capacity = count;

    return true;
}

void clip_set_byte_at(const CLIP *clip, size_t index, uint8_t value) {
    if (index < clip_get_size(clip)) {
        clip_get_byte_array(clip)[index] = value;
    }
    else FUSE();
}

void clip_set_char_at(const CLIP *clip, size_t index, char value) {
    if (index < clip_get_size(clip)) {
        clip_get_char_array(clip)[index] = value;
    }
    else FUSE();
}

void clip_set_long_at(const CLIP *clip, size_t index, long value) {
    if (index < clip_get_size(clip)) {
        clip_get_long_array(clip)[index] = value;
    }
    else FUSE();
}

void clip_set_voidptr_at(const CLIP *clip, size_t index, void *value) {
    if (index < clip_get_size(clip)) {
        clip_get_voidptr_array(clip)[index] = value;
    }
    else FUSE();
}

void clip_set_ucs4_at(const CLIP *clip, size_t index, ucs4_t value) {
    if (index < clip_get_size(clip)) {
        clip_get_ucs4_array(clip)[index] = value;
    }
    else FUSE();
}

uint8_t *clip_get_byte_array(const CLIP *clip) {
    if (clip->type != CLIP_BYTE) {
        FUSE();
        return nullptr;
    }

    return clip->memory->data;
}

char *clip_get_char_array(const CLIP *clip) {
    if (clip->type != CLIP_CHAR) {
        FUSE();
        return nullptr;
    }

    return clip->memory->data;
}

long *clip_get_long_array(const CLIP *clip) {
    if (clip->type != CLIP_LONG) {
        FUSE();
        return nullptr;
    }

    return clip->memory->data;
}

void **clip_get_voidptr_array(const CLIP *clip) {
    if (clip->type != CLIP_VOIDPTR) {
        FUSE();
        return nullptr;
    }

    return clip->memory->data;
}

ucs4_t *clip_get_ucs4_array(const CLIP *clip) {
    if (clip->type != CLIP_UCS4) {
        FUSE();
        return nullptr;
    }

    return clip->memory->data;
}

uint8_t clip_get_byte_at(const CLIP *clip, size_t index) {
    if (clip_get_size(clip) > index) {
        return clip_get_byte_array(clip)[index];
    }

    FUSE();

    return 0;
}

char clip_get_char_at(const CLIP *clip, size_t index) {
    if (clip_get_size(clip) > index) {
        return clip_get_char_array(clip)[index];
    }

    FUSE();

    return 0;
}

long clip_get_long_at(const CLIP *clip, size_t index) {
    if (clip_get_size(clip) > index) {
        return clip_get_long_array(clip)[index];
    }

    FUSE();

    return 0;
}

void *clip_get_voidptr_at (const CLIP *clip, size_t index) {
    if (clip_get_size(clip) > index) {
        return clip_get_voidptr_array(clip)[index];
    }

    FUSE();

    return nullptr;
}

ucs4_t clip_get_ucs4_at(const CLIP *clip, size_t index) {
    if (clip_get_size(clip) > index) {
        return clip_get_ucs4_array(clip)[index];
    }

    FUSE();

    return 0;
}

static bool clip_set_array(CLIP *clip, const void *data, size_t count) {
    if (clip->type == CLIP_NONE) {
        FUSE();
        return false;
    }

    const size_t element_size = clip_type_get_size(clip->type);

    if (!element_size) {
        FUSE();
        return false;
    }

    if (count > clip->capacity) {
        if (!clip_reserve(clip, count)) {
            return false;
        }
    }

    if (count) {
        memcpy(clip->memory->data, data, count * element_size);
    }

    clip->size = count;

    return true;
}

bool clip_set_byte_array(CLIP *clip, const uint8_t *data, size_t count) {
    if (clip->type != CLIP_BYTE) {
        FUSE();
        return false;
    }

    return clip_set_array(clip, data, count);
}

bool clip_set_char_array(CLIP *clip, const char *data, size_t count) {
    if (clip->type != CLIP_CHAR) {
        FUSE();
        return false;
    }

    return clip_set_array(clip, data, count);
}

bool clip_set_long_array(CLIP *clip, const long *data, size_t count) {
    if (clip->type != CLIP_LONG) {
        FUSE();
        return false;
    }

    return clip_set_array(clip, data, count);
}

bool clip_set_voidptr_array(CLIP *clip, const void **data, size_t count) {
    if (clip->type != CLIP_VOIDPTR) {
        FUSE();
        return false;
    }

    return clip_set_array(clip, data, count);
}

bool clip_set_ucs4_array(CLIP *clip, const ucs4_t *data, size_t count) {
    if (clip->type != CLIP_UCS4) {
        FUSE();
        return false;
    }

    return clip_set_array(clip, data, count);
}

static bool clip_append_array(CLIP *clip, const void *data, size_t count) {
    if (clip->type == CLIP_NONE) {
        FUSE();
        return false;
    }

    const size_t element_size = clip_type_get_size(clip->type);

    if (!element_size) {
        FUSE();
        return false;
    }

    if (!count) {
        return true;
    }

    if (clip->size + count > clip->capacity) {
        if (!clip_reserve(clip, clip->size + count)) {
            return false;
        }
    }

    memcpy(
        ((uint8_t *) clip->memory->data) + clip->size * element_size,
        data, count * element_size
    );

    clip->size += count;

    return true;
}

bool clip_append_byte_array(CLIP *clip, const uint8_t *data, size_t count) {
    if (clip->type != CLIP_BYTE) {
        FUSE();
        return false;
    }

    return clip_append_array(clip, data, count);
}

bool clip_append_char_array(CLIP *clip, const char *data, size_t count) {
    if (clip->type != CLIP_CHAR) {
        FUSE();
        return false;
    }

    return clip_append_array(clip, data, count);
}

bool clip_append_long_array(CLIP *clip, const long *data, size_t count) {
    if (clip->type != CLIP_LONG) {
        FUSE();
        return false;
    }

    return clip_append_array(clip, data, count);
}

bool clip_append_voidptr_array(CLIP *clip, const void **data, size_t count) {
    if (clip->type != CLIP_VOIDPTR) {
        FUSE();
        return false;
    }

    return clip_append_array(clip, data, count);
}

bool clip_append_ucs4_array(CLIP *clip, const ucs4_t *data, size_t count) {
    if (clip->type != CLIP_UCS4) {
        FUSE();
        return false;
    }

    return clip_append_array(clip, data, count);
}

bool clip_push_byte(CLIP *clip, uint8_t value) {
    return clip_append_byte_array(clip, &value, 1);
}

bool clip_push_char(CLIP *clip, char value) {
    return clip_append_char_array(clip, &value, 1);
}

bool clip_push_long(CLIP *clip, long value) {
    return clip_append_long_array(clip, &value, 1);
}

bool clip_push_voidptr(CLIP *clip, const void *value) {
    return clip_append_voidptr_array(clip, &value, 1);
}

bool clip_push_ucs4(CLIP *clip, ucs4_t value) {
    return clip_append_ucs4_array(clip, &value, 1);
}

bool clip_append_clip(CLIP *dst, const CLIP *src) {
    if (dst->type != src->type) {
        FUSE();
        return false;
    }

    if (clip_is_empty(src)) {
        return true;
    }

    return clip_append_array(dst, src->memory->data, src->size);
}

void clip_swap(CLIP *a, CLIP *b) {
    struct CLIP buf = *a;
    *a = *b;
    *b = buf;
}

CLIP *clip_shift(CLIP *clip, size_t count) {
    CLIP *new_clip = clip_create(clip->type);

    if (!new_clip) {
        FUSE();
        return nullptr;
    }

    if (!count) {
        return new_clip;
    }

    clip_swap(clip, new_clip);

    if (count >= new_clip->size) {
        return new_clip;
    }

    const size_t element_size = clip_type_get_size(clip->type);

    clip_set_array(
        clip,
        ((uint8_t *) new_clip->memory->data) + count * element_size,
        new_clip->size - count
    );

    new_clip->size = count;

    return new_clip;
}

void clip_clear(CLIP *clip) {
    clip->size = 0;
}

size_t clip_get_size(const CLIP *clip) {
    return clip->size;
}

size_t clip_get_capacity(const CLIP *clip) {
    return clip->capacity;
}

bool clip_is_empty(const CLIP *clip) {
    return clip_get_size(clip) == 0;
}
