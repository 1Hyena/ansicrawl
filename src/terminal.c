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

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN
};

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

static bool terminal_task_get_screen_size(TERMINAL *terminal) {
    CLIP *clip = terminal->io.interface.incoming.clip;

    if (!clip_push_byte(clip, 0)) {
        BUG("failed to get screen size");
        terminal_die(terminal);
        return false;
    }

    const char *buf = (const char *) clip_get_byte_array(clip);
    int rows;
    int cols;
    bool success = false;

    if (buf[0] == '\x1b'
    &&  buf[1] == '['
    &&  sscanf(&buf[2], "%d;%d", &rows, &cols) == 2) {
        success = true;
    }

    clip_pop_byte(clip);

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

static bool terminal_task_ask_screen_size(TERMINAL *terminal) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
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
/*
static void terminal_draw_rows(TERMINAL *terminal, CLIP *clip) {
    bool success = true;
    int y;

    for (y = 0; y < terminal->screen.height; y++) {
        if (y == terminal->screen.height / 3) {
            char welcome[80];
            int welcomelen = snprintf(
                welcome, sizeof(welcome),
                "ANSI Crawl -- update %lu", global.count.update
            );

            if (welcomelen > terminal->screen.width) {
                welcomelen = terminal->screen.width;
            }

            int padding = (terminal->screen.width - welcomelen) / 2;

            if (padding) {
                success &= clip_append_char_array(clip, "~", 1);
                padding--;
            }

            while (padding--) {
                success &= clip_append_char_array(clip, " ", 1);
            }

            success &= clip_append_char_array(
                clip, welcome, SIZEVAL(welcomelen)
            );
        } else {
            success &= clip_append_char_array(clip, "~", 1);
        }

        success &= clip_append_char_array(clip, "\x1b[K", 3);

        if (y < terminal->screen.height - 1) {
            success &= clip_append_char_array(clip, "\r\n", 2);
        }
    }

    if (!success) {
        FUSE();
        terminal_die(terminal);
    }
}*/

static int terminal_read_key(TERMINAL *terminal) {
    CLIP *clip = terminal->io.interface.incoming.clip;

    if (clip_is_empty(clip)) {
        FUSE();
        terminal_die(terminal);
        return CTRL_KEY('q');
    }

    char c = (char) clip_get_byte_at(clip, 0);

    clip_destroy(clip_shift(clip, 1));

    if (c == '\x1b') {
        int key = '\x1b';

        if (clip_get_size(clip) >= 2) {
            const char *seq = (const char *) clip_get_byte_array(clip);

            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A': key = ARROW_UP; break;
                    case 'B': key = ARROW_DOWN; break;
                    case 'C': key = ARROW_RIGHT; break;
                    case 'D': key = ARROW_LEFT; break;
                    default: {
                        break;
                    }
                }
            }

            clip_destroy(clip_shift(clip, 2));
        }
        else {
            FUSE();
            clip_clear(clip);
        }

        return key;
    }
    else {
        return c;
    }
}

static void terminal_move_cursor(TERMINAL *terminal, int key) {
    switch (key) {
        case ARROW_LEFT: {
            if (terminal->screen.cx != 0) {
                terminal->screen.cx--;
            }

            break;
        }
        case ARROW_RIGHT: {
            if (terminal->screen.cx != terminal->screen.width - 1) {
                terminal->screen.cx++;
            }

            break;
        }
        case ARROW_UP: {
            if (terminal->screen.cy != 0) {
                terminal->screen.cy--;
            }

            break;
        }
        case ARROW_DOWN: {
            if (terminal->screen.cy != terminal->screen.height - 1) {
                terminal->screen.cy++;
            }

            break;
        }
    }
}

static void terminal_process_keypress(TERMINAL *terminal) {
    if (clip_is_empty(terminal->io.interface.incoming.clip)) {
        return;
    }

    int c = terminal_read_key(terminal);

    switch (c) {
        case CTRL_KEY('q'): {
            global.bitset.shutdown = true;
            break;
        }
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT: {
            terminal_move_cursor(terminal, c);
            break;
        }
    }
}

static bool terminal_task_idle(TERMINAL *terminal) {
    terminal_process_keypress(terminal);

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
                repeat = terminal_task_get_screen_size(terminal);
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

        if (terminal->telopt.client.naws.recv_do
        &&  terminal->telopt.client.naws.sent_will) {
            uint16_t width  = USHORTVAL(terminal->screen.width);
            uint16_t height = USHORTVAL(terminal->screen.height);

            if (width  != terminal->telopt.client.naws.state.width
            ||  height != terminal->telopt.client.naws.state.height) {
                auto packet = telnet_serialize_naws_message(width, height);

                terminal_write_to_client(
                    terminal, (const char *) packet.data,
                    ARRAY_LENGTH(packet.data)
                );

                terminal->telopt.client.naws.state.width = width;
                terminal->telopt.client.naws.state.height = height;
            }
        }
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

    if (data[0] == TELNET_IAC
    &&  data[1] == TELNET_DO
    &&  data[2] == TELNET_OPT_NAWS) {
        terminal->telopt.client.naws.recv_do = true;
        terminal_write_to_client(terminal, TELNET_IAC_WILL_NAWS, 0);
        terminal->telopt.client.naws.sent_will = true;

        uint16_t width = USHORTVAL(terminal->screen.width);
        uint16_t height = USHORTVAL(terminal->screen.height);
        auto packet = telnet_serialize_naws_message(width, height);

        terminal_write_to_client(
            terminal, (const char *) packet.data, ARRAY_LENGTH(packet.data)
        );

        terminal->telopt.client.naws.state.width = width;
        terminal->telopt.client.naws.state.height = height;

        return;
    }
}

static bool terminal_read_from_interface(TERMINAL *terminal) {
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
        CLIP *if_out = terminal->io.interface.outgoing.clip;

        if (clip_is_empty(if_out)) {
            clip_swap(if_out, nonblocking);
        }
        else {
            clip_append_clip(if_out, nonblocking);
        }

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
