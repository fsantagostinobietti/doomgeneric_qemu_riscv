#include "syscon.h"
#include <errno.h>

int errno;

// -----------------------------

#include <ctype.h>

int isspace(int c) {
    return c == ' '  || c == '\f' ||
           c == '\n' || c == '\r' ||
           c == '\t' || c == '\v';
}

int tolower(int c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

int toupper(int c) {
    if (c >= 'a' && c <= 'z') {
        return c - ('a' - 'A');
    }
    return c;
}


// -----------------------------

#include <stdio.h>
#include "uart_serial.h"

FILE * stderr;
FILE * stdout;

int fprintf(FILE *stream, const char *format, ...) {
    if (stream!=stderr && stream!=stdout ) {
        kprintf("Error - fprintf: unsupported stream\n");
        return -1;
    }

    int ret;
    /* Declare a va_list type variable */
    va_list myargs;
    /* Initialise the va_list variable with the ... after fmt */
    va_start(myargs, format);
    /* Forward the '...' to vprintf */
    ret = kvprintf(format, myargs);
    /* Clean up the va_list */
    va_end(myargs);
    return ret;
}

int snprintf(char * destination, size_t size, const char * format, ...) {
    int ret;
    /* Declare a va_list type variable */
    va_list myargs;
    /* Initialise the va_list variable with the ... after fmt */
    va_start(myargs, format);
    /* Forward the '...' to vprintf */
    ret = kvsnprintf(destination, size, format, myargs);
    /* Clean up the va_list */
    va_end(myargs);
    return ret;
}

/* int printf(const char *format, ...) {
    return 0;
} */

int vfprintf(FILE *stream, const char *format, va_list argptr) {
    if (stream!=stderr && stream!=stdout ) {
        kprintf("Error - fprintf: unsupported stream\n");
        return -1;
    }

    return kvprintf(format, argptr);
}

int vsnprintf(char *buffer, size_t buf_size, const char *format, va_list argptr) {
    return kvsnprintf(buffer, buf_size, format, argptr);
}

int sscanf(const char *buffer, const char *format, ...) {
    printf("sscanf: ERROR unimplemented function\n");
    poweroff();
    return 0;
}

FILE *fopen(const char *filename, const char *mode) {
    printf("fopen: ERROR unimplemented function\n");
    return NULL;
}

int fclose(FILE *stream) {
    return 0;
}

long ftell(FILE *stream) {
    printf("ftell: ERROR unimplemented function\n");
    return 0;
}

int fflush(FILE *stream) {
    return 0;
}

int fseek(FILE *stream, long offset, int origin) {
    printf("fseek: ERROR unimplemented function\n");
    return 0;
}

size_t fwrite(const void *buffer, size_t size, size_t count, FILE *stream) {
    printf("fwrite: ERROR unimplemented function\n");
    return 0;
}

size_t fread(void *buffer, size_t size, size_t count, FILE *stream) {
    printf("fread: ERROR unimplemented function\n");
    poweroff();
    return 0;
}

int remove (const char * filename) {
    printf("remove: ERROR unimplemented function\n");
    return 0;
}

int rename(const char *oldname, const char *newname) {
    printf("rename: ERROR unimplemented function\n");
    return 0;
}

int puts(const char *string) {
    kprint((uint8_t *)string);
    kprint("\n");
    return 0;
}

int putchar(int c) {
    return kputchar(c);
}

char *strstr(const char *str, const char *strSearch) {
    if (!*strSearch) {
        return (char *)str; // Empty substring matches at the beginning
    }

    for (; *str != '\0'; str++) {
        const char *s = str;
        const char *p = strSearch;

        while (*s && *p && (*s == *p)) {
            s++;
            p++;
        }

        if (*p == '\0') {
            return (char *)str; // Match found
        }
    }

    return NULL; // No match
}

int strncmp(const char *s1, const char *s2, size_t count) {
    for (size_t i = 0; i < count; i++) {
        unsigned char c1 = (unsigned char)s1[i];
        unsigned char c2 = (unsigned char)s2[i];

        if (c1 != c2) {
            return c1 - c2;
        }

        if (c1 == '\0') {
            break;
        }
    }

    return 0;
}


// -----------------------------

#include <stdlib.h>
#include <string.h>

// init heap memory address
extern uint64_t _stack_top;
uint64_t heap_start = (uint64_t)&_stack_top;

void free(void *ptr) {
    printf("free() ignored!\n");
}

void *malloc(size_t size) {
    void *addr = (void*) heap_start;
    heap_start += size;
    printf("malloc: addr [%p], size [%d]\n", addr, size);
    return addr;
}

void *realloc(void *memblock, size_t size) {
    printf("realloc\n");
    if (memblock == NULL) {
        // Equivalent to malloc
        return malloc(size);
    }

    if (size == 0) {
        // Equivalent to free
        free(memblock);
        return NULL;
    }

    // Allocate new block
    void *new_block = malloc(size);
    if (!new_block) {
        return NULL;
    }

    // Copy old data to new block
    // NOTE: In a freestanding environment, we don't know the original size,
    // so this assumes the caller tracks it or uses a fixed-size allocation.
    // For demonstration, we assume a safe minimum copy size.
    // You should replace this with actual size tracking if available.
    size_t copy_size = size; // Replace with actual old size if known
    memcpy(new_block, memblock, copy_size);

    // Free old block
    free(memblock);

    return new_block;
}


void *calloc(size_t number, size_t size) {
    printf("calloc\n");
    size_t total = number * size;
    void *ptr = malloc(total);
    if (!ptr) {
        return NULL;
    }
    return memset(ptr, 0, total);
}

void exit(int status) {
    poweroff();
}

void abort(void) {
    poweroff();
}

int atoi(const char *str) {
    int result = 0;
    int sign = 1;

    // Skip leading whitespace
    while (*str == ' ' || *str == '\t' || *str == '\n' ||
           *str == '\v' || *str == '\f' || *str == '\r') {
        str++;
    }

    // Handle optional sign
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    // Convert digits
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }

    return sign * result;
}

