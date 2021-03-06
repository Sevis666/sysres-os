#include "uart.h"
#include "misc.h"
#include "debug.h"
#include "string.h"

#define IO_BUFF_SIZE 500
#ifdef HARDWARE
/*
 * The following code was originally written
 * by David Welch, and published unlicensed
 *
 * The full source code is available at
 * http://www.github.com/dwelch67/raspberrypi
 */

extern void PUT32 ( unsigned int, unsigned int );
extern unsigned int GET32 ( unsigned int );
extern void dummy ( unsigned int );


/* void PUT32 ( unsigned int addr, unsigned int data){ */
/*     *((uint32_t *) addr) = (uint32_t) data; */
/* } */
/* unsigned int GET32 ( unsigned int addr){ */
/*     return *((uint32_t *) addr); */
/* } */
/* void dummy ( unsigned int addr __attribute__((unused))){ */
/*     return; */
/* } */

#define GPFSEL1         (GPIO_BASE + 0x00000004)
#define GPSET0          (GPIO_BASE + 0x0000001C)
#define GPCLR0          (GPIO_BASE + 0x00000028)
#define GPPUD           (GPIO_BASE + 0x00000094)
#define GPPUDCLK0       (GPIO_BASE + 0x00000098)

#define AUX_ENABLES     (GPIO_BASE + 0x00015004)
#define AUX_MU_IO_REG   (GPIO_BASE + 0x00015040)
#define AUX_MU_IER_REG  (GPIO_BASE + 0x00015044)
#define AUX_MU_IIR_REG  (GPIO_BASE + 0x00015048)
#define AUX_MU_LCR_REG  (GPIO_BASE + 0x0001504C)
#define AUX_MU_MCR_REG  (GPIO_BASE + 0x00015050)
#define AUX_MU_LSR_REG  (GPIO_BASE + 0x00015054)
#define AUX_MU_MSR_REG  (GPIO_BASE + 0x00015058)
#define AUX_MU_SCRATCH  (GPIO_BASE + 0x0001505C)
#define AUX_MU_CNTL_REG (GPIO_BASE + 0x00015060)
#define AUX_MU_STAT_REG (GPIO_BASE + 0x00015064)
#define AUX_MU_BAUD_REG (GPIO_BASE + 0x00015068)

//GPIO14  TXD0 and TXD1
//GPIO15  RXD0 and RXD1
//alt function 5 for uart1
//alt function 0 for uart0

//((250,000,000/115200)/8)-1 = 270

unsigned int uart_recv ( void ) {
    while(1) {
        if(GET32(AUX_MU_LSR_REG)&0x01) break;
    }
    return(GET32(AUX_MU_IO_REG)&0xFF);
}

void uart_send ( unsigned int c ) {
    while(1) {
        if(GET32(AUX_MU_LSR_REG)&0x20) break;
    }
    PUT32(AUX_MU_IO_REG,c);
}

