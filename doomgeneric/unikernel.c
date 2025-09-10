#include <stdint.h>
#include "syscon.h"

// "test" syscon-compatible device is at memory-mapped address 0x100000
// according to our device tree
#define SYSCON_ADDR 0x100000

void poweroff(void) {
  //kputs("Poweroff requested");
  *(uint32_t *)SYSCON_ADDR = 0x5555;
}

void reboot(void) {
  //kputs("Reboot requested");
  *(uint32_t *)SYSCON_ADDR = 0x7777;
}

#include  "rtc.h"

/* Real-time-clock

See https://www.vociferousvoid.org/index.php/2019/12/10/risc-v-bare-metal-programming-chapter-5-its-a-trap/
*/

#define CLINT_BASE 0x2000000 // The base address of the CLINT module
#define CLINT_MTIME (CLINT_BASE + 0xbff8)   // Address of the MTIME register

/**
 * @brief Gets the mtime value.
 *        mtime register exposes the current value of the real-time counter.
 *        This value expresses the number of clock cycles that have elapsed since the processor was reset.
 * 
 * @return uint64_t 
 */
uint64_t kmtime() {
    volatile uint64_t* mtime = (uint64_t*)CLINT_MTIME;
    uint64_t current_time = *mtime;
    return current_time;
}

/**
 * @brief  Causes the calling thread to be suspended from execution until the number of realtime microseconds 
 *         specified by the argument useconds has elapsed.
 * 
 * @param useconds 
 */
void kusleep(uint64_t useconds) {
    // qemu virt RTC clock is 10M hz
    uint64_t delta = 10 * useconds;
    uint64_t t0 = kmtime();
    while (kmtime() - t0 < delta);
}