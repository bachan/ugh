#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include "logger.h"

volatile int log_level = LOG_info;

int log_levels(const char* level)
{
	switch (level[0])
	{
	case 'a': case 'A':
		switch (level[1])
		{
		case 'c': case 'C': return LOG_access;
		case 'l': case 'L': return LOG_alert;
		}
		break;
	case 'd': case 'D': return LOG_debug;
	case 'c': case 'C': return LOG_crit;
	case 'e': case 'E':
		switch (level[1])
		{
		case 'm': case 'M': return LOG_emerg;
		case 'r': case 'R': return LOG_error;
		}
		break;
	case 'i': case 'I': return LOG_info;
	case 'n': case 'N': return LOG_notice;
	case 'w': case 'W': return LOG_warn;
	}

	return LOG_info; /* default */
}

int log_create(const char* path, int level)
{
	if (0 > aux_mkpath((char *) path)) return -1;
	if (0 > aux_fdopen(STDERR_FILENO, path, O_CREAT|O_APPEND|O_WRONLY)) return -1;

	log_level = level;

	return STDERR_FILENO;
}

