// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////
#include "all.h"
////////////////////////////////////////////////////////////////////////////////
#include <string.h>
////////////////////////////////////////////////////////////////////////////////


CLIENT *client_create() {
    CLIENT *client = mem_new_client();

    if (!client) {
        return nullptr;
    }

    if ((client->io.incoming.clip = clip_create_byte_array()) == nullptr
    || !(client->io.outgoing.clip = clip_create_byte_array())) {
        client_destroy(client);

        return nullptr;
    }

    return client;
}

void client_destroy(CLIENT *client) {
    if (!client) {
        return;
    }

    clip_destroy(client->io.incoming.clip);
    clip_destroy(client->io.outgoing.clip);

    mem_free_client(client);
}

void client_init(CLIENT *client) {
}

void client_deinit(CLIENT *client) {
}

void client_pulse(CLIENT *client) {
}

bool client_write(CLIENT *client, const char *str, size_t len) {
    return clip_append_byte_array(
        client->io.outgoing.clip, (const uint8_t *) str, len ? len : strlen(str)
    );
}
