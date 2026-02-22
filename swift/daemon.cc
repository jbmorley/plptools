#include "config.h"
#include "daemon.h"

void *ncp_session_init(int sockNum,
                       int baudRate,
                       const char *host,
                       const char* serialDevice,
                       bool autoexit,
                       unsigned short nverbose,
                       NCPStatusCallback statusCallback,
                       void *callbackContext) {
    return new NCPSession(sockNum, baudRate, host, serialDevice, autoexit, nverbose, statusCallback, callbackContext);
}

void ncp_session_start(void *session) {
    static_cast<NCPSession *>(session)->start();
}

void ncp_session_cancel(void *session) {
    static_cast<NCPSession *>(session)->cancel();
}

void ncp_session_wait(void *session) {
    static_cast<NCPSession *>(session)->wait();
}
