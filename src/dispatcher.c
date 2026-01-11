// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////
#include "all.h"
////////////////////////////////////////////////////////////////////////////////


static bool dispatcher_read_from_terminal(DISPATCHER *);
static bool dispatcher_read_from_client(DISPATCHER *);
static void dispatcher_update_terminal(DISPATCHER *);
static void dispatcher_update_client(DISPATCHER *);
static bool dispatcher_flush_outgoing(DISPATCHER *);

DISPATCHER *dispatcher_create() {
    DISPATCHER *dispatcher = mem_new_dispatcher();

    if (!dispatcher) {
        return nullptr;
    }

    if (!(dispatcher->io.terminal.incoming.clip = clip_create_byte_array())
    ||  !(dispatcher->io.terminal.outgoing.clip = clip_create_byte_array())
    ||  !(dispatcher->io.client.incoming.clip = clip_create_byte_array())
    ||  !(dispatcher->io.client.outgoing.clip = clip_create_byte_array())) {
        dispatcher_destroy(dispatcher);

        return nullptr;
    }

    return dispatcher;
}

void dispatcher_destroy(DISPATCHER *dispatcher) {
    if (!dispatcher) {
        return;
    }

    clip_destroy(dispatcher->io.terminal.incoming.clip);
    clip_destroy(dispatcher->io.terminal.outgoing.clip);
    clip_destroy(dispatcher->io.client.incoming.clip);
    clip_destroy(dispatcher->io.client.outgoing.clip);

    mem_free_dispatcher(dispatcher);
}

void dispatcher_init(DISPATCHER *) {
}

void dispatcher_deinit(DISPATCHER *) {
}

bool dispatcher_update(DISPATCHER *dispatcher) {
    if (!dispatcher) {
        return false;
    }

    CLIP *src = global.io.incoming.clip;
    const bool incoming = !clip_is_empty(src);

    if (incoming) {
        CLIP *dst = (
            global.terminal ? (
                global.terminal->io.dispatcher.incoming.clip
            ) : global.client->io.dispatcher.incoming.clip
        );

        if (dst == nullptr) {
            clip_clear(src);
        }
        else if (clip_is_empty(dst)) {
            clip_swap(dst, src);
        }
        else {
            bool appended = clip_append_clip(dst, src);

            if (!appended) {
                FUSE();
            }

            clip_clear(src);
        }
    }

    dispatcher_update_terminal(dispatcher);
    dispatcher_update_client(dispatcher);

    return dispatcher_flush_outgoing(dispatcher) || incoming;
}

static void dispatcher_update_terminal(DISPATCHER *dispatcher) {
    while (dispatcher_read_from_terminal(dispatcher));
}

static void dispatcher_update_client(DISPATCHER *dispatcher) {
    while (dispatcher_read_from_client(dispatcher));
}

static bool dispatcher_read_from_terminal(DISPATCHER *dispatcher) {
    CLIP *src = dispatcher->io.terminal.incoming.clip;

    if (clip_is_empty(src)) {
        return false;
    }

    CLIP *dst = global.io.outgoing.clip;

    if (clip_is_empty(dst)) {
        clip_swap(dst, src);
        return true;
    }

    bool appended = clip_append_clip(dst, src);

    if (!appended) {
        FUSE();
    }

    clip_clear(src);
    return true;
}

static bool dispatcher_read_from_client(DISPATCHER *dispatcher) {
    CLIP *src = dispatcher->io.client.incoming.clip;

    if (clip_is_empty(src)) {
        return false;
    }

    CLIP *dst = global.io.outgoing.clip;

    if (clip_is_empty(dst)) {
        clip_swap(dst, src);
        return true;
    }

    bool appended = clip_append_clip(dst, src);

    if (!appended) {
        FUSE();
    }

    clip_clear(src);
    return true;
}

static bool dispatcher_flush_outgoing(DISPATCHER *dispatcher) {
    if (!dispatcher) {
        return false;
    }

    bool flushed = false;

    {
        CLIP *src = dispatcher->io.terminal.outgoing.clip;

        if (!clip_is_empty(src)) {
            CLIP *dst = global.terminal->io.dispatcher.incoming.clip;

            bool appended = clip_append_clip(dst, src);

            if (!appended) {
                FUSE();
            }

            clip_clear(src);

            flushed = true;
        }
    }

    {
        CLIP *src = dispatcher->io.client.outgoing.clip;

        if (!clip_is_empty(src)) {
            CLIP *dst = global.client->io.dispatcher.incoming.clip;

            bool appended = clip_append_clip(dst, src);

            if (!appended) {
                FUSE();
            }

            clip_clear(src);

            flushed = true;
        }
    }

    return flushed;
}
