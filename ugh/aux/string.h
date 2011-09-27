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
typedef char scht, *schp;
typedef unsigned char ucht, *uchp;
typedef unsigned int uint;
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
#define aux_memberof(t,n,p) (((t*)(((uchp)(p))-offsetof(t,n))))
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

extern const ucht aux_ctypes_table [UCHAR_MAX + 1];
extern const ucht aux_ch2dig_table [UCHAR_MAX + 1];

#define aux_ctypes(c) aux_ctypes_table[(ucht) (c)]
#define aux_ch2dig(c) aux_ch2dig_table[(ucht) (c)]

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
#define aux_tolower(c) ((ucht) (((c) >= 'A' && (c) <= 'Z') ? ((c) |  0x20) : (c)))
#define aux_toupper(c) ((ucht) (((c) >= 'a' && (c) <= 'z') ? ((c) & ~0x20) : (c)))

#define hex_hi(x) "0123456789abcdef" [((ucht) (x)) >>  4]
#define hex_lo(x) "0123456789abcdef" [((ucht) (x)) & 0xf]
#define HEX_hi(x) "0123456789ABCDEF" [((ucht) (x)) >>  4]
#define HEX_lo(x) "0123456789ABCDEF" [((ucht) (x)) & 0xf]
#define hex_pk(l,r) ((*l++ = hex_hi(r)), (*l++ = hex_lo(r)))
#define HEX_pk(l,r) ((*l++ = HEX_hi(r)), (*l++ = HEX_lo(r)))
/* #define hex_pk(l,r) do { *l++ = hex_hi(r); *l++ = hex_lo(r); } while (0) */
/* #define HEX_pk(l,r) do { *l++ = HEX_hi(r); *l++ = HEX_lo(r); } while (0) */

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
	ucht dig;

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

/* --- */

#define AUX_IFELSE(b,y,n) CONCAT_DATA(AUX_IFELSE_, AUX_TOBOOL(b))(y,n)
#define AUX_IFELSE_0(y,n) n
#define AUX_IFELSE_1(y,n) y

#define AUX_TOBOOL(b) CONCAT_DATA(AUX_TOBOOL_, b)
/* AUX_TOBOOL_ {{{ */
#define AUX_TOBOOL_0  0
#define AUX_TOBOOL_1  1
#define AUX_TOBOOL_2  1
#define AUX_TOBOOL_3  1
#define AUX_TOBOOL_4  1
#define AUX_TOBOOL_5  1
#define AUX_TOBOOL_6  1
#define AUX_TOBOOL_7  1
#define AUX_TOBOOL_8  1
#define AUX_TOBOOL_9  1
#define AUX_TOBOOL_10 1
#define AUX_TOBOOL_11 1
#define AUX_TOBOOL_12 1
#define AUX_TOBOOL_13 1
#define AUX_TOBOOL_14 1
#define AUX_TOBOOL_15 1
#define AUX_TOBOOL_16 1
#define AUX_TOBOOL_17 1
#define AUX_TOBOOL_18 1
#define AUX_TOBOOL_19 1
#define AUX_TOBOOL_20 1
#define AUX_TOBOOL_21 1
#define AUX_TOBOOL_22 1
#define AUX_TOBOOL_23 1
#define AUX_TOBOOL_24 1
#define AUX_TOBOOL_25 1
#define AUX_TOBOOL_26 1
#define AUX_TOBOOL_27 1
#define AUX_TOBOOL_28 1
#define AUX_TOBOOL_29 1
#define AUX_TOBOOL_30 1
#define AUX_TOBOOL_31 1
#define AUX_TOBOOL_32 1
#define AUX_TOBOOL_33 1
#define AUX_TOBOOL_34 1
#define AUX_TOBOOL_35 1
#define AUX_TOBOOL_36 1
#define AUX_TOBOOL_37 1
#define AUX_TOBOOL_38 1
#define AUX_TOBOOL_39 1
#define AUX_TOBOOL_40 1
#define AUX_TOBOOL_41 1
#define AUX_TOBOOL_42 1
#define AUX_TOBOOL_43 1
#define AUX_TOBOOL_44 1
#define AUX_TOBOOL_45 1
#define AUX_TOBOOL_46 1
#define AUX_TOBOOL_47 1
#define AUX_TOBOOL_48 1
#define AUX_TOBOOL_49 1
/* }}} */

