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

#ifndef __MY_TARGETS_PRIV_H
#define __MY_TARGETS_PRIV_H

#include "core/targets.h"

typedef struct my_target_impl_s my_target_impl_t;
typedef struct my_target_priv_s my_target_priv_t;

typedef my_target_t *(*my_target_create_fn_t)(my_target_conf_t *conf);
typedef void (*my_target_destroy_fn_t)(my_target_t *target);

typedef int (*my_target_open_fn_t)(my_target_t *target);
typedef int (*my_target_close_fn_t)(my_target_t *target);

struct my_target_impl_s {
	char *name;
	char *desc;
	my_target_create_fn_t create;
	my_target_destroy_fn_t destroy;
	my_target_open_fn_t open;
	my_target_close_fn_t close;
};

struct my_target_priv_s {
	my_target_t _inherited;
	my_target_impl_t *impl;
};

#define MY_TARGET_IMPL(p) ((my_target_impl_t *)(p))
#define MY_TARGET_PRIV(p) ((my_target_priv_t *)(p))

#define MY_TARGET_GET_IMPL(p) (MY_TARGET_PRIV(p)->impl)

#endif /* __MY_TARGETS_PRIV_H */
