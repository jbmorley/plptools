/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stream.h>
#include <stdlib.h>
#include <fstream.h>
#include <iomanip.h>
#include <time.h>
#include <string.h>

#include <rclip.h>
#include <bufferstore.h>
#include <ppsocket.h>
#include <bufferarray.h>
#include <Enum.h>

rclip::rclip(ppsocket * _skt)
{
    skt = _skt;
    reset();
}

rclip::~rclip()
{
    skt->closeSocket();
}

//
// public common API
//
void rclip::
reconnect(void)
{
    //skt->closeSocket();
    skt->reconnect();
    reset();
}

void rclip::
reset(void)
{
    bufferStore a;
    status = rfsv::E_PSI_FILE_DISC;
    a.addStringT(getConnectName());
    if (skt->sendBufferStore(a)) {
	if (skt->getBufferStore(a) == 1) {
	    if (!strcmp(a.getString(0), "Ok"))
		status = rfsv::E_PSI_GEN_NONE;
	}
    }
}

Enum<rfsv::errs> rclip::
getStatus(void)
{
    return status;
}

const char *rclip::
getConnectName(void)
{
    return "SYS$RCLIP";
}

//
// protected internals
//
bool rclip::
sendCommand(enum commands cc, bufferStore & data)
{
    if (status == rfsv::E_PSI_FILE_DISC) {
	reconnect();
	if (status == rfsv::E_PSI_FILE_DISC)
	    return false;
    }
    bool result;
    bufferStore a;
    a.addByte(cc);
    a.addBuff(data);
    result = skt->sendBufferStore(a);
    if (!result) {
	reconnect();
	result = skt->sendBufferStore(a);
	if (!result)
	    status = rfsv::E_PSI_FILE_DISC;
    }
    return result;
}

Enum<rfsv::errs> rclip::
putData(bufferStore &) {
    Enum<rfsv::errs> ret;

    bufferStore a;
    a.addWord(0);
    sendCommand(RCLIP_INIT, a);
    if ((ret = getResponse(a)) != rfsv::E_PSI_GEN_NONE)
	cerr << "RCLIP ERR:" << a << endl;
    else {
	if (a.getLen() != 3)
	    ret = rfsv::E_PSI_GEN_FAIL;
	if ((a.getByte(0) != 0) || (a.getWord(1) != 2))
	    ret = rfsv::E_PSI_GEN_FAIL;
    }
    return ret;
}

Enum<rfsv::errs> rclip::
getData(bufferStore &buf) {
    Enum<rfsv::errs> ret;
    bufferStore a;

    sendCommand(RCLIP_GET, buf);
    if ((ret = getResponse(buf)) != rfsv::E_PSI_GEN_NONE)
	cerr << "RCLIP ERR:" << a << endl;
    return ret;
}

Enum<rfsv::errs> rclip::
getResponse(bufferStore & data)
{
    Enum<rfsv::errs> ret = rfsv::E_PSI_GEN_NONE;
    if (skt->getBufferStore(data) == 1)
	return ret;
    else
	status = rfsv::E_PSI_FILE_DISC;
    return status;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