/* --- */

#define AUX_READER_SIGN(sign,data,size) switch (data[0])        \
{                                                               \
    case '-': sign = -1; data++; size--; break;                 \
    case '+': sign =  1; data++; size--; break;                 \
    default : sign =  1; break;                                 \
}
#define AUX_READER_BASE(base,data,size) switch (data[0])        \
{                                                               \
    case '0': data++; size--;                                   \
        switch (data[0])                                        \
        {                                                       \
        case 'B':                                               \
        case 'b': base =  2; data++; size--; break;             \
        case 'X':                                               \
        case 'x': base = 16; data++; size--; break;             \
        default : base =  8; break;                             \
        }                                                       \
        break;                                                  \
    default : base = 10; break;                                 \
}
#define AUX_READER_UINT(dest,data,size,base)                    \
                                                                \
    for (; size--; ++data)                                      \
    {                                                           \
        ucht dig = aux_ch2dig(*data);                           \
        if (base <= dig) break;                                 \
                                                                \
        dest = dest * base + dig;                               \
    }
#define AUX_READER_UFPT(dest,data,size,base)                    \
                                                                \
    AUX_READER_UINT(dest,data,size,base);                       \
                                                                \
    if (data[0] == '.')                                         \
    {                                                           \
        typeof(dest) curr = base;                               \
                                                                \
        data++;                                                 \
                                                                \
        for (; size--; ++data)                                  \
        {                                                       \
            ucht dig = aux_ch2dig(*data);                       \
            if (base <= dig) break;                             \
                                                                \
            dest += dig / curr;                                 \
            curr *= base;                                       \
        }                                                       \
    }
/* TODO: Парсить экспоненциальную часть. В 10-тичных floating point числах
 * экспоненциальная часть предваряется [eE] и выражает степень числа 10.
 * Шестнадцатеричная запись использует [pP], число после которого означает
 * степень двойки (а не 16, как могло бы быть по аналогии с десятичной
 * записью). */
#if 0
    if (data[0] == 'e' || data[0] == 'E')                       \
    {                                                           \
        u32t temp = 0; data++;                                  \
        AUX_READER_UINT(temp,data,size,10);                     \
        dest *= pow(10, temp);                                  \
    }
#endif

