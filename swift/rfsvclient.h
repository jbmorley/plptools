#ifndef _libplp_h_
#define _libplp_h_

#include <plpdirent.h>
#include <tcpsocket.h>
#include <psiprocess.h>
#include <rfsv.h>
#include <rfsvfactory.h>

class RFSVClient {
public:
    RFSVClient();
    ~RFSVClient();

    bool connect(const char * const Peer, int PeerPort);
    RFSV::errs dir(const char * const name, PlpDir &ret);
    RFSV::errs devlist(uint32_t &devbits);
    RFSV::errs devinfo(const char drive, Drive &dinfo);
    RFSV::errs pathtest(const char * const name);
    RFSV::errs mkdir(const char * const name);
    RFSV::errs rmdir(const char * const name);
    RFSV::errs remove(const char * const name);
    RFSV::errs rename(const char * const oldname, const char * newname);
    RFSV::errs copyFromPsion(const char *from, const char *to, void *ptr, cpCallback_t cb);
    RFSV::errs copyToPsion(const char *from, const char *to, void *ptr, cpCallback_t cb);
    RFSV::errs fgeteattr(const char * const name, PlpDirent &e);
    RFSV::errs fgetattr(const char * const name, uint32_t &attr);
private:
    TCPSocket *_socket;
    RFSVFactory *_rfsvfactory;
    RFSV *_rfsv;
};

extern const char *plpdirent_get_name(PlpDirent *dirent);
extern const char *string_cstr(std::string string);
extern const char *psiprocess_get_name(PsiProcess *process);

#endif

