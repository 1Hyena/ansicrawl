// SPDX-License-Identifier: MIT
#ifndef TELNET_H_08_01_2026
#define TELNET_H_08_01_2026


typedef enum : uint8_t {
    TELNET_OPT_BINARY       = 0,   // 8-bit data path
    TELNET_OPT_ECHO         = 1,   // echo
    TELNET_MSDP_VAR         = 1,
    TELNET_MSSP_VAR         = 1,
    TELNET_MSSP_VAL         = 2,
    TELNET_MSDP_VAL         = 2,
    TELNET_OPT_RCP          = 2,   // prepare to reconnect
    TELNET_MSDP_TABLE_OPEN  = 3,
    TELNET_OPT_SGA          = 3,   // suppress go ahead
    TELNET_MSDP_TABLE_CLOSE = 4,
    TELNET_OPT_NAMS         = 4,   // approximate message size
    TELNET_MSDP_ARRAY_OPEN  = 5,
    TELNET_OPT_STATUS       = 5,   // give status
    TELNET_MSDP_ARRAY_CLOSE = 6,
    TELNET_OPT_TM           = 6,   // timing mark
    TELNET_OPT_RCTE         = 7,   // remote controlled transmission and echo
    TELNET_OPT_NAOL         = 8,   // negotiate about output line width
    TELNET_OPT_NAOP         = 9,   // negotiate about output page size
    TELNET_OPT_NAOCRD       = 10,  // negotiate about CR disposition
    TELNET_OPT_NAOHTS       = 11,  // negotiate about horizontal tabstops
    TELNET_OPT_NAOHTD       = 12,  // negotiate about horizontal tab disposition
    TELNET_OPT_NAOFFD       = 13,  // negotiate about formfeed disposition
    TELNET_OPT_NAOVTS       = 14,  // negotiate about vertical tab stops
    TELNET_OPT_NAOVTD       = 15,  // negotiate about vertical tab disposition
    TELNET_OPT_NAOLFD       = 16,  // negotiate about output LF disposition
    TELNET_OPT_XASCII       = 17,  // extended ascic character set
    TELNET_OPT_LOGOUT       = 18,  // force logout
    TELNET_OPT_BM           = 19,  // byte macro
    TELNET_OPT_DET          = 20,  // data entry terminal
    TELNET_OPT_SUPDUP       = 21,  // supdup protocol
    TELNET_OPT_SUPDUPOUTPUT = 22,  // supdup output
    TELNET_OPT_SNDLOC       = 23,  // send location
    TELNET_OPT_TTYPE        = 24,  // terminal type
    TELNET_OPT_EOR          = 25,  // end or record
    TELNET_OPT_NAWS         = 31,  // negotiate about window size
    TELNET_OPT_TS           = 32,  // terminal speed
    TELNET_OPT_EOPT         = 36,  // environment option
    TELNET_OPT_NEO          = 39,  // new environment option
    TELNET_OPT_CHARSET      = 42,
    TELNET_OPT_MSDP         = 69,
    TELNET_OPT_MSSP         = 70,  // mud server status protocol
    TELNET_OPT_MCCP         = 86,
    TELNET_EOF              = 236,
    TELNET_EOR              = 239, // end of record (transparent mode)
    TELNET_SE               = 240,
    TELNET_NOP              = 241, // no operation
    TELNET_DM               = 242, // data mark--for connect. cleaning
    TELNET_BREAK            = 243, // break
    TELNET_IP               = 244, // interrupt process--permanently
    TELNET_AO               = 245, // abort output--but let prog finish
    TELNET_AYT              = 246, // are you there
    TELNET_EC               = 247, // erase the current character
    TELNET_EL               = 248, // erase the current line
    TELNET_GA               = 249, // you may reverse the line
    TELNET_SB               = 250,
    TELNET_WILL             = 251,
    TELNET_WONT             = 252,
    TELNET_DO               = 253,
    TELNET_DONT             = 254,
    TELNET_IAC              = 255
} TELNET_CODE;

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

const char *telnet_uchar_to_printable(unsigned char c);
const char *telnet_uchar_to_string(unsigned char c);
const char *telnet_iac_byte_to_code(unsigned char c);

const char *telnet_get_iac_sequence_code(
    const unsigned char *data, size_t size, size_t index
);

size_t telnet_get_iac_nonblocking_length(const unsigned char *, size_t size);
size_t telnet_get_iac_sequence_length(const unsigned char *, size_t size);

static inline struct telnet_naws_packet_type {
    uint8_t data[9];
} telnet_serialize_naws_packet(uint16_t width, uint16_t height) {
    return (struct telnet_naws_packet_type) {
        .data = {
            TELNET_IAC, TELNET_SB, TELNET_OPT_NAWS,
            (uint8_t) (width  >> 8), (uint8_t) (width  % 256),
            (uint8_t) (height >> 8), (uint8_t) (height % 256),
            TELNET_IAC, TELNET_SE
        }
    };
}

static inline struct telnet_naws_message_type {
    uint16_t width;
    uint16_t height;
} telnet_deserialize_naws_packet(const uint8_t *data, size_t size) {
    struct telnet_naws_message_type packet = {};

    if (size <= 9) {
        packet.width  = (uint16_t) (data[3] * 256 + data[4]);
        packet.height = (uint16_t) (data[5] * 256 + data[6]);
    }

    return packet;
}

#endif
