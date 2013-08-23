#ifndef __AUX_RANDOM_H__
#define __AUX_RANDOM_H__

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

int aux_random_init();
#define aux_random() random()

#ifdef __cplusplus
}
#endif

#endif /* __AUX_RANDOM_H__ */
