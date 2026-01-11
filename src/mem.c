// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////
#include "all.h"
////////////////////////////////////////////////////////////////////////////////
#include <stdbit.h>
#include <stdlib.h>
////////////////////////////////////////////////////////////////////////////////


MEM *mem_new(size_t alignment, size_t size) {
    if (!size) {
        FUSE();
        return nullptr;
    }

    size = stdc_bit_ceil(size);

    static MEM mem_zero;
    const size_t cap_index = stdc_trailing_zeros(size);

    MEM *mem;

    if (cap_index >= ARRAY_LENGTH(global.free.memory)) {
        FUSE();
        return nullptr;
    }

    if (!stdc_has_single_bit(alignment)
    || alignof(max_align_t) < alignment) {
        BUG_ONCE("alignment of %lu is unacceptable", alignment);
        return nullptr;
    }

    alignment = umax_size(alignment, alignof(MEM));

    const size_t align_index = stdc_trailing_zeros(alignment);

    if (align_index >= ARRAY_LENGTH(global.free.memory[cap_index])) {
        FUSE();
        return nullptr;
    }

    if (global.free.memory[cap_index][align_index] == nullptr) {
        size_t padding = 0;

        if (sizeof(MEM) % alignment) {
            padding = (
                alignment + (sizeof(MEM) / alignment) * alignment
            ) - sizeof(MEM);
        }

        size_t total_size = sizeof(MEM) + padding + size;

        mem = aligned_alloc(alignment, total_size);

        if (!mem) {
            BUG_ONCE("failed to allocate %lu bytes", total_size);
            return nullptr;
        }

        *mem = mem_zero;

        mem->capacity = size;
        mem->alignment = alignment;
        mem->data = ((char *) mem) + sizeof(MEM) + padding;
    }
    else {
        mem = global.free.memory[cap_index][align_index];
        global.free.memory[cap_index][align_index] = mem->next;

        if (mem->next) {
            mem->next->prev = nullptr;
        }

        mem->prev = nullptr;
    }

    mem->next = global.list.memory;

    if (mem->next) {
        mem->next->prev = mem;
    }

    global.list.memory = mem;

    return mem;
}

void mem_free(MEM *mem) {
    if (!mem) {
        return;
    }

    if (!mem->capacity) {
        FUSE();
        return;
    }

    const size_t cap_index = stdc_trailing_zeros(mem->capacity);

    if (cap_index >= ARRAY_LENGTH(global.free.memory)) {
        FUSE();
        return;
    }

    const size_t align_index = stdc_trailing_zeros(mem->alignment);

    if (align_index >= ARRAY_LENGTH(global.free.memory[cap_index])) {
        FUSE();
        return;
    }

    if (global.list.memory == mem) {
        global.list.memory = mem->next;

        if (mem->next) {
            mem->next->prev = nullptr;
        }
    }
    else {
        if (mem->prev == nullptr) {
            BUG_ONCE("memory not found in the list");
        }
        else {
            mem->prev->next = mem->next;

            if (mem->next) {
                mem->next->prev = mem->prev;
            }
        }
    }

    mem->prev = nullptr;
    mem->next = global.free.memory[cap_index][align_index];

    if (mem->next) {
        mem->next->prev = mem;
    }

    global.free.memory[cap_index][align_index] = mem;
}

size_t mem_get_footprint(const MEM *mem) {
    size_t padding = 0;

    if (sizeof(MEM) % mem->alignment) {
        padding = (
            mem->alignment + (sizeof(*mem) / mem->alignment) * mem->alignment
        ) - sizeof(MEM);
    }

    size_t footprint = sizeof(*mem) + padding + mem->capacity;

    if (footprint % mem->alignment) {
        footprint = (
            mem->alignment + (footprint / mem->alignment) * mem->alignment
        );
    }

    return footprint;
}

