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
    return _rfsv->dir(name, ret).value;
}

rfsv::errs RFSVClient::devlist(uint32_t &devbits) {
    assert(_rfsv);
    return _rfsv->devlist(devbits).value;
}

rfsv::errs RFSVClient::devinfo(const char drive, PlpDrive &dinfo) {
    assert(_rfsv);
    return _rfsv->devinfo(drive, dinfo).value;
}

rfsv::errs RFSVClient::mkdir(const char * const name) {
    assert(_rfsv);
    return _rfsv->mkdir(name).value;
}

rfsv::errs RFSVClient::rmdir(const char * const name) {
    assert(_rfsv);
    return _rfsv->rmdir(name).value;
}

rfsv::errs RFSVClient::remove(const char * const name) {
    assert(_rfsv);
    return _rfsv->remove(name);
}

rfsv::errs RFSVClient::copyFromPsion(const char *from, const char *to, void *ptr, cpCallback_t cb) {
    assert(_rfsv);
    return _rfsv->copyFromPsion(from, to, ptr, cb);
}

const char *plpdirent_get_name(PlpDirent *dirent) {
    return dirent->getName();
}

const char *string_cstr(std::string string) {
    return string.c_str();
}
