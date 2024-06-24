

#include "config.h"

#include "libplp.h"
#include <malloc/_malloc.h>
#include <iostream>

using namespace std;

int plp_init(Context *context) {
    (*context).fileServerSocket = new ppsocket();
    return 0;
}

//int epocerr_to_errno(long epocerr) {
//  int unixerr = (int)epocerr;
//
//  if (epocerr < 0) {
//    switch (epocerr) {
//    case rfsv::E_PSI_GEN_NONE:
//      unixerr = 0;
//      break;
//    case rfsv::E_PSI_FILE_EXIST:
//      unixerr = -EEXIST;
//      break;
//    case rfsv::E_PSI_FILE_NXIST:
//    case rfsv::E_PSI_FILE_DIR:
//      unixerr = -ENOENT;
//      break;
//    case rfsv::E_PSI_FILE_WRITE:
//    case rfsv::E_PSI_FILE_READ:
//    case rfsv::E_PSI_FILE_EOF: // Can't err = EOF as it's not an error code
//    case rfsv::E_PSI_FILE_ALLOC: // FIXME: No idea what this is
//    case rfsv::E_PSI_FILE_UNKNOWN:
//    case rfsv::E_PSI_FILE_DIRFULL:
//      unixerr = -EPERM;
//      break;
//    case rfsv::E_PSI_FILE_FULL:
//      unixerr = -ENOSPC;
//      break;
//    case rfsv::E_PSI_FILE_NAME:
//    case rfsv::E_PSI_FILE_RECORD:
//    case rfsv::E_PSI_FILE_VOLUME:
//      unixerr = -EINVAL;
//      break;
//    case rfsv::E_PSI_FILE_ACCESS:
//    case rfsv::E_PSI_FILE_LOCKED:
//    case rfsv::E_PSI_FILE_RDONLY:
//    case rfsv::E_PSI_FILE_PROTECT:
//      unixerr = -EACCES;
//      break;
//    case rfsv::E_PSI_GEN_INUSE:
//    case rfsv::E_PSI_FILE_DEVICE:
//    case rfsv::E_PSI_FILE_PENDING:
//    case rfsv::E_PSI_FILE_NOTREADY:
//      unixerr = -EBUSY;
//      break;
//    case rfsv::E_PSI_FILE_INV:
//    case rfsv::E_PSI_FILE_RETRAN:
//    case rfsv::E_PSI_FILE_LINE:
//    case rfsv::E_PSI_FILE_INACT:
//    case rfsv::E_PSI_FILE_PARITY:
//    case rfsv::E_PSI_FILE_FRAME:
//    case rfsv::E_PSI_FILE_OVERRUN:
//    case rfsv::E_PSI_FILE_CORRUPT:
//    case rfsv::E_PSI_FILE_INVALID:
//    case rfsv::E_PSI_FILE_ABORT:
//    case rfsv::E_PSI_FILE_ERASE:
//    case rfsv::E_PSI_FILE_NDISC:
//    case rfsv::E_PSI_FILE_DRIVER:
//    case rfsv::E_PSI_FILE_COMPLETION:
//    default:
//      unixerr = -EIO;
//      break;
//    case rfsv::E_PSI_FILE_CANCEL:
//      unixerr = -EINTR;
//      break;
//    case rfsv::E_PSI_FILE_DISC:
//    case rfsv::E_PSI_FILE_CONNECT:
//      unixerr = -ENODEV;
//      break;
//    case rfsv::E_PSI_FILE_TOOBIG:
//      unixerr = -EFBIG;
//      break;
//    case rfsv::E_PSI_FILE_HANDLE:
//      unixerr = -EBADF;
//      break;
//    }
//  }
//
////  debuglog("EPOC error %ld became UNIX code %d", epocerr, unixerr);
//  return unixerr;
//}


int plp_connect(Context *context, const char * const host, int port) {
    if (!context->fileServerSocket->connect(host, port)) {
        cout << _("plpftp: could not connect to ncpd") << endl;
        return -1;
    }
    (*context).fileServerFactory = new rfsvfactory(context->fileServerSocket);
    (*context).fileServer = context->fileServerFactory->create(false);

    if (context->fileServer == NULL) {
        return -1;
    }
}

Enum<rfsv::errs> plp_dir(Context *context, const char *path, PlpDir &ret) {
    context->fileServer->dir(path, ret);

    for (int i = 0; i < ret.size(); i++) {
        PlpDirent pe = ret[i];
        cout << pe.getName() << endl;
    }

    return rfsv::E_PSI_GEN_NONE;
}

DirectoryList *rfsv_dir(Context *context, const char *file) {
    PlpDir entries;
    long ret;

    if (context->fileServer == NULL) {
        return NULL;
    }

    ret = context->fileServer->dir(file, entries);
    if (ret < 0) {
        return NULL;
    }

    DirectoryList *list = (DirectoryList *)malloc(sizeof(DirectoryList));
    list->entries = (DirectoryEntry *)malloc(entries.size() * sizeof(DirectoryEntry));
    list->count = entries.size();

    for (int i = 0; i < entries.size(); i++) {
        PlpDirent entry = entries[i];
        list->entries[i].name = strdup(entry.getName());
        list->entries[i].time = entry.getPsiTime().getTime();
        list->entries[i].attr = entry.getAttr();
        list->entries[i].size = entry.getSize();
        list->entries[i].links = 0;  // <-- what is this?
    }
    
    return list;
//    return epocerr_to_errno(ret);
}

extern void directory_list_free(DirectoryList *directoryList) {
    for (int i = 0; i < directoryList->count; i++) {
        free(directoryList->entries[i].name);
    }
    free(directoryList);
}
