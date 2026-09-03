#ifndef TENCHU_STRING_H
#define TENCHU_STRING_H

#include "types.h"

void *memcpy(void *destination, const void *source, size_t count);
void *memset(void *destination, int value, size_t count);
int memcmp(const void *left, const void *right, size_t count);
char *strcpy(char *destination, const char *source);
char *strcat(char *destination, const char *source);
char *strncpy(char *destination, const char *source, size_t count);
int strcmp(const char *left, const char *right);
int strncmp(const char *left, const char *right, size_t count);
size_t strlen(const char *string);

#endif
