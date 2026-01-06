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

static void client_die(const char *s) {
    auto err = errno;

    log_text("\x1b[2J");
    log_text("\x1b[H");

    if (s && *s) {
        BUG("%s: %s", s, strerror(err));
    }
    else {
        BUG("%s", strerror(err));
    }

    exit(1);
}

static void client_disable_raw_mode() {
    auto ret = tcsetattr(
        STDIN_FILENO, TCSAFLUSH, &global.client->tui.orig_termios
    );

    if (ret == -1) {
        client_die("tcsetattr");
    }

    global.bitset.tty = false;
}

static void client_enable_raw_mode(CLIENT *client) {
    if (tcgetattr(STDIN_FILENO, &client->tui.orig_termios) == -1) {
        client_die("tcgetattr");
    }

    atexit(client_disable_raw_mode);

    struct termios raw = client->tui.orig_termios;
    raw.c_iflag &= (tcflag_t) ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= (tcflag_t) ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= (tcflag_t) ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        client_die("tcsetattr");
    }

    global.bitset.tty = true;
}

static int client_get_cursor_position(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
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

static int client_get_window_size(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
            return -1;
        }

        return client_get_cursor_position(rows, cols);
    }

    *cols = ws.ws_col;
    *rows = ws.ws_row;

    return 0;
}

static void client_init_editor(CLIENT *client) {
    client->tui.cx = 0;
    client->tui.cy = 0;

    auto ret = client_get_window_size(
        &client->tui.screenrows, &client->tui.screencols
    );

    if (ret == -1) {
        client_die("getWindowSize");
    }
}

void client_init(CLIENT *client) {
    client_enable_raw_mode(client);
    client_init_editor(client);
    LOG("%d x %d", client->tui.screencols, client->tui.screenrows);
}

void client_deinit(CLIENT *client) {
    if (client) {
        client_disable_raw_mode();
    }
}

static void client_draw_rows(CLIENT *client, CLIP *ab) {
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
                clip_append_char_array(ab, "~", 1);
                padding--;
            }
            while (padding--) clip_append_char_array(ab, " ", 1);

            clip_append_char_array(ab, welcome, SIZEVAL(welcomelen));
        } else {
            clip_append_char_array(ab, "~", 1);
        }

        clip_append_char_array(ab, "\x1b[K", 3);

        if (y < client->tui.screenrows - 1) {
            clip_append_char_array(ab, "\r\n", 2);
        }
    }
}

static void client_refresh_screen(CLIENT *client) {
    CLIP *ab = clip_create_char_array();

    clip_append_char_array(ab, "\x1b[?25l", 6);
    clip_append_char_array(ab, "\x1b[H", 3);

    client_draw_rows(client, ab);

    char buf[32];

    snprintf(
        buf, sizeof(buf), "\x1b[%d;%dH", client->tui.cy + 1, client->tui.cx + 1
    );

    clip_append_char_array(ab, buf, strlen(buf));

    clip_append_char_array(ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, clip_get_char_array(ab), clip_get_size(ab));
    clip_destroy(ab);
}


static int client_read_key(CLIENT *client) {
    ssize_t nread;
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) client_die("read");
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
    case ARROW_LEFT:
      if (client->tui.cx != 0) {
        client->tui.cx--;
      }
      break;
    case ARROW_RIGHT:
      if (client->tui.cx != client->tui.screencols - 1) {
        client->tui.cx++;
      }
      break;
    case ARROW_UP:
      if (client->tui.cy != 0) {
        client->tui.cy--;
      }
      break;
    case ARROW_DOWN:
      if (client->tui.cy != client->tui.screenrows - 1) {
        client->tui.cy++;
      }
      break;
  }
}

static void client_process_keypress(CLIENT *client) {
  int c = client_read_key(client);

  switch (c) {
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      global.bitset.shutdown = true;
      break;

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      client_move_cursor(client, c);
      break;
  }
}

void client_pulse(CLIENT *client) {
    client_refresh_screen(client);
    client_process_keypress(client);
}
