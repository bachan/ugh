#ifndef __AUX_RESOLVER_H__
#define __AUX_RESOLVER_H__

#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "memory.h"
#include "string.h"
#include "system.h"

#ifdef __cplusplus
extern "C" {
#endif

int create_name_query(char *p, char *name, size_t size);
int process_response(char *data, size_t size, in_addr_t *addrs, strp name);

const char *parse_resolv_conf(aux_pool_t *pool);

#ifdef __cplusplus
}
#endif

#endif /* __AUX_RESOLVER_H__ */
