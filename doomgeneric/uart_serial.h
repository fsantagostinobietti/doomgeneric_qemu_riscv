#ifndef UART_SERIAL
#define UART_SERIAL

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

int kputchar(int ch);
int kprint(uint8_t *print_string);
int kprintf(const char *format, ...);
int kvprintf(const char *format, va_list arg);
int kvsnprintf(char *buffer, size_t buffer_size, const char *format, va_list arg);
int kgetchar();
int kreadchar();

#endif