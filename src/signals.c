////////////////////////////////////////////////////////////////////////////////
#include "all.h"
////////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <errno.h>
////////////////////////////////////////////////////////////////////////////////


static bool signals_init_signal(int sig);
static void signals_handle_signal(int sig);

bool signals_init() {
    return (
        signals_init_signal(SIGALRM) &&
        signals_init_signal(SIGPIPE) &&
        signals_init_signal(SIGINT ) &&
        signals_init_signal(SIGTERM) &&
        signals_init_signal(SIGQUIT) &&
        signals_init_signal(SIGSEGV) &&
        signals_init_signal(SIGILL ) &&
        signals_init_signal(SIGABRT) &&
        signals_init_signal(SIGFPE ) &&
        signals_init_signal(SIGBUS ) &&
        signals_init_signal(SIGIOT ) &&
        signals_init_signal(SIGTRAP) &&
        signals_init_signal(SIGSYS ) &&
        signals_init_signal(SIGWINCH)
    );
}

int signals_next() {
    if (global.signal.interrupt) {
        global.signal.interrupt = 0;
        return SIGINT;
    }

    if (global.signal.terminate) {
        global.signal.terminate = 0;
        return SIGTERM;
    }

    if (global.signal.alarm) {
        global.signal.alarm = 0;
        return SIGALRM;
    }

    if (global.signal.pipe) {
        global.signal.pipe = 0;
        return SIGPIPE;
    }

    if (global.signal.quit) {
        global.signal.quit = 0;
        return SIGQUIT;
    }

    if (global.signal.window) {
        global.signal.window = 0;
        return SIGWINCH;
    }

    return 0;
}

static bool signals_init_signal(int sig) {
    struct sigaction sa;
    sa.sa_handler = signals_handle_signal;
    sa.sa_flags   = 0;

    if (sigemptyset(&sa.sa_mask) == -1) {
        BUG("%s", strerror(errno));
        return false;
    }

    if (sigaction(sig, &sa, nullptr) == -1) {
        BUG("%s", strerror(errno));
        return false;
    }

    return true;
}

static void signals_handle_signal(int sig) {
    switch (sig) {
        case SIGINT:  {
            if (global.signal.interrupt) {
                // Receiving it again will force termination immediately.

                break;
            }

            global.signal.interrupt = 1;
            return;
        }
        case SIGTERM:   global.signal.terminate   = 1; return;
        case SIGQUIT:   global.signal.quit        = 1; return;
        case SIGPIPE:   global.signal.pipe        = 1; return;
        case SIGALRM:   global.signal.alarm       = 1; return;
        case SIGWINCH:  global.signal.window      = 1; return;
        default: break;
    }

    // The only portable use of signal() is to set a signal's disposition to
    // SIG_DFL or SIG_IGN.
    signal(sig, SIG_DFL);
    raise(sig);
}
