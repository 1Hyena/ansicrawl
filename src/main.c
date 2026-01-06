////////////////////////////////////////////////////////////////////////////////
#include "all.h"
////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <locale.h>
#include <time.h>
////////////////////////////////////////////////////////////////////////////////


//static void main_loop();
//static void main_pulse();
static void main_init(int argc, char **argv);
static void main_deinit();

int main(int argc, char **argv) {
    main_init(argc, argv);
    //main_loop();
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
        "starting up (compiled %s, %s)%s\033]0;Aronia\007%s",
        __DATE__, __TIME__, TERMINAL_ESC_HIDDEN, TERMINAL_ESC_HIDDEN_RESET
    );
}

static void main_deinit() {
    LOG("%s", "normal termination");

    if (mem_get_usage()) {
        WARN("%lu bytes of memory left hanging", mem_get_usage());
        mem_clear();
    }
}
