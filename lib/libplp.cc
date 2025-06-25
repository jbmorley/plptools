#include "config.h"

#include "libplp.h"
#include <malloc/_malloc.h>
#include <iostream>

using namespace std;

RFSVClient::RFSVClient() : _socket(0), _rfsvfactory(0), _rfsv(0) {
    _socket = new ppsocket();
}

RFSVClient::~RFSVClient() {
    if (_rfsv) {
        delete _rfsv;
    }
    if (_rfsvfactory) {
        delete _rfsvfactory;
    }
    delete _socket;
}

bool RFSVClient::connect(const char * const Peer, int PeerPort) {
    if (_rfsv) {
        return true;
    }
    if (!_socket->connect(Peer, PeerPort)) {
        return false;
    }
    _rfsvfactory = new rfsvfactory(_socket);
    _rfsv = _rfsvfactory->create(true);
    if (!_rfsv) {
        return false;
    }
    return true;
}

rfsv::errs RFSVClient::dir(const char * const name, PlpDir &ret) {
    assert(_rfsv);
    return _rfsv->dir(name, ret);
}

rfsv::errs RFSVClient::devlist(uint32_t &devbits) {
    assert(_rfsv);
    return _rfsv->devlist(devbits);
}

rfsv::errs RFSVClient::devinfo(const char drive, PlpDrive &dinfo) {
    assert(_rfsv);
    return _rfsv->devinfo(drive, dinfo);
}

rfsv::errs RFSVClient::pathtest(const char * const name, bool &exists) {
    assert(_rfsv);
    return _rfsv->pathtest(name, exists);
}

rfsv::errs RFSVClient::mkdir(const char * const name) {
    assert(_rfsv);
    return _rfsv->mkdir(name);
}

rfsv::errs RFSVClient::rmdir(const char * const name) {
    assert(_rfsv);
    return _rfsv->rmdir(name);
}

rfsv::errs RFSVClient::rename(const char * const oldname, const char * newname) {
    assert(_rfsv);
    return _rfsv->rename(oldname, newname);
}

rfsv::errs RFSVClient::remove(const char * const name) {
    assert(_rfsv);
    return _rfsv->remove(name);
}

rfsv::errs RFSVClient::copyFromPsion(const char *from, const char *to, void *ptr, cpCallback_t cb) {
    assert(_rfsv);
    return _rfsv->copyFromPsion(from, to, ptr, cb);
}

rfsv::errs RFSVClient::copyToPsion(const char *from, const char *to, void *ptr, cpCallback_t cb) {
    assert(_rfsv);
    return _rfsv->copyToPsion(from, to, ptr, cb);
}

rfsv::errs RFSVClient::fgeteattr(const char * const name, PlpDirent &e) {
    assert(_rfsv);
    return _rfsv->fgeteattr(name, e);
}

const char *plpdirent_get_name(PlpDirent *dirent) {
    return dirent->getName();
}

const char *string_cstr(std::string string) {
    return string.c_str();
}

const char *psiprocess_get_name(PsiProcess *process) {
    return process->getName();
}

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
        value.append(to_string(i->getPID()));
        printf("Stopping '%s'...\n", value.c_str());
        res = _rpcs->stopProgram(value.c_str());
        if (res != rfsv::E_PSI_GEN_NONE) {
            return res.value;
        }
    }
    return rfsv::errs::E_PSI_GEN_NONE;
}
