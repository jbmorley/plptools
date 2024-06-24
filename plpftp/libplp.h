#ifndef _libplp_h_
#define _libplp_h_

#include <ppsocket.h>
#include <rfsv.h>
#include <rfsvfactory.h>
#include <plpdirent.h>

typedef struct Context {
    ppsocket *fileServerSocket;
    rfsvfactory *fileServerFactory;
    rfsv *fileServer;
} Context;

typedef struct DirectoryEntry {
    char *name;
    long time;
    long attr;
    long size;
    long links;
    struct DirectoryEntry *next;
} DirectoryEntry;

typedef struct DirectoryList {
    DirectoryEntry *entries;
    int count;
} DirectoryList;

#endif

extern void directory_list_free(DirectoryList *directoryList);

extern int plp_init(Context *context);
extern int plp_connect(Context *context, const char * const host, int port);
extern Enum<rfsv::errs> plp_dir(Context *context, const char *path, PlpDir &ret);

extern DirectoryList *rfsv_dir(Context *context, const char *file);
