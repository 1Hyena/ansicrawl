// SPDX-License-Identifier: MIT
#ifndef TELNET_H_08_01_2026
#define TELNET_H_08_01_2026

static const uint8_t TELNET_IAC         = 255;
static const uint8_t TELNET_WILL        = 251;
static const uint8_t TELNET_WONT        = 252;
static const uint8_t TELNET_DO          = 253;
static const uint8_t TELNET_DONT        = 254;
static const uint8_t TELNET_SB          = 250;
static const uint8_t TELNET_SE          = 240;
static const uint8_t TELNET_OPT_NAWS    = 31;

static const char TELNET_IAC_WILL_NAWS[] = {
    (char) TELNET_IAC, (char) TELNET_WILL, (char) TELNET_OPT_NAWS, 0
};

static const char TELNET_IAC_WONT_NAWS[] = {
    (char) TELNET_IAC, (char) TELNET_WONT, (char) TELNET_OPT_NAWS, 0
};

static const char TELNET_IAC_DO_NAWS[] = {
    (char) TELNET_IAC, (char) TELNET_DO, (char) TELNET_OPT_NAWS, 0
};

static const char TELNET_IAC_SB_NAWS[] = {
    (char) TELNET_IAC, (char) TELNET_SB, (char) TELNET_OPT_NAWS, 0
};

static const char TELNET_IAC_SE[] = {
    (char) TELNET_IAC, (char) TELNET_SE, 0
};

#endif
