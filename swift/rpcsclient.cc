#include "config.h"

#include "rpcsclient.h"

#include <malloc/_malloc.h>
#include <iostream>

RPCSClient::RPCSClient() : _socket(0), _rpcsfactory(0), _rpcs(0) {
    _socket = new ppsocket();
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

rfsv::errs RPCSClient::getMachineType(rpcs::machs &machineType) {
    assert(_rpcs);
    Enum<rfsv::errs> res;
    Enum<rpcs::machs> type;
    res = _rpcs->getMachineType(type);
    if (res != rfsv::E_PSI_GEN_NONE) {
        return res;
    }
    machineType = type.value;
    return rfsv::E_PSI_GEN_NONE;
}

rfsv::errs RPCSClient::getMachineInfo(rpcs::machineInfo &machineInfo) {
    assert(_rpcs);
    return _rpcs->getMachineInfo(machineInfo);
}

rfsv::errs RPCSClient::getOwnerInfo(bufferArray &owner) {
    assert(_rpcs);
    return _rpcs->getOwnerInfo(owner);
}

rfsv::errs RPCSClient::execProgram(const char *program, const char *args) {
    assert(_rpcs);
    return _rpcs->execProgram(program, args);
}

rfsv::errs RPCSClient::stopPrograms() {
    assert(_rpcs);
    Enum<rfsv::errs> res;
    processList tmp;
    if ((res = _rpcs->queryPrograms(tmp)) != rfsv::E_PSI_GEN_NONE) {
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
        if (res != rfsv::E_PSI_GEN_NONE) {
            return res.value;
        }
    }
    return rfsv::errs::E_PSI_GEN_NONE;
}
