/**
 * @file uart_serial.c
 * @brief UART driver for ns16550a device, QEMU Virt emulation.
 * 
 * @see https://github.com/michaeljclark/riscv-probe/blob/master/libfemto/drivers/ns16550a.c
 * 
 */
#include <stddef.h>
#include <stdarg.h>
#include "uart_serial.h"


#define UART_BASE       0x10000000UL  // Adjust this to your actual UART base address
#define UART_LSR        (UART_BASE + 0x05) // Line Status Register address
#define UART_RBR        (UART_BASE + 0x00) // Receiver Buffer Register address
#define UART_THR        (UART_BASE + 0x00) // Transmit Hold Register address

#define UART_LSR_DR     0x01  // Data Ready

#define mmio_read_char(ADDR)         (*(volatile uint8_t *)(ADDR))
#define mmio_write_char(ADDR, CHAR)  *((volatile uint8_t *)(ADDR)) = (CHAR)


/**
 * @brief Puts single byte into serial uart. 
 * 
 * @param ch 
 */
void uart_putchar(uint8_t ch) {
    mmio_write_char(UART_THR, ch);
}

bool uart_data_is_ready() {
    return ((mmio_read_char(UART_LSR) & UART_LSR_DR) != 0);
}

/**
 * @brief Gets single char from serial uart. Blocks until char is available.
 * 
 * @return uint8_t 
 */
uint8_t uart_getchar() {
    // Wait until data is available
    while (!uart_data_is_ready());
    // Read and return the received byte
    return mmio_read_char(UART_RBR);
}

/**
 * @brief Non-blocking version of uart_getcahr()
 * 
 * @return int 0-255 when data is available; -1 otherwise
 */
int uart_readchar() {
    if (!uart_data_is_ready())
        // data not available
        return -1;
    // Read and return the received byte
    return mmio_read_char(UART_RBR);
}

//
// Utilities
//

int _toupper(int c) {
  return 'a' <= c && c <= 'z' ? c + 'A' - 'a' : c;
}

#define to_hex_digit(n) ('0' + (n) + ((n) < 10 ? 0 : 'a' - '0' - 10))

void swap(char *xp, char *yp) {
    char temp = *xp;
    *xp = *yp;
    *yp = temp;
}
 
void reverse(char str[], int length) {
    int start = 0;
    int end = length -1;
    while (start < end)
    {
        swap(str+start, str+end);
        start++;
        end--;
    }
}

uint8_t* uitoa(uint8_t *str, uint64_t num, int base) {
    int i = 0;
 
    /* Handle 0 explicitly, otherwise empty string is printed for 0 */
    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
 
    // Process individual digits
    while (num != 0)
    {
        int rem = num % base;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }
 
    str[i] = '\0';
 
    reverse(str, i);
 
    return str;
}


//
// Kernel IO functions
//

/**
 * @brief Puts single char into serial output w/o buffering. 
 * 
 * @param ch 
 * @return int 0-255 value
 */
int kputchar(int ch) {
    uart_putchar(ch & 0xff);
    return ch;
}

/**
 * @brief Gets single char from serial input (keyboard). Blocks until char is available.
 * 
 * @return int 0-255 value char; -1 otherwise
 */
int kgetchar() {
    return uart_getchar();
}

/**
 * @brief Non-blocking version of kgetchar()
 * 
 * @return int 0-255 value char; -1 otherwise
 */
int kreadchar() {
    return uart_readchar();
}

int kprint(uint8_t *print_string) {
    uint64_t i = 0;
    while(1) {
        if (print_string[i] == 0) {
            break;
        }
        kputchar(print_string[i]);
        i++;
    }
	return i;
}

int ksprint(char *buffer, uint8_t *print_string) {
    uint64_t i = 0;
    while(1) {
        if (print_string[i] == 0) {
            break;
        }
        buffer[i++] = print_string[i]; //kputchar(print_string[i]);
    }
	return i;
}

void kprint_ui(uint64_t inp) {
    uint8_t str[20];
    uitoa(str, inp, 10);
    kprint(str);
}

