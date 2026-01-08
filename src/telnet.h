// SPDX-License-Identifier: MIT
#ifndef TELNET_H_08_01_2026
#define TELNET_H_08_01_2026

static const char TELNET_IAC        = (char) 255;
static const char TELNET_WILL       = (char) 251;
static const char TELNET_OPT_NAWS   = (char) 31;

static const char TELNET_IAC_WILL_NAWS[] = {
    TELNET_IAC, TELNET_WILL, TELNET_OPT_NAWS, 0
};

#endif
