// Copyright (c) 2012-2013 Plenluno All rights reserved.

#include <libj/config.h>
#include <libj/endian.h>
#include <libj/glue/cvtutf.h>

#include <assert.h>

#ifdef LIBJ_USE_ICONV
    #include <errno.h>
    #include <iconv.h>
#else
    #include <ConvertUTF.h>
#endif

namespace libj {
namespace glue {

static bool isBigEndian() {
    static bool big = endian() == BIG;
    return big;
}

std::u32string utf16ToUtf32(const std::u16string& str) {
    assert(sizeof(char16_t) == 2);

    if (isBigEndian()) {
        return toUtf32(str.c_str(), UTF16BE, -1);
    } else {
        return toUtf32(str.c_str(), UTF16LE, -1);
    }
}

#ifdef LIBJ_USE_ICONV

#ifdef LIBJ_CVTUTF_DEBUG
    #include <stdio.h>
    #define ICONV iconvDebug
#else
    #define ICONV iconv
#endif

static iconv_t iconvOpen(const char* inEnc, UnicodeEncoding outEnc) {
    iconv_t cd;
    switch (outEnc) {
    case UTF8:
        cd = iconv_open("UTF-8", inEnc);
        break;
    case UTF16BE:
        cd = iconv_open("UTF-16BE", inEnc);
        break;
    case UTF16LE:
        cd = iconv_open("UTF-16LE", inEnc);
        break;
    case UTF32BE:
        cd = iconv_open("UTF-32BE", inEnc);
        break;
    case UTF32LE:
        cd = iconv_open("UTF-32LE", inEnc);
        break;
    default:
        assert(false);
    }
    assert(cd != reinterpret_cast<iconv_t>(-1));
    return cd;
}

static iconv_t iconvOpen(UnicodeEncoding inEnc, const char* outEnc) {
    iconv_t cd;
    switch (inEnc) {
    case UTF8:
        cd = iconv_open(outEnc, "UTF-8");
        break;
    case UTF16BE:
        cd = iconv_open(outEnc, "UTF-16BE");
        break;
    case UTF16LE:
        cd = iconv_open(outEnc, "UTF-16LE");
        break;
    case UTF32BE:
        cd = iconv_open(outEnc, "UTF-32BE");
        break;
    case UTF32LE:
        cd = iconv_open(outEnc, "UTF-32LE");
        break;
    default:
        assert(false);
    }
    assert(cd != reinterpret_cast<iconv_t>(-1));
    return cd;
}

static iconv_t iconvOpenFromUtf32(UnicodeEncoding outEnc) {
    const char* inEnc;
    if (isBigEndian()) {
        inEnc = "UTF-32BE";
    } else {
        inEnc = "UTF-32LE";
    }
    return iconvOpen(inEnc, outEnc);
}

static iconv_t iconvOpenToUtf32(UnicodeEncoding inEnc) {
    const char* outEnc;
    if (isBigEndian()) {
        outEnc = "UTF-32BE";
    } else {
        outEnc = "UTF-32LE";
    }
    return iconvOpen(inEnc, outEnc);
}

static void iconClose(iconv_t cd) {
    iconv_close(cd);
}

#ifdef LIBJ_CVTUTF_DEBUG
static size_t iconvDebug(
    iconv_t cd,
    char **inBuf, size_t *inBytesLeft,
    char **outBuf, size_t *outBytesLeft) {
    char* inStart = *inBuf;
    char* outStart = *outBuf;
    size_t inBytes = *inBytesLeft;
    size_t outBytes = *outBytesLeft;

    size_t ret = iconv(cd, inBuf, inBytesLeft, outBuf, outBytesLeft);

    printf("return: %zd\n", ret);
    if (ret) {
        switch (errno) {
        case EILSEQ:
            printf("errno: EILSEQ\n");
            break;
        case EINVAL:
            printf("errno: EINVAL\n");
            break;
        case E2BIG:
            printf("errno: E2BIG\n");
            break;
        default:
            assert(false);
        }
    }

    printf("inbytesleft: %zd -> %zd\n", inBytes, *inBytesLeft);
    printf("outbytesleft: %zd -> %zd\n", outBytes, *outBytesLeft);
    assert(*inBuf - inStart == inBytes - *inBytesLeft);
    assert(*outBuf - outStart == outBytes - *outBytesLeft);

    return ret;
}
#endif  // LIBJ_CVTUTF_DEBUG

std::string fromUtf32(const std::u32string& str, UnicodeEncoding enc) {
    assert(sizeof(char) == 1 && sizeof(char32_t) == 4);

    std::string s;
    iconv_t cd = iconvOpenFromUtf32(enc);
    char* inBuf = reinterpret_cast<char*>(const_cast<char32_t*>(str.c_str()));
    size_t inLen = str.length();
    for (size_t i = 0; i < inLen; i++) {
#ifdef LIBJ_CVTUTF_DEBUG
        printf("fromUtf32[%zd]: %d\n", i, str[i]);
#endif  // LIBJ_CVTUTF_DEBUG
        size_t inBytesLeft = 4;
        char c8[8];
        char* outBuf = c8;
        size_t outBytesLeft = 8;
        ICONV(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft);
        assert(outBytesLeft < 8);
        size_t outLen = 8 - outBytesLeft;
        for (size_t j = 0; j < outLen; j++) {
            s += c8[j];
        }
    }
    if (enc == UTF16BE || enc == UTF16LE) {
        for (int i = 0; i < 2; i++) {
            s += static_cast<char>(0);
        }
    } else if (enc == UTF32BE || enc == UTF32LE) {
        for (int i = 0; i < 4; i++) {
            s += static_cast<char>(0);
        }
    }
    iconClose(cd);
    return s;
}

static size_t getByteLength(const void* data, UnicodeEncoding enc) {
    switch (enc) {
    case UTF8: {
            uint8_t c = static_cast<const uint8_t*>(data)[0];
            if (c < 0x80) {
                return 1;
            } else if (c < 0xe0) {
                return 2;
            } else if (c < 0xf0) {
                return 3;
            } else if (c < 0xf8) {
                return 4;
            } else if (c < 0xfc) {
                return 5;
            } else if (c < 0xfe) {
                return 6;
            } else {
                return 0;
            }
        }
    case UTF16BE: {
            uint16_t c = static_cast<const uint16_t*>(data)[0];
            if ((c & 0x00f8) == 0x00d8) {
                return 4;
            } else {
                return 2;
            }
        }
    case UTF16LE: {
            uint16_t c = static_cast<const uint16_t*>(data)[0];
            if ((c & 0xf800) == 0xd800) {
                return 4;
            } else {
                return 2;
            }
        }
    case UTF32BE:
    case UTF32LE:
        return 4;
    default:
        assert(false);
    }
}

std::u32string toUtf32(const void* data, UnicodeEncoding enc, size_t max) {
    assert(sizeof(char) == 1 && sizeof(char32_t) == 4);

    if (!data) return std::u32string();

    std::u32string s32;
    iconv_t cd = iconvOpenToUtf32(enc);
    char* inBuf = static_cast<char*>(const_cast<void*>(data));
    for (size_t i = 0; i < max; i++) {
        size_t inBytesLeft = getByteLength(inBuf, enc);
        if (!inBytesLeft) break;
        union {
            char32_t c32;
            char c8[4];
        } u;
        char* outBuf = u.c8;
        size_t outBytesLeft = 4;
        ICONV(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft);
        if (outBytesLeft || !u.c32) {
            break;
        } else {
#ifdef LIBJ_CVTUTF_DEBUG
        printf("toUtf32[%zd]: %d\n", i, u.c32);
#endif  // LIBJ_CVTUTF_DEBUG
            s32 += u.c32;
        }
    }
    iconClose(cd);
    return s32;
}

std::u16string utf32ToUtf16(const std::u32string& str) {
    assert(sizeof(char) == 1 && sizeof(char16_t) == 2 && sizeof(char32_t) == 4);

    std::u16string s;
    iconv_t cd;
    if (isBigEndian()) {
        cd = iconvOpenFromUtf32(UTF16BE);
    } else {
        cd = iconvOpenFromUtf32(UTF16LE);
    }
    char* inBuf = reinterpret_cast<char*>(const_cast<char32_t*>(str.c_str()));
    size_t inLen = str.length();
    for (size_t i = 0; i < inLen; i++) {
        size_t inBytesLeft = 4;
        union {
            char16_t c16[4];
            char c8[8];
        } u;
        char* outBuf = u.c8;
        size_t outBytesLeft = 8;
        ICONV(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft);
        assert(outBytesLeft < 8);
        size_t outLen = (8 - outBytesLeft) >> 1;
        for (size_t j = 0; j < outLen; j++) {
            s += u.c16[j];
        }
    }
    iconClose(cd);
    return s;
}

#else  // LIBJ_USE_ICONV

static Boolean needsSwap(UnicodeEncoding enc) {
    switch (enc) {
    case UTF16BE:
        return !isBigEndian();
    case UTF16LE:
        return isBigEndian();
    case UTF32BE:
        return !isBigEndian();
    case UTF32LE:
        return isBigEndian();
    default:
        return false;
    }
}

static char16_t swapUtf16(char16_t c16) {
    uint16_t u16 = static_cast<uint16_t>(c16);
    return (u16 << 8) | (u16 >> 8);
}

static char32_t swapUtf32(char32_t c32) {
    uint32_t u32 = static_cast<uint32_t>(c32);
    u32 = ((u32 << 8) & 0xFF00FF00) | ((u32 >> 8) & 0xFF00FF);
    return (u32 << 16) | (u32 >> 16);
}

static size_t findUtf8End(
    const unsigned char* start,
    size_t max,
    const unsigned char** end) {
    size_t n = 0;
    const unsigned char* pos = start;
    for (; n < max; n++) {
        unsigned char c = *pos;
        if (c) {
            pos += getNumBytesForUTF8(c);
        } else {
            break;
        }
    }
    *end = pos;
    return n;
}

static size_t findUtf16End(
    const char16_t* start,
    UnicodeEncoding enc,
    size_t max,
    const char16_t** end,
    std::u16string** swaped) {
    if (!max) {
        *end = start;
        *swaped = NULL;
        return 0;
    }

    Boolean swap = needsSwap(enc);
    if (swap) {
        *swaped = new std::u16string();
    } else {
        *swaped = NULL;
    }

    size_t n = 0;
    const char16_t* pos = start;
    for (; n < max; n++) {
        char16_t c16 = *pos;
        if (c16) {
            if (swap) {
                c16 = swapUtf16(c16);
                **swaped += c16;
            }
            pos++;

            if (isSurrogatePair(c16)) {
                if (swap) {
                    **swaped += swapUtf16(*pos);
                }
                pos++;
            }
        } else {
            break;
        }
    }
    *end = pos;
    return n;
}

static std::u32string utf8ToUtf32(
    const unsigned char* data,
    size_t max) {
    assert(data && sizeof(unsigned char) == 1 && sizeof(char32_t) == 4);

    const unsigned char* end;
    size_t n = findUtf8End(data, max, &end);
    if (!n) return std::u32string();

    const unsigned char* sourceStart = data;
    const unsigned char* sourceEnd = end;

    char32_t* out = new char32_t[n];
    char32_t* targetStart = out;
    char32_t* targetEnd = out + n;

    ConvertUTF8toUTF32(
        &sourceStart,
        sourceEnd,
        &targetStart,
        targetEnd,
        strictConversion);

    std::u32string u32s(out, targetStart - out);
    delete[] out;
    return u32s;
}

static std::u32string utf16ToUtf32(
    const char16_t* data,
    UnicodeEncoding enc,
    size_t max) {
    assert(data && sizeof(char16_t) == 2 && sizeof(char32_t) == 4);

    const char16_t* end;
    std::u16string* u16s;
    size_t n = findUtf16End(data, enc, max, &end, &u16s);
    if (!n) {
        delete u16s;
        return std::u32string();
    }

    const char16_t* sourceStart;
    const char16_t* sourceEnd;
    if (u16s) {
        sourceStart = u16s->c_str();
        sourceEnd = sourceStart + u16s->length();
    } else {
        sourceStart = data;
        sourceEnd = end;
    }

    char32_t* out = new char32_t[n];
    char32_t* targetStart = out;
    char32_t* targetEnd = out + n;

    ConvertUTF16toUTF32(
        &sourceStart,
        sourceEnd,
        &targetStart,
        targetEnd,
        strictConversion);

    std::u32string u32s(out, targetStart - out);
    delete u16s;
    delete[] out;
    return u32s;
}

static std::u32string utf32ToUtf32(
    const char32_t* data,
    UnicodeEncoding enc,
    size_t max) {
    assert(data);

    if (!max) return std::u32string();

    std::u32string u32s;
    size_t n = 0;
    const char32_t* pos = data;
    Boolean swap = needsSwap(enc);
    for (; n < max; n++, pos++) {
        char32_t c32 = *pos;
        if (c32) {
            if (swap) c32 = swapUtf32(c32);
            if (c32 <= UNI_MAX_LEGAL_UTF32) {
                u32s += c32;
            } else {
                break;
            }
        } else {
            break;
        }
    }
    return u32s;
}

static std::string utf32ToUtf8(const std::u32string& str) {
    assert(sizeof(unsigned char) == 1 && sizeof(char32_t) == 4);

    if (!str.length()) return std::string();

    const char32_t* sourceStart = str.c_str();
    const char32_t* sourceEnd = sourceStart + str.length();

    size_t n = str.length() * 6;
    unsigned char* out = new unsigned char[n];
    unsigned char* targetStart = out;
    unsigned char* targetEnd = out + n;

    ConvertUTF32toUTF8(
        &sourceStart,
        sourceEnd,
        &targetStart,
        targetEnd,
        strictConversion);

    std::string u8s(reinterpret_cast<char*>(out), targetStart - out);
    delete[] out;
    return u8s;
}

std::u16string utf32ToUtf16(const std::u32string& str) {
    assert(sizeof(char16_t) == 2 && sizeof(char32_t) == 4);

    if (!str.length()) return std::u16string();

    const char32_t* sourceStart = str.c_str();
    const char32_t* sourceEnd = sourceStart + str.length();

    size_t n = str.length() * 2;
    char16_t* out = new char16_t[n];
    char16_t* targetStart = out;
    char16_t* targetEnd = out + n;

    ConvertUTF32toUTF16(
        &sourceStart,
        sourceEnd,
        &targetStart,
        targetEnd,
        strictConversion);

    std::u16string u16s(reinterpret_cast<char16_t*>(out), targetStart - out);
    delete[] out;
    return u16s;
}

static std::string toStdString(
    const std::u16string& str,
    UnicodeEncoding enc) {
    std::u16string u16s;
    Boolean swap = needsSwap(enc);
    size_t len = str.length();
    for (size_t i = 0; i < len; i++) {
        char16_t c16 = str[i];
        if (c16) {
            if (swap) c16 = swapUtf16(c16);
            u16s += c16;
        } else {
            break;
        }
    }
    u16s += static_cast<char16_t>(0);
    return std::string(
        reinterpret_cast<const char*>(u16s.c_str()),
        u16s.length() << 1);
}

static std::string toStdString(
    const std::u32string& str,
    UnicodeEncoding enc) {
    std::u32string u32s;
    Boolean swap = needsSwap(enc);
    size_t len = str.length();
    for (size_t i = 0; i < len; i++) {
        char32_t c32 = str[i];
        if (c32 > 0 && c32 <= UNI_MAX_LEGAL_UTF32) {
            if (swap) c32 = swapUtf32(c32);
            u32s += c32;
        } else {
            break;
        }
    }
    u32s += static_cast<char32_t>(0);
    return std::string(
        reinterpret_cast<const char*>(u32s.c_str()),
        u32s.length() << 2);
}

std::string fromUtf32(const std::u32string& str, UnicodeEncoding enc) {
    switch (enc) {
    case UTF8:
        return utf32ToUtf8(str);
    case UTF16BE:
    case UTF16LE:
        return toStdString(utf32ToUtf16(str), enc);
    case UTF32BE:
    case UTF32LE:
        return toStdString(str, enc);
    }
}

std::u32string toUtf32(const void* data, UnicodeEncoding enc, size_t max) {
    switch (enc) {
    case UTF8:
        return utf8ToUtf32(static_cast<const unsigned char*>(data), max);
    case UTF16BE:
    case UTF16LE:
        return utf16ToUtf32(static_cast<const char16_t*>(data), enc, max);
    case UTF32BE:
    case UTF32LE:
        return utf32ToUtf32(static_cast<const char32_t*>(data), enc, max);
    }
}

#endif  // LIBJ_USE_ICONV

}  // namespace glue
}  // namespace libj
