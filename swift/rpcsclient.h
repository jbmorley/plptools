#ifndef _rpcsclient_h_
#define _rpcsclient_h_

#include <tcpsocket.h>
#include <rfsv.h>
#include <rpcsfactory.h>

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
    TCPSocket *_socket;
    rpcsfactory *_rpcsfactory;
    rpcs *_rpcs;
};

#endif
