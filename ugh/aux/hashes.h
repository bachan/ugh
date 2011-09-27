#ifndef __AUX_HASHES_H__
#define __AUX_HASHES_H__

#include <stdint.h>
#include "string.h"

#define aux_hash(key,c) ((uintptr_t) (key) * 31 + ((uint8_t) (c)))

#ifdef __cplusplus
extern "C" {
#endif

static inline
uintptr_t aux_hash_key(const char *data, size_t size)
{
    uintptr_t key = 0;

    for (; size--; ++data)
    {
        key = aux_hash(key, *data);
    }

    return key;
}

static inline
uintptr_t aux_hash_key_nt(const char *data)
{
	uintptr_t key = 0;

	for (; *data; ++data)
	{
		key = aux_hash(key, *data);
	}

	return key;
}

static inline
uintptr_t aux_hash_key_lc(const char *data, size_t size)
{
    uintptr_t key = 0;

    for (; size--; ++data)
    {
        key = aux_hash(key, aux_tolower(*data));
    }

    return key;
}

static inline
uintptr_t aux_hash_key_lc_nt(const char *data)
{
	uintptr_t key = 0;

	for (; *data; ++data)
	{
		key = aux_hash(key, aux_tolower(*data));
	}

	return key;
}

static inline
uintptr_t aux_hash_key_lc_header(const char *data, size_t size)
{
	uintptr_t key = 0;

	for (; size--; ++data)
	{
		key = aux_hash(key, ('-' != *data) ? aux_tolower(*data) : '_');
	}

	return key;
}

static inline
uintptr_t aux_hash_key_lc_header_nt(const char *data)
{
	uintptr_t key = 0;

	for (; *data; ++data)
	{
		key = aux_hash(key, ('-' != *data) ? aux_tolower(*data) : '_');
	}

	return key;
}

static inline
uintptr_t aux_hash_strlow(char *dest, const char *data, size_t size)
{
    uintptr_t key = 0;

    for (; size--; ++dest, ++data)
    {
        *dest = aux_tolower(*data);
        key = aux_hash(key, *dest);
    }

    return key;
}

#ifdef __cplusplus
}
#endif

#endif /* __AUX_HASHES_H__ */
