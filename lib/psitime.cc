/*
 * This file is part of plptools.
 *
 *  Copyright (C) 2000-2002 Fritz Elfert <felfert@to.com>
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
#include "config.h"

#include "psitime.h"

#include <stdint.h>
#include <stdlib.h>

using namespace std;

PsiTime::PsiTime(void) {
    ptzValid = false;
    tryPsiZone();
    setUnixNow();
}

PsiTime::PsiTime(time_t time) {
    ptzValid = false;
    gettimeofday(&utv, &utz);
    setUnixTime(time);
}

PsiTime::PsiTime(psi_timeval *_ptv, psi_timezone *_ptz) {
    if (_ptv != 0L)
        ptv = *_ptv;
    if (_ptz != 0L) {
        ptz = *_ptz;
        ptzValid = true;
    } else {
        ptzValid = false;
        tryPsiZone();
    }
    /* get our own timezone */
    gettimeofday(&utv, &utz);
    psi2unix();
}

PsiTime::PsiTime(const uint32_t _ptvHi, const uint32_t _ptvLo) {
    ptv.tv_high = _ptvHi;
    ptv.tv_low = _ptvLo;
    ptzValid = false;
    tryPsiZone();
    /* get our own timezone */
    gettimeofday(&utv, &utz);
    psi2unix();
}

PsiTime::PsiTime(struct timeval *_utv, struct timezone *_utz) {
    if (_utv != 0L)
        utv = *_utv;
    if (_utz != 0L)
        utz = *_utz;
    tryPsiZone();
    unix2psi();
}

PsiTime::PsiTime(const PsiTime &t) {
    utv = t.utv;
    utz = t.utz;
    ptv = t.ptv;
    ptz = t.ptz;
    ptzValid = t.ptzValid;
    tryPsiZone();
}

PsiTime::~PsiTime() {
    tryPsiZone();
}

void PsiTime::setUnixTime(struct timeval *_utv) {
    if (_utv != 0L)
        utv = *_utv;
    unix2psi();
}

void PsiTime::setUnixTime(time_t time) {
    utv.tv_sec = time;
    utv.tv_usec = 0;
    unix2psi();
}

void PsiTime::setUnixNow(void) {
    gettimeofday(&utv, &utz);
    unix2psi();
}


void PsiTime::setPsiTime(psi_timeval *_ptv) {
    if (_ptv != 0L)
        ptv = *_ptv;
    psi2unix();
}

void PsiTime::setPsiTime(const uint32_t _ptvHi, const uint32_t _ptvLo) {
    ptv.tv_high = _ptvHi;
    ptv.tv_low = _ptvLo;
    psi2unix();
}

void PsiTime::setPsiZone(psi_timezone *_ptz) {
    if (_ptz != 0L) {
        ptz = *_ptz;
        ptzValid = true;
    }
    psi2unix();
}

struct timeval &PsiTime::getTimeval(void) {
    return utv;
}

time_t PsiTime::getTime(void) {
    return utv.tv_sec;
}

psi_timeval &PsiTime::getPsiTimeval(void) {
    return ptv;
}

uint32_t PsiTime::getPsiTimeLo(void) {
    return ptv.tv_low;
}

uint32_t PsiTime::getPsiTimeHi(void) {
    return ptv.tv_high;
}

PsiTime &PsiTime::operator=(const PsiTime &t) {
    utv = t.utv;
    utz = t.utz;
    ptv = t.ptv;
    ptz = t.ptz;
    ptzValid = t.ptzValid;
    tryPsiZone();
    return *this;
}

bool PsiTime::operator==(const PsiTime &t) {
    psi2unix();
    return ((utv.tv_sec == t.utv.tv_sec) &&
	    (utv.tv_usec == t.utv.tv_usec));
}

bool PsiTime::operator<(const PsiTime &t) {
    psi2unix();
    if (utv.tv_sec == t.utv.tv_sec)
        return (utv.tv_usec < t.utv.tv_usec);
    else
        return (utv.tv_sec < t.utv.tv_sec);
}

bool PsiTime::operator>(const PsiTime &t) {
    psi2unix();
    if (utv.tv_sec == t.utv.tv_sec)
        return (utv.tv_usec > t.utv.tv_usec);
    else
        return (utv.tv_sec > t.utv.tv_sec);
}