void uart_init ( void ) {
    unsigned int ra;

    PUT32(AUX_ENABLES,1);
    PUT32(AUX_MU_IER_REG,0);
    PUT32(AUX_MU_CNTL_REG,0);
    PUT32(AUX_MU_LCR_REG,3);
    PUT32(AUX_MU_MCR_REG,0);
    PUT32(AUX_MU_IER_REG,0);
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

/* End of external code */

void uart_putc(unsigned char c) {
    uart_send(c);
}

unsigned char uart_getc() {
    return uart_recv();
}


#else
/* The following code comes from OSDev */
enum {
    // The offsets for reach register.

    // Controls actuation of pull up/down to ALL GPIO pins.
    GPPUD = (GPIO_BASE + 0x94),

    // Controls actuation of pull up/down for specific GPIO pin.
    GPPUDCLK0 = (GPIO_BASE + 0x98),

    // The base address for UART.
    UART0_BASE = (GPIO_BASE + 0x00001000),

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

// Memory-Mapped I/O output
static inline void mmio_write(uint64_t reg, uint64_t data) {
    *(volatile uint64_t *) reg = data;
}

// Memory-Mapped I/O input
static inline uint64_t mmio_read(uint64_t reg) {
    return *(volatile uint64_t *)reg;
}

void uart_init() {
    // Disable UART0.
    mmio_write(UART0_CR, 0x00000000);
    // Setup the GPIO pin 14 && 15.

    // Disable pull up/down for all GPIO pins & delay for 150 cycles.
    mmio_write(GPPUD, 0x00000000);
    delay(150);

    // Disable pull up/down for pin 14,15 & delay for 150 cycles.
    mmio_write(GPPUDCLK0, (1 << 14) | (1 << 15));
    delay(150);

    // Write 0 to GPPUDCLK0 to make it take effect.
    mmio_write(GPPUDCLK0, 0x00000000);

    // Clear pending interrupts.
    mmio_write(UART0_ICR, 0x7FF);

    // Set integer & fractional part of baud rate.
    // Divider = UART_CLOCK/(16 * Baud)
    // Fraction part register = (Fractional part * 64) + 0.5
    // UART_CLOCK = 3000000; Baud = 115200.

    // Divider = 3000000 / (16 * 115200) = 1.627 = ~1.
    mmio_write(UART0_IBRD, 1);
    // Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
    mmio_write(UART0_FBRD, 40);

    // Enable FIFO & 8 bit data transmissio (1 stop bit, no parity).
    mmio_write(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));

    // Mask all interrupts.
    mmio_write(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
            (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));

    // Enable UART0, receive & transfer part of UART.
    mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

void uart_putc(unsigned char c) {
    // Wait for UART to become ready to transmit.
    while ( mmio_read(UART0_FR) & (1 << 5) ) { }
    mmio_write(UART0_DR, c);
}

unsigned char uart_getc() {
    // Wait for UART to have received something.
    while ( mmio_read(UART0_FR) & (1 << 4) ) { }
    return mmio_read(UART0_DR);
}

/* End of external code */
#endif

int uart_puts(const char* str) {
    int i = 0;
    for (int i = 0; str[i] != '\0'; i ++)
        uart_putc((unsigned char)str[i]);
    return i;
}

/* Generic function for printing an integer, warning displays nothing for 0 */
int uart_put_uint(uint64_t x, unsigned int base, bool upper_hexa) {
    if(x == 0) {
        return 0;
    }
    uint64_t y = x % base;
    int written = uart_put_uint(x/base, base, upper_hexa);
    if(y <= 9) uart_putc(48 + y);
    else if(upper_hexa) uart_putc(55 + y);
    else uart_putc(87 + y);
    return written + 1;
}


int uart_put_int(int64_t x, unsigned int base, bool unsign, bool upper_hexa) {
    int written = 0;
    if(x == 0) {
        uart_putc('0');
        written ++;
    }
    else if (x < 0 && !unsign){
        uart_putc('-');
        written += 1 + uart_put_uint(1 + ~x, base, upper_hexa);
    }
    else written += uart_put_uint(x, base, upper_hexa);
    return written;
}

/* internal_uart_printf */
/* the last argument indicates whther or not there is a label */
int internal_printf(const char* format, va_list adpar, int label,void (*putc) (unsigned char), int (*puts)(const char*), int (*put_int)(int64_t, unsigned int, bool, bool)) {
    char buff[IO_BUFF_SIZE];
    int written = 0;
    int buff_index = 0;
    int i = 0;
    while(format[i]) {
        if(format[i] != '%' && !(format[i] == '\n' && format[i+1] != '\0' && label)) {
            buff[buff_index] = format[i];
            buff_index ++;
            if (buff_index == IO_BUFF_SIZE - 1) {
                buff[buff_index] = 0;
                puts (buff);
                buff_index = 0;
            }
            i++;
            continue;
        }
        if(buff_index > 0){
            assert(buff_index <= IO_BUFF_SIZE - 1);
            buff[buff_index] = '\0';
            written += puts(buff);
            buff_index = 0;
        }
        if(format[i] == '\n' && format[i+1] != '\0' && label) {
            written += puts("\n\t  ");
        }
        else {
            switch(format[i+1]) {
                case '%' :
                    putc('%');
                    break;
                case 'd':
                    written += put_int(va_arg(adpar, int), 10, 0, 0);
                    break;
                case 'X':
                    written += put_int(va_arg(adpar, int), 16, 0, 1);
                    break;
                case 'o':
                    written += put_int(va_arg(adpar, uint64_t), 8, 1, 0);
                    break;
                case 'u':
                    written += put_int(va_arg(adpar, uint64_t), 10, 1, 0);
                    break;
                case 'x':
                    written += put_int(va_arg(adpar, uint64_t), 16, 1, 0);
                    break;
                case 'b':
                    written += put_int(va_arg(adpar, uint64_t), 2, 1, 0);
                    break;
                case 'c':
                    putc(va_arg(adpar,unsigned int));
                    written ++;
                    break;
                case 's':
                    written += puts(va_arg(adpar, char*));
                    break;
                default:
                    /* TODO : set ERRNO ? */
                    return -1;
            }
            i ++;
        }
        i ++;
    }
    if(buff_index > 0){
        assert(buff_index < IO_BUFF_SIZE - 1);
        buff[buff_index] = '\0';
        written += puts(buff);
        buff_index = 0;
    }
    return written;
}

/* uart_printf :
 * Aims at mimicking the behaviour of the C printf but for GPIO
 * Doesn't yet support all the options : for now only conversion flags :
 * d,o,u,x,X,c,s,b%*/
int uart_printf(const char* format,...) {
    va_list adpar;
    va_start(adpar, format);
    int written = internal_printf(format, adpar, 0, uart_putc, uart_puts, uart_put_int);
    va_end(adpar);
    return written;
}

/* The return value is the number of char written, including the label */
int uart_verbose(const char* format __attribute__((__unused__)),...) {
    int written = 0;
#if LOG_LEVEL >= _LOG_VERBOSE_
    va_list adpar;
    va_start(adpar, format);
    uart_puts("[VERBOSE] ");
    written = internal_printf(format, adpar, 1, uart_putc, uart_puts, uart_put_int);
    va_end(adpar);
#endif
    return written;
}

int uart_debug(const char* format __attribute__((__unused__)),...) {
    int written = 0;
#if LOG_LEVEL >= _LOG_DEBUG_
    va_list adpar;
    va_start(adpar, format);
    uart_puts("[ DEBUG ] ");
    written = internal_printf(format, adpar, 1, uart_putc, uart_puts, uart_put_int);
    va_end(adpar);
#endif
    return written;
}

int uart_info(const char* format __attribute__((__unused__)),...) {
    int written = 0;
#if LOG_LEVEL >= _LOG_INFO_
    va_list adpar;
    va_start(adpar, format);
    uart_puts("[ INFO  ] ");
    written = internal_printf(format, adpar, 1, uart_putc, uart_puts, uart_put_int);
    va_end(adpar);
#endif
    return written;
}

int uart_warning(const char* format __attribute__((__unused__)),...) {
    int written = 0;
#if LOG_LEVEL >= _LOG_WARNING_
    va_list adpar;
    va_start(adpar, format);
    uart_puts("[WARNING] ");
    written = internal_printf(format, adpar, 1, uart_putc, uart_puts, uart_put_int);
    va_end(adpar);
#endif
    return written;
}

int uart_error(const char* format __attribute__((__unused__)),...) {
    int written = 0;
#if LOG_LEVEL >= _LOG_ERROR_
    va_list adpar;
    va_start(adpar, format);
    uart_puts("[ ERROR ] ");
    written = internal_printf(format, adpar, 1, uart_putc, uart_puts, uart_put_int);
    va_end(adpar);
#endif
    return written;
}

int uart_wtf(const char* format __attribute__((__unused__)),...) {
    int written = 0;
#if LOG_LEVEL >= LOG_WTF
    va_list adpar;
    va_start(adpar, format);
    uart_puts("[  WTF  ] ");
    written = internal_printf(format, adpar, 1, uart_putc, uart_puts, uart_put_int);
    va_end(adpar);
#endif
    return written;
}

void uart_simple_put_reg(uint64_t reg) {
    uart_printf("Reg : 0x%x\n",reg);
}

/* discard non read characters until finds new line */
/* returns the nb of discraded chars */
int uart_get_string(char * buff, size_t size){
    char c;
    size_t i = 0;
    uart_putc(c = uart_getc());
    /* Keyboard "Enter" sends a '\r' */
    while(c != '\r' && c != '\n' && i < size - 1){
        buff[i] = c;
        i ++;
        uart_putc(c = uart_getc());
    }
    buff[i + 1] = '\0';
    /* doesn't include '\n' */
    int non_copied_char = 0;
    while(c != '\r' && c != '\n'){
        non_copied_char ++;
        c = uart_getc();
    }
    return non_copied_char;
}
