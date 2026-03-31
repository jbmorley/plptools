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
    RFSV::errs getMachineType(rpcs::machs &machineType);
    RFSV::errs getMachineInfo(rpcs::machineInfo &machineInfo);
    RFSV::errs getOwnerInfo(BufferArray &owner);
    RFSV::errs execProgram(const char *program, const char *args);
    RFSV::errs stopPrograms();
private:
    TCPSocket *_socket;
    rpcsfactory *_rpcsfactory;
    rpcs *_rpcs;
};

#endif
