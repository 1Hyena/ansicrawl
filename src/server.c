// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////
#include "all.h"
////////////////////////////////////////////////////////////////////////////////


SERVER *server_create() {
    SERVER *server = mem_new_server();

    if (!server) {
        return nullptr;
    }

    if ((server->io.incoming.clip = clip_create_byte_array()) == nullptr
    || !(server->io.outgoing.clip = clip_create_byte_array())) {
        server_destroy(server);

        return nullptr;
    }

    return server;
}

void server_destroy(SERVER *server) {
    if (!server) {
        return;
    }

    clip_destroy(server->io.incoming.clip);
    clip_destroy(server->io.outgoing.clip);

    mem_free_server(server);
}
