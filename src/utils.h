// SPDX-License-Identifier: MIT
#ifndef UTILS_H_19_11_2024
#define UTILS_H_19_11_2024


bool fuse(const char *file, int line);
size_t umax_size(size_t, size_t);
size_t to_size(long a, const char *file, int line);

#define FUSE() fuse( __builtin_FILE(), __builtin_LINE() )

#define SIZEVAL(a) to_size((a), __FILE__, __LINE__)

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

#define SET_BIT(var, bit) do {                                                 \
    (var) |= _Generic(                                                         \
        (var),                                                                 \
        size_t:  (size_t) (bit),                                               \
        default: (bit)                                                         \
    );                                                                         \
} while (false)

#define REMOVE_BIT(var, bit) do {                                              \
    (var) &= ~_Generic(                                                        \
        (var),                                                                 \
        size_t:         (size_t) (bit),                                        \
        unsigned int:   _Generic(                                              \
                            (bit),                                             \
                            long:       (unsigned int) (bit),                  \
                            int:        (unsigned int) (bit),                  \
                            default:    (bit)                                  \
                        ),                                                     \
        default:        (bit)                                                  \
    );                                                                         \
} while (false)

#define REM_BIT(var, bit) REMOVE_BIT((var), (bit))

#define IS_SET(flags, bit) (                                                   \
    (((size_t)(flags)) & ((size_t)(bit))) == (size_t)(bit)                     \
)

#define IS_ALL_SET(flag, bit)   IS_SET((flag), (bit))
#define IS_ANY_SET(flags, bit)  ((((size_t)(flags)) & ((size_t)(bit))) != 0)

#define FORMAT(buf, ...) do {                                                  \
    if (!str_format( (buf), sizeof( (buf) ), __VA_ARGS__ )) {                  \
        FUSE();                                                                \
    }                                                                          \
} while (0)

#endif
