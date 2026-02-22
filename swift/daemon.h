#ifndef _daemon_h_
#define _daemon_h_

#include <ncp_session.h>

extern void *ncp_session_init(int sockNum,
                              int baudRate,
                              const char *host,
                              const char* serialDevice,
                              bool autoexit,
                              unsigned short nverbose,
                              NCPStatusCallback statusCallback,
                              void *callbackContext);
extern void ncp_session_start(void *session);
extern void ncp_session_cancel(void *session);
extern void ncp_session_wait(void *session);

#endif
