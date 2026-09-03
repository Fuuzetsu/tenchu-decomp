#ifndef PSXSDK_LIBSN_H
#define PSXSDK_LIBSN_H

int PCopen(char *name, int mode, int share);
int PCclose(int fd);
int PClseek(int fd, int offset, int whence);
int PCcreat(char *name, int mode);
void PCinit(void);
int PCread(int fd, void *buffer, int size);
int PCwrite(int fd, void *buffer, int size);

#endif
