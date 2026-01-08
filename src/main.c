// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////
#include "all.h"
////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <locale.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
////////////////////////////////////////////////////////////////////////////////


static void main_loop();
static void main_pulse();
static void main_init(int argc, char **argv);
static void main_deinit();
static void main_io_read();
static void main_io_write();


int main(int argc, char **argv) {
    main_init(argc, argv);
    main_loop();
    main_deinit();

    return EXIT_SUCCESS;
}

static void main_init(int argc, char **argv) {
    const char *locales[] = { "C.UTF8", "C.utf8", "en_US.UTF-8", "en_US.utf8" };

    for (size_t i=0; i<ARRAY_LENGTH(locales); ++i) {
        if (setlocale(LC_ALL, locales[i])) {
            break;
        }
    }

    timespec_get(&global.time.boot, TIME_UTC);
    global.time.pulse = global.time.boot;

    LOG("using locale: %s", setlocale(LC_ALL, nullptr));

    if (!signals_init()) {
        BUG("failed to initialize signals");
        global.bitset.broken = true;
    }

    LOG(
        "starting up (compiled %s, %s)%s\033]0;ANSI Crawl\007%s",
        __DATE__, __TIME__, TERMINAL_ESC_HIDDEN, TERMINAL_ESC_HIDDEN_RESET
    );

    global.terminal = isatty(STDIN_FILENO) ? terminal_create() : nullptr;
    global.logbuf = clip_create_char_array();
    global.client = client_create();
    global.server = server_create();

    terminal_init(global.terminal);
    client_init(global.client);
}

static void main_deinit() {
    client_deinit(global.client);
    terminal_deinit(global.terminal);

    main_io_write(); // Terminal deinitialization writes to stdout.

    if (global.bitset.broken) {
        WARN("%s", "abnormal termination");
    }
    else {
        LOG("%s", "normal termination");
    }

    server_destroy(global.server);
    global.server = nullptr;

    client_destroy(global.client);
    global.client = nullptr;

    clip_destroy(global.logbuf);
    global.logbuf = nullptr;

    terminal_destroy(global.terminal);
    global.terminal = nullptr;

    mem_recycle();

    if (mem_get_usage()) {
        WARN("%lu bytes of memory left hanging", mem_get_usage());
        mem_clear();
    }
}

static void main_loop() {
    while (!global.bitset.shutdown) {
        main_pulse();
    }
}

static void main_io_read() {
    uint8_t buf[MAX_STACKBUF_SIZE];
    ssize_t count = read(STDIN_FILENO, buf, ARRAY_LENGTH(buf));
    auto read_errno = errno;
    TERMINAL *terminal = global.terminal;

    if (count < 0) {
        if (count != -1) {
            FUSE();
        }

        switch (read_errno) {
            case EAGAIN:
            case EINTR: {
                LOG("%s: %s", __func__, strerror(read_errno));
                break;
            }
            default: {
                BUG("%s", strerror(read_errno));
                global.bitset.broken = true;
                return;
            }
        }
    }
    else if (terminal && count > 0) {
        bool appended = clip_append_byte_array(
            terminal->io.incoming.clip, buf, (size_t) count
        );

        if (!appended) {
            FUSE();
            global.bitset.broken = true;
            return;
        }
    }

    if (count >= 0) {
        LOG("%lu read", (size_t) count);
    }
}

static void main_io_write() {
    TERMINAL *terminal = global.terminal;

    if (!terminal || clip_is_empty(terminal->io.outgoing.clip)) {
        return;
    }

    auto written = write(
        STDOUT_FILENO, clip_get_byte_array(terminal->io.outgoing.clip),
        clip_get_size(terminal->io.outgoing.clip)
    );
    auto write_errno = errno;

    if (written != (ssize_t) clip_get_size(terminal->io.outgoing.clip)) {
        if (written > 0) {
            clip_destroy(
                clip_shift(terminal->io.outgoing.clip, (size_t) written)
            );
        }
        else {
            if (written == -1) {
                switch (write_errno) {
                    case EAGAIN:
                    case EINTR: {
                        break;
                    }
                    default: {
                        global.bitset.broken = true;
                        BUG("%s", strerror(write_errno));
                    }
                }
            }
            else {
                global.bitset.broken = true;
                FUSE();
            }
        }
    }
    else {
        clip_clear(terminal->io.outgoing.clip);
    }

    if (written >= 0) {
        LOG("%lu written", (size_t) written);
    }
}

static void main_pulse() {
    global.count.pulse++;

    client_pulse(global.client);
    terminal_pulse(global.terminal);

    main_io_write();

    for (auto sig = signals_next(); sig; sig = signals_next()) {
        LOG("signal received (%s)", strsignal(sig));

        switch (sig) {
            case SIGINT:
            case SIGTERM:
            case SIGQUIT: {
                global.bitset.shutdown = true;
                break;
            }
            case SIGPIPE: break;
            case SIGALRM: break;
            case SIGWINCH: {
                if (global.terminal) {
                    global.terminal->bitset.reformat = true;
                }

                break;
            }
            default: break;
        }
    }

    if (global.bitset.broken) {
        global.bitset.shutdown = true;
    }

    if (global.bitset.shutdown) {
        return;
    }

    main_io_read();
}
