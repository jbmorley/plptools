#ifndef _libplp_h_
#define _libplp_h_

#include <ppsocket.h>
#include <rfsv.h>
#include <rfsvfactory.h>
#include <plpdirent.h>

class RFSVClient {
public:
    RFSVClient();
    ~RFSVClient();

    bool connect(const char * const Peer, int PeerPort);
    rfsv::errs dir(const char * const name, PlpDir &ret);
    rfsv::errs devlist(uint32_t &devbits);
    rfsv::errs mkdir(const char * const name);
    rfsv::errs rmdir(const char * const name);
    rfsv::errs remove(const char * const name);
    rfsv::errs copyFromPsion(const char *from, const char *to, void *ptr, cpCallback_t cb);
private:
    ppsocket *_socket;
    rfsvfactory *_rfsvfactory;
    rfsv *_rfsv;
};

extern const char *plpdirent_get_name(PlpDirent *dirent);

#endif
