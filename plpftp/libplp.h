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
    Enum<rfsv::errs> dir(const char * const name, PlpDir &ret);
    Enum<rfsv::errs> devlist(uint32_t &devbits);
private:
    ppsocket *_socket;
    rfsvfactory *_rfsvfactory;
    rfsv *_rfsv;
};

extern const char *plpdirent_get_name(PlpDirent *dirent);

#endif
