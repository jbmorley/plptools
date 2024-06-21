//
//  Header.h
//  PsiMac
//
//  Created by Jason Barrie Morley on 18/06/2024.
//

#ifndef RUN_H
#define RUN_H

typedef void (*statusCallback_t)(void *, int);

#ifdef __cplusplus
extern "C" {
#endif
int run(int argc, char **argv);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
int ncpd(int sockNum,
         int baudRate,
         const char *host,
         const char *serialDevice,
         unsigned short nverbose,
         statusCallback_t statusCallback,
         void *context);
#ifdef __cplusplus
}
#endif

#endif /* RUN_H */
