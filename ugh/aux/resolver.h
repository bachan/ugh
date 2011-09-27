#ifndef __AUX_RESOLVER_H__
#define __AUX_RESOLVER_H__

#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

int create_name_query(char *p, char *name, size_t size);
int process_response(char *data, size_t size, in_addr_t *addrs, strp name);

#ifdef __cplusplus
}
#endif

#endif /* __AUX_RESOLVER_H__ */
