#ifndef _NSTRING_H
#define _NSTRING_H

#include "types.h"

/**
 * @struct pstring
 * Stub for a string that exists at an external address, such as a string table. Any fixed length
 * "fstringXX" can be converted to a pstring using the to_pstring function, accepting the loss of
 * known buffer length. The memory pointed to by c_str is not allocated or freed by the pstring
 * instance (no RAII), so the fstring would have to outlive the pstring after conversion.
 */
struct pstring {
	size_t sizeB;
    char* c_str;
};

/**
 * @struct fstring14
 * Fixed length string that fits 14 bytes plus the null terminating character.
 */
struct fstring14 {
    u8 sizeB;
    char c_str[15];
};

/**
 * @struct fstring30
 * Fixed length string that fits 30 bytes plus the null terminating character.
 */
struct fstring30 {
    u8 sizeB;
    char c_str[31];
};

/**
 * @struct fstring62
 * Fixed length string that fits 62 bytes plus the null terminating character.
 */
struct fstring62 {
    u8 sizeB;
    char c_str[63];
};

/**
 * @struct fstring126
 * Fixed length string that fits 126 bytes plus the null terminating character.
 */
struct fstring126 {
    u8 sizeB;
    char c_str[127];
};

/**
 * @struct fstring254
 * Fixed length string that fits 254 bytes plus the null terminating character.
 */
struct fstring254 {
    u8 sizeB;
    char c_str[255];
};

/**
 * @struct fstring509
 * Fixed length string that fits 509 bytes plus the null terminating character.
 */
struct fstring509 {
    u16 sizeB;
    char c_str[510];
};

/**
 * @struct fstring1021
 * Fixed length string that fits 1021 bytes plus the null terminating character.
 */
struct fstring1021 {
    u16 sizeB;
    char c_str[1022];
};


inline pstring to_pstring(fstring14& s)   { return { s.sizeB, s.c_str }; }
inline pstring to_pstring(fstring30& s)   { return { s.sizeB, s.c_str }; }
inline pstring to_pstring(fstring62& s)   { return { s.sizeB, s.c_str }; }
inline pstring to_pstring(fstring126& s)  { return { s.sizeB, s.c_str }; }
inline pstring to_pstring(fstring254& s)  { return { s.sizeB, s.c_str }; }
inline pstring to_pstring(fstring509& s)  { return { s.sizeB, s.c_str }; }
inline pstring to_pstring(fstring1021& s) { return { s.sizeB, s.c_str }; }

//inline i8 cmp()

#endif