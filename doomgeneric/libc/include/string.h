#ifndef __STRING_H__
#define __STRING_H__

#include <stddef.h>

void *memset(void *dest, int c, size_t count);
void *memcpy(void *dest, const void *src, size_t count);
void *memmove(void *dest, const void *src, size_t count);

size_t strlen(const char *str);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *string1, const char *string2, size_t count);
char *strdup(const char *string);
int strcmp(const char *string1, const char *string2);
char *strrchr(const char *string, int c);
char *strchr(const char *str, int c);
char *strncpy(char *strDest, const char *strSource, size_t count);

#endif