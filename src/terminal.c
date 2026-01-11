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
static void terminal_update_dispatcher(TERMINAL *terminal);
static void terminal_update_client(TERMINAL *terminal);
static void terminal_update_state(TERMINAL *terminal);
static bool terminal_read_from_client(TERMINAL *);
static bool terminal_read_from_dispatcher(TERMINAL *);
static bool terminal_write_to_dispatcher(TERMINAL *, const char *, size_t len);
static bool terminal_write_to_client(TERMINAL *, const char *, size_t len);
static bool terminal_flush_outgoing(TERMINAL *);
static void terminal_shutdown(TERMINAL *);
static void terminal_handle_incoming_client_iac(
    TERMINAL *, const uint8_t *data, size_t sz
);
static void terminal_handle_incoming_client_txt(
    TERMINAL *terminal, const uint8_t *data, size_t size
);
static void terminal_handle_incoming_dispatcher_iac(
    TERMINAL *, const uint8_t *data, size_t sz
);
static void terminal_handle_incoming_dispatcher_esc(
    TERMINAL *, const uint8_t *data, size_t sz
);
static void terminal_handle_incoming_dispatcher_txt(
    TERMINAL *, const uint8_t *data, size_t sz
);
static bool terminal_handle_incoming_dispatcher_esc_screen_size(
    TERMINAL *terminal, const uint8_t *data, size_t size
);
static size_t terminal_get_esc_blocking_length(
    const unsigned char *, size_t size
);
static size_t terminal_get_esc_nonblocking_length(
    const unsigned char *, size_t size
);


