#ifndef LOG_H_06_01_2026
#define LOG_H_06_01_2026
////////////////////////////////////////////////////////////////////////////////
#include <stdarg.h>
////////////////////////////////////////////////////////////////////////////////


void log_wiznet(long flg, long flg_skip, int min_lvl, const char *string);
void log_wiznetf(
    long flg, long flg_skip, int min_lvl, const char *fmt, ...
) __attribute__((format (printf, 4, 5)));

void log_part   (const char *str, size_t len);
void log_text   (const char *str);
void log_time   ();
void log_vstrf  (const char *fmt, va_list args);
void log_strf   (const char *fmt, ...) __attribute__((format (printf, 1, 2)));

#define LOG(fmt, ...) do {                                                     \
    log_time();                                                                \
    log_text(" :: ");                                                          \
    log_strf((fmt), ##__VA_ARGS__);                                            \
    log_text("\x1B[0m\n");                                                     \
} while (0)

#define WARN(fmt, ...) do {                                                    \
    log_time();                                                                \
    log_text(" :: \x1B[1;33m");                                                \
    log_strf((fmt), ##__VA_ARGS__);                                            \
    log_text("\x1B[0m\n");                                                     \
} while (0)

#define BUG(fmt, ...) do {                                                     \
    char log_bug_buffer[1024];                                                 \
    FORMAT(log_bug_buffer, (fmt), ##__VA_ARGS__);                              \
    const char *log_bug_fpath = __FILE__;                                      \
    const char *log_bug_fname = log_bug_fpath;                                 \
                                                                               \
    while (*log_bug_fpath) {                                                   \
        if (*log_bug_fpath++ == '/') {                                         \
            log_bug_fname = log_bug_fpath;                                     \
        }                                                                      \
    }                                                                          \
                                                                               \
    log_wiznetf(                                                               \
        LOG_FLAG_BUGS, 0, 0, "%s: %s (%s:%d)",                                 \
        __func__, log_bug_buffer, log_bug_fname, __LINE__                      \
    );                                                                         \
} while (0)

#define BUG_ONCE(fmt, ...) do {                                                \
    if (FUSE()) {                                                              \
        char fuse_fmt_buffer[1024];                                            \
        FORMAT(fuse_fmt_buffer, (fmt), ##__VA_ARGS__);                         \
        const char *fuse_fmt_fpath = __FILE__;                                 \
        const char *fuse_fmt_fname = fuse_fmt_fpath;                           \
                                                                               \
        while (*fuse_fmt_fpath) {                                              \
            if (*fuse_fmt_fpath++ == '/') {                                    \
                fuse_fmt_fname = fuse_fmt_fpath;                               \
            }                                                                  \
        }                                                                      \
                                                                               \
        log_wiznetf(                                                           \
            LOG_FLAG_BUGS, 0, 0, "%s: %s (%s:%d)",                             \
            __func__, fuse_fmt_buffer, fuse_fmt_fname, __LINE__                \
        );                                                                     \
    }                                                                          \
} while (0)

#define WIZNET(channel, fmt, ...) do {                                         \
    log_wiznetf((channel), 0, 0, (fmt), ##__VA_ARGS__);                        \
} while (0)

#define WIZNET_LOGIN(ch, fmt, ...) do {                                        \
    log_wiznetf(LOG_FLAG_LOGINS, 0, (ch)->level, (fmt), ##__VA_ARGS__);        \
} while (0)

#define WIZNET_SPAM(ch, fmt, ...) do {                                         \
    log_wiznetf(LOG_FLAG_SPAM, 0, (ch)->level, (fmt), ##__VA_ARGS__);          \
} while (0)

#define WIZNET_DEATH(ch, fmt, ...) do {                                        \
    log_wiznetf(LOG_FLAG_DEATHS, 0, (ch)->level, (fmt), ##__VA_ARGS__);        \
} while (0)

#define WIZNET_RESET(fmt, ...) do {                                            \
    log_wiznetf(LOG_FLAG_RESETS, 0, 0, (fmt), ##__VA_ARGS__);                  \
} while (0)

#define WIZNET_PENALTY(ch, fmt, ...) do {                                      \
    log_wiznetf(LOG_FLAG_PENALTIES, 0, (ch)->level, (fmt), ##__VA_ARGS__);     \
} while (0)

#define WIZNET_LEVEL(ch, fmt, ...) do {                                        \
    log_wiznetf(LOG_FLAG_LEVELS, 0, (ch)->level, (fmt), ##__VA_ARGS__);        \
} while (0)

#define WIZNET_IMMO(ch, fmt, ...) do {                                         \
    log_wiznetf(LOG_FLAG_IMMORTALS, 0, (ch)->level, (fmt), ##__VA_ARGS__);     \
} while (0)

#define WIZNET_BUG(fmt, ...) do {                                              \
    log_wiznetf(LOG_FLAG_BUGS, 0, 0, (fmt), ##__VA_ARGS__);                    \
} while (0)

#define WIZNET_DEBUG(fmt, ...) do {                                            \
    log_wiznetf(LOG_FLAG_DEBUG, 0, 0, (fmt), ##__VA_ARGS__);                   \
} while (0)

#define WIZNET_TICK(fmt, ...) do {                                             \
    log_wiznetf(LOG_FLAG_TICKS, 0, 0, (fmt), ##__VA_ARGS__);                   \
} while (0)

#define WIZNET_OTHER(fmt, ...) do {                                            \
    log_wiznetf(LOG_FLAG_OTHER, 0, 0, (fmt), ##__VA_ARGS__);                   \
} while (0)

#define WIZNET_WARN(fmt, ...) do {                                             \
    log_wiznetf(LOG_FLAG_WARNINGS, 0, 0, (fmt), ##__VA_ARGS__);                \
} while (0)

#define WIZNET_TELCOM(fmt, ...) do {                                           \
    log_wiznetf(LOG_FLAG_TELCOM, 0, 0, (fmt), ##__VA_ARGS__);                  \
} while (0)

#endif
