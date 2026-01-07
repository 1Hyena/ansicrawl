// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////
#include "all.h"
////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <locale.h>
#include <time.h>
////////////////////////////////////////////////////////////////////////////////


static void main_loop();
static void main_pulse();
static void main_init(int argc, char **argv);
static void main_deinit();

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

    //init_signals();

    LOG(
        "starting up (compiled %s, %s)%s\033]0;ANSI Crawl\007%s",
        __DATE__, __TIME__, TERMINAL_ESC_HIDDEN, TERMINAL_ESC_HIDDEN_RESET
    );

    global.logbuf = clip_create_char_array();
    global.client = client_create();
    global.server = server_create();

    client_init(global.client);
}

static void main_deinit() {
    client_deinit(global.client);

    if (global.client->bitset.broken) {
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

static void main_pulse() {
    client_pulse(global.client);
}
