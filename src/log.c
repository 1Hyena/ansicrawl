// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////
#include "all.h"
////////////////////////////////////////////////////////////////////////////////
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
////////////////////////////////////////////////////////////////////////////////


void log_part(const char *str, size_t len) {
    (void)!write(STDERR_FILENO, str, len);
}

void log_text(const char *str) {
    log_part(str, strlen(str));
}

void log_time() {
    time_t rawtime;
    struct tm *timeinfo;
    char now[80];

    time (&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(now, sizeof(now), "%d-%m-%Y %H:%M:%S", timeinfo);
    size_t len = strlen(now);

    log_part(now, len && now[len-1] == '\n' ? len-1 : len);
}

void log_vstrf(const char *fmt, va_list args) {
    char stackbuf[MAX_STACKBUF_SIZE];

    char *str = str_mem_vnprintf(stackbuf, ARRAY_LENGTH(stackbuf), fmt, args);

    if (str) {
        log_text(str);

        if (str != stackbuf) {
            mem_free(mem_get_metadata(str, alignof(typeof(*str))));
        }
    }
    else FUSE();
}

void log_strf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_vstrf(fmt, args);
    va_end(args);
}

void log_wiznet(
    long flag, long flag_skip, int min_level, const char *string
) {
    /*
    char *category = "";

    if (IS_SET(flag, LOG_FLAG_LOGINS)) {
        category = "Logins";
    }
    else if (IS_SET(flag, LOG_FLAG_SPAM)) {
        category = "Spam";
    }
    else if (IS_SET(flag, LOG_FLAG_DEATHS)) {
        category = "Deaths";
    }
    else if (IS_SET(flag, LOG_FLAG_RESETS)) {
        category = "Resets";
    }
    else if (IS_SET(flag, LOG_FLAG_PENALTIES)) {
        category = "Penalties";
    }
    else if (IS_SET(flag, LOG_FLAG_LEVELS)) {
        category = "Levels";
    }
    else if (IS_SET(flag, LOG_FLAG_IMMORTALS)) {
        category = "Immortals";
    }
    else if (IS_SET(flag, LOG_FLAG_BUGS)) {
        category = "Bugs";
    }
    else if (IS_SET(flag, LOG_FLAG_TICKS)) {
        category = "Ticks";
    }
    else if (IS_SET(flag, LOG_FLAG_DEBUG)) {
        category = "Debug";
    }
    else if (IS_SET(flag, LOG_FLAG_OTHER)) {
        category = "Other";
    }
    */

    {
        // This must be executed before sending the message to users
        // because the latter might silently crash the program.

        if (IS_SET(flag, LOG_FLAG_BUGS)) {
            LOG("\x1B[1;31mBUG:\x1B[0m %s", string);
        }
        else if (!IS_SET(flag, LOG_FLAG_DEBUG) && !IS_SET(flag, LOG_FLAG_TELCOM)) {
            LOG("%s", string);
        }
    }

    /*
    for (d = user_list; d != nullptr; d = d->next) {
        if (d->connected == CON_PLAYING
        && d->character != nullptr
        && char_is_immortal(d->character)
        && IS_SET(d->character->wiznet, LOG_FLAG_ON)
        && (!flag || IS_SET(d->character->wiznet, flag))
        && (!flag_skip || !IS_SET(d->character->wiznet, flag_skip))
        && d->character->level >= min_level) {
            send_to_userf(d, "{BWizNet %s{x: %s\r\n", category, string);
        }
    }
    */

    return;
}

static void log_vwiznetf(
    long flag, long flag_skip, int min_level, const char *fmt, va_list args
) {
    char stackbuf[MAX_STACKBUF_SIZE];

    char *str = str_mem_vnprintf(stackbuf, ARRAY_LENGTH(stackbuf), fmt, args);

    if (str) {
        log_wiznet(flag, flag_skip, min_level, str);

        if (str != stackbuf) {
            mem_free(mem_get_metadata(str, alignof(typeof(*str))));
        }
    }
    else FUSE();
}

void log_wiznetf(long flag, long flag_skip, int min_level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_vwiznetf(flag, flag_skip, min_level, fmt, args);
    va_end(args);
}
