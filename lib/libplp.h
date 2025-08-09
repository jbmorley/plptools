#ifndef _libplp_h_
#define _libplp_h_

#include <ppsocket.h>
#include <rfsv.h>
#include <rfsvfactory.h>
#include <rpcsfactory.h>
#include <plpdirent.h>

class RFSVClient {
public:
    RFSVClient();
    ~RFSVClient();

    bool connect(const char * const Peer, int PeerPort);
    rfsv::errs dir(const char * const name, PlpDir &ret);
    rfsv::errs devlist(uint32_t &devbits);
    rfsv::errs devinfo(const char drive, PlpDrive &dinfo);
    rfsv::errs pathtest(const char * const name);
    rfsv::errs mkdir(const char * const name);
    rfsv::errs rmdir(const char * const name);
    rfsv::errs remove(const char * const name);
    rfsv::errs rename(const char * const oldname, const char * newname);
    rfsv::errs copyFromPsion(const char *from, const char *to, void *ptr, cpCallback_t cb);
    rfsv::errs copyToPsion(const char *from, const char *to, void *ptr, cpCallback_t cb);
    rfsv::errs fgeteattr(const char * const name, PlpDirent &e);
    rfsv::errs fgetattr(const char * const name, uint32_t &attr);
private:
    ppsocket *_socket;
    rfsvfactory *_rfsvfactory;
    rfsv *_rfsv;
};

extern const char *plpdirent_get_name(PlpDirent *dirent);
extern const char *string_cstr(std::string string);
extern const char *psiprocess_get_name(PsiProcess *process);

class RPCSClient {
public:
    RPCSClient();
    ~RPCSClient();

    bool connect(const char * const Peer, int PeerPort);
    rfsv::errs getMachineType(rpcs::machs &machineType);
    rfsv::errs getMachineInfo(rpcs::machineInfo &machineInfo);
    rfsv::errs getOwnerInfo(bufferArray &owner);
    rfsv::errs execProgram(const char *program, const char *args);
    rfsv::errs stopPrograms();
private:
    ppsocket *_socket;
    rpcsfactory *_rpcsfactory;
    rpcs *_rpcs;
};

#endif

