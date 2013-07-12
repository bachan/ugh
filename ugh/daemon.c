#include "ugh.h"

struct ev_loop *loop;

#if 1
ugh_module_handle_fp ugh_module_handles [UGH_MODULE_HANDLES_MAX];
void * ugh_module_configs [UGH_MODULE_HANDLES_MAX];
size_t ugh_module_handles_size = 0;

int ugh_module_handle(ugh_client_t *c)
{
	size_t i;
	int status = UGH_HTTP_OK;

	for (i = 0; i < ugh_module_handles_size; ++i)
	{
		if (NULL == ugh_module_handles[i]) continue;

		int tmp_status = ugh_module_handles[i](c, ugh_module_configs[i], &c->bufs[i]);

#if 1 /* XXX UGH_AGAIN means here, that module was not supposed to be called */
		if (tmp_status != UGH_AGAIN)
		{
			status = tmp_status;
		}
#endif
	}

	return status;
}
#endif

#if 1
coro_context ctx_main;
unsigned char is_main_coro = 1;
#endif

void ugh_wcb_silent(EV_P_ ev_timer *w, int tev)
{
	if (0 != aux_terminate)
	{
		ev_timer_stop(EV_A_ w);
		ev_break(EV_A_ EVBREAK_ALL);
		return;
	}

	if (0 != aux_rotatelog)
	{
		ugh_daemon_t *d;

		aux_rotatelog = 0;
		d = aux_memberof(ugh_daemon_t, wev_silent, w);

		if (-1 == log_rotate(d->cfg.log_error))
		{
			log_warn("log_rotate(%s) (%d: %s)", d->cfg.log_error,
				errno, aux_strerror(errno));
		}
	}
}

int ugh_daemon_exec(const char *cfg_filename, unsigned daemon)
{
	int rc;
	ugh_daemon_t d;

	aux_clrptr(&d);

	loop = ev_default_loop(0);

	/* TODO make it possible to set default values for each module config and
	 * global config via ugh_make_command macro (by setting all this at
	 * module_handle_add stage)
	 */
	rc = ugh_config_init(&d.cfg);
	if (0 > rc) return -1;

	rc = ugh_config_load(&d.cfg, cfg_filename);
	if (0 > rc) return -1;

	if (daemon)
	{
		rc = log_create(d.cfg.log_error, log_levels(d.cfg.log_level));
		if (0 > rc) return -1;
	}
	else
	{
		log_level = log_levels(d.cfg.log_level);
	}

	rc = ugh_resolver_init(&d.resolver, &d.cfg);
	if (0 > rc) return -1;

	size_t i;

	for (i = 0; i < ugh_modules_size; ++i)
	{
		if (ugh_modules[i]->init)
		{
			rc = ugh_modules[i]->init(&d.cfg);
			if (0 > rc) return -1;
		}
	}

	rc = ugh_server_listen(&d.srv, &d.cfg, &d.resolver);
	if (0 > rc) return -1;

	ev_timer_init(&d.wev_silent, ugh_wcb_silent, 0, UGH_CONFIG_SILENT_TIMEOUT);
	ev_timer_again(loop, &d.wev_silent);

	ev_run(loop, 0);

	ugh_server_enough(&d.srv);

	for (i = 0; i < ugh_modules_size; ++i)
	{
		if (ugh_modules[i]->free)
		{
			rc = ugh_modules[i]->free();
			if (0 > rc) return -1;
		}
	}

	ugh_resolver_free(&d.resolver);

	ugh_config_free(&d.cfg);

	return 0;
}

int main(int argc, char **argv)
{
	int rc;
	aux_getopt_t opt;

	rc = aux_getopt_parse(argc, argv, &opt);

	if (0 > rc || NULL == opt.config)
	{
		aux_getopt_usage(argc, argv);
		return -1;
	}

	rc = aux_daemon_load(&opt);
	if (0 > rc) return -1;

	rc = ugh_daemon_exec(opt.config, opt.daemon);
	if (0 > rc) return -1;

	rc = aux_daemon_stop(&opt);
	if (0 > rc) return -1;

	return 0;
}

