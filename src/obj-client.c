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

CLIENT *client_create() {
    CLIENT *client = mem_new_client();

    if (!client) {
        return nullptr;
    }

    if ((client->io.incoming.clip = clip_create_byte_array()) == nullptr
    || !(client->io.outgoing.clip = clip_create_byte_array())) {
        client_destroy(client);

        return nullptr;
    }

    return client;
}

void client_destroy(CLIENT *client) {
    if (!client) {
        return;
    }

    clip_destroy(client->io.incoming.clip);
    clip_destroy(client->io.outgoing.clip);

    mem_free_client(client);
}

static void client_die(CLIENT *client) {
    client->bitset.broken = true;
}

static void client_disable_raw_mode(CLIENT *client) {
    if (global.bitset.tty == false) {
        return;
    }

    auto ret = tcsetattr(
        STDIN_FILENO, TCSAFLUSH, &global.client->tui.orig_termios
    );

    if (ret == -1) {
        BUG("%s", strerror(errno));
        client_die(client);
        return;
    }

    global.bitset.tty = false;

    write(STDOUT_FILENO, "\x1b[?47l", 6);
    write(STDOUT_FILENO, "\x1b" "8", 2);
}

static void client_enable_raw_mode(CLIENT *client) {
    if (tcgetattr(STDIN_FILENO, &client->tui.orig_termios) == -1) {
        BUG("%s", strerror(errno));
        client_die(client);
        return;
    }

    struct termios raw = client->tui.orig_termios;
    raw.c_iflag &= (tcflag_t) ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= (tcflag_t) ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= (tcflag_t) ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        BUG("%s", strerror(errno));
        client_die(client);
        return;
    }

    global.bitset.tty = true;

    write(STDOUT_FILENO, "\x1b" "7", 2);
    write(STDOUT_FILENO, "\x1b[?47h", 6);
}

static int client_get_cursor_position(CLIENT *client, int *rows, int *cols) {
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

static int client_get_window_size(CLIENT *client, int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        auto written = write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12);

        if (written != 12) {
            if (written == -1) {
                BUG("%s", strerror(errno));
            }

            return -1;
        }

        return client_get_cursor_position(client, rows, cols);
    }

    *cols = ws.ws_col;
    *rows = ws.ws_row;

    return 0;
}

static void client_init_editor(CLIENT *client) {
    client->tui.cx = 0;
    client->tui.cy = 0;

    auto ret = client_get_window_size(
        client, &client->tui.screenrows, &client->tui.screencols
    );

    if (ret == -1) {
        BUG("failed to get window size");
        client_die(client);
    }
}

void client_init(CLIENT *client) {
    client_enable_raw_mode(client);

    if (client->bitset.broken) {
        return;
    }

    client_init_editor(client);

    if (client->bitset.broken) {
        return;
    }

    LOG("%d x %d", client->tui.screencols, client->tui.screenrows);
}

void client_deinit(CLIENT *client) {
    if (client) {
        client_disable_raw_mode(client);
    }
}

static void client_draw_rows(CLIENT *client, CLIP *clip) {
    bool success = true;
    int y;

    for (y = 0; y < client->tui.screenrows; y++) {
        if (y == client->tui.screenrows / 3) {
            char welcome[80];
            int welcomelen = snprintf(
                welcome, sizeof(welcome), "ANSI Crawl -- version %s", "0.01"
            );

            if (welcomelen > client->tui.screencols) {
                welcomelen = client->tui.screencols;
            }

            int padding = (client->tui.screencols - welcomelen) / 2;

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

        if (y < client->tui.screenrows - 1) {
            success &= clip_append_char_array(clip, "\r\n", 2);
        }
    }

    if (!success) {
        FUSE();
        client_die(client);
    }
}

static void client_refresh_screen(CLIENT *client) {
    bool success = true;
    CLIP *clip = clip_create_char_array();

    success &= clip_append_char_array(clip, "\x1b[?25l", 6);
    success &= clip_append_char_array(clip, "\x1b[H", 3);

    client_draw_rows(client, clip);

    char buf[32];

    snprintf(
        buf, sizeof(buf), "\x1b[%d;%dH", client->tui.cy + 1, client->tui.cx + 1
    );

    success &= clip_append_char_array(clip, buf, strlen(buf));
    success &= clip_append_char_array(clip, "\x1b[?25h", 6);

    if (client->bitset.broken == false) {
        if (success) {
            client_write(
                client, clip_get_char_array(clip), clip_get_size(clip)
            );
        }
        else {
            FUSE();
            client_die(client);
        }
    }

    clip_destroy(clip);
}

static int client_read_key(CLIENT *client) {
    ssize_t nread;
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            BUG("%s", strerror(errno));
            client_die(client);
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

static void client_move_cursor(CLIENT *client, int key) {
    switch (key) {
        case ARROW_LEFT: {
            if (client->tui.cx != 0) {
                client->tui.cx--;
            }

            break;
        }
        case ARROW_RIGHT: {
            if (client->tui.cx != client->tui.screencols - 1) {
                client->tui.cx++;
            }

            break;
        }
        case ARROW_UP: {
            if (client->tui.cy != 0) {
                client->tui.cy--;
            }

            break;
        }
        case ARROW_DOWN: {
            if (client->tui.cy != client->tui.screenrows - 1) {
                client->tui.cy++;
            }

            break;
        }
    }
}

static void client_process_keypress(CLIENT *client) {
    int c = client_read_key(client);

    switch (c) {
        case CTRL_KEY('q'): {
            global.bitset.shutdown = true;
            break;
        }
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT: {
            client_move_cursor(client, c);
            break;
        }
    }
}

void client_pulse(CLIENT *client) {
    if (client->bitset.broken) {
        global.bitset.shutdown = true;
        return;
    }

    client_refresh_screen(client);

    if (!clip_is_empty(client->io.outgoing.clip)) {
        auto written = write(
            STDOUT_FILENO, clip_get_byte_array(client->io.outgoing.clip),
            clip_get_size(client->io.outgoing.clip)
        );

        if (written != (ssize_t) clip_get_size(client->io.outgoing.clip)) {
            if (written == -1) {
                BUG("%s", strerror(errno));
            }
            else if (written > 0) {
                clip_shift(client->io.outgoing.clip, (size_t) written);
            }
            else FUSE();
        }
        else {
            clip_clear(client->io.outgoing.clip);
        }
    }

    client_process_keypress(client);
}

bool client_write(CLIENT *client, const char *str, size_t len) {
    return clip_append_byte_array(
        client->io.outgoing.clip, (const uint8_t *) str, len ? len : strlen(str)
    );
}
