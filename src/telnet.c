////////////////////////////////////////////////////////////////////////////////
#include "all.h"
////////////////////////////////////////////////////////////////////////////////
#include <ctype.h>
#include <string.h>
////////////////////////////////////////////////////////////////////////////////


const char *telnet_get_iac_sequence_code(
    const unsigned char *data, size_t size, size_t index
) {
    if (*data != TELNET_IAC || index >= size) {
        return nullptr;
    }

    switch (data[1]) {
        case TELNET_SB: {
            if (index <= 2 || index + 2 >= size) {
                break;
            }

            switch (data[2]) {
                case TELNET_OPT_MSDP: {
                    switch (data[index]) {
                        case TELNET_MSDP_VAR:           return "VAR";
                        case TELNET_MSDP_VAL:           return "VAL";
                        case TELNET_MSDP_TABLE_OPEN:    return "{";
                        case TELNET_MSDP_TABLE_CLOSE:   return "}";
                        case TELNET_MSDP_ARRAY_OPEN:    return "[";
                        case TELNET_MSDP_ARRAY_CLOSE:   return "]";
                        default: {
                            if (isprint(data[index])) {
                                const char *code = telnet_uchar_to_printable(
                                    data[index]
                                );

                                if (code != nullptr) {
                                    return code;
                                }
                            }

                            break;
                        }
                    }
                } [[fallthrough]];
                default: {
                    return telnet_uchar_to_string(data[index]);
                }
            }

            break;
        }
        default: break;
    }

    return telnet_iac_byte_to_code(data[index]);
}

size_t telnet_get_iac_sequence_length(
    const unsigned char *data, size_t length
) {
    if (!data) {
        FUSE();
        return 0;
    }

    bool iac_active = false;
    size_t i = 0;

    for (; i < length; ++i) {
        if (!iac_active) {
            if (data[i] == TELNET_IAC) {
                iac_active = true;

                continue;
            }

            return 0;
        }

        switch (data[i]) {
            case TELNET_DO:
            case TELNET_DONT:
            case TELNET_WILL:
            case TELNET_WONT: {
                return length < 3 ? 0 : 3;
            }
            case TELNET_SB: {
                // subnegotiation

                for (size_t j = i + 2; ; j++) {
                    if (j + 1 >= length) {
                        return 0;
                    }

                    if (data[j] != TELNET_IAC) {
                        continue;
                    }

                    switch (data[j+1]) {
                        case TELNET_SE: return j+2;
                        case TELNET_IAC: {
                            ++j;
                            continue;
                        }
                        default: {
                            // Anything other than SE or IAC after IAC actually
                            // violates the telnet option subnegotiation protcol
                            // because the meaning of it has not been defined.

                            continue;
                        }
                    }
                }

                return 0;
            }
            default: {
                // 2-byte commands such as IAC IAC or IAC GA
                return 2;
            }
        }
    }

    return 0;
}

size_t telnet_get_iac_nonblocking_length(
    const unsigned char *data, size_t length
) {
    if (!data) {
        FUSE();
        return 0;
    }

    const char *iac = memchr(data, TELNET_IAC, length);

    return iac ? SIZEVAL(iac - ((const char *) data)) : length;
}

