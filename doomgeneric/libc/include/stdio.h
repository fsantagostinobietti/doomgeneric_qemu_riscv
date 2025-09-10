/**
 * @brief Fake stdio.h
 * 
 */
#ifndef __STDIO_H__
#define __STDIO_H__

#include <stddef.h>
#include "../../uart_serial.h"

struct __sFile 
{
    int unused;
};
typedef struct __sFILE FILE;

int snprintf(char * destination, size_t size, const char * format, ...);
int fprintf(FILE *stream, const char *format, ...);
//int printf(const char *format, ...);
#define printf  kprintf
int vfprintf(FILE *stream, const char *format, va_list argptr);
int vsnprintf(char *buffer, size_t count, const char *format, va_list argptr);
int sscanf(const char *buffer, const char *format, ...);
FILE *fopen(const char *filename, const char *mode);
int fclose(FILE *stream);
long ftell(FILE *stream);
int fflush(FILE *stream);
int fseek(FILE *stream, long offset, int origin);
size_t fwrite(const void *buffer, size_t size, size_t count, FILE *stream);
size_t fread(void *buffer, size_t size, size_t count, FILE *stream);
int remove (const char * filename);
int rename(const char *oldname, const char *newname);
int puts(const char *string);
int putchar(int c);
char *strstr(const char *str, const char *strSearch);
int strncmp(const char *string1, const char *string2, size_t count);

#define SEEK_END 2
#define SEEK_SET 3

extern FILE * stderr;
extern FILE * stdout;


#endif