/*
   //  PLP - An implementation of the PSION link protocol
   //
   //
   //  The code in this file was written by Rudolf Koenig
   //  (rfkoenig@immd4.informatik.uni-erlangen.de). (from his p3nfs code)
   //  The Copyright remains his
   //
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
 */


#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>		/* for usleep() */
#include <string.h>		/* for bzero() */
#include <termios.h>
#if defined(linux) || defined(_IBMR2) || defined(__NetBSD__)
#include <sys/ioctl.h>		/* for ioctl() */
#endif
#include <sys/errno.h>
#ifdef sun
#include <sys/ttold.h>		/* sun has TIOCEXCL there */
#endif
#if defined (__SVR4)
#include <stdlib.h>
#endif

#ifndef hpux
#define mflag int
#else				/* hpux */
#include <sys/termiox.h>
#include <sys/modem.h>
#endif


/*
   #if !defined(CRTSCTS) && defined(_IBMR2)
   #define CRTSCTS 0x80000000
   #endif
 */

#ifdef __sgi
#define CRTSCTS CNEW_RTSCTS
#endif

#ifndef O_NOCTTY
#define O_NOCTTY 0
#endif

int
init_serial(const char *dev, int speed, int debug)
{
	int fd, baud, clocal;
	int uid, euid;
	struct termios ti;
#ifdef hpux
	struct termiox tx;
#endif
	static struct baud {
		int speed, baud;
	} btable[] = {
		{ 9600, B9600 },
#ifdef B19200
		{ 19200, B19200 },
#else
#ifdef EXTA
		{ 19200, EXTA },
#endif
#endif
#ifdef B38400
		{ 38400, B38400 },
#else
#ifdef EXTB
		{ 38400, EXTB },
#endif
#endif
#ifdef B57600
		{ 57600, B57600 },
#endif
#ifdef B115200
		{ 115200, B115200 },
#endif
		{ 4800, B4800 },
		{ 2400, B2400 },
		{ 1200, B1200 },
		{ 300, B300 },
		{ 75, B75 },
		{ 50, B50 },
		{ 0, 0 }
	}, *bptr;

	if (speed) {
		for (bptr = btable; bptr->speed; bptr++)
			if (bptr->speed == speed)
				break;
		if (!bptr->baud) {
			fprintf(stderr, "Cannot match selected speed %d\n", speed);
			exit(1);
		}
		baud = bptr->baud;
	} else
		baud = 0;

	if (debug)
		printf("using %s...\n", dev);
	euid = geteuid();
	uid = getuid();

#ifdef hpux
#define seteuid(a) setresuid(-1, a, -1)
#endif

	clocal = CLOCAL;
	if (seteuid(uid)) {
		perror("seteuid");
		exit(1);
	}
	if ((fd = open(dev, O_RDWR | O_NDELAY | O_NOCTTY, 0)) < 0) {
		perror(dev);
		exit(1);
	}
	if (seteuid(euid)) {
		perror("seteuid back");
		exit(1);
	}
	if (debug)
		printf("open done\n");
#ifdef TIOCEXCL
	ioctl(fd, TIOCEXCL, (char *) 0);	/* additional open() calls shall fail */
#else
	fprintf(stderr, "WARNING: opened %s non-exclusive!\n", dev);
#endif

	memset(&ti, 0, sizeof(struct termios));
#if defined(hpux) || defined(_IBMR2)
	ti.c_cflag = CS8 | HUPCL | clocal | CREAD;
#endif
#if defined(sun) || defined(linux) || defined(__sgi) || defined(__NetBSD__)
	ti.c_cflag = CS8 | HUPCL | clocal | CRTSCTS | CREAD;
	ti.c_iflag = IGNBRK | IGNPAR;
	ti.c_cc[VMIN] = 1;
	ti.c_cc[VTIME] = 0;
#endif
	cfsetispeed(&ti, baud);
	cfsetospeed(&ti, baud);

	if (tcsetattr(fd, TCSADRAIN, &ti) < 0)
		perror("tcsetattr TCSADRAIN");

#ifdef hpux
	bzero(&tx, sizeof(struct termiox));
	tx.x_hflag = RTSXOFF | CTSXON;
	if (ioctl(fd, TCSETXW, &tx) < 0)
		perror("TCSETXW");
#endif

#if defined(_IBMR2)
	ioctl(fd, TXDELCD, "dtr");
	ioctl(fd, TXDELCD, "xon");
	ioctl(fd, TXADDCD, "rts");	/* That's how AIX does CRTSCTS */
#endif
	return fd;
}

void
ser_exit(int fd)
{
	struct termios ti;
	if (ioctl(fd, TCGETS, (caddr_t) & ti) < 0)
		perror("TCGETSW");
	ti.c_cflag &= ~CRTSCTS;
	if (tcsetattr(fd, TCSANOW, &ti) < 0)
		perror("TCSETSW");
	(void) close(fd);
}