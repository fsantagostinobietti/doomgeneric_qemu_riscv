#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <stddef.h>

void free(void *ptr);
void *malloc(size_t size);
void *realloc(void *memblock, size_t size);
void *calloc(size_t number, size_t size);

void exit(int status);
void abort(void);
int atoi(const char *string);
double atof(const char *str);
int system(const char *string);
int abs( int n );

#endif