#define AUX_READER_SIZE_K 1024
#define AUX_READER_SIZE_M 1048576
#define AUX_READER_SIZE_G 1073741824
#define AUX_READER_TIME_S 1
#define AUX_READER_TIME_M 60
#define AUX_READER_TIME_H 3600
#define AUX_READER_TIME_D 86400    /* 3600 * 24       */
#define AUX_READER_TIME_W 604800   /* 3600 * 24 * 7   */
#define AUX_READER_TIME_C 2592000  /* 3600 * 24 * 30  */
#define AUX_READER_TIME_Y 31536000 /* 3600 * 24 * 365 */
#define AUX_READER_MSEC_S 1000
#define AUX_READER_MSEC_M 1
#define AUX_READER_MSEC_U 1000
#define AUX_READER_MSEC_N 1000000
#define AUX_READER_FLAG_0 0
#define AUX_READER_FLAG_1 1
#define AUX_READER_IDLE(dest,data,size)
#define AUX_READER_SIZE(dest,data,size) switch (data[0])        \
{                                                               \
    case 'K': case 'k': dest *= AUX_READER_SIZE_K; break;       \
    case 'M': case 'm': dest *= AUX_READER_SIZE_M; break;       \
    case 'G': case 'g': dest *= AUX_READER_SIZE_G; break;       \
}
#define AUX_READER_TIME(dest,data,size) switch (data[0])        \
{                                                               \
    case 'M': case 'm': dest *= AUX_READER_TIME_M; break;       \
    case 'H': case 'h': dest *= AUX_READER_TIME_H; break;       \
    case 'D': case 'd': dest *= AUX_READER_TIME_D; break;       \
    case 'W': case 'w': dest *= AUX_READER_TIME_W; break;       \
    case 'C': case 'c': dest *= AUX_READER_TIME_C; break;       \
    case 'Y': case 'y': dest *= AUX_READER_TIME_Y; break;       \
}
#define AUX_READER_MSEC(dest,data,size) switch (data[0])        \
{                                                               \
    case 'S': case 's': dest *= AUX_READER_MSEC_S; break;       \
    case 'U': case 'u': dest /= AUX_READER_MSEC_U; break;       \
    case 'N': case 'n': dest /= AUX_READER_MSEC_N; break;       \
}
#define AUX_READER_FLAG(dest,data,size) switch (data[0])        \
{                                                               \
    case 'N': case 'n': dest = AUX_READER_FLAG_0; break;        \
    case 'F': case 'f': dest = AUX_READER_FLAG_0; break;        \
    case 'Y': case 'y': dest = AUX_READER_FLAG_1; break;        \
    case 'T': case 't': dest = AUX_READER_FLAG_1; break;        \
    default : dest = !!dest; break;                             \
}

#define AUX_STR2SI(dest,data,size,base,sign,EX) ({              \
                                                                \
    typeof(data) d = data;                                      \
    typeof(size) s = size;                                      \
    scht i;                                                     \
    ucht b;                                                     \
                                                                \
    AUX_IFELSE(sign, i = sign, AUX_READER_SIGN(i,d,s));         \
    AUX_IFELSE(base, b = base, AUX_READER_BASE(b,d,s));         \
                                                                \
    dest = 0;                                                   \
                                                                \
    AUX_READER_UINT(dest,d,s,b);                                \
    AUX_READER_##EX(dest,d,s);                                  \
                                                                \
    dest *= i;                                                  \
    dest;                                                       \
                                                                \
    })

#if 0
#define AUX_STR2SF(dest,data,size,base,EX) ({                   \
                                                                \
    if (0 == aux_strncmp(data, "inf", size))                    \
    {                                                           \
        dest = AUX_INF; /* not type-specific version */         \
    }                                                           \
    else                                                        \
    if (0 == aux_strncmp(data, "nan", size))                    \
    {                                                           \
        dest = AUX_NAN; /* not type-specific version */         \
    }                                                           \
    else                                                        \
    {                                                           \
        typeof(data) d = data;                                  \
        typeof(size) s = size;                                  \
                                                                \
        scht i;                                                 \
        ucht b;                                                 \
                                                                \
        AUX_IFELSE(sign, i = sign, AUX_READER_SIGN(i,d,s));     \
        AUX_IFELSE(base, b = base, AUX_READER_BASE(b,d,s));     \
                                                                \
        dest = 0;                                               \
                                                                \
        AUX_READER_UFPT(dest,data,size,base);                   \
        AUX_READER_##EX(dest,data,size,base);                   \
                                                                \
        dest *= i;                                              \
    }                                                           \
                                                                \
    dest;                                                       \
                                                                \
    })
#endif

#define aux_str2ui(dest,data,size) AUX_STR2SI(dest,data,size,0,1,IDLE)
#define aux_str2si(dest,data,size) AUX_STR2SI(dest,data,size,0,0,IDLE)
#if 0
#define aux_str2uf(dest,data,size) AUX_STR2SF(dest,data,size,0,1,IDLE)
#define aux_str2sf(dest,data,size) AUX_STR2SF(dest,data,size,0,0,IDLE)
#endif

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* __AUX_STRING_H__ */