int kvsnprintf(char *buffer, size_t buffer_size, const char *format, va_list arg) {
  int i = 0;
  while (*format && i<buffer_size) {
    if (*format == '%') {
      ++format;
      if (!*format)
	    return -1;
      switch (*format) {
      	case 'd':
      	case 'i':
			{
				int n = va_arg(arg, int);
				if (n < 0) {
					buffer[i++] = '-';
					n = ~n + 1;
				}
				char lsh = '0' + n % 10;
				n /= 10;
				char buf[9];
				char *p_buf = buf;
				while (n) {
					*p_buf++ = '0' + n % 10;
					n /= 10;
				}
				while (p_buf != buf) {
					buffer[i++] = (*--p_buf);
				}
				buffer[i++] = lsh;
			}
			break;
      	case 'u':
			{
				unsigned n = va_arg(arg, unsigned);
				char lsh = '0' + n % 10;
				n /= 10;
				char buf[9];
				char *p_buf = buf;
				while (n) {
					*p_buf++ = '0' + n % 10;
					n /= 10;
				}
				while (p_buf != buf) {
					buffer[i++] = (*--p_buf);
				}
				buffer[i++] = lsh;
			}
			break;
      	case 'o':
			{
				unsigned n = va_arg(arg, unsigned);
				char lsh = '0' + n % 8;
				n /= 8;
				char buf[10];
				char *p_buf = buf;
				while (n) {
					*p_buf++ = '0' + n % 8;
					n /= 8;
				}
				while (p_buf != buf) {
					buffer[i++] = (*--p_buf);
				}
				buffer[i++] = lsh;
			}
			break;
      	case 'x':
			{
				unsigned n = va_arg(arg, unsigned);
				char lsh = to_hex_digit(n % 16);
				n /= 16;
				char buf[7];
				char *p_buf = buf;
				while (n) {
					*p_buf++ = to_hex_digit(n % 16);
					n /= 16;
				}
				while (p_buf != buf) {
					buffer[i++] = (*--p_buf);
				}
				buffer[i++] = lsh;
			}
			break;
      	case 'X':
			{
				unsigned n = va_arg(arg, unsigned);
				char lsh = to_hex_digit(n % 16);
				n /= 16;
				char buf[7];
				char *p_buf = buf;
				while (n) {
					*p_buf++ = to_hex_digit(n % 16);
					n /= 16;
				}
				while (p_buf != buf) {
					buffer[i++] = _toupper(*--p_buf);
				}
				buffer[i++] = _toupper(lsh);
			}
			break;
      	case 'c':
			buffer[i++] = va_arg(arg, int);
			break;
      	case 's':
			i += ksprint(buffer+i, va_arg(arg, char *));
			break;
      	case 'p':
			{
				i += ksprint(buffer+i, "0x");
				size_t ptr = va_arg(arg, size_t);
				char lsh = to_hex_digit(ptr % 16);
				ptr /= 16;
				char buf[15];
				char *p_buf = buf;
				while (ptr) {
					*p_buf++ = to_hex_digit(ptr % 16);
					ptr /= 16;
				}
				while (p_buf != buf) {
					buffer[i++] = (*--p_buf);
				}
				buffer[i++] = lsh;
			}
			break;
      	case '%':
			buffer[i++] = '%';
			break;
      	default:
			kprintf("kvsnprintf: WARNING unsuppoerted format [%%%s]\n", format);
			buffer[i++] = '%';
			buffer[i++] = (*format);
      }
    } else {
      buffer[i++] = (*format);
	}
    ++format;
  }
  if (i==buffer_size)
  	return -1;
  buffer[i] = 0;
  return i;
}


