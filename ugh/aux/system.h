#ifndef __AUX_SYSTEM_H__
#define __AUX_SYSTEM_H__

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

int aux_fdopen(int fd, const char *path, int flags);
int aux_fdmove(int fd, int nd);

#define aux_fnopen(p,f)   open((const char *) p, f, 0644)
#define aux_mkfold(p)    mkdir((const char *) p, 0755)
#define aux_rmfold(p)    rmdir((const char *) p)
#define aux_rmfile(p)   unlink((const char *) p)
#define aux_rlpath(p) realpath((const char *) p, alloca(PATH_MAX))

int aux_mkpath(char *path);
int aux_mkpidf(const char *path);

int aux_mmap(strp area, int prot_flags, int mmap_flags, const char *filename);

#define aux_umap(area) munmap((area)->data, (area)->size)
#define aux_mmap_file(a,p) aux_mmap((strp) aux_clrptr(a), PROT_READ, MAP_PRIVATE, p)

#ifdef __cplusplus
}
#endif

#endif /* __AUX_SYSTEM_H__ */
