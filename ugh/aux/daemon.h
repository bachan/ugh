#ifndef __AUX_DAEMON_H__
#define __AUX_DAEMON_H__

#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aux_getopt
	aux_getopt_t;

struct aux_getopt
{
	const char* config;
	const char* pid;

	unsigned daemon:1;
};

int aux_getopt_usage(int argc, char** argv);
int aux_getopt_parse(int argc, char** argv, aux_getopt_t* opt);

int aux_daemon_fork();
int aux_daemon_load(aux_getopt_t* opt);
int aux_daemon_stop(aux_getopt_t* opt);

extern volatile int aux_terminate;
extern volatile int aux_changecfg;
extern volatile int aux_rotatelog;
extern volatile int aux_changebin;

int aux_signal_init();
int aux_signal_mask(int how, unsigned mask);

#ifdef __cplusplus
}
#endif

#endif /* __AUX_DAEMON_H__ */
