// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////
#include "all.h"
////////////////////////////////////////////////////////////////////////////////
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
////////////////////////////////////////////////////////////////////////////////


static void terminal_die(TERMINAL *terminal);
static void terminal_enable_raw_mode(TERMINAL *terminal);
static void terminal_disable_raw_mode(TERMINAL *terminal);
static void terminal_update_interface(TERMINAL *terminal);
static void terminal_update_client(TERMINAL *terminal);
static void terminal_update_state(TERMINAL *terminal);
static bool terminal_read_from_client(TERMINAL *);
static bool terminal_read_from_interface(TERMINAL *);
static bool terminal_write_to_interface(TERMINAL *, const char *, size_t len);
static bool terminal_write_to_client(TERMINAL *, const char *, size_t len);
static bool terminal_flush_outgoing(TERMINAL *);
static bool terminal_fetch_incoming(TERMINAL *);
static void terminal_shutdown(TERMINAL *);
static void terminal_handle_incoming_client_iac(
    TERMINAL *, const uint8_t *data, size_t sz
);
static void terminal_handle_incoming_client_txt(
    TERMINAL *terminal, const uint8_t *data, size_t size
);
static void terminal_handle_incoming_interface_iac(
    TERMINAL *, const uint8_t *data, size_t sz
);
static void terminal_handle_incoming_interface_esc(
    TERMINAL *, const uint8_t *data, size_t sz
);
static void terminal_handle_incoming_interface_txt(
    TERMINAL *, const uint8_t *data, size_t sz
);
static bool terminal_handle_incoming_interface_esc_screen_size(
    TERMINAL *terminal, const uint8_t *data, size_t size
);
static size_t terminal_get_esc_blocking_length(
    const unsigned char *, size_t size
);
static size_t terminal_get_esc_nonblocking_length(
    const unsigned char *, size_t size
);
static const char *terminal_get_esc_sequence_code(
    const unsigned char *data, size_t size, size_t index
);

TERMINAL *terminal_create() {
    TERMINAL *terminal = mem_new_terminal();

    if (!terminal) {
        return nullptr;
    }

    if (!(terminal->io.interface.incoming.clip = clip_create_byte_array())
    ||  !(terminal->io.interface.outgoing.clip = clip_create_byte_array())
    ||  !(terminal->io.client.incoming.clip = clip_create_byte_array())
    ||  !(terminal->io.client.outgoing.clip = clip_create_byte_array())) {
        terminal_destroy(terminal);

        return nullptr;
    }

    return terminal;
}

void terminal_destroy(TERMINAL *terminal) {
    if (!terminal) {
        return;
    }

    clip_destroy(terminal->io.interface.incoming.clip);
    clip_destroy(terminal->io.interface.outgoing.clip);
    clip_destroy(terminal->io.client.incoming.clip);
    clip_destroy(terminal->io.client.outgoing.clip);

    mem_free_terminal(terminal);
}

void terminal_init(TERMINAL *terminal) {
    if (!terminal) {
        return;
    }

    terminal_enable_raw_mode(terminal);

    if (terminal->bitset.broken) {
        return;
    }

    terminal->telopt.client.naws.local.wanted = true;
    terminal->telopt.client.sga.local.wanted = true;
    terminal->telopt.client.sga.remote.wanted = true;

    terminal->state = TERMINAL_INIT_EDITOR;
}

void terminal_deinit(TERMINAL *terminal) {
    if (!terminal) {
        return;
    }

    terminal_disable_raw_mode(terminal);

    terminal->state = TERMINAL_STATE_NONE;
}

bool terminal_update(TERMINAL *terminal) {
    if (!terminal) {
        return false;
    }

    if (terminal->bitset.broken) {
        global.bitset.shutdown = true;
        return false;
    }

    if (global.bitset.shutdown) {
        terminal_shutdown(terminal);
    }

    bool fetched = terminal_fetch_incoming(terminal);

    terminal_update_interface(terminal);
    terminal_update_state(terminal);
    terminal_update_client(terminal);

    return terminal_flush_outgoing(terminal) || fetched;
}

static void terminal_die(TERMINAL *terminal) {
    terminal->bitset.broken = true;
    global.bitset.broken = true;
}

