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
static bool main_update();
static void main_init(int argc, char **argv);
static void main_deinit();
static bool main_fetch_incoming();
static bool main_flush_outgoing();


int main(int argc, char **argv) {
/*
    char img_data[2000];
    struct amp_image_type amp = {
        .width = 10,
        .height = 10
    };

    size_t img_size = amp_init(&amp, img_data, sizeof(img_data));

    LOG("img size: %lu", img_size);

    for (uint32_t y = 0; y < amp.height; ++y) {
        for (uint32_t x = 0; x < amp.width; ++x) {
            amp_set_glyph(&amp, x, y, "ä");
        }
    }

    struct amp_style_cell_type style = {
        .bitset = {
            .italic = true
        }
    };

    amp_set_style(&amp, 5, 5, style);

    char buf[256];

    main_init(argc, argv);

    for (uint32_t y=0; y<amp.height; ++y) {
        size_t sz=amp_row_to_str(&amp, y, buf, sizeof(buf));
        LOG("%s (%lu)", buf, sz);
    }

    main_deinit();

    return EXIT_SUCCESS;
*/

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
    global.time.update = global.time.boot;

    LOG("using locale: %s", setlocale(LC_ALL, nullptr));

    if (!signals_init()) {
        BUG("failed to initialize signals");
        global.bitset.broken = true;
    }

    LOG(
        "starting up (compiled %s, %s)%s\033]0;ANSI Crawl\007%s",
        __DATE__, __TIME__, TERMINAL_ESC_HIDDEN, TERMINAL_ESC_HIDDEN_RESET
    );

    global.io.incoming.clip = clip_create_byte_array();
    global.io.outgoing.clip = clip_create_byte_array();
    global.dispatcher = dispatcher_create();
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
    dispatcher_deinit(global.dispatcher);

    main_flush_outgoing();

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

    dispatcher_destroy(global.dispatcher);
    global.dispatcher = nullptr;

    clip_destroy(global.io.incoming.clip);
    global.io.incoming.clip = nullptr;

    clip_destroy(global.io.outgoing.clip);
    global.io.outgoing.clip = nullptr;

    mem_recycle();

    if (mem_get_usage()) {
        WARN("%lu bytes of memory left hanging", mem_get_usage());
        mem_clear();
    }
}

static void main_loop() {
    while (!global.bitset.broken) {
        if (!main_update()) {
            LOG("shutting down");
            break;
        }
    }
}

static bool main_fetch_incoming() {
    uint8_t buf[MAX_STACKBUF_SIZE];
    ssize_t count = read(STDIN_FILENO, buf, ARRAY_LENGTH(buf));
    auto read_errno = errno;
    CLIP *clip = global.io.incoming.clip;

    if (count < 0) {
        if (count != -1) {
            FUSE();
        }

        switch (read_errno) {
            case EAGAIN:
            case EINTR: {
                LOG("%s: %s", __func__, strerror(read_errno));
                return true;
            }
            default: {
                BUG("%s", strerror(read_errno));
                global.bitset.broken = true;
                return false;
            }
        }
    }
    else if (clip && count > 0) {
        bool appended = clip_append_byte_array(clip, buf, (size_t) count);

        if (!appended) {
            FUSE();
            global.bitset.broken = true;
            return false;
        }
    }

    return count > 0 || global.terminal;
}

static void main_flush_logbuf() {
    if (global.terminal && global.terminal->bitset.raw) {
        return;
    }

    if (global.logbuf && !clip_is_empty(global.logbuf)) {
        (void)!write(
            STDERR_FILENO,
            clip_get_char_array(global.logbuf),
            clip_get_size(global.logbuf)
        );

        clip_clear(global.logbuf);
    }
}

static bool main_flush_outgoing() {
    CLIP *clip = global.io.outgoing.clip;

    if (!clip || clip_is_empty(clip)) {
        main_flush_logbuf();
        return false;
    }

    auto written = write(
        STDOUT_FILENO, clip_get_byte_array(clip), clip_get_size(clip)
    );
    auto write_errno = errno;

    if (written != (ssize_t) clip_get_size(clip)) {
        if (written > 0) {
            clip_destroy(clip_shift(clip, (size_t) written));
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
        clip_clear(clip);
    }

    main_flush_logbuf();

    return written > 0;
}

static bool main_update() {
    global.count.update++;
    LOG("main update %lu", global.count.update);

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

    bool updated = false;

    updated |= dispatcher_update(global.dispatcher);
    updated |= client_update(global.client);
    updated |= terminal_update(global.terminal);

    main_flush_outgoing();

    if (!updated && !global.bitset.shutdown) {
        LOG("waiting for user input");
        updated |= main_fetch_incoming();

        if (!updated) {
            global.bitset.shutdown = true;
        }
    }

    return updated;
}
