/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */
#ifndef _ncpd_h_
#define _ncpd_h_

typedef void (*statusCallback_t)(void *, int);

#ifdef __cplusplus
extern "C" {
#endif
int run(int argc, char **argv);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
int ncpd(int sockNum,
         int baudRate,
         const char *host,
         const char *serialDevice,
         unsigned short nverbose,
         statusCallback_t statusCallback,
         void *context);
#ifdef __cplusplus
}
#endif

#endif
