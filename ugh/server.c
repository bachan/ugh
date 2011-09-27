#include "ugh.h"

static
void ugh_wcb_accept(EV_P_ ev_io *w, int tev)
{
	int sd, rc;
	struct sockaddr_in addr;
	socklen_t addrlen;

	ugh_server_t *s = aux_memberof(ugh_server_t, wev_accept, w);

	if (EV_ERROR & tev)
	{
		/* TODO: is errno defined here? */
		log_alert("accept: tev:%d, w->fd:%d", tev, w->fd);

		ugh_server_enough(s);
		return;
	}

	addrlen = sizeof(addr);

	if (0 > (sd = accept(w->fd, (struct sockaddr *) &addr, &addrlen)))
	{
		log_error("accept: w->fd:%d (%d: %s)", w->fd, errno, aux_strerror(errno));
		return;
	}

	if (0 > (rc = aux_set_nonblk(sd, 1)))
	{
		log_error("set_nonblk: sd:%d (%d: %s)", sd, errno, aux_strerror(errno));
		return;
	}

	ugh_client_add(s, sd, &addr);
}

int ugh_server_listen(ugh_server_t *s, ugh_config_t *cfg, ugh_resolver_t *resolver)
{
	int sd, rc;

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (0 > sd) return -1;

	rc = aux_set_nonblk(sd, 1);
	if (0 > rc) return -1;

	rc = aux_set_sckopt(sd, SOL_SOCKET, SO_REUSEADDR, 1);
	if (0 > rc) return -1;

	aux_clrptr(s);

	s->cfg = cfg;
	s->resolver = resolver;

	s->addr.sin_family = AF_INET;
	s->addr.sin_addr.s_addr = inet_addr(cfg->listen_host);
	s->addr.sin_port = htons(cfg->listen_port);

	rc = bind(sd, (struct sockaddr *) &s->addr, sizeof(s->addr));
	if (0 > rc) return -1;

	/*
	 * TODO (optional)
	 * Don't forget about inherited sockets. As an option at least.
	 * Plus LINGERING_CLOSE and TCP_NOPUSH/TCP_NODELAY double strike.
	 * Finally, don't forget about SO_RCVBUF, SO_SNDBUF, TCP_DEFER_ACCEPT.
	 */

	rc = listen(sd, UGH_CONFIG_LISTEN_BACKLOG);
	if (0 > rc) return -1;

	ev_io_init(&s->wev_accept, ugh_wcb_accept, sd, EV_READ);
	ev_io_start(loop, &s->wev_accept);

	return 0;
}

int ugh_server_enough(ugh_server_t *s)
{
	ev_io_stop(loop, &s->wev_accept);

	return close(s->wev_accept.fd);
}

