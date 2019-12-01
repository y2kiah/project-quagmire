#ifndef _NSTRING_H
#define _NSTRING_H
// TODO: keep calling this nstring?

#include "common.h"

#define STRING_CONSTRUCTORS(name) \
	name() {}\
	name(const char* s) {\
		size_t slen = strlen(s);\
		assert(slen < capacity && "string constructor buffer overflow");\
		sizeB = (size_type)slen;\
		_strncpy_s(c_str, capacity, s, sizeB);\
	}
#define STRING_ASSIGN_FROM_CONST_CHAR(name) \
	name& operator=(const char* s) {\
		size_t slen = strlen(s);\
		assert(c_str != nullptr && "string assignment to uninitialized");\
		assert(slen < capacity && "string assignment buffer overflow");\
		sizeB = (size_type)slen;\
		_strncpy_s(c_str, capacity, s, sizeB);\
		return *this;\
	}

/**
 * @struct pstring
 * Stub for a string that exists at an external address, such as a string table. Any fixed length
 * "fstringXX" can be converted to a pstring using the to_pstring function. The memory pointed to
 * by c_str is not allocated or freed by the pstring instance (no RAII), so the fstring or other
 * backing buffer would have to outlive the pstring.
 */
struct pstring {
	typedef u32 size_type;

	u32 sizeB;		// current byte length of c_str, NOT including null terminating character
	u32 capacity;	// capacity of c_str buffer, including null terminating character
	char* c_str;

	STRING_ASSIGN_FROM_CONST_CHAR(pstring)
};

/**
 * @struct fstring14
 * Fixed length string that fits 14 bytes plus the null terminating character.
 */
struct fstring14 {
	typedef u8 size_type;
	enum { capacity=15 };
	
	size_type sizeB = 0;
	char c_str[capacity] = {};
	
	STRING_CONSTRUCTORS(fstring14)
	STRING_ASSIGN_FROM_CONST_CHAR(fstring14)
};

/**
 * @struct fstring30
 * Fixed length string that fits 30 bytes plus the null terminating character.
 */
struct fstring30 {
	typedef u8 size_type;
	enum { capacity=31 };
	
	size_type sizeB = 0;
	char c_str[capacity] = {};

	STRING_CONSTRUCTORS(fstring30)
	STRING_ASSIGN_FROM_CONST_CHAR(fstring30)
};

/**
 * @struct fstring62
 * Fixed length string that fits 62 bytes plus the null terminating character.
 */
struct fstring62 {
	typedef u8 size_type;
	enum { capacity=63 };
	
	size_type sizeB = 0;
	char c_str[capacity] = {};

	STRING_CONSTRUCTORS(fstring62)
	STRING_ASSIGN_FROM_CONST_CHAR(fstring62)
};

/**
 * @struct fstring126
 * Fixed length string that fits 126 bytes plus the null terminating character.
 */
struct fstring126 {
	typedef u8 size_type;
	enum { capacity=127 };
	
	size_type sizeB = 0;
	char c_str[capacity] = {};

	STRING_CONSTRUCTORS(fstring126)
	STRING_ASSIGN_FROM_CONST_CHAR(fstring126)
};

/**
 * @struct fstring254
 * Fixed length string that fits 254 bytes plus the null terminating character.
 */
struct fstring254 {
	typedef u8 size_type;
	enum { capacity=255 };
	
	size_type sizeB = 0;
	char c_str[255] = {};

	STRING_CONSTRUCTORS(fstring254)
	STRING_ASSIGN_FROM_CONST_CHAR(fstring254)
};

/**
 * @struct fstring509
 * Fixed length string that fits 509 bytes plus the null terminating character.
 */
struct fstring509 {
	typedef u16 size_type;
	enum { capacity=510 };
	
	size_type sizeB = 0;
	char c_str[510] = {};

	STRING_CONSTRUCTORS(fstring509)
	STRING_ASSIGN_FROM_CONST_CHAR(fstring509)
};

/**
 * @struct fstring1021
 * Fixed length string that fits 1021 bytes plus the null terminating character.
 */
struct fstring1021 {
	typedef u16 size_type;
	enum { capacity=1022 };
	
	size_type sizeB = 0;
	char c_str[1022] = {};

	STRING_CONSTRUCTORS(fstring1021)
	STRING_ASSIGN_FROM_CONST_CHAR(fstring1021)
};


// Conversions from fixed length fstrings to pstring (for doing bounds-checked string ops)

inline pstring to_pstring(fstring14& s)   { return { s.sizeB, fstring14::capacity, s.c_str }; }
inline pstring to_pstring(fstring30& s)   { return { s.sizeB, fstring30::capacity, s.c_str }; }
inline pstring to_pstring(fstring62& s)   { return { s.sizeB, fstring62::capacity, s.c_str }; }
inline pstring to_pstring(fstring126& s)  { return { s.sizeB, fstring126::capacity, s.c_str }; }
inline pstring to_pstring(fstring254& s)  { return { s.sizeB, fstring254::capacity, s.c_str }; }
inline pstring to_pstring(fstring509& s)  { return { s.sizeB, fstring509::capacity, s.c_str }; }
inline pstring to_pstring(fstring1021& s) { return { s.sizeB, fstring1021::capacity, s.c_str }; }
inline pstring to_pstring(char *s, u32 capacity) { return { (u32)strlen(s), capacity, s }; }

// String operations

inline pstring& operator+=(const char*s, pstring& pd) {
	size_t slen = strlen(s);
	assert(pd.sizeB + slen < pd.capacity && "string append buffer overflow");
	memmove(pd.c_str+slen, pd.c_str, pd.sizeB+1);
	_memcpy_s(pd.c_str, pd.capacity, s, slen);
	pd.sizeB += (u32)slen;
	return pd;
}

inline pstring& operator+=(pstring& pd, const char*s) {
	size_t slen = strlen(s);
	assert(pd.sizeB + slen < pd.capacity && "string append buffer overflow");
	_memcpy_s(pd.c_str+pd.sizeB, pd.capacity-pd.sizeB, s, slen+1);
	pd.sizeB += (u32)slen;
	return pd;
}

inline pstring& operator+=(pstring& pd, pstring& ps) {
	assert(pd.sizeB + ps.sizeB < pd.capacity && "string append buffer overflow");
	_memcpy_s(pd.c_str+pd.sizeB, pd.capacity-pd.sizeB, ps.c_str, ps.sizeB+1);
	pd.sizeB += ps.sizeB;
	return pd;
}

#endif