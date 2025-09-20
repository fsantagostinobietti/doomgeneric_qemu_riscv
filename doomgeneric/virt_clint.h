#ifndef __VIRT_CLINT__
#define __VIRT_CLINT__

// CLINT (Core Local Interruptor) registers
#include  <stdint.h>

int init_interrupts();

void sleep_us(uint64_t us);

#endif