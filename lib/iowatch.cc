//
//  PLP - An implementation of the PSION link protocol
//
//  Copyright (C) 1999  Philip Proudman
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  e-mail philip.proudman@btinternet.com

#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stream.h>
#include <memory.h>

#include "bool.h"
#include "iowatch.h"

IOWatch::IOWatch() {
  num = 0;
  io = new int [MAX_IO];
}

IOWatch::~IOWatch() {
  delete [] io;
}

void IOWatch::addIO(int a) {
  int pos;
  for (pos = 0; pos < num && a < io[pos]; pos++);
  for (int i = num; i > pos; i--) io[i] = io[i-1];
  io[pos] = a;
  num++;
}

void IOWatch::remIO(int a) {
  int pos;
  for (pos = 0; pos < num && a != io[pos]; pos++);
  if (pos != num) {
    num--;
    for (int i = pos; i <num; i++) io[i] = io[i+1];
  }
}

bool IOWatch::watch(long secs, long usecs) {
  if (num > 0) {
    fd_set iop;
    FD_ZERO(&iop);
    for (int i=0; i<num; i++) {
      FD_SET(io[i], &iop);
    }
    struct timeval t;
    t.tv_usec = usecs;
    t.tv_sec = secs;
    return select(io[0]+1, &iop, NULL, NULL, &t);
  }
  else {
    sleep(secs);
    usleep(usecs);
  }
  return false;
}

