// SPDX-License-Identifier: MIT
#ifndef FLAGS_H_06_01_2026
#define FLAGS_H_06_01_2026


#define A   1
#define B   2
#define C   4
#define D   8
#define E   16
#define F   32
#define G   64
#define H   128
#define I   256
#define J   512
#define K   1024
#define L   2048
#define M   4096
#define N   8192
#define O   16384
#define P   32768
#define Q   65536
#define R   131072
#define S   262144
#define T   524288
#define U   1048576
#define V   2097152
#define W   4194304
#define X   8388608
#define Y   16777216
#define Z   33554432
#define aa  67108864
#define bb  134217728
#define cc  268435456
#define dd  536870912
#define ee  1073741824
#define ff  2147483648
#define gg  4294967296
#define hh  8589934592
#define ii  17179869184
#define jj  34359738368
#define kk  68719476736
#define ll  137438953472
#define mm  274877906944
#define nn  549755813888
#define oo  1099511627776
#define pp  2199023255552
#define qq  4398046511104
#define rr  8796093022208
#define ss  17592186044416
#define tt  35184372088832
#define uu  70368744177664
#define vv  140737488355328
#define ww  281474976710656
#define xx  562949953421312
#define yy  1125899906842624
#define zz  2251799813685248
#define A_  4503599627370496
#define B_  9007199254740992
#define C_  18014398509481984
#define D_  36028797018963968
#define E_  72057594037927936
#define F_  144115188075855872
#define G_  288230376151711744
#define H_  576460752303423488
#define I_  1152921504606846976
#define J_  2305843009213693952
#define K_  4611686018427387904
#define L_  9223372036854775808

typedef enum : long {
    LOG_FLAG_NONE = 0,
    ////////////////////////////////////////////////////////////////////////////
    LOG_FLAG_ON         =A, LOG_FLAG_TICKS      =B, LOG_FLAG_LOGINS     =C,
    LOG_FLAG_DEATHS     =D, LOG_FLAG_PENALTIES  =E, LOG_FLAG_RESETS     =F,
    LOG_FLAG_LEVELS     =G, LOG_FLAG_IMMORTALS  =H, LOG_FLAG_SPAM       =I,
    LOG_FLAG_BUGS       =J, LOG_FLAG_DEBUG      =K, LOG_FLAG_OTHER      =L,
    LOG_FLAG_TELCOM     =M, LOG_FLAG_WARNINGS   =N
} LOG_FLAG;

#endif
