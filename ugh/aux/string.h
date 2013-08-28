#ifndef __AUX_STRING_H__
#define __AUX_STRING_H__

#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#if 1
typedef struct { size_t size; char *data; } strt, *strp;

#define aux_string(s) { sizeof(s) - 1, s }
#define aux_null_string { 0, NULL }
extern strt aux_empty_string;

#define CR    0x0d
#define LF    0x0a
#define CRLF "\r\n"

#define QUOTES_NAME(val) #val
#define QUOTES_DATA(val) QUOTES_NAME(val)
#define CONCAT_NAME(a,b) a##b
#define CONCAT_DATA(a,b) CONCAT_NAME(a,b)

#define aux_clrptr(p)   memset((p),0,sizeof(*(p)))
#define aux_clrvec(p,n) memset((p),0,sizeof(*(p))*(n))
#define aux_clrmem(p,n) memset((p),0,(n))

#define aux_memcpy(d,s,n) ((memcpy(d,s,n)))
#define aux_cpymsz(d,s,n) ((memcpy(d,s,n),(n)))
#define aux_cpymem(d,s,n) (((char *)memcpy(d,s,n))+(n))

/* rowsof - static allocated arrays only */

#define aux_bitsof(x)       (sizeof(x) * 8)
#define aux_rowsof(x)       (sizeof(x)/sizeof((x)[0]))
#define aux_member(t,n)     (((t*)0)->(n))
#define aux_memberof(t,n,p) (((t*)(((unsigned char *)(p))-offsetof(t,n))))
#define aux_mebitsof(t,n)   (aux_bitsof(aux_member(t,n)))
#define aux_mesizeof(t,n)   (sizeof(aux_member(t,n)))
#define aux_metypeof(t,n)   (typeof(aux_member(t,n)))
#endif

/* ----------------- */

#define aux_strerror(err) aux_strerror_r(err, (char *) alloca(4096), 4096)

static inline
const char* aux_strerror_r(int err, char* data, size_t size)
{
#if !defined(__USE_GNU) || defined(__FreeBSD__)
    if (0 != strerror_r(err, data, size)) return "XSI strerror_r returned error";
    return data;
#else
    return strerror_r(err, data, size);
#endif /* __USE_GNU */
}

/* ----------------- */

extern const unsigned char aux_ctypes_table [UCHAR_MAX + 1];
extern const unsigned char aux_ch2dig_table [UCHAR_MAX + 1];

#define aux_ctypes(c) aux_ctypes_table[(unsigned char) (c)]
#define aux_ch2dig(c) aux_ch2dig_table[(unsigned char) (c)]

#define AUX_SPACE 0x01 /* space */
#define AUX_OCTAL 0x02 /* octal */
#define AUX_DECAL 0x04 /* digit */
#define AUX_HEXUP 0x08 /* hexup */
#define AUX_HEXLO 0x10 /* hexlo */
#define AUX_UPPER 0x20 /* upper */
#define AUX_LOWER 0x40 /* lower */

#define aux_isspace(c) (aux_ctypes(c) & (AUX_SPACE))
#define aux_isoctal(c) (aux_ctypes(c) & (AUX_OCTAL))
#define aux_isdecal(c) (aux_ctypes(c) & (AUX_OCTAL|AUX_DECAL))
#define aux_ishexit(c) (aux_ctypes(c) & (AUX_OCTAL|AUX_DECAL|AUX_HEXUP|AUX_HEXLO))
#define aux_ishexup(c) (aux_ctypes(c) & (AUX_HEXUP))
#define aux_ishexlo(c) (aux_ctypes(c) & (AUX_HEXLO))
#define aux_isupper(c) (aux_ctypes(c) & (AUX_HEXUP|AUX_UPPER))
#define aux_islower(c) (aux_ctypes(c) & (AUX_HEXLO|AUX_LOWER))
#define aux_isalpha(c) (aux_ctypes(c) & (AUX_HEXUP|AUX_HEXLO|AUX_UPPER|AUX_LOWER))
#define aux_isalnum(c) (aux_ctypes(c) & (AUX_OCTAL|AUX_DECAL|AUX_HEXUP|AUX_HEXLO|AUX_UPPER|AUX_LOWER))
#define aux_tolower(c) ((unsigned char) (((c) >= 'A' && (c) <= 'Z') ? ((c) |  0x20) : (c)))
#define aux_toupper(c) ((unsigned char) (((c) >= 'a' && (c) <= 'z') ? ((c) & ~0x20) : (c)))

#define hex_hi(x) "0123456789abcdef" [((unsigned char) (x)) >>  4]
#define hex_lo(x) "0123456789abcdef" [((unsigned char) (x)) & 0xf]
#define HEX_hi(x) "0123456789ABCDEF" [((unsigned char) (x)) >>  4]
#define HEX_lo(x) "0123456789ABCDEF" [((unsigned char) (x)) & 0xf]
#define hex_pk(l,r) ((*l++ = hex_hi(r)), (*l++ = hex_lo(r)))
#define HEX_pk(l,r) ((*l++ = HEX_hi(r)), (*l++ = HEX_lo(r)))

static inline
int aux_bin2str(char *dest, const char *data, size_t size)
{
	for (; size--; ++data)
	{
		HEX_pk(dest,*data);
	}

	return 0;
}

static inline
int aux_str2bin(char *dest, const char *data, size_t size)
{
	unsigned char dig;

	for (;; ++dest)
	{
		if (!size--) break;
		if (16 <= (dig = aux_ch2dig(*data++))) return -1;
		*dest |= (dig & 0x0f) << 4;

		if (!size--) break;
		if (16 <= (dig = aux_ch2dig(*data++))) return -1;
		*dest |= dig & 0x0f;
	}

	return 0;
}

char *aux_stristr(const char *s, const char *n);              /* search for [n] in [s] ignoring case */
char *aux_strnstr(const char *s, const char *n, size_t len);  /* search for [n] in first [len] bytes of [s] */
char *aux_strxstr(const char *s, const char *n, size_t len);  /* search for [n] in first [len] bytes of [s] ignoring case */

static inline
char *aux_memmem(const char *m, size_t sz_m, const char *n, size_t sz_n)
{
	const char *bm, *em, *bn, *en;

	if (!m || !sz_m) return NULL;
	if (!n || !sz_n) return (char *) m;
	if (sz_m < sz_n) return NULL;

	bm = m; em = bm + sz_m;
	bn = n; en = bn + sz_n;

	for (; bm < em; ++bm)
	{
		if (*bm == *bn)
		{
			const char *pm, *pn;

			for (pm = bm, pn = bn; pm < em && pn < en; ++pm, ++pn)
			{
				if (*pm != *pn) break;
			}

			if (pn == en) return (char *) bm;
		}
	}

	return NULL;
}

/* two functions below will add terminating 0 byte! */
size_t aux_urlenc(char *dst, const char *src, size_t sz_src);
size_t aux_urldec(char *dst, const char *src, size_t sz_src);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* __AUX_STRING_H__ */
