// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////
#include "all.h"
////////////////////////////////////////////////////////////////////////////////
#include <string.h>
////////////////////////////////////////////////////////////////////////////////


static bool client_read_from_terminal(CLIENT *client);
static bool client_write_to_terminal(CLIENT *, const char *str, size_t len);
static bool client_flush_outgoing(CLIENT *);
static bool client_fetch_incoming(CLIENT *);
static void client_update_terminal(CLIENT *client);
static void client_update_screen(CLIENT *client);
static void client_shutdown(CLIENT *);
static void client_handle_incoming_terminal_iac(
    CLIENT *, const uint8_t *data, size_t sz
);
static void client_handle_incoming_terminal_esc(
    CLIENT *, const uint8_t *data, size_t sz
);
static void client_handle_incoming_terminal_txt(
    CLIENT *, const uint8_t *data, size_t sz
);
static bool client_handle_incoming_terminal_txt_ctrl_key(
    CLIENT *, const uint8_t *data, size_t sz
);
static bool client_handle_incoming_terminal_esc_atomic_key(
    CLIENT *client, const uint8_t *data, size_t size
);
static bool client_handle_incoming_terminal_esc_tilde_key(
    CLIENT *client, const uint8_t *data, size_t size
);
static size_t client_get_esc_blocking_length(
    const unsigned char *data, size_t size
);
static size_t client_get_esc_nonblocking_length(
    const unsigned char *data, size_t length
);
static const char *client_get_esc_sequence_code(
    const unsigned char *data, size_t size, size_t index
);

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
    client->telopt.terminal.naws.remote.wanted = true;
    client->telopt.terminal.echo.local.wanted = true;
    client->telopt.terminal.sga.local.wanted = true;
    client->telopt.terminal.sga.remote.wanted = true;
    client->telopt.terminal.bin.local.wanted = true;
    client->telopt.terminal.bin.remote.wanted = true;
    //client_write_to_terminal(client, "\x1b[7l", 0);
}

void client_deinit(CLIENT *client) {
}

bool client_update(CLIENT *client) {
    if (!client || client->bitset.shutdown) {
        return false;
    }

    if (global.bitset.shutdown) {
        client_shutdown(client);
    }

    bool fetched = client_fetch_incoming(client);

    client_update_terminal(client);
    client_update_screen(client);

    return client_flush_outgoing(client) || fetched;
}

static void client_shutdown(CLIENT *client) {
    if (!client || client->bitset.shutdown) {
        return;
    }

    client_write_to_terminal(client, "\x1b[9999;1H", 0);
    client->bitset.shutdown = true;
}

static void client_handle_incoming_terminal_txt(
    CLIENT *client, const uint8_t *data, size_t size
) {
    if (!data || size < 1) {
        FUSE();
        return;
    }

    {
        char stackbuf[MAX_STACKBUF_SIZE] = "";

        for (size_t i = 0; i<size; ++i) {
            const char *code = telnet_uchar_to_printable(data[i]);

            if (code == nullptr) {
                code = telnet_uchar_to_string(data[i]);
            }

            if (code == nullptr) {
                code = "NUL";
                FUSE();
            }

            if (strlen(stackbuf) + strlen(code) + 2 >= sizeof(stackbuf)) {
                LOG("client: long TXT sequence (size %lu)", size);

                break;
            }

            strncat(stackbuf, code, sizeof(stackbuf) - (strlen(stackbuf) + 1));

            if (i + 1 < size) {
                strncat(
                    stackbuf, " ",  sizeof(stackbuf) - (strlen(stackbuf) + 1)
                );
            }
        }

        LOG("terminal:txt -> client: %s", stackbuf);
    }

    bool handled = client_handle_incoming_terminal_txt_ctrl_key(
        client, data, size
    );

    if (handled) {
        return;
    }
}