static void terminal_disable_raw_mode(TERMINAL *terminal) {
    if (terminal->bitset.raw == false) {
        return;
    }

    auto ret = tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal->original);

    if (ret == -1) {
        BUG("%s", strerror(errno));
        terminal_die(terminal);
        return;
    }

    terminal_write_to_interface(terminal, "\x1b[?47l", 0);
    terminal_write_to_interface(terminal, "\x1b" "8", 0);
    terminal_flush_outgoing(terminal);

    terminal->bitset.raw = false;
    LOG("terminal: disabled raw mode");
}

static void terminal_enable_raw_mode(TERMINAL *terminal) {
    if (tcgetattr(STDIN_FILENO, &terminal->original) == -1) {
        BUG("%s", strerror(errno));
        terminal_die(terminal);
        return;
    }

    struct termios raw = terminal->original;
    raw.c_iflag &= (tcflag_t) ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= (tcflag_t) ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= (tcflag_t) ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 10;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        BUG("%s", strerror(errno));
        terminal_die(terminal);
        return;
    }

    terminal->bitset.raw = true;

    terminal_write_to_interface(terminal, "\x1b" "7", 0);
    terminal_write_to_interface(terminal, "\x1b[?47h", 0);
    LOG("terminal: enabled raw mode");
}

static bool terminal_task_ask_screen_size(TERMINAL *terminal) {
    struct winsize ws;

    if (true || ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (!terminal_write_to_interface(terminal, "\x1b[999C\x1b[999B", 0)
        ||  !terminal_write_to_interface(terminal, "\x1b[6n", 0)) {
            BUG("failed to ask screen size");
            terminal_die(terminal);
            return false;
        }

        terminal->state = TERMINAL_GET_SCREEN_SIZE;

        return false;
    }

    terminal->screen.width = ws.ws_col;
    terminal->screen.height = ws.ws_row;
    terminal->bitset.reformat = false;

    terminal->state = TERMINAL_IDLE;

    return true;
}

static bool terminal_task_init_editor(TERMINAL *terminal) {
    terminal->screen.cx = 0;
    terminal->screen.cy = 0;
    terminal->state = TERMINAL_ASK_SCREEN_SIZE;

    return true;
}

static bool terminal_task_idle(TERMINAL *terminal) {
    if (terminal->bitset.reformat) {
        terminal->state = TERMINAL_ASK_SCREEN_SIZE;

        return true;
    }

    return false;
}

static void terminal_shutdown(TERMINAL *terminal) {
    if (!terminal || terminal->bitset.shutdown) {
        return;
    }

    terminal->bitset.shutdown = true;
}

static void terminal_update_state(TERMINAL *terminal) {
    if (terminal->bitset.shutdown) {
        return;
    }

    for (bool repeat = true; repeat && !terminal->bitset.broken;) {
        switch (terminal->state) {
            case MAX_TERMINAL_STATE:
            case TERMINAL_STATE_NONE: {
                FUSE();
                terminal_die(terminal);
                return;
            }
            case TERMINAL_ASK_SCREEN_SIZE: {
                repeat = terminal_task_ask_screen_size(terminal);
                break;
            }
            case TERMINAL_GET_SCREEN_SIZE: {
                repeat = false;
                break;
            }
            case TERMINAL_INIT_EDITOR: {
                repeat = terminal_task_init_editor(terminal);
                break;
            }
            case TERMINAL_IDLE: {
                repeat = terminal_task_idle(terminal);
                break;
            }
        }
    }
}

static void terminal_update_interface(TERMINAL *terminal) {
    while (terminal_read_from_interface(terminal));
}

static void terminal_update_client(TERMINAL *terminal) {
    if (terminal->screen.width
    ||  terminal->screen.height
    ||  terminal->bitset.shutdown) {
        while (terminal_read_from_client(terminal));
    }

    if (terminal->screen.width || terminal->screen.height) {
        // Without this check it may happen that we report the screen size as
        // 0 x 0.

        if (terminal->telopt.client.naws.local.enabled) {
            uint16_t width  = USHORTVAL(terminal->screen.width);
            uint16_t height = USHORTVAL(terminal->screen.height);

            if (width  != terminal->telopt.client.state.local.naws.width
            ||  height != terminal->telopt.client.state.local.naws.height) {
                auto packet = telnet_serialize_naws_message(width, height);

                terminal_write_to_client(
                    terminal, (const char *) packet.data, packet.size
                );

                terminal->telopt.client.state.local.naws.width = width;
                terminal->telopt.client.state.local.naws.height = height;
            }
        }

        if (terminal->telopt.client.naws.local.wanted
        && !telnet_opt_local_is_pending(terminal->telopt.client.naws)) {
            terminal_write_to_client(terminal, TELNET_IAC_WILL_NAWS, 0);
            terminal->telopt.client.naws.local.sent_will = true;
        }
    }

    if (terminal->telopt.client.sga.local.wanted
    && !telnet_opt_local_is_pending(terminal->telopt.client.sga)) {
        terminal_write_to_client(terminal, TELNET_IAC_WILL_SGA, 0);
        terminal->telopt.client.sga.local.sent_will = true;
    }

    if (terminal->telopt.client.sga.remote.wanted
    && !telnet_opt_remote_is_pending(terminal->telopt.client.sga)) {
        terminal_write_to_client(terminal, TELNET_IAC_DO_SGA, 0);
        terminal->telopt.client.sga.remote.sent_do = true;
    }
}

