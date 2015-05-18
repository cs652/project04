
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "serial.h"
#include "../threads/synch.h"
#include "../threads/thread.h"
#include "../threads/interrupt.h"

#define RXBUFMASK 0xFFF
#define IRQ_29 29
static volatile unsigned int rxhead;
static volatile unsigned int rxtail;
static volatile unsigned char rxbuffer[RXBUFMASK+1];
void serial_init(void) {
    test_serial();
}
void serial_putc (char character) {
    //uart_putc(96);
    //uart_putc('B');
    uart_putc(character);
}
void serial_flush (void) {
    // TODO Implement the method.
}

void serial_notify (void) {
    // TODO Implement the method.
}

static inline void mmio_write(uint32_t reg, uint32_t data)
{
    uint32_t *ptr = (uint32_t*) reg;
    asm volatile("str %[data], [%[reg]]" : : [reg]"r"(ptr), [data]"r"(data));
}

static inline uint32_t mmio_read(uint32_t reg)
{
    uint32_t *ptr = (uint32_t*)reg;
    uint32_t data;
    asm volatile("ldr %[data], [%[reg]]" : [data]"=r"(data) : [reg]"r"(ptr));
    return data;
}

/* Loop <delay> times in a way that the compiler won't optimize away. */
static inline void delay(int32_t count)
{
    asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
            : : [count]"r"(count) : "cc");
}

enum
{
    // The GPIO registers base address.
    GPIO_BASE = 0x20200000,

    // The offsets for reach register.

    // Controls actuation of pull up/down to ALL GPIO pins.
    GPPUD = (GPIO_BASE + 0x94),

    // Controls actuation of pull up/down for specific GPIO pin.
    GPPUDCLK0 = (GPIO_BASE + 0x98),

    // The base address for UART.
    UART0_BASE = 0x20201000,

    // The offsets for reach register for the UART.
    UART0_DR     = (UART0_BASE + 0x00),
    UART0_RSRECR = (UART0_BASE + 0x04),
    UART0_FR     = (UART0_BASE + 0x18),
    UART0_ILPR   = (UART0_BASE + 0x20),
    UART0_IBRD   = (UART0_BASE + 0x24),
    UART0_FBRD   = (UART0_BASE + 0x28),
    UART0_LCRH   = (UART0_BASE + 0x2C),
    UART0_CR     = (UART0_BASE + 0x30),
    UART0_IFLS   = (UART0_BASE + 0x34),
    UART0_IMSC   = (UART0_BASE + 0x38),
    UART0_RIS    = (UART0_BASE + 0x3C),
    UART0_MIS    = (UART0_BASE + 0x40),
    UART0_ICR    = (UART0_BASE + 0x44),
    UART0_DMACR  = (UART0_BASE + 0x48),
    UART0_ITCR   = (UART0_BASE + 0x80),
    UART0_ITIP   = (UART0_BASE + 0x84),
    UART0_ITOP   = (UART0_BASE + 0x88),
    UART0_TDR    = (UART0_BASE + 0x8C),
};

void uart_init()
{
    unsigned int ra;

    PUT32(AUX_ENABLES,1);
    PUT32(AUX_MU_IER_REG,0);
    PUT32(AUX_MU_CNTL_REG,0);
    PUT32(AUX_MU_LCR_REG,3);
    PUT32(AUX_MU_MCR_REG,0);
    PUT32(AUX_MU_IER_REG,0x5); //enable rx interrupts
    PUT32(AUX_MU_IIR_REG,0xC6);
    PUT32(AUX_MU_BAUD_REG,270);

    ra=GET32(GPFSEL1);
    ra&=~(7<<12); //gpio14
    ra|=2<<12;    //alt5
    ra&=~(7<<15); //gpio15
    ra|=2<<15;    //alt5
    PUT32(GPFSEL1,ra);

    PUT32(GPPUD,0);
    for(ra=0;ra<150;ra++) dummy(ra);
    PUT32(GPPUDCLK0,(1<<14)|(1<<15));
    for(ra=0;ra<150;ra++) dummy(ra);
    PUT32(GPPUDCLK0,0);

    PUT32(AUX_MU_CNTL_REG,3);
}

void uart_putc_helper(unsigned char byte) {
    PUT32(AUX_MU_IO_REG,byte);
}

void uart_putc(unsigned char byte)
{
    if (byte == '\n') {
        uart_putc_helper('\n');
        uart_putc_helper('\r');
    } else {
        uart_putc_helper(byte);
    }
}

// Test using screen /dev/cu.PL2303-00001004 115200

unsigned char uart_getc()
{
    enum interrupts_level old_level;
    if (rxtail==rxhead){
        old_level = interrupts_disable();
        thread_block(); 
        interrupts_set_level(old_level);
    }
    unsigned char byte;
    byte = rxbuffer[rxtail];
    rxtail=(rxtail+1)&RXBUFMASK;
    return byte;
}

void uart_write(const unsigned char* buffer, size_t size)
{
    size_t i;
    for ( i = 0; i < size; i++ )
        uart_putc(buffer[i]);
}

void uart_puts(const char* str)
{
    uart_write((const unsigned char*) str, strlen(str));
}

void test_serial() {

    uart_init();
}
static void keyboard_irq_handler(struct interrupts_stack_frame *stack_frame) {
    unsigned int rb,rc;
    struct list_elem *e;
    struct thread *t;
    while(1)
    {
        rb=GET32(AUX_MU_IIR_REG);
        if((rb&1)==1) break; 
        if((rb&6)==4)
        {
            rc=GET32(AUX_MU_IO_REG);
            rxbuffer[rxhead]=rc&0xFF;
            rxhead=(rxhead+1)&RXBUFMASK;
        }
    }
    struct list *all_list = get_all_list();
    for (e = list_begin(all_list); e != list_end(all_list); e = list_next(e)) {
        t = list_entry(e, struct thread, allelem);
        if (!strcmp(t->name, "Shell")) {
            thread_unblock(t);
        }
    }
    set_current_interrupts_stack_frame(stack_frame);
    interrupts_yield_on_return();
}
void keyboard_init() {
    printf("\nInitializing keyboard.....");
    interrupts_register_irq(IRQ_29, keyboard_irq_handler, "Keyboard");
}