TERMINAL *terminal_create() {
    TERMINAL *terminal = mem_new_terminal();

    if (!terminal) {
        return nullptr;
    }

    if (!(terminal->io.dispatcher.incoming.clip = clip_create_byte_array())
    ||  !(terminal->io.dispatcher.outgoing.clip = clip_create_byte_array())
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

    clip_destroy(terminal->io.dispatcher.incoming.clip);
    clip_destroy(terminal->io.dispatcher.outgoing.clip);
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
    terminal->telopt.client.eor.local.wanted = true;

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

    terminal_update_dispatcher(terminal);
    terminal_update_state(terminal);
    terminal_update_client(terminal);

    return terminal_flush_outgoing(terminal);
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

    LOG("terminal: enabled raw mode");
}

static bool terminal_task_ask_screen_size(TERMINAL *terminal) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        terminal_write_to_dispatcher(terminal, TERMINAL_ESC_SAVE_CURSOR, 0);
        terminal_write_to_dispatcher(terminal, TERMINAL_ESC_CURSOR_MAX_DOWN, 0);
        terminal_write_to_dispatcher(terminal, TERMINAL_ESC_CURSOR_MAX_RIGHT,0);
        terminal_write_to_dispatcher(terminal, TERMINAL_ESC_REQUEST_CURSOR, 0);
        terminal_write_to_dispatcher(terminal, TERMINAL_ESC_RESTORE_CURSOR, 0);

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

static void terminal_update_dispatcher(TERMINAL *terminal) {
    while (terminal_read_from_dispatcher(terminal));
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

    if (terminal->telopt.client.eor.local.wanted
    && !telnet_opt_local_is_pending(terminal->telopt.client.eor)) {
        terminal_write_to_client(terminal, TELNET_IAC_WILL_EOR, 0);
        terminal->telopt.client.eor.local.sent_will = true;
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

    log_iac("client", "terminal", data, size);

    struct {
        bool (*write) (TERMINAL *, const char *, size_t);
        struct telnet_opt_type* opt;
        TELNET_FLAG flags;
    } opt_handlers[] = {
        [TELNET_OPT_NAWS] = {
            .write  = terminal_write_to_client,
            .opt    = &terminal->telopt.client.naws,
            .flags  = TELNET_FLAG_LOCAL
        },
        [TELNET_OPT_ECHO] = {
            .write  = terminal_write_to_client,
            .opt    = &terminal->telopt.client.echo,
            .flags  = TELNET_FLAG_REMOTE
        },
        [TELNET_OPT_SGA] = {
            .write  = terminal_write_to_client,
            .opt    = &terminal->telopt.client.sga,
            .flags  = TELNET_FLAG_LOCAL|TELNET_FLAG_REMOTE
        },
        [TELNET_OPT_BINARY] = {
            .write  = terminal_write_to_client,
            .opt    = &terminal->telopt.client.bin,
            .flags  = TELNET_FLAG_LOCAL|TELNET_FLAG_REMOTE
        },
        [TELNET_OPT_EOR] = {
            .write  = terminal_write_to_client,
            .opt    = &terminal->telopt.client.eor,
            .flags  = TELNET_FLAG_LOCAL
        }
    };

    struct telnet_opt_handler_response_type response = {};

    if (data[2] < ARRAY_LENGTH(opt_handlers) && opt_handlers[data[2]].opt) {
        auto handler = opt_handlers[data[2]];
        response = telnet_opt_handle(
            handler.opt, data[1], data[2], handler.flags
        );

        handler.write(terminal, (char *) response.data, response.size);
    }
    else {
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
}

static void terminal_handle_incoming_client_txt(
    TERMINAL *terminal, const uint8_t *data, size_t size
) {
    if (!data || size < 1) {
        FUSE();
        return;
    }

    LOG("client:txt -> terminal: %lu byte%s", size, size == 1 ? "" : "s");

    terminal_write_to_dispatcher(terminal, (const char *) data, size);
}

static void terminal_handle_incoming_dispatcher_iac(
    TERMINAL *terminal, const uint8_t *data, size_t size
) {
    if (!data || size < 2 || data[0] != TELNET_IAC) {
        FUSE();
        return;
    }

    terminal_write_to_client(terminal, (const char *) data, size);
}

static void terminal_handle_incoming_dispatcher_esc(
    TERMINAL *terminal, const uint8_t *data, size_t size
) {
    if (!data || size < 1 || data[0] != TERMINAL_ESC) {
        FUSE();
        return;
    }

    log_esc("dispatcher", "terminal", data, size);

    bool handled = terminal_handle_incoming_dispatcher_esc_screen_size(
        terminal, data, size
    );

    if (handled) {
        return;
    }

    terminal_write_to_client(terminal, (const char *) data, size);
}

static void terminal_handle_incoming_dispatcher_txt(
    TERMINAL *terminal, const uint8_t *data, size_t size
) {
    if (!data || size < 1) {
        FUSE();
        return;
    }

    log_txt("dispatcher", "terminal", data, size);

    terminal_write_to_client(terminal, (const char *) data, size);
}

static bool terminal_read_from_dispatcher(TERMINAL *terminal) {
    CLIP *clip = terminal->io.dispatcher.incoming.clip;

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

            terminal_handle_incoming_dispatcher_txt(
                terminal,
                clip_get_byte_array(nonblocking), clip_get_size(nonblocking)
            );

            clip_destroy(nonblocking);

            return true;
        }

        size_t blocking_sz = terminal_get_esc_blocking_length(data, size);

        if (blocking_sz) {
            CLIP *blocking = clip_shift(clip, blocking_sz);

            terminal_handle_incoming_dispatcher_esc(
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

        terminal_handle_incoming_dispatcher_iac(
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

static bool terminal_write_to_dispatcher(
    TERMINAL *terminal, const char *str, size_t len
) {
    return clip_append_byte_array(
        terminal->io.dispatcher.outgoing.clip,
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

static bool terminal_flush_outgoing(TERMINAL *terminal) {
    if (!terminal) {
        return false;
    }

    bool flushed = false;

    {
        CLIP *src = terminal->io.dispatcher.outgoing.clip;

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

static struct terminal_incoming_dispatcher_esc_screen_size_type{
    const char *end;
    struct {
        const char *str;
        uint8_t size;
        long value;
    } height;
    struct {
        const char *str;
        uint8_t size;
        long value;
    } width;
} terminal_parse_incoming_dispatcher_esc_screen_size(
    const char *str, size_t str_sz
) {
    const struct terminal_incoming_dispatcher_esc_screen_size_type
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
    else if (*next != ';') {
        return invalid;
    }

    const char *height_str = s;
    uint8_t height_size = UINT8VAL(next - s);
    long height_value = 0;

    if (!str_seg_to_long(s, height_size, &height_value)) {
        return invalid;
    }

    s = ++next;

    if (s == str + str_sz) return incomplete;

    next = str_seg_skip_digits(s, str_sz - SIZEVAL(s - str));

    if (next == s) {
        return invalid;
    }
    else if (next == str + str_sz) {
        return incomplete;
    }
    else if (*next != 'R') {
        return invalid;
    }

    const char *width_str = s;
    uint8_t width_size = UINT8VAL(next - s);
    long width_value = 0;

    if (!str_seg_to_long(s, width_size, &width_value)) {
        return invalid;
    }

    return (struct terminal_incoming_dispatcher_esc_screen_size_type) {
        .end = ++next,
        .height = {
            .str = height_str,
            .size = height_size,
            .value = height_value
        },
        .width = {
            .str = width_str,
            .size = width_size,
            .value = width_value
        }
    };
}

static bool terminal_handle_incoming_dispatcher_esc_screen_size(
    TERMINAL *terminal, const uint8_t *data, size_t size
) {
    if (terminal->state != TERMINAL_GET_SCREEN_SIZE) {
        return false;
    }

    const char *str = (const char *) data;
    auto result = terminal_parse_incoming_dispatcher_esc_screen_size(str, size);

    if (result.end != str + size) {
        BUG("failed to get screen size");
        terminal_die(terminal);
        return false;
    }

    long rows = result.height.value;
    long cols = result.width.value;

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
    const char *next = str;

    if (next == str) {
        next = terminal_parse_incoming_dispatcher_esc_screen_size(
            str, size
        ).end;
    }

    return next == nullptr ? 0 : next == str ? 1 : SIZEVAL(next - str);
}