static void terminal_handle_incoming_client_iac(
    TERMINAL *terminal, const uint8_t *data, size_t size
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
                LOG("terminal: long IAC sequence (size %lu)", size);

                break;
            }

            strncat(stackbuf, code, sizeof(stackbuf) - (strlen(stackbuf) + 1));

            if (i + 1 < size) {
                strncat(
                    stackbuf, " ",  sizeof(stackbuf) - (strlen(stackbuf) + 1)
                );
            }
        }

        LOG("client:iac -> terminal: %s", stackbuf);
    }

    switch (data[2]) {
        case TELNET_OPT_NAWS: {
            switch (data[1]) {
                case TELNET_DO:
                case TELNET_DONT:
                case TELNET_WILL:
                case TELNET_WONT: {
                    auto response = telnet_opt_handle_local(
                        &terminal->telopt.client.naws, data[1], data[2]
                    );

                    if (response.size) {
                        terminal_write_to_client(
                            terminal, (const char *) response.data,
                            response.size
                        );
                    }

                    if (response.size == 3
                    &&  response.data[0] == TELNET_IAC
                    &&  response.data[1] == TELNET_WILL
                    &&  response.data[2] == TELNET_OPT_NAWS) {
                        uint16_t w = USHORTVAL(terminal->screen.width);
                        uint16_t h = USHORTVAL(terminal->screen.height);
                        auto packet = telnet_serialize_naws_message(w, h);

                        terminal_write_to_client(
                            terminal, (const char *) packet.data, packet.size
                        );

                        terminal->telopt.client.state.local.naws.width = w;
                        terminal->telopt.client.state.local.naws.height = h;
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
                    auto response = telnet_opt_handle_remote(
                        &terminal->telopt.client.echo, data[1], data[2]
                    );

                    if (response.size) {
                        terminal_write_to_client(
                            terminal, (const char *) response.data,
                            response.size
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
                            &terminal->telopt.client.sga, data[1], data[2]
                        );

                        if (response.size) {
                            terminal_write_to_client(
                                terminal, (const char *) response.data,
                                response.size
                            );
                        }
                    }

                    {
                        auto response = telnet_opt_handle_remote(
                            &terminal->telopt.client.sga, data[1], data[2]
                        );

                        if (response.size) {
                            terminal_write_to_client(
                                terminal, (const char *) response.data,
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
                    terminal_write_to_client(terminal, TELNET_IAC_WONT, 0);
                    terminal_write_to_client(
                        terminal, (const char *) data+2, 1
                    );
                    break;
                }
                case TELNET_WILL: {
                    terminal_write_to_client(terminal, TELNET_IAC_DONT, 0);
                    terminal_write_to_client(
                        terminal, (const char *) data+2, 1
                    );
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

static void terminal_handle_incoming_client_txt(
    TERMINAL *terminal, const uint8_t *data, size_t size
) {
    if (!data || size < 1) {
        FUSE();
        return;
    }

    LOG("client:txt -> terminal: %lu byte%s", size, size == 1 ? "" : "s");

    terminal_write_to_interface(terminal, (const char *) data, size);
}

static void terminal_handle_incoming_interface_iac(
    TERMINAL *terminal, const uint8_t *data, size_t size
) {
    if (!data || size < 2 || data[0] != TELNET_IAC) {
        FUSE();
        return;
    }

    terminal_write_to_client(terminal, (const char *) data, size);
}

static void terminal_handle_incoming_interface_esc(
    TERMINAL *terminal, const uint8_t *data, size_t size
) {
    if (!data || size < 1 || data[0] != TERMINAL_ESC) {
        FUSE();
        return;
    }

    {
        char stackbuf[MAX_STACKBUF_SIZE] = "";

        for (size_t i = 0; i<size; ++i) {
            const char *code = terminal_get_esc_sequence_code(data, size, i);

            if (code == nullptr) {
                code = "NUL";
                FUSE();
            }

            if (strlen(stackbuf) + strlen(code) + 2 >= sizeof(stackbuf)) {
                LOG("terminal: long ESC sequence (size %lu)", size);

                break;
            }

            strncat(stackbuf, code, sizeof(stackbuf) - (strlen(stackbuf) + 1));

            if (i + 1 < size) {
                strncat(
                    stackbuf, " ",  sizeof(stackbuf) - (strlen(stackbuf) + 1)
                );
            }
        }

        LOG("interface:esc -> terminal: %s", stackbuf);
    }

    bool handled = terminal_handle_incoming_interface_esc_screen_size(
        terminal, data, size
    );

    if (handled) {
        return;
    }

    terminal_write_to_client(terminal, (const char *) data, size);
}

static void terminal_handle_incoming_interface_txt(
    TERMINAL *terminal, const uint8_t *data, size_t size
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
                LOG("terminal: long TXT sequence (size %lu)", size);

                break;
            }

            strncat(stackbuf, code, sizeof(stackbuf) - (strlen(stackbuf) + 1));

            if (i + 1 < size) {
                strncat(
                    stackbuf, " ",  sizeof(stackbuf) - (strlen(stackbuf) + 1)
                );
            }
        }

        LOG("interface:txt -> terminal: %s", stackbuf);
    }

    terminal_write_to_client(terminal, (const char *) data, size);
}

static bool terminal_read_from_interface(TERMINAL *terminal) {
    CLIP *clip = terminal->io.interface.incoming.clip;

    if (clip_is_empty(clip)) {
        return false;
    }

    const unsigned char *data = clip_get_byte_array(clip);
    const size_t size = clip_get_size(clip);

    size_t nonblocking_sz = telnet_get_iac_nonblocking_length(data, size);

    if (nonblocking_sz) {
        nonblocking_sz = terminal_get_esc_nonblocking_length(data, size);

        if (nonblocking_sz) {
            CLIP *nonblocking = clip_shift(clip, nonblocking_sz);

            terminal_handle_incoming_interface_txt(
                terminal,
                clip_get_byte_array(nonblocking), clip_get_size(nonblocking)
            );

            clip_destroy(nonblocking);

            return true;
        }

        size_t blocking_sz = terminal_get_esc_blocking_length(data, size);

        if (blocking_sz) {
            CLIP *blocking = clip_shift(clip, blocking_sz);

            terminal_handle_incoming_interface_esc(
                terminal,
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

        terminal_handle_incoming_interface_iac(
            terminal, clip_get_byte_array(blocking), clip_get_size(blocking)
        );

        clip_destroy(blocking);

        return true;
    }

    return false;
}

static bool terminal_read_from_client(TERMINAL *terminal) {
    CLIP *clip = terminal->io.client.incoming.clip;

    if (clip_is_empty(clip)) {
        return false;
    }

    const unsigned char *data = clip_get_byte_array(clip);
    const size_t size = clip_get_size(clip);

    size_t nonblocking_sz = telnet_get_iac_nonblocking_length(data, size);

    if (nonblocking_sz) {
        CLIP *nonblocking = clip_shift(clip, nonblocking_sz);

        terminal_handle_incoming_client_txt(
            terminal, clip_get_byte_array(nonblocking),
            clip_get_size(nonblocking)
        );

        clip_destroy(nonblocking);

        return true;
    }

    size_t blocking_sz = telnet_get_iac_sequence_length(data, size);

    if (blocking_sz) {
        CLIP *blocking = clip_shift(clip, blocking_sz);

        terminal_handle_incoming_client_iac(
            terminal, clip_get_byte_array(blocking), clip_get_size(blocking)
        );

        clip_destroy(blocking);

        return true;
    }

    return false;
}

static bool terminal_write_to_interface(
    TERMINAL *terminal, const char *str, size_t len
) {
    return clip_append_byte_array(
        terminal->io.interface.outgoing.clip,
        (const uint8_t *) str, len ? len : strlen(str)
    );
}

static bool terminal_write_to_client(
    TERMINAL *terminal, const char *str, size_t len
) {
    if (terminal->bitset.shutdown) {
        return true;
    }

    return clip_append_byte_array(
        terminal->io.client.outgoing.clip,
        (const uint8_t *) str, len ? len : strlen(str)
    );
}

static bool terminal_fetch_incoming(TERMINAL *terminal) {
    if (!terminal) {
        return false;
    }

    bool fetched = false;

    {
        CLIP *src = global.io.incoming.clip;

        if (!clip_is_empty(src)) {
            CLIP *dst = terminal->io.interface.incoming.clip;

            bool appended = clip_append_clip(dst, src);

            if (!appended) {
                FUSE();
            }

            clip_clear(src);

            fetched = true;
        }
    }

    {
        CLIP *src = global.client->io.terminal.outgoing.clip;

        if (!clip_is_empty(src)) {
            CLIP *dst = terminal->io.client.incoming.clip;

            bool appended = clip_append_clip(dst, src);

            if (!appended) {
                FUSE();
            }

            clip_clear(src);

            fetched = true;
        }
    }

    return fetched;
}

static bool terminal_flush_outgoing(TERMINAL *terminal) {
    if (!terminal) {
        return false;
    }

    bool flushed = false;

    {
        CLIP *src = terminal->io.interface.outgoing.clip;

        if (!clip_is_empty(src)) {
            CLIP *dst = global.io.outgoing.clip;

            bool appended = clip_append_clip(dst, src);

            if (!appended) {
                FUSE();
            }

            clip_clear(src);

            flushed = true;
        }
    }

    {
        CLIP *src = terminal->io.client.outgoing.clip;

        if (!clip_is_empty(src)) {
            CLIP *dst = global.client->io.terminal.incoming.clip;

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

static size_t terminal_get_esc_nonblocking_length(
    const unsigned char *data, size_t length
) {
    if (!data) {
        FUSE();
        return 0;
    }

    const char *esc = memchr(data, TERMINAL_ESC, length);

    return esc ? SIZEVAL(esc - ((const char *) data)) : length;
}

static struct terminal_incoming_interface_esc_screen_size_type{
    const char *end;
    struct {
        const char *str;
        size_t size;
    } height;
    struct {
        const char *str;
        size_t size;
    } width;
} terminal_parse_incoming_interface_esc_screen_size(
    const char *str, size_t str_sz
) {
    struct terminal_incoming_interface_esc_screen_size_type result = {
        .end = str
    };

    if (str_sz <= 2 || *str != TERMINAL_ESC || str[1] != '[') {
        return result;
    }

    const char *s = str + 2;
    const char *next = s;

    next = str_seg_skip_digits(s, str_sz - SIZEVAL(s - str));

    if (next > str + str_sz || *next != ';') return result;

    result.height.str = s;
    result.height.size = SIZEVAL(next - s);

    s = ++next;

    if (s > str + str_sz) return result;

    next = str_seg_skip_digits(s, str_sz - SIZEVAL(s - str));

    if (next > str + str_sz || *next != 'R') return result;

    result.width.str = s;
    result.width.size = SIZEVAL(next - s);

    result.end = ++next;

    return result;
}

static bool terminal_handle_incoming_interface_esc_screen_size(
    TERMINAL *terminal, const uint8_t *data, size_t size
) {
    if (terminal->state != TERMINAL_GET_SCREEN_SIZE) {
        return false;
    }

    const char *str = (const char *) data;
    auto result = terminal_parse_incoming_interface_esc_screen_size(str, size);

    long rows = 0;
    long cols = 0;

    bool success = (
        str_seg_to_long(result.width.str, result.width.size, &cols) &&
        str_seg_to_long(result.height.str, result.height.size, &rows)
    );

    if (!success) {
        BUG("failed to get screen size");
        terminal_die(terminal);
        return false;
    }

    terminal->screen.width = cols;
    terminal->screen.height = rows;
    terminal->bitset.reformat = false;

    terminal->state = TERMINAL_IDLE;

    return true;
}

static size_t terminal_get_esc_blocking_length(
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
    auto result = terminal_parse_incoming_interface_esc_screen_size(str, size);
    const char *next = result.end;

    return next == str ? 1 : SIZEVAL(next - str);
}

static const char *terminal_get_esc_sequence_code(
    const unsigned char *data, size_t size, size_t index
) {
    if (*data != TERMINAL_ESC || index >= size) {
        return nullptr;
    }

    const char *code = telnet_uchar_to_printable(data[index]);

    return code ? code : telnet_uchar_to_string(data[index]);
}