const char *telnet_uchar_to_printable(unsigned char c) {
    switch (c) {
        case 33: return "!";
        case 34: return "\"";
        case 35: return "#";
        case 36: return "$";
        case 37: return "%";
        case 38: return "&";
        case 39: return "'";
        case 40: return "(";
        case 41: return ")";
        case 42: return "*";
        case 43: return "+";
        case 44: return ",";
        case 45: return "-";
        case 46: return ".";
        case 47: return "/";
        case 48: return "0";
        case 49: return "1";
        case 50: return "2";
        case 51: return "3";
        case 52: return "4";
        case 53: return "5";
        case 54: return "6";
        case 55: return "7";
        case 56: return "8";
        case 57: return "9";
        case 58: return ":";
        case 59: return ";";
        case 60: return "<";
        case 61: return "=";
        case 62: return ">";
        case 63: return "?";
        case 64: return "@";
        case 65: return "A";
        case 66: return "B";
        case 67: return "C";
        case 68: return "D";
        case 69: return "E";
        case 70: return "F";
        case 71: return "G";
        case 72: return "H";
        case 73: return "I";
        case 74: return "J";
        case 75: return "K";
        case 76: return "L";
        case 77: return "M";
        case 78: return "N";
        case 79: return "O";
        case 80: return "P";
        case 81: return "Q";
        case 82: return "R";
        case 83: return "S";
        case 84: return "T";
        case 85: return "U";
        case 86: return "V";
        case 87: return "W";
        case 88: return "X";
        case 89: return "Y";
        case 90: return "Z";
        case 91: return "[";
        case 92: return "\\";
        case 93: return "]";
        case 94: return "^";
        case 95: return "_";
        case 96: return "`";
        case 97: return "a";
        case 98: return "b";
        case 99: return "c";
        case 100: return "d";
        case 101: return "e";
        case 102: return "f";
        case 103: return "g";
        case 104: return "h";
        case 105: return "i";
        case 106: return "j";
        case 107: return "k";
        case 108: return "l";
        case 109: return "m";
        case 110: return "n";
        case 111: return "o";
        case 112: return "p";
        case 113: return "q";
        case 114: return "r";
        case 115: return "s";
        case 116: return "t";
        case 117: return "u";
        case 118: return "v";
        case 119: return "w";
        case 120: return "x";
        case 121: return "y";
        case 122: return "z";
        case 123: return "{";
        case 124: return "|";
        case 125: return "}";
        case 126: return "~";
        default: break;
    }

    return nullptr;
}

