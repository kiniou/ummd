/*
 *  ummd ( Micro MultiMedia Daemon )
 *
 *  Copyright (C) 2010 Nicolas Thill <nicolas.thill@gmail.com>
 */

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "core/ports.h"

#include "util/log.h"
#include "util/mem.h"
#include "util/url.h"

typedef struct my_control_data_s my_control_data_t;

struct my_control_data_s {
	my_cport_t _inherited;
	char *path;
	int sock;
};

#define MY_CONTROL_DATA(p) ((my_control_data_t *)(p))
#define MY_CONTROL_DATA_SIZE (sizeof(my_control_data_t))

static my_port_t *my_control_sock_create(my_port_conf_t *conf)
{
	my_port_t *control;
	char *url;
	char url_prot[5];
	char url_path[255];

	control = my_port_priv_create(conf, MY_CONTROL_DATA_SIZE);
	if (!control) {
		goto _MY_ERR_alloc;
	}

	url = my_prop_lookup(conf->properties, "url");
	my_url_split(
		url_prot, sizeof(url_prot),
		NULL, 0, /* auth */
		NULL, 0, /* hostname */
		NULL, /* port */
		url_path, sizeof(url_path),
		url
	);
	if (strlen(url_prot) > 0) {
		if ((strcmp(url_prot, "file") != 0) && (strcmp(url_prot, "sock") != 0)) {
			my_log(MY_LOG_ERROR, "core/control: unknown url protocol '%s' in '%s'", url_prot, url);
			goto _MY_ERR_parse_url;
		}
	}
	if (strlen(url_path) == 0) {
		my_log(MY_LOG_ERROR, "core/control: missing path component in '%s'", url);
		goto _MY_ERR_parse_url;
	}

	MY_CONTROL_DATA(control)->path = strdup(url_path);

	return control;

	free(MY_CONTROL_DATA(control)->path);
_MY_ERR_parse_url:
	my_port_priv_destroy(control);
_MY_ERR_alloc:
	return NULL;
}

static void my_control_sock_destroy(my_port_t *control)
{
	free(MY_CONTROL_DATA(control)->path);
	my_port_priv_destroy(control);
}

static int my_control_sock_open(my_port_t *control)
{
	struct sockaddr_un sa;

	MY_DEBUG("core/control: creating unix socket '%s'", MY_CONTROL_DATA(control)->path);
	MY_CONTROL_DATA(control)->sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (MY_CONTROL_DATA(control)->sock == -1) {
		my_log(MY_LOG_ERROR, "core/control: error creating socket '%s' (%s)", MY_CONTROL_DATA(control)->path, strerror(errno));
		goto _MY_ERR_create_sock;
	}

	my_mem_zero(&sa, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, MY_CONTROL_DATA(control)->path, sizeof(sa.sun_path));
	
	MY_DEBUG("core/control: binding unix socket '%s'", MY_CONTROL_DATA(control)->path);
	if (bind(MY_CONTROL_DATA(control)->sock, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
		my_log(MY_LOG_ERROR, "core/control: error binding unix socket '%s' (%s)", MY_CONTROL_DATA(control)->path, strerror(errno));
		goto _MY_ERR_bind_sock;
	}

	return 0;

_MY_ERR_bind_sock:
	close(MY_CONTROL_DATA(control)->sock);
_MY_ERR_create_sock:
	return -1;
}

static int my_control_sock_close(my_port_t *control)
{
	MY_DEBUG("core/control: closing unix socket '%s'", MY_CONTROL_DATA(control)->path);
	if (close(MY_CONTROL_DATA(control)->sock) == -1) {
		my_log(MY_LOG_ERROR, "core/control: error closing unix socket '%s' (%s)", MY_CONTROL_DATA(control)->path, strerror(errno));
	}

	MY_DEBUG("core/control: removing unix socket '%s'", MY_CONTROL_DATA(control)->path);
	if (unlink(MY_CONTROL_DATA(control)->path) == -1) {
		my_log(MY_LOG_ERROR, "core/control: error removing unix socket '%s' (%s)", MY_CONTROL_DATA(control)->path, strerror(errno));
	}

	return 0;
}

my_port_impl_t my_control_sock = {
	.name = "sock",
	.desc = "Unix socket control interface",
	.create = my_control_sock_create,
	.destroy = my_control_sock_destroy,
	.open = my_control_sock_open,
	.close = my_control_sock_close,
};