size_t mem_get_usage() {
    size_t usage = 0;

    for (size_t i=0; i<ARRAY_LENGTH(global.free.memory); ++i) {
        for (size_t j=0; j<ARRAY_LENGTH(global.free.memory[i]); ++j) {
            MEM *mem = global.free.memory[i][j];

            while (mem) {
                usage += mem_get_footprint(mem);
                mem = mem->next;
            }
        }
    }

    MEM *mem = global.list.memory;

    while (mem) {
        usage += mem_get_footprint(mem);
        mem = mem->next;
    }

    return usage;
}

void mem_recycle() {
    for (size_t i=0; i<ARRAY_LENGTH(global.free.memory); ++i) {
        for (size_t j=0; j<ARRAY_LENGTH(global.free.memory[i]); ++j) {
            MEM *mem = global.free.memory[i][j];

            while (mem) {
                MEM *mem_next = mem->next;
                free(mem);
                mem = mem_next;
            }

            global.free.memory[i][j] = nullptr;
        }
    }
}

void mem_clear() {
    mem_recycle();

    MEM *mem = global.list.memory;

    while (mem) {
        MEM *mem_next = mem->next;
        free(mem);
        mem = mem_next;
    }

    global.list.memory = nullptr;
}

MEM *mem_get_metadata(const void *data, size_t alignment) {
    size_t padding = 0;

    alignment = umax_size(alignment, alignof(MEM));

    if (sizeof(MEM) % alignment) {
        padding = (
            alignment + (sizeof(MEM) / alignment) * alignment
        ) - sizeof(MEM);
    }

    MEM *metadata = (MEM *) (((const char *) data) - (sizeof(MEM) + padding));

    if (metadata->data != data || metadata->alignment != alignment) {
        FUSE();
        abort();
    }

    return metadata;
}

CLIP *mem_new_clip() {
    static CLIP zero;
    CLIP *ds = mem_new(alignof(typeof(zero)), sizeof(zero))->data;

    *ds = zero;

    return ds;
}

void mem_free_clip(CLIP *ds) {
    mem_free(ds->memory);
    mem_free(mem_get_metadata(ds, alignof(typeof(*ds))));
}

USER *mem_new_user() {
    static USER zero;

    MEM *mem = mem_new(alignof(typeof(zero)), sizeof(zero));
    USER *user = mem ? mem->data : nullptr;

    if (user) {
        *user = zero;
    }

    return user;
}

void mem_free_user(USER *user) {
    mem_free(mem_get_metadata(user, alignof(typeof(*user))));
}

SERVER *mem_new_server() {
    static SERVER zero;

    MEM *mem = mem_new(alignof(typeof(zero)), sizeof(zero));
    SERVER *server = mem ? mem->data : nullptr;

    if (server) {
        *server = zero;
    }

    return server;
}

void mem_free_server(SERVER *server) {
    mem_free(mem_get_metadata(server, alignof(typeof(*server))));
}

CLIENT *mem_new_client() {
    static CLIENT zero;

    MEM *mem = mem_new(alignof(typeof(zero)), sizeof(zero));
    CLIENT *client = mem ? mem->data : nullptr;

    if (client) {
        *client = zero;
    }

    return client;
}

void mem_free_client(CLIENT *client) {
    mem_free(mem_get_metadata(client, alignof(typeof(*client))));
}

TERMINAL *mem_new_terminal() {
    static TERMINAL zero;

    MEM *mem = mem_new(alignof(typeof(zero)), sizeof(zero));
    TERMINAL *terminal = mem ? mem->data : nullptr;

    if (terminal) {
        *terminal = zero;
    }

    return terminal;
}

void mem_free_terminal(TERMINAL *terminal) {
    mem_free(mem_get_metadata(terminal, alignof(typeof(*terminal))));
}

DISPATCHER *mem_new_dispatcher() {
    static DISPATCHER zero;

    MEM *mem = mem_new(alignof(typeof(zero)), sizeof(zero));
    DISPATCHER *dispatcher = mem ? mem->data : nullptr;

    if (dispatcher) {
        *dispatcher = zero;
    }

    return dispatcher;
}

void mem_free_dispatcher(DISPATCHER *dispatcher) {
    mem_free(mem_get_metadata(dispatcher, alignof(typeof(*dispatcher))));
}