const char *telnet_uchar_to_string(unsigned char c) {
    switch (c) {
        case   0: return   "0";
        case   1: return   "1";
        case   2: return   "2";
        case   3: return   "3";
        case   4: return   "4";
        case   5: return   "5";
        case   6: return   "6";
        case   7: return   "7";
        case   8: return   "8";
        case   9: return   "9";
        case  10: return  "10";
        case  11: return  "11";
        case  12: return  "12";
        case  13: return  "13";
        case  14: return  "14";
        case  15: return  "15";
        case  16: return  "16";
        case  17: return  "17";
        case  18: return  "18";
        case  19: return  "19";
        case  20: return  "20";
        case  21: return  "21";
        case  22: return  "22";
        case  23: return  "23";
        case  24: return  "24";
        case  25: return  "25";
        case  26: return  "26";
        case  27: return  "27";
        case  28: return  "28";
        case  29: return  "29";
        case  30: return  "30";
        case  31: return  "31";
        case  32: return  "32";
        case  33: return  "33";
        case  34: return  "34";
        case  35: return  "35";
        case  36: return  "36";
        case  37: return  "37";
        case  38: return  "38";
        case  39: return  "39";
        case  40: return  "40";
        case  41: return  "41";
        case  42: return  "42";
        case  43: return  "43";
        case  44: return  "44";
        case  45: return  "45";
        case  46: return  "46";
        case  47: return  "47";
        case  48: return  "48";
        case  49: return  "49";
        case  50: return  "50";
        case  51: return  "51";
        case  52: return  "52";
        case  53: return  "53";
        case  54: return  "54";
        case  55: return  "55";
        case  56: return  "56";
        case  57: return  "57";
        case  58: return  "58";
        case  59: return  "59";
        case  60: return  "60";
        case  61: return  "61";
        case  62: return  "62";
        case  63: return  "63";
        case  64: return  "64";
        case  65: return  "65";
        case  66: return  "66";
        case  67: return  "67";
        case  68: return  "68";
        case  69: return  "69";
        case  70: return  "70";
        case  71: return  "71";
        case  72: return  "72";
        case  73: return  "73";
        case  74: return  "74";
        case  75: return  "75";
        case  76: return  "76";
        case  77: return  "77";
        case  78: return  "78";
        case  79: return  "79";
        case  80: return  "80";
        case  81: return  "81";
        case  82: return  "82";
        case  83: return  "83";
        case  84: return  "84";
        case  85: return  "85";
        case  86: return  "86";
        case  87: return  "87";
        case  88: return  "88";
        case  89: return  "89";
        case  90: return  "90";
        case  91: return  "91";
        case  92: return  "92";
        case  93: return  "93";
        case  94: return  "94";
        case  95: return  "95";
        case  96: return  "96";
        case  97: return  "97";
        case  98: return  "98";
        case  99: return  "99";
        case 100: return "100";
        case 101: return "101";
        case 102: return "102";
        case 103: return "103";
        case 104: return "104";
        case 105: return "105";
        case 106: return "106";
        case 107: return "107";
        case 108: return "108";
        case 109: return "109";
        case 110: return "110";
        case 111: return "111";
        case 112: return "112";
        case 113: return "113";
        case 114: return "114";
        case 115: return "115";
        case 116: return "116";
        case 117: return "117";
        case 118: return "118";
        case 119: return "119";
        case 120: return "120";
        case 121: return "121";
        case 122: return "122";
        case 123: return "123";
        case 124: return "124";
        case 125: return "125";
        case 126: return "126";
        case 127: return "127";
        case 128: return "128";
        case 129: return "129";
        case 130: return "130";
        case 131: return "131";
        case 132: return "132";
        case 133: return "133";
        case 134: return "134";
        case 135: return "135";
        case 136: return "136";
        case 137: return "137";
        case 138: return "138";
        case 139: return "139";
        case 140: return "140";
        case 141: return "141";
        case 142: return "142";
        case 143: return "143";
        case 144: return "144";
        case 145: return "145";
        case 146: return "146";
        case 147: return "147";
        case 148: return "148";
        case 149: return "149";
        case 150: return "150";
        case 151: return "151";
        case 152: return "152";
        case 153: return "153";
        case 154: return "154";
        case 155: return "155";
        case 156: return "156";
        case 157: return "157";
        case 158: return "158";
        case 159: return "159";
        case 160: return "160";
        case 161: return "161";
        case 162: return "162";
        case 163: return "163";
        case 164: return "164";
        case 165: return "165";
        case 166: return "166";
        case 167: return "167";
        case 168: return "168";
        case 169: return "169";
        case 170: return "170";
        case 171: return "171";
        case 172: return "172";
        case 173: return "173";
        case 174: return "174";
        case 175: return "175";
        case 176: return "176";
        case 177: return "177";
        case 178: return "178";
        case 179: return "179";
        case 180: return "180";
        case 181: return "181";
        case 182: return "182";
        case 183: return "183";
        case 184: return "184";
        case 185: return "185";
        case 186: return "186";
        case 187: return "187";
        case 188: return "188";
        case 189: return "189";
        case 190: return "190";
        case 191: return "191";
        case 192: return "192";
        case 193: return "193";
        case 194: return "194";
        case 195: return "195";
        case 196: return "196";
        case 197: return "197";
        case 198: return "198";
        case 199: return "199";
        case 200: return "200";
        case 201: return "201";
        case 202: return "202";
        case 203: return "203";
        case 204: return "204";
        case 205: return "205";
        case 206: return "206";
        case 207: return "207";
        case 208: return "208";
        case 209: return "209";
        case 210: return "210";
        case 211: return "211";
        case 212: return "212";
        case 213: return "213";
        case 214: return "214";
        case 215: return "215";
        case 216: return "216";
        case 217: return "217";
        case 218: return "218";
        case 219: return "219";
        case 220: return "220";
        case 221: return "221";
        case 222: return "222";
        case 223: return "223";
        case 224: return "224";
        case 225: return "225";
        case 226: return "226";
        case 227: return "227";
        case 228: return "228";
        case 229: return "229";
        case 230: return "230";
        case 231: return "231";
        case 232: return "232";
        case 233: return "233";
        case 234: return "234";
        case 235: return "235";
        case 236: return "236";
        case 237: return "237";
        case 238: return "238";
        case 239: return "239";
        case 240: return "240";
        case 241: return "241";
        case 242: return "242";
        case 243: return "243";
        case 244: return "244";
        case 245: return "245";
        case 246: return "246";
        case 247: return "247";
        case 248: return "248";
        case 249: return "249";
        case 250: return "250";
        case 251: return "251";
        case 252: return "252";
        case 253: return "253";
        case 254: return "254";
        case 255: return "255";
    }

    return "?";
}

