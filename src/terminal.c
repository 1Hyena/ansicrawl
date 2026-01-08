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

TERMINAL *terminal_create() {
    TERMINAL *terminal = mem_new_terminal();

    if (!terminal) {
        return nullptr;
    }

    if ((terminal->io.incoming.clip = clip_create_byte_array()) == nullptr
    || !(terminal->io.outgoing.clip = clip_create_byte_array())) {
        terminal_destroy(terminal);

        return nullptr;
    }

    return terminal;
}

void terminal_destroy(TERMINAL *terminal) {
    if (!terminal) {
        return;
    }

    clip_destroy(terminal->io.incoming.clip);
    clip_destroy(terminal->io.outgoing.clip);

    mem_free_terminal(terminal);
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

    terminal->bitset.raw = false;

    terminal_write(terminal, "\x1b[?47l", 0);
    terminal_write(terminal, "\x1b" "8", 0);
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

    terminal_write(terminal, "\x1b" "7", 0);
    terminal_write(terminal, "\x1b[?47h", 0);
}

static bool terminal_task_get_screen_size(TERMINAL *terminal) {
    if (!clip_push_byte(terminal->io.incoming.clip, 0)) {
        BUG("failed to get screen size");
        terminal_die(terminal);
        return false;
    }

    const char *buf = (const char *) clip_get_byte_array(
        terminal->io.incoming.clip
    );

    int rows;
    int cols;
    bool success = false;

    if (buf[0] == '\x1b'
    &&  buf[1] == '['
    &&  sscanf(&buf[2], "%d;%d", &rows, &cols) == 2) {
        success = true;
    }

    clip_pop_byte(terminal->io.incoming.clip);

    if (!success) {
        BUG("failed to get screen size");
        terminal_die(terminal);
        return false;
    }

    terminal->screen.width = cols;
    terminal->screen.height = rows;
    terminal->bitset.redraw = true;
    terminal->bitset.reformat = false;

    terminal->state = TERMINAL_IDLE;

    return true;
}

static bool terminal_task_ask_screen_size(TERMINAL *terminal) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (!terminal_write(terminal, "\x1b[999C\x1b[999B", 0)
        ||  !terminal_write(terminal, "\x1b[6n", 0)) {
            BUG("failed to ask screen size");
            terminal_die(terminal);
            return false;
        }

        terminal->state = TERMINAL_GET_SCREEN_SIZE;

        return false;
    }

    terminal->screen.width = ws.ws_col;
    terminal->screen.height = ws.ws_row;
    terminal->bitset.redraw = true;
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

static void terminal_draw_rows(TERMINAL *terminal, CLIP *clip) {
    bool success = true;
    int y;

    for (y = 0; y < terminal->screen.height; y++) {
        if (y == terminal->screen.height / 3) {
            char welcome[80];
            int welcomelen = snprintf(
                welcome, sizeof(welcome),
                "ANSI Crawl -- pulse %lu", global.count.pulse
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
}

static void terminal_redraw_screen(TERMINAL *terminal) {
    bool success = true;
    CLIP *clip = clip_create_char_array();

    success &= clip_append_char_array(clip, "\x1b[?25l", 6);
    success &= clip_append_char_array(clip, "\x1b[H", 3);

    terminal_draw_rows(terminal, clip);

    char buf[32];

    snprintf(
        buf, sizeof(buf), "\x1b[%d;%dH",
        terminal->screen.cy + 1, terminal->screen.cx + 1
    );

    success &= clip_append_char_array(clip, buf, strlen(buf));
    success &= clip_append_char_array(clip, "\x1b[?25h", 6);

    if (terminal->bitset.broken == false) {
        if (success) {
            terminal_write(
                terminal, clip_get_char_array(clip), clip_get_size(clip)
            );
        }
        else {
            FUSE();
            terminal_die(terminal);
        }
    }

    clip_destroy(clip);

    terminal->bitset.redraw = false;
}

static int terminal_read_key(TERMINAL *terminal) {
    if (clip_is_empty(terminal->io.incoming.clip)) {
        FUSE();
        terminal_die(terminal);
        return CTRL_KEY('q');
    }

    char c = (char) clip_get_byte_at(terminal->io.incoming.clip, 0);

    clip_destroy(clip_shift(terminal->io.incoming.clip, 1));

    if (c == '\x1b') {
        int key = '\x1b';

        if (clip_get_size(terminal->io.incoming.clip) >= 2) {
            const char *seq = (const char *) clip_get_byte_array(
                terminal->io.incoming.clip
            );

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

            clip_destroy(clip_shift(terminal->io.incoming.clip, 2));
        }
        else {
            FUSE();
            clip_clear(terminal->io.incoming.clip);
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
    if (clip_is_empty(terminal->io.incoming.clip)) {
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

    terminal->bitset.redraw = true;
}

static bool terminal_task_idle(TERMINAL *terminal) {
    terminal_process_keypress(terminal);

    if (terminal->bitset.redraw) {
        terminal_redraw_screen(terminal);
    }

    if (terminal->bitset.reformat) {
        terminal->state = TERMINAL_ASK_SCREEN_SIZE;

        return true;
    }

    return false;
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

void terminal_pulse(TERMINAL *terminal) {
    if (!terminal) {
        BUG("not a TTY");
        global.bitset.broken = true;
        global.bitset.shutdown = true;
        return;
    }

    if (terminal->bitset.broken) {
        global.bitset.shutdown = true;
        return;
    }

    LOG("%d x %d", terminal->screen.width, terminal->screen.height);

    do {
        switch (terminal->state) {
            case MAX_TERMINAL_STATE:
            case TERMINAL_STATE_NONE: {
                FUSE();
                terminal_die(terminal);
                return;
            }
            case TERMINAL_ASK_SCREEN_SIZE: {
                if (!terminal_task_ask_screen_size(terminal)) {
                    return;
                }

                break;
            }
            case TERMINAL_GET_SCREEN_SIZE: {
                if (!terminal_task_get_screen_size(terminal)) {
                    return;
                }

                break;
            }
            case TERMINAL_INIT_EDITOR: {
                if (!terminal_task_init_editor(terminal)) {
                    return;
                }

                break;
            }
            case TERMINAL_IDLE: {
                if (!terminal_task_idle(terminal)) {
                    return;
                }

                break;
            }
        }
    } while (!terminal->bitset.broken);
}

bool terminal_write(TERMINAL *terminal, const char *str, size_t len) {
    return clip_append_byte_array(
        terminal->io.outgoing.clip,
        (const uint8_t *) str, len ? len : strlen(str)
    );
}
