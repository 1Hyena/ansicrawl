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

    LOG("client read from terminal: %lu", clip_get_size(clip));

    for (;;) {
        if (clip_get_size(clip) >= 3) {
            if (clip_get_byte_at(clip, 0) == TELNET_IAC
            &&  clip_get_byte_at(clip, 2) == TELNET_OPT_NAWS) {
                if (clip_get_byte_at(clip, 1) == TELNET_DO) {
                    client->telopt.terminal.naws.recv_do = true;
                    clip_destroy(clip_shift(clip, 3));
                    continue;
                }
                else if (clip_get_byte_at(clip, 1) == TELNET_DONT) {
                    client->telopt.terminal.naws.recv_dont = true;

                    if (!client->telopt.terminal.naws.sent_will
                    &&  !client->telopt.terminal.naws.sent_wont) {
                        client_write_to_terminal(
                            client, TELNET_IAC_WONT_NAWS, 0
                        );

                        client->telopt.terminal.naws.sent_wont = true;
                    }

                    clip_destroy(clip_shift(clip, 3));
                    continue;
                }
                else if (clip_get_byte_at(clip, 1) == TELNET_SB) {
                    LOG("client received NAWS data from terminal");
                }
                else FUSE();
            }
        }

        break;
    }

    clip_clear(clip);
    return true;
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