ostream &operator<<(ostream &s, const PsiTime &t) {
    const char *fmt = "%c";
    char buf[100];
    // Local time zone and DST offsets are added by localtime()
    strftime(buf, sizeof(buf), fmt, localtime(&t.utv.tv_sec));
    s << buf;
    return s;
}

/**
 * The difference between EPOC epoch (01.01.0000 00:00:00)
 * and Unix epoch (01.01.1970 00:00:00) in microseconds.
 * This constant is part of EPOC's standard library (Estlib.dll).
 */
#define EPOCH_DIFF 0x00dcddb30f2f8000ULL

/* evalOffset()
 * Returns the difference between a local time from the Psion and the UTC
 * time of the local machine, in microseconds.
 */
static long long evalOffset(psi_timezone ptz, time_t ptime, bool valid) {
    int64_t offset = 0;

    // Get the difference between Psion's and local machine's current offsets.
    if (valid) { // EPOC32 only, once getMachineInfo() has been called
        offset = ptz.utc_offset; // timezone offset (without DST offset)
        offset += ptz.dst_zones & PsiTime::PSI_TZ_HOME ? 3600 : 0; // DST
        time_t now = time(NULL);
        offset -= localtime(&now)->tm_gmtoff; // timezone + DST offsets
    } else { // Fallback. Use PSI_TZ environment variable (e.g. "-3600").
        const char *offstr = getenv("PSI_TZ");
        if (offstr != 0) {
            char *err = 0;
            offset = strtol(offstr, &err, 10);
            if (*err != '\0') {
                offset = 0;
            }
        }
    }
    // Fallback. Assume that both Psion and local machine have the same
    // timezone and DST settings. Offset is still be 0 at this point.

    // Substract local timezone and DST offsets to match local machine's UTC
    // time, they get added later by localtime() when timestamp is displayed.
    offset += localtime(&ptime)->tm_gmtoff;
    offset *= 1000000;

    return offset;
}

/* setSiboTime()
 * Store SIBO 'local' time as Unix UTC time.
 */
void PsiTime::setSiboTime(uint32_t stime) {
    long long micro = evalOffset(ptz, stime, false);

    utv.tv_sec = stime - micro/1000000;
    utv.tv_usec = 0;
}

/* getSiboTime()
 * Get stored Unix UTC time as SIBO 'local' time.
 */
uint32_t PsiTime::getSiboTime(void) {
    long long micro = evalOffset(ptz, utv.tv_sec, false);

    return utv.tv_sec + micro/1000000;
}

/* psi2unix()
 * Convert EPOC local time to Unix UTC time.
 */
void PsiTime::psi2unix(void) { // epoc2unix

    uint64_t micro = ptv.tv_high;
    micro = (micro << 32) | ptv.tv_low;

    /* Substract EPOC's idea of UTC offset */
    micro -= EPOCH_DIFF;
    micro -= evalOffset(ptz, micro / 1000000, ptzValid);

    utv.tv_sec = micro / 1000000;
    utv.tv_usec = micro % 1000000;
}

/* unix2psi()
 * Convert Unix UTC time to EPOC local time.
 */
void PsiTime::unix2psi(void) { // unix2epoc

    uint64_t micro = (uint64_t)utv.tv_sec * 1000000ULL + utv.tv_usec;

    /* Add EPOC's idea of UTC offset */
    micro += evalOffset(ptz, utv.tv_sec, ptzValid);
    micro += EPOCH_DIFF;

    ptv.tv_low = micro & 0x0ffffffff;
    ptv.tv_high = (micro >> 32) & 0x0ffffffff;
}

void PsiTime::tryPsiZone() {
    if (ptzValid)
        return;
    if (PsiZone::getInstance().getZone(ptz))
        ptzValid = true;
}

PsiZone *PsiZone::_instance = 0L;

PsiZone &PsiZone::getInstance() {
    if (_instance == 0L)
        _instance = new PsiZone();
    return *_instance;
}

PsiZone::PsiZone() {
    _ptzValid = false;
}

void PsiZone::setZone(psi_timezone &ptz) {
    _ptz = ptz;
    _ptzValid = true;
}

bool PsiZone::getZone(psi_timezone &ptz) {
    if (_ptzValid)
        ptz = _ptz;
    return _ptzValid;
}
