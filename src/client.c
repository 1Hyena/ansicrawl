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
    || !(client->io.terminal.outgoing.clip = clip_create_byte_array())
    || !(client->screen.clip = clip_create_byte_array())) {
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
    clip_destroy(client->screen.clip);

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

    {
        char stackbuf[MAX_STACKBUF_SIZE] = "";

        for (size_t i = 0; i<size; ++i) {
            const char *code = telnet_get_iac_sequence_code(data, size, i);

            if (code == nullptr) {
                code = "NUL";
                FUSE();
            }

            if (strlen(stackbuf) + strlen(code) + 2 >= sizeof(stackbuf)) {
                LOG("client: long IAC sequence (size %lu)", size);

                break;
            }

            strncat(stackbuf, code, sizeof(stackbuf) - (strlen(stackbuf) + 1));

            if (i + 1 < size) {
                strncat(
                    stackbuf, " ",  sizeof(stackbuf) - (strlen(stackbuf) + 1)
                );
            }
        }

        LOG("client: got %s", stackbuf);
    }

    switch (data[2]) {
        case TELNET_OPT_NAWS: {
            switch (data[1]) {
                case TELNET_WILL: {
                    client->telopt.terminal.naws.recv_will = true;
                    break;
                }
                case TELNET_WONT: {
                    client->telopt.terminal.naws.recv_wont = true;

                    if (!client->telopt.terminal.naws.sent_do
                    &&  !client->telopt.terminal.naws.sent_dont) {
                        client_write_to_terminal(
                            client, TELNET_IAC_DONT_NAWS, 0
                        );

                        client->telopt.terminal.naws.sent_dont = true;
                    }

                    break;
                }
                case TELNET_SB: {
                    auto message = telnet_deserialize_naws_packet(data, size);

                    if (client->screen.width != message.width
                    ||  client->screen.height != message.height) {
                        client->screen.width = message.width;
                        client->screen.height = message.height;
                        client->bitset.reformat = true;
                    }

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

static void client_screen_reformat(CLIENT *client) {
    client->bitset.reformat = false;
    client->bitset.redraw = true;
}

static void client_screen_redraw(CLIENT *client) {
    client->bitset.redraw = false;

    CLIP *clip = clip_create_byte_array();

    for (size_t y=0; y<client->screen.height; ++y) {
        for (size_t x=0; x<client->screen.width; ++x) {
            if (x == 0
            ||  y == 0
            ||  x + 1 == client->screen.width
            ||  y + 1 == client->screen.height) {
                clip_push_byte(clip, (uint8_t) '#');
            }
            else {
                clip_push_byte(clip, (uint8_t) ' ');
            }
        }

        if (y + 1 < client->screen.height) {
            clip_push_byte(clip, (uint8_t) '\r');
            clip_push_byte(clip, (uint8_t) '\n');
        }
    }

    clip_push_byte(clip, 0);

    size_t hash = str_hash((const char *) clip_get_byte_array(clip));

    if (hash != client->screen.hash) {
        clip_pop_byte(clip);
        clip_swap(clip, client->screen.clip);
        client->screen.hash = hash;

        client_write_to_terminal(client, TELNET_ANSI_HIDE_CURSOR, 0);
        client_write_to_terminal(client, TELNET_ANSI_HOME_CURSOR, 0);
        client_write_to_terminal(
            client,
            (const char *) clip_get_byte_array(client->screen.clip),
            clip_get_size(client->screen.clip)
        );
        client_write_to_terminal(client, TELNET_ANSI_HOME_CURSOR, 0);
        client_write_to_terminal(client, TELNET_ANSI_SHOW_CURSOR, 0);
    }

    clip_destroy(clip);
}

bool client_update(CLIENT *client) {
    if (!client) {
        return false;
    }

    bool fetched = client_fetch_incoming(client);

    while (client_read_from_terminal(client));

    if (!client->telopt.terminal.naws.sent_do) {
        client_write_to_terminal(client, TELNET_IAC_DO_NAWS, 0);
        client->telopt.terminal.naws.sent_do = true;
    }

    if (client->bitset.reformat) {
        client_screen_reformat(client);
    }

    if (client->bitset.redraw) {
        client_screen_redraw(client);
    }

    return client_flush_outgoing(client) || fetched;
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

bool client_fetch_incoming(CLIENT *client) {
    if (!client) {
        return false;
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

        return true;
    }

    return false;
}

bool client_flush_outgoing(CLIENT *client) {
    if (!client) {
        return false;
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

        return true;
    }

    return false;
}
