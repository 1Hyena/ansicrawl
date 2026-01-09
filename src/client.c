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

    if ((client->io.terminal.incoming.clip = clip_create_byte_array()) ==nullptr
    || !(client->io.terminal.outgoing.clip = clip_create_byte_array())) {
        client_destroy(client);

        return nullptr;
    }

    return client;
}

void client_destroy(CLIENT *client) {
    if (!client) {
        return;
    }

    clip_destroy(client->io.terminal.incoming.clip);
    clip_destroy(client->io.terminal.outgoing.clip);

    mem_free_client(client);
}

void client_init(CLIENT *client) {
}

void client_deinit(CLIENT *client) {
}

void client_handle_incoming_terminal_iac(
    CLIENT *client, const uint8_t *data, size_t size
) {
    if (!data || size < 2 || data[0] != TELNET_IAC) {
        FUSE();
        return;
    }

    if (size == 2) {
        return;
    }

    switch (data[2]) {
        case TELNET_OPT_NAWS: {
            switch (data[1]) {
                case TELNET_DO: {
                    client->telopt.terminal.naws.recv_do = true;
                    break;
                }
                case TELNET_DONT: {
                    client->telopt.terminal.naws.recv_dont = true;

                    if (!client->telopt.terminal.naws.sent_will
                    &&  !client->telopt.terminal.naws.sent_wont) {
                        client_write_to_terminal(
                            client, TELNET_IAC_WONT_NAWS, 0
                        );

                        client->telopt.terminal.naws.sent_wont = true;
                    }

                    break;
                }
                case TELNET_SB: {
                    auto packet = telnet_deserialize_naws_packet(data, size);

                    LOG(
                        "client: window size is %d x %d",
                        packet.width, packet.height
                    );

                    break;
                }
                default: {
                    break;
                }
            }

            break;
        }
        default: break;
    }
}

void client_pulse(CLIENT *client) {
    client_read_from_terminal(client);

    if (!client->telopt.terminal.naws.sent_will) {
        client_write_to_terminal(client, TELNET_IAC_WILL_NAWS, 0);
        client->telopt.terminal.naws.sent_will = true;
    }
}

bool client_read_from_terminal(CLIENT *client) {
    CLIP *clip = client->io.terminal.incoming.clip;

    if (clip_is_empty(clip)) {
        return false;
    }

    const unsigned char *data = clip_get_byte_array(clip);
    const size_t size = clip_get_size(clip);

    size_t nonblocking_sz = telnet_get_iac_nonblocking_length(data, size);

    if (nonblocking_sz) {
        CLIP *nonblocking = clip_shift(clip, nonblocking_sz);
        clip_destroy(nonblocking);

        return true;
    }

    size_t blocking_sz = telnet_get_iac_sequence_length(data, size);

    if (blocking_sz) {
        CLIP *blocking = clip_shift(clip, blocking_sz);

        client_handle_incoming_terminal_iac(
            client, clip_get_byte_array(blocking), clip_get_size(blocking)
        );

        clip_destroy(blocking);

        return true;
    }

    return false;
}

bool client_write_to_terminal(CLIENT *client, const char *str, size_t len) {
    return clip_append_byte_array(
        client->io.terminal.outgoing.clip,
        (const uint8_t *) str, len ? len : strlen(str)
    );
}

void client_fetch_incoming(CLIENT *client) {
    if (!client) {
        return;
    }

    CLIP *src = (
        global.terminal ? (
            global.terminal->io.client.outgoing.clip
        ) : global.io.incoming.clip
    );

    if (!clip_is_empty(src)) {
        CLIP *dst = client->io.terminal.incoming.clip;

        bool appended = clip_append_clip(dst, src);

        if (!appended) {
            FUSE();
        }

        clip_clear(src);
    }
}

void client_flush_outgoing(CLIENT *client) {
    if (!client) {
        return;
    }

    CLIP *src = client->io.terminal.outgoing.clip;

    if (!clip_is_empty(src)) {
        CLIP *dst = (
            global.terminal ? (
                global.terminal->io.client.incoming.clip
            ) : global.io.outgoing.clip
        );

        bool appended = clip_append_clip(dst, src);

        if (!appended) {
            FUSE();
        }

        clip_clear(src);
    }
}