const char *telnet_iac_byte_to_code(unsigned char c) {
    switch (c) {
        case TELNET_OPT_MCCP:           return "MCCP";
        case TELNET_OPT_MSSP:           return "MSSP";
        case TELNET_OPT_MSDP:           return "MSDP";
        case TELNET_OPT_BINARY:         return "OPT_BIN";
        case TELNET_OPT_ECHO:           return "OPT_ECHO";
        case TELNET_OPT_RCP:            return "OPT_RCP";
        case TELNET_OPT_SGA:            return "OPT_SGA";
        case TELNET_OPT_NAMS:           return "OPT_NAMS";
        case TELNET_OPT_STATUS:         return "OPT_STATUS";
        case TELNET_OPT_TM:             return "OPT_TM";
        case TELNET_OPT_RCTE:           return "OPT_RCTE";
        case TELNET_OPT_NAOL:           return "OPT_NAOL";
        case TELNET_OPT_NAOP:           return "OPT_NAOP";
        case TELNET_OPT_NAOCRD:         return "OPT_NAOCRD";
        case TELNET_OPT_NAOHTS:         return "OPT_NAOHTS";
        case TELNET_OPT_NAOHTD:         return "OPT_NAOHTD";
        case TELNET_OPT_NAOFFD:         return "OPT_NAOFFD";
        case TELNET_OPT_NAOVTS:         return "OPT_NAOVTS";
        case TELNET_OPT_NAOVTD:         return "OPT_NAOVTD";
        case TELNET_OPT_NAOLFD:         return "OPT_NAOLFD";
        case TELNET_OPT_XASCII:         return "OPT_XASCII";
        case TELNET_OPT_LOGOUT:         return "OPT_LOGOUT";
        case TELNET_OPT_BM:             return "OPT_BM";
        case TELNET_OPT_DET:            return "OPT_DET";
        case TELNET_OPT_SUPDUP:         return "OPT_SUPDUP";
        case TELNET_OPT_SUPDUPOUTPUT:   return "OPT_SUPDUPOUTPUT";
        case TELNET_OPT_SNDLOC:         return "OPT_SNDLOC";
        case TELNET_OPT_TTYPE:          return "OPT_TTYPE";
        case TELNET_OPT_EOR:            return "OPT_EOR";
        case TELNET_OPT_NAWS:           return "OPT_NAWS";
        case TELNET_OPT_TS:             return "OPT_TS";
        case TELNET_OPT_EOPT:           return "OPT_EOPT";
        case TELNET_OPT_NEO:            return "OPT_NEO";
        case TELNET_OPT_CHARSET:        return "OPT_CHARSET";
        case TELNET_EOF:                return "EOF";
        case TELNET_EOR:                return "EOR";
        case TELNET_SE:                 return "SE";
        case TELNET_NOP:                return "NOP";
        case TELNET_DM:                 return "DM/SYNCH";
        case TELNET_BREAK:              return "BREAK";
        case TELNET_IP:                 return "IP";
        case TELNET_AO:                 return "AO";
        case TELNET_AYT:                return "AYT";
        case TELNET_EC:                 return "EC";
        case TELNET_EL:                 return "EL";
        case TELNET_GA:                 return "GA";
        case TELNET_SB:                 return "SB";
        case TELNET_WILL:               return "WILL";
        case TELNET_WONT:               return "WONT";
        case TELNET_DO:                 return "DO";
        case TELNET_DONT:               return "DONT";
        case TELNET_IAC:                return "IAC";
        default: break;
    }

    return telnet_uchar_to_string(c);
}
