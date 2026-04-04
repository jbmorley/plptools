#include "config.h"

#include "rpcsclient.h"

#include <malloc/_malloc.h>
#include <iostream>

RPCSClient::RPCSClient() : _socket(0), _rpcsfactory(0), _rpcs(0) {
    _socket = new TCPSocket();
}

RPCSClient::~RPCSClient() {
    if (_rpcs) {
        delete _rpcs;
    }
    if (_rpcsfactory) {
        delete _rpcsfactory;
    }
    delete _socket;
}

bool RPCSClient::connect(const char * const Peer, int PeerPort) {
    if (_rpcs) {
        return true;
    }
    if (!_socket->connect(Peer, PeerPort)) {
        return false;
    }
    _rpcsfactory = new rpcsfactory(_socket);
    _rpcs = _rpcsfactory->create(true);
    if (!_rpcs) {
        return false;
    }
    return true;
}

RFSV::errs RPCSClient::getMachineType(RPCS::machs &machineType) {
    assert(_rpcs);
    Enum<RFSV::errs> res;
    Enum<RPCS::machs> type;
    res = _rpcs->getMachineType(type);
    if (res != RFSV::E_PSI_GEN_NONE) {
        return res;
    }
    machineType = type.value;
    return RFSV::E_PSI_GEN_NONE;
}

RFSV::errs RPCSClient::getMachineInfo(RPCS::machineInfo &machineInfo) {
    assert(_rpcs);
    return _rpcs->getMachineInfo(machineInfo);
}

RFSV::errs RPCSClient::getOwnerInfo(BufferArray &owner) {
    assert(_rpcs);
    return _rpcs->getOwnerInfo(owner);
}

RFSV::errs RPCSClient::execProgram(const char *program, const char *args) {
    assert(_rpcs);
    return _rpcs->execProgram(program, args);
}

RFSV::errs RPCSClient::stopPrograms() {
    assert(_rpcs);
    Enum<RFSV::errs> res;
    processList tmp;
    if ((res = _rpcs->queryPrograms(tmp)) != RFSV::E_PSI_GEN_NONE) {
        return res.value;
    }
    for (processList::iterator i = tmp.begin(); i != tmp.end(); i++) {
        // TODO: For some reason the getProcId isn't behaving correctly here so we assemble our own.
        std::string value = "";
        value.append(i->getName());
        value.append(".$");
        if (i->getPID() < 10) {
            value.append("0");
        }
        value.append(std::to_string(i->getPID()));
        printf("Stopping '%s'...\n", value.c_str());
        res = _rpcs->stopProgram(value.c_str());
        if (res != RFSV::E_PSI_GEN_NONE) {
            return res.value;
        }
    }
    return RFSV::errs::E_PSI_GEN_NONE;
}