static void client_handle_incoming_terminal_esc(
    CLIENT *client, const uint8_t *data, size_t size
) {
    if (!data || size < 1 || data[0] != TERMINAL_ESC) {
        FUSE();
        return;
    }

    {
        char stackbuf[MAX_STACKBUF_SIZE] = "";

        for (size_t i = 0; i<size; ++i) {
            const char *code = client_get_esc_sequence_code(data, size, i);

            if (code == nullptr) {
                code = "NUL";
                FUSE();
            }

            if (strlen(stackbuf) + strlen(code) + 2 >= sizeof(stackbuf)) {
                LOG("client: long ESC sequence (size %lu)", size);

                break;
            }

            strncat(stackbuf, code, sizeof(stackbuf) - (strlen(stackbuf) + 1));

            if (i + 1 < size) {
                strncat(
                    stackbuf, " ",  sizeof(stackbuf) - (strlen(stackbuf) + 1)
                );
            }
        }

        LOG("terminal:esc -> client: %s", stackbuf);
    }

    bool handled = (
        client_handle_incoming_terminal_esc_atomic_key(client, data, size) ||
        client_handle_incoming_terminal_esc_tilde_key(client, data, size)
    );

    if (handled) {
        return;
    }
}

static void client_handle_incoming_terminal_iac(
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

        LOG("terminal:iac -> client: %s", stackbuf);
    }

    switch (data[2]) {
        case TELNET_OPT_NAWS: {
            switch (data[1]) {
                case TELNET_DO:
                case TELNET_DONT:
                case TELNET_WILL:
                case TELNET_WONT: {
                    auto response = telnet_opt_handle_remote(
                        &client->telopt.terminal.naws, data[1], data[2]
                    );

                    if (response.size) {
                        client_write_to_terminal(
                            client, (const char *) response.data, response.size
                        );
                    }

                    break;
                }
                case TELNET_SB: {
                    if (!client->telopt.terminal.naws.remote.enabled) {
                        break;
                    }

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
        case TELNET_OPT_ECHO: {
            switch (data[1]) {
                case TELNET_WILL:
                case TELNET_WONT:
                case TELNET_DO:
                case TELNET_DONT: {
                    auto response = telnet_opt_handle_local(
                        &client->telopt.terminal.echo, data[1], data[2]
                    );

                    if (response.size) {
                        client_write_to_terminal(
                            client, (const char *) response.data, response.size
                        );
                    }

                    break;
                }
                default: {
                    break;
                }
            }

            break;
        }
        case TELNET_OPT_SGA: {
            switch (data[1]) {
                case TELNET_WILL:
                case TELNET_WONT:
                case TELNET_DO:
                case TELNET_DONT: {
                    {
                        auto response = telnet_opt_handle_local(
                            &client->telopt.terminal.sga, data[1], data[2]
                        );

                        if (response.size) {
                            client_write_to_terminal(
                                client, (const char *) response.data,
                                response.size
                            );
                        }
                    }

                    {
                        auto response = telnet_opt_handle_remote(
                            &client->telopt.terminal.sga, data[1], data[2]
                        );

                        if (response.size) {
                            client_write_to_terminal(
                                client, (const char *) response.data,
                                response.size
                            );
                        }
                    }

                    break;
                }
                default: {
                    break;
                }
            }

            break;
        }
        case TELNET_OPT_BINARY: {
            switch (data[1]) {
                case TELNET_WILL:
                case TELNET_WONT:
                case TELNET_DO:
                case TELNET_DONT: {
                    {
                        auto response = telnet_opt_handle_local(
                            &client->telopt.terminal.bin, data[1], data[2]
                        );

                        if (response.size) {
                            client_write_to_terminal(
                                client, (const char *) response.data,
                                response.size
                            );
                        }
                    }

                    {
                        auto response = telnet_opt_handle_remote(
                            &client->telopt.terminal.bin, data[1], data[2]
                        );

                        if (response.size) {
                            client_write_to_terminal(
                                client, (const char *) response.data,
                                response.size
                            );
                        }
                    }

                    break;
                }
                default: {
                    break;
                }
            }

            break;
        }
        default: {
            switch (data[1]) {
                case TELNET_DO: {
                    client_write_to_terminal(client, TELNET_IAC_WONT, 0);
                    client_write_to_terminal(client, (const char *) data+2, 1);
                    break;
                }
                case TELNET_WILL: {
                    client_write_to_terminal(client, TELNET_IAC_DONT, 0);
                    client_write_to_terminal(client, (const char *) data+2, 1);
                    break;
                }
                default: {
                    break;
                }
            }

            break;
        }
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

        client_write_to_terminal(client, TERMINAL_ESC_HIDE_CURSOR, 0);
        client_write_to_terminal(client, TERMINAL_ESC_HOME_CURSOR, 0);
        client_write_to_terminal(
            client,
            (const char *) clip_get_byte_array(client->screen.clip),
            clip_get_size(client->screen.clip)
        );
        client_write_to_terminal(client, TERMINAL_ESC_HOME_CURSOR, 0);
        client_write_to_terminal(client, TERMINAL_ESC_SHOW_CURSOR, 0);
    }

    clip_destroy(clip);
}

static void client_update_screen(CLIENT *client) {
    if (client->bitset.shutdown) {
        return;
    }

    if (client->bitset.reformat) {
        client_screen_reformat(client);
    }

    if (client->bitset.redraw) {
        client_screen_redraw(client);
    }
}

static void client_update_terminal(CLIENT *client) {
    if (client->bitset.shutdown) {
        return;
    }

    while (client_read_from_terminal(client));

    if (client->telopt.terminal.naws.remote.wanted
    && !telnet_opt_remote_is_pending(client->telopt.terminal.naws)) {
        client_write_to_terminal(client, TELNET_IAC_DO_NAWS, 0);
        client->telopt.terminal.naws.remote.sent_do = true;
    }

    if (client->telopt.terminal.echo.local.wanted
    && !telnet_opt_local_is_pending(client->telopt.terminal.echo)) {
        client_write_to_terminal(client, TELNET_IAC_WILL_ECHO, 0);
        client->telopt.terminal.echo.local.sent_will = true;
    }

    if (client->telopt.terminal.sga.local.wanted
    && !telnet_opt_local_is_pending(client->telopt.terminal.sga)) {
        client_write_to_terminal(client, TELNET_IAC_WILL_SGA, 0);
        client->telopt.terminal.sga.local.sent_will = true;
    }

    if (client->telopt.terminal.sga.remote.wanted
    && !telnet_opt_remote_is_pending(client->telopt.terminal.sga)) {
        client_write_to_terminal(client, TELNET_IAC_DO_SGA, 0);
        client->telopt.terminal.sga.remote.sent_do = true;
    }

    if (client->telopt.terminal.bin.local.wanted
    && !telnet_opt_local_is_pending(client->telopt.terminal.bin)) {
        client_write_to_terminal(client, TELNET_IAC_WILL_BINARY, 3);
        client->telopt.terminal.bin.local.sent_will = true;
    }

    if (client->telopt.terminal.bin.remote.wanted
    && !telnet_opt_remote_is_pending(client->telopt.terminal.bin)) {
        client_write_to_terminal(client, TELNET_IAC_DO_BINARY, 3);
        client->telopt.terminal.bin.remote.sent_do = true;
    }
}

static bool client_read_from_terminal(CLIENT *client) {
    CLIP *clip = client->io.terminal.incoming.clip;

    if (clip_is_empty(clip)) {
        return false;
    }

    const unsigned char *data = clip_get_byte_array(clip);
    const size_t size = clip_get_size(clip);

    size_t nonblocking_sz = telnet_get_iac_nonblocking_length(data, size);

    if (nonblocking_sz) {
        nonblocking_sz = client_get_esc_nonblocking_length(data, size);

        if (nonblocking_sz) {
            CLIP *nonblocking = clip_shift(clip, nonblocking_sz);

            client_handle_incoming_terminal_txt(
                client,
                clip_get_byte_array(nonblocking), clip_get_size(nonblocking)
            );

            clip_destroy(nonblocking);

            return true;
        }

        size_t blocking_sz = client_get_esc_blocking_length(data, size);

        if (blocking_sz) {
            CLIP *blocking = clip_shift(clip, blocking_sz);

            client_handle_incoming_terminal_esc(
                client,
                clip_get_byte_array(blocking), clip_get_size(blocking)
            );

            clip_destroy(blocking);

            return true;
        }

        return false;
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

static bool client_write_to_terminal(
    CLIENT *client, const char *str, size_t len
) {
    client->io.terminal.outgoing.total += len;

    return clip_append_byte_array(
        client->io.terminal.outgoing.clip,
        (const uint8_t *) str, len ? len : strlen(str)
    );
}

static bool client_fetch_incoming(CLIENT *client) {
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

static bool client_flush_outgoing(CLIENT *client) {
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

static struct client_incoming_terminal_esc_tilde_key_type{
    const char *end;
    const char *str;
    size_t size;
    TERMINAL_KEY key;
} client_parse_incoming_terminal_esc_tilde_key(
    const char *str, size_t str_sz
) {
    const struct client_incoming_terminal_esc_tilde_key_type
    incomplete = { .end = nullptr },
    invalid    = { .end = str };

    if (!str_sz) {
        return incomplete;
    }
    else if (*str != TERMINAL_ESC) {
        return invalid;
    }
    else if (str_sz < 2) {
        return incomplete;
    }
    else if (str[1] != '[') {
        return invalid;
    }
    else if (str_sz < 3) {
        return incomplete;
    }

    const char *s = str + 2;
    const char *next = s;

    next = str_seg_skip_digits(s, str_sz - SIZEVAL(s - str));

    if (next == s) {
        return invalid;
    }
    else if (next == str + str_sz) {
        return incomplete;
    }
    else if (*next != '~') {
        return invalid;
    }

    size_t size = SIZEVAL(next - s);
    long value = 0;
    TERMINAL_KEY key = TERMINAL_KEY_NONE;

    if (str_seg_to_long(s, size, &value)) {
        switch (value) {
            case 2: key = TERMINAL_KEY_INS; break;
            case 3: key = TERMINAL_KEY_DEL; break;
            case 5: key = TERMINAL_KEY_PGUP; break;
            case 6: key = TERMINAL_KEY_PGDN; break;
            default: return invalid;
        }
    }

    return (struct client_incoming_terminal_esc_tilde_key_type) {
        .key = key,
        .str = s,
        .size = size,
        .end = ++next
    };
}

static bool client_handle_incoming_terminal_esc_tilde_key(
    CLIENT *client, const uint8_t *data, size_t size
) {
    const char *str = (const char *) data;
    auto result = client_parse_incoming_terminal_esc_tilde_key(str, size);

    if (result.key == TERMINAL_KEY_NONE) {
        return false;
    }

    switch (result.key) {
        case TERMINAL_KEY_PGUP: {
            LOG("user pressed PGUP");
            break;
        }
        case TERMINAL_KEY_PGDN: {
            LOG("user pressed PGDN");
            break;
        }
        case TERMINAL_KEY_INS: {
            LOG("user pressed INS");
            break;
        }
        case TERMINAL_KEY_DEL: {
            LOG("user pressed DEL");
            break;
        }
        default: break;
    }

    return true;
}

static struct client_incoming_terminal_esc_atomic_key_type{
    const char *end;
    const char *str;
    size_t size;
    TERMINAL_KEY key;
} client_parse_incoming_terminal_esc_atomic_key(
    const char *str, size_t str_sz
) {
    const struct client_incoming_terminal_esc_atomic_key_type
    incomplete = { .end = nullptr },
    invalid    = { .end = str };

    if (!str_sz) {
        return incomplete;
    }
    else if (*str != TERMINAL_ESC) {
        return invalid;
    }
    else if (str_sz < 2) {
        return incomplete;
    }
    else if (str[1] != '[') {
        return invalid;
    }
    else if (str_sz < 3) {
        return incomplete;
    }

    const char *s = str + 2;
    const char *next = s;

    next = str_seg_skip_utf8_symbol(s, str_sz - SIZEVAL(s - str));

    if (next == s) {
        return invalid;
    }

    size_t size = SIZEVAL(next - s);
    TERMINAL_KEY key = TERMINAL_KEY_NONE;

    switch (*s) {
        case 'A': key = TERMINAL_KEY_UP; break;
        case 'B': key = TERMINAL_KEY_DOWN; break;
        case 'C': key = TERMINAL_KEY_RIGHT; break;
        case 'D': key = TERMINAL_KEY_LEFT; break;
        case 'E': key = TERMINAL_KEY_NOP; break;
        case 'H': key = TERMINAL_KEY_HOME; break;
        case 'F': key = TERMINAL_KEY_END; break;
        default: return invalid;
    }

    return (struct client_incoming_terminal_esc_atomic_key_type) {
        .key = key,
        .str = s,
        .size = size,
        .end = next
    };
}

static bool client_handle_incoming_terminal_esc_atomic_key(
    CLIENT *client, const uint8_t *data, size_t size
) {
    const char *str = (const char *) data;
    auto result = client_parse_incoming_terminal_esc_atomic_key(str, size);

    if (result.key == TERMINAL_KEY_NONE) {
        return false;
    }

    switch (result.key) {
        case TERMINAL_KEY_UP: {
            LOG("user pressed UP");
            break;
        }
        case TERMINAL_KEY_DOWN: {
            LOG("user pressed DOWN");
            break;
        }
        case TERMINAL_KEY_LEFT: {
            LOG("user pressed LEFT");
            break;
        }
        case TERMINAL_KEY_RIGHT: {
            LOG("user pressed RIGHT");
            break;
        }
        case TERMINAL_KEY_NOP: {
            LOG("user pressed NOP");
            break;
        }
        case TERMINAL_KEY_HOME: {
            LOG("user pressed HOME");
            break;
        }
        case TERMINAL_KEY_END: {
            LOG("user pressed END");
            break;
        }
        default: break;
    }

    return true;
}

static size_t client_get_esc_blocking_length(
    const unsigned char *data, size_t size
) {
    if (!data) {
        FUSE();
        return 0;
    }

    if (size < 1 || *data != TERMINAL_ESC) {
        return 0;
    }

    const char *str = (const char *) data;
    const char *next = str;

    if (next == str) {
        next = client_parse_incoming_terminal_esc_atomic_key(str, size).end;
    }

    if (next == str) {
        next = client_parse_incoming_terminal_esc_tilde_key(str, size).end;
    }

    return next == nullptr ? 0 : next == str ? 1 : SIZEVAL(next - str);
}

static size_t client_get_esc_nonblocking_length(
    const unsigned char *data, size_t length
) {
    if (!data) {
        FUSE();
        return 0;
    }

    const char *esc = memchr(data, TERMINAL_ESC, length);

    return esc ? SIZEVAL(esc - ((const char *) data)) : length;
}

static const char *client_get_esc_sequence_code(
    const unsigned char *data, size_t size, size_t index
) {
    if (*data != TERMINAL_ESC || index >= size) {
        return nullptr;
    }

    const char *code = telnet_uchar_to_printable(data[index]);

    return code ? code : telnet_uchar_to_string(data[index]);
}

static bool client_handle_incoming_terminal_txt_ctrl_key(
    CLIENT *client, const uint8_t *data, size_t size
) {
    if (!size) {
        return false;
    }

    switch (data[0]) {
        case TERMINAL_CTRL_C:
        case TERMINAL_CTRL_D:
        case TERMINAL_CTRL_Q: {
            LOG("user terminates the session");
            global.bitset.shutdown = true;
            break;
        }
        default: return false;
    }

    return true;
}
