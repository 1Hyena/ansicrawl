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

    write(STDOUT_FILENO, "\x1b[?47l", 6);
    write(STDOUT_FILENO, "\x1b" "8", 2);
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
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        BUG("%s", strerror(errno));
        terminal_die(terminal);
        return;
    }

    terminal->bitset.raw = true;

    write(STDOUT_FILENO, "\x1b" "7", 2);
    write(STDOUT_FILENO, "\x1b[?47h", 6);
}

static int terminal_get_cursor_position(
    TERMINAL *terminal, int *rows, int *cols
) {
    char buf[32];
    unsigned int i = 0;
    auto written = write(STDOUT_FILENO, "\x1b[6n", 4);

    if (written != 4) {
        if (written == -1) {
            BUG("%s", strerror(errno));
        }
        else FUSE();

        return -1;
    }

    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) {
            break;
        }

        if (buf[i] == 'R') {
            break;
        }

        i++;
    }

    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') {
        return -1;
    }

    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) {
        return -1;
    }

    return 0;
}

static int terminal_get_window_size(TERMINAL *terminal, int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        auto written = write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12);

        if (written != 12) {
            if (written == -1) {
                BUG("%s", strerror(errno));
            }

            return -1;
        }

        return terminal_get_cursor_position(terminal, rows, cols);
    }

    *cols = ws.ws_col;
    *rows = ws.ws_row;

    return 0;
}

static void terminal_init_editor(TERMINAL *terminal) {
    terminal->screen.cx = 0;
    terminal->screen.cy = 0;

    auto ret = terminal_get_window_size(
        terminal, &terminal->screen.height, &terminal->screen.width
    );

    if (ret == -1) {
        BUG("failed to get window size");
        terminal_die(terminal);
    }
}

void terminal_init(TERMINAL *terminal) {
    if (!terminal) {
        return;
    }

    terminal_enable_raw_mode(terminal);

    if (terminal->bitset.broken) {
        return;
    }

    terminal_init_editor(terminal);

    if (terminal->bitset.broken) {
        return;
    }

    LOG("%d x %d", terminal->screen.width, terminal->screen.height);
}

void terminal_deinit(TERMINAL *terminal) {
    if (!terminal) {
        return;
    }

    terminal_disable_raw_mode(terminal);
}

static void terminal_draw_rows(TERMINAL *terminal, CLIP *clip) {
    bool success = true;
    int y;

    for (y = 0; y < terminal->screen.height; y++) {
        if (y == terminal->screen.height / 3) {
            char welcome[80];
            int welcomelen = snprintf(
                welcome, sizeof(welcome), "ANSI Crawl -- version %s", "0.01"
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

static void terminal_refresh_screen(TERMINAL *terminal) {
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
}

static int terminal_read_key(TERMINAL *terminal) {
    ssize_t nread;
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            BUG("%s", strerror(errno));
            terminal_die(terminal);
            return CTRL_KEY('q');
        }
    }

    if (c == '\x1b') {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
            }
        }

        return '\x1b';
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

    terminal_refresh_screen(terminal);

    if (!clip_is_empty(terminal->io.outgoing.clip)) {
        auto written = write(
            STDOUT_FILENO, clip_get_byte_array(terminal->io.outgoing.clip),
            clip_get_size(terminal->io.outgoing.clip)
        );

        if (written != (ssize_t) clip_get_size(terminal->io.outgoing.clip)) {
            if (written == -1) {
                BUG("%s", strerror(errno));
            }
            else if (written > 0) {
                clip_shift(terminal->io.outgoing.clip, (size_t) written);
            }
            else FUSE();
        }
        else {
            clip_clear(terminal->io.outgoing.clip);
        }
    }

    terminal_process_keypress(terminal);
}

bool terminal_write(TERMINAL *terminal, const char *str, size_t len) {
    return clip_append_byte_array(
        terminal->io.outgoing.clip,
        (const uint8_t *) str, len ? len : strlen(str)
    );
}