double atof(const char *str) {
    double result = 0.0;
    double fraction = 0.0;
    int sign = 1;
    int divisor = 1;
    int seen_dot = 0;

    // Skip leading whitespace
    while (*str == ' ' || *str == '\t' || *str == '\n' ||
           *str == '\v' || *str == '\f' || *str == '\r') {
        str++;
    }

    // Handle optional sign
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    // Parse digits and optional decimal point
    while ((*str >= '0' && *str <= '9') || (*str == '.' && !seen_dot)) {
        if (*str == '.') {
            seen_dot = 1;
            str++;
            continue;
        }

        if (!seen_dot) {
            result = result * 10.0 + (*str - '0');
        } else {
            fraction = fraction * 10.0 + (*str - '0');
            divisor *= 10;
            str++;
        }
    }

    return sign * (result + fraction / divisor);
}


int system(const char *command) {
    printf("system: command [%s]. ERROR unimplemented system function\n", command);
    poweroff();
    return 0;
}

int abs(int n) {
    return (n < 0) ? -n : n;
}


// -----------------------------

#include <string.h>

void *memset(void *dest, int c, size_t count) {
    unsigned char *d = (unsigned char *)dest;
    unsigned char value = (unsigned char)c;

    for (size_t i = 0; i < count; i++) {
        d[i] = value;
    }

    return dest;
}


void *memcpy(void *dest, const void *src, size_t count) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    for (size_t i = 0; i < count; i++) {
        d[i] = s[i];
    }

    return dest;
}


void *memmove(void *dest, const void *src, size_t count) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    if (d == s || count == 0) {
        return dest;
    }

    if (d < s) {
        // Copy forward
        for (size_t i = 0; i < count; i++) {
            d[i] = s[i];
        }
    } else {
        // Copy backward
        for (size_t i = count; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }

    return dest;
}



size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

int strcasecmp(const char *s1, const char *s2) {
    unsigned char c1, c2;
    while (*s1 && *s2) {
        c1 = (unsigned char)tolower((unsigned char)*s1);
        c2 = (unsigned char)tolower((unsigned char)*s2);
        if (c1 != c2) {
            return c1 - c2;
        }
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

int strncasecmp(const char *s1, const char *s2, size_t count) {
    unsigned char c1, c2;

    while (count-- > 0) {
        c1 = (unsigned char)tolower((unsigned char)*s1);
        c2 = (unsigned char)tolower((unsigned char)*s2);

        if (c1 != c2) {
            return c1 - c2;
        }

        if (c1 == '\0') {
            break;
        }

        s1++;
        s2++;
    }

    return 0;
}

char *strdup(const char *string) {
    size_t len = strlen(string) + 1; // +1 for null terminator
    char *copy = (char *)malloc(len);
    if (!copy) {
        return NULL;
    }

    for (size_t i = 0; i < len; i++) {
        copy[i] = string[i];
    }

    return copy;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

char *strrchr(const char *str, int c) {
    const char *last = NULL;
    char ch = (char)c;

    while (*str) {
        if (*str == ch) {
            last = str;
        }
        str++;
    }

    // Check for null terminator match if c == '\0'
    if (ch == '\0') {
        return (char *)str;
    }

    return (char *)last;
}

char *strchr(const char *str, int c) {
    char ch = (char)c;

    while (*str) {
        if (*str == ch) {
            return (char *)str;
        }
        str++;
    }

    // If c is '\0', return pointer to null terminator
    if (ch == '\0') {
        return (char *)str;
    }

    return NULL;
}

char *strncpy(char *dest, const char *src, size_t count) {
    size_t i = 0;

    // Copy characters from src to dest
    while (i < count && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }

    // Pad with null bytes if src is shorter than count
    while (i < count) {
        dest[i] = '\0';
        i++;
    }

    return dest;
}



// -----------------------------

#include <sys/stat.h>

int mkdir(const char *path, mode_t mode) {
    return 0;
}

#include <math.h>

double fabs(double x) {
    return 0.0;
}