int kvprintf(const char *format, va_list arg) {
  int res = 0;
  while (*format) {
    if (*format == '%') {
      ++format;
      if (!*format)
	    return res;
      switch (*format) {
      case 'd':
      case 'i':
	{
	  int n = va_arg(arg, int);
	  if (n < 0) {
	    kputchar('-');
		res++;
	    n = ~n + 1;
	  }
	  char lsh = '0' + n % 10;
	  n /= 10;
	  char buf[9];
	  char *p_buf = buf;
	  while (n) {
            *p_buf++ = '0' + n % 10;
	    n /= 10;
	  }
	  while (p_buf != buf) {
	    kputchar(*--p_buf);
		res++;
	  }
	  kputchar(lsh);
	  res++;
	}
	break;
      case 'u':
        {
	  unsigned n = va_arg(arg, unsigned);
	  char lsh = '0' + n % 10;
	  n /= 10;
	  char buf[9];
	  char *p_buf = buf;
	  while (n) {
            *p_buf++ = '0' + n % 10;
	    n /= 10;
	  }
	  while (p_buf != buf) {
	    kputchar(*--p_buf);
		res++;
	  }
	  kputchar(lsh);
	  res++;
	}
	break;
      case 'o':
        {
	  unsigned n = va_arg(arg, unsigned);
	  char lsh = '0' + n % 8;
	  n /= 8;
	  char buf[10];
	  char *p_buf = buf;
	  while (n) {
            *p_buf++ = '0' + n % 8;
	    n /= 8;
	  }
	  while (p_buf != buf) {
	    kputchar(*--p_buf);
		res++;
	  }
	  kputchar(lsh);
	  res++;
	}
	break;
      case 'x':
        {
	  unsigned n = va_arg(arg, unsigned);
	  char lsh = to_hex_digit(n % 16);
	  n /= 16;
	  char buf[7];
	  char *p_buf = buf;
	  while (n) {
            *p_buf++ = to_hex_digit(n % 16);
	    n /= 16;
	  }
	  while (p_buf != buf) {
	    kputchar(*--p_buf);
		res++;
	  }
	  kputchar(lsh);
	  res++;
	}
	break;
      case 'X':
        {
	  unsigned n = va_arg(arg, unsigned);
	  char lsh = to_hex_digit(n % 16);
	  n /= 16;
	  char buf[7];
	  char *p_buf = buf;
	  while (n) {
            *p_buf++ = to_hex_digit(n % 16);
	    n /= 16;
	  }
	  while (p_buf != buf) {
	    kputchar(_toupper(*--p_buf));
		res++;
	  }
	  kputchar(_toupper(lsh));
	  res++;
	}
	break;
      case 'c':
	kputchar(va_arg(arg, int));
	res++;
	break;
      case 's':
	res += kprint(va_arg(arg, char *));
	break;
      case 'p':
	{
          kprint("0x");
	  size_t ptr = va_arg(arg, size_t);
	  char lsh = to_hex_digit(ptr % 16);
	  ptr /= 16;
	  char buf[15];
	  char *p_buf = buf;
	  while (ptr) {
            *p_buf++ = to_hex_digit(ptr % 16);
	    ptr /= 16;
	  }
	  while (p_buf != buf) {
	    kputchar(*--p_buf);
		res++;
	  }
	  kputchar(lsh);
	  res++;
	}
	break;
      case '%':
	kputchar('%');
	res++;
	break;
      default:
	kputchar('%');
	res++;
	kputchar(*format);
	res++;
      }
    } else {
      kputchar(*format);
	  res++;
	}
    ++format;
  }
  return res;
}

/**
 * @brief Limited version of printf() which only supports the following specifiers:
 * 
 * - d/i: Signed decimal integer
 * 
 * - u: Unsigned decimal integer
 * 
 * - o: Unsigned octal
 * 
 * - x: Unsigned hexadecimal integer
 * 
 * - X: Unsigned hexadecimal integer (uppercase)
 * 
 * - c: Character
 * 
 * - s: String of characters
 * 
 * - p: Pointer address
 * 
 * - %: Literal '%'
 * 
 * None of the sub-specifiers are supported for the sake of simplicity.
 * The `n` specifier is not supported since that is a major source of
 * security vulnerabilities. None of the floating-point specifiers are
 * supported since floating point operations don't make sense in kernel
 * space
 * Anyway, this subset should suffice for printf debugging
 * 
 * @param format 
 * @param ... 
 */
int kprintf(const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  int res = kvprintf(format, arg);
  va_end(arg);
  return res;
}
