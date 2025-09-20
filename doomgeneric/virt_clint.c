// CLINT (Core Local Interruptor) registers
#include "virt_clint.h"
#include "uart_serial.h"

// inline asm in C language
// see https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html
// and https://gcc.gnu.org/onlinedocs/gcc/Simple-Constraints.html

// which hart (core) is this?
uint64_t read_mhartid()
{
  uint64_t x;
  asm volatile("csrr %0, mhartid" : "=r" (x) );
  return x;
}

// base address of the CLINT module (see Device Tree)
#define CLINT_BASE        0x02000000UL  
#define CLINT_MTIME       (CLINT_BASE + 0xBFF8) // MTIME register address
#define CLINT_MTIMECMP    (CLINT_BASE + 0x4000) // MTIMECMP register address

#define MTIME_FREQ        10000000UL  // QEMU default: 10 MHz

uint64_t read_mtime() {
    volatile uint64_t* mtime = (uint64_t*)CLINT_MTIME;
    uint64_t current_time = *mtime;
    return current_time;
}

// debug only
uint64_t read_mtimecmp() {
    volatile uint64_t* mtimecmp = (uint64_t*)CLINT_MTIMECMP;
    uint64_t current_timecmp = *mtimecmp;
    return current_timecmp;
}

// Set 'time' when timer will trigger interrupt
void write_mtimecmp(uint64_t time) {
    volatile uint64_t* mtimecmp = (uint64_t*)CLINT_MTIMECMP;
    *mtimecmp = time;
}

#define MIE_MTIE      (1 << 7)  // Machine-mode Timer Interrupt Enable Flag
#define MSTATUS_MIE   (1 << 3)  // Machine-mode Interrupt Enable Flag
#define MSTATUS_MPP_M   0x1800    // Machine Previous Privilege

void enable_timer_interrupt() {
    // enable specific timer interrupt flag
    uint64_t mie_value;
    asm volatile("csrr %0, mie" : "=r"(mie_value)); // read mie register value
    mie_value |= MIE_MTIE;  // set flag
    asm volatile("csrs mie, %0" :: "r"(mie_value)); // write mie register value

    // enable interrupts flag (in general)
    uint64_t mstatus_value;
    asm volatile("csrr %0, mstatus" : "=r"(mstatus_value));
    mstatus_value |= MSTATUS_MIE; // MSTATUS_MPP_M | MSTATUS_MIE;
    asm volatile("csrs mstatus, %0" :: "r"(mstatus_value));
    //kprintf("enable_timer_interrupt(): completed\n");
}

// See https://five-embeddev.com/riscv-priv-isa-manual/Priv-v1.12/machine.html#sec:mcause
#define MCAUSE_INTERRUPT 0x8000000000000000UL
#define MCAUSE_CODE_MASK 0x7ff
#define MCAUSE_MTI 7    // Machine Timer Interrupt


__attribute__((interrupt ("machine")))
__attribute__((aligned(4)))     // IMPORTANT setting 'mvect' register requires aligned address
/* Very basic interrupt handler, based on the assumption that we have just timer interrupt events */
void handle_interrupt(void) {
    //kprintf("handle_interrupt()\n");
    asm volatile("csrc mie, %0" 
        : /* Outputs: */
        : /* Inputs: */ "r"(MIE_MTIE)
        : /* Clobbered registers: */ 
    );
}

// Initialize interupts
int init_interrupts() {
    // register handle_interrupt() in mtvect (Machin-mode Trap Vector) register
    asm volatile("csrw mtvec, %0" :: "r"(&handle_interrupt));

    uint64_t mtvec_value;
    asm volatile("csrr %0, mtvec" : "=r"(mtvec_value));
    kprintf("init_interrupts(): mtvec register value [%p]\n", mtvec_value);
    if (mtvec_value != (uint64_t)&handle_interrupt) {
        kprintf("init_interrupts(): ERROR since 'mtvect' update value failed!\n");
        return -1;
    }
    return 0;
}

// sleep execution for specified number of microseconds
void sleep_us(uint64_t us) {
    //kprintf("sleep_us(): us [%d]\n", us);
    uint64_t now = read_mtime();
    uint64_t target = now + (us * (MTIME_FREQ/1000000));
    write_mtimecmp(target);
    
    enable_timer_interrupt();

    asm volatile("wfi");    // interrupt controlled waiting (Wait For Interrupt)
}
