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

#ifndef __MY_PORTS_H
#define __MY_PORTS_H

#include "autoconf.h"

#include "core.h"

#include "util/list.h"

/* generic port */

typedef struct my_port_s my_port_t;
typedef struct my_port_conf_s my_port_conf_t;
typedef struct my_port_impl_s my_port_impl_t;

struct my_port_s {
	my_core_t *core;
	my_port_conf_t *conf;
	my_port_impl_t *impl;
};

struct my_port_conf_s {
	int index;
	char *name;
	my_list_t *properties;
};

typedef my_port_t *(*my_port_create_fn_t)(my_port_conf_t *conf);
typedef void (*my_port_destroy_fn_t)(my_port_t *port);

typedef int (*my_port_open_fn_t)(my_port_t *port);
typedef int (*my_port_close_fn_t)(my_port_t *port);

typedef int (*my_port_get_fn_t)(my_port_t *port, void *buf, int len);
typedef int (*my_port_put_fn_t)(my_port_t *port, void *buf, int len);

struct my_port_impl_s {
	char *name;
	char *desc;
	my_port_create_fn_t create;
	my_port_destroy_fn_t destroy;
	my_port_open_fn_t open;
	my_port_close_fn_t close;
	my_port_get_fn_t get;
	my_port_put_fn_t put;
};

#define MY_PORT(p) ((my_port_t *)(p))
#define MY_PORT_CONF(p) ((my_port_conf_t *)(p))
#define MY_PORT_IMPL(p) ((my_port_impl_t *)(p))

#define MY_PORT_GET_CORE(p) (MY_PORT(p)->core)
#define MY_PORT_GET_CONF(p) (MY_PORT(p)->conf)
#define MY_PORT_GET_IMPL(p) (MY_PORT(p)->impl)

extern my_port_conf_t *my_port_conf_create(int port_index, char *port_name);
extern void my_port_conf_destroy(my_port_conf_t *port_conf);

extern my_port_impl_t *my_port_impl_lookup(my_list_t *list, char *name);
extern void my_port_impl_register(my_list_t *list, my_port_impl_t *impl);

extern my_port_t *my_port_priv_create(my_port_conf_t *conf, int size);
extern void my_port_priv_destroy(my_port_t *port);

extern my_port_t *my_port_create(my_port_conf_t *conf, my_port_impl_t *impl);
extern void my_port_destroy(my_port_t *port);

extern my_port_t *my_port_lookup_by_name(my_list_t *list, char *name);

extern int my_port_destroy_all(my_list_t *list);

extern int my_port_open_all(my_list_t *list);
extern int my_port_close_all(my_list_t *list);

#ifdef MY_DEBUGGING
extern void my_port_dump_all(void);
#endif


/* control port */

typedef struct my_cport_s my_cport_t;

struct my_cport_s {
	my_port_t _inherited;
};


/* data port */

typedef struct my_dport_s my_dport_t;

struct my_dport_s {
	my_port_t _inherited;
	my_port_t *peer;
};

#define MY_DPORT(p) ((my_dport_t *)(p))

extern void my_port_link(my_port_t *port, my_port_t *peer);

#endif /* __MY_PORTS_H */
