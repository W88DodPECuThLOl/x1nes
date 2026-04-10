#ifndef INCL_BasicTypes_h
#define INCL_BasicTypes_h

#include <catZ80Lib.h>

/**
 * @brief 32bit用のポインタ
 */
typedef union Ptr32 {
    u8* ptr;
    u32 ptr32;
} Ptr32;

#define LOG(logLevel, fmt, ...)

#endif // INCL_BasicTypes_h
