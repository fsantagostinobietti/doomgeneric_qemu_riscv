#ifndef RTC
#define RTC

#include  <stdint.h>

void kusleep(uint64_t useconds);
uint64_t kmtime();

#endif