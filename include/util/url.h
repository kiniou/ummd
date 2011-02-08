/*
 *  ummd ( Micro MultiMedia Daemon )
 *
 *  Copyright (C) 2011 Nicolas Thill <nicolas.thill@gmail.com>
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

#ifndef __MY_UTIL_URL_H
#define __MY_UTIL_URL_H

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>

#if ( LIBAVUTIL_VERSION_MAJOR < 50 )
# define my_url_split  url_split
#else
# define my_url_split  ff_url_split
#endif

#endif /* __MY_UTIL_URL_H */