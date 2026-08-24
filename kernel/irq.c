#include "irq.h"
#include "debug.h"
#include <asm/io.h>
#include <drivers/novium/console.h>

// programmable interrupt controller port and command definitions
#define PIC1_CMD    0x20
#define PIC1_DATA   0x21
#define PIC2_CMD    0xA0
#define PIC2_DATA   0xA1
#define PIC_EOI     0x20


// IDT entry tracks the handler address, selector, and gate attributes
struct idt_entry {                        
    u16 offset_low;                      
    u16 selector;                         
    u8  zero;                           
    u8  type_attr;             
    u16 offset_high;                      
} __attribute__((packed));  

// IDTR tells the location and size of the idt_entry array
struct idtr {
    u16 limit;
    u32 base;
} __attribute__((packed));  

static struct idt_entry idt[INT_VECTORS];
static struct idtr idtp;
static irq_handler_t handlers[INT_VECTORS];

// sets up an interrupt gate in the idt_entry
static void idt_set_gate(int n, u32 fn) {  
    idt[n].offset_low  = fn & 0xFFFF;    
    idt[n].selector    = 0x08;       
    idt[n].zero        = 0;              
    idt[n].type_attr   = 0x8E;             
    idt[n].offset_high = (fn >> 16) & 0xFFFF;
}       

// fallback to prevent crashes
__attribute__((naked)) static void stub_placeholder(void) { 
    __asm__ __volatile__("iret");           
}                

// I mean like im not gonna write 48+ lines when i can just use a macro table
#define FOR_EACH_STUB(X) \
    X(0)  X(1)  X(2)  X(3)  X(4)  X(5)  X(6)  X(7)   \
    X(8)  X(9)  X(10) X(11) X(12) X(13) X(14) X(15)  \
    X(16) X(17) X(18) X(19) X(20) X(21) X(22) X(23)  \
    X(24) X(25) X(26) X(27) X(28) X(29) X(30) X(31)  \
    X(32) X(33) X(34) X(35) X(36) X(37) X(38) X(39)  \
    X(40) X(41) X(42) X(43) X(44) X(45) X(46) X(47)

#define DECLARE(n) extern void isr_stub_##n(void);
FOR_EACH_STUB(DECLARE)
#undef DECLARE

static void (*const stub_table[48])(void) = {
#define ENTRY(n) isr_stub_##n,
    FOR_EACH_STUB(ENTRY)
#undef ENTRY
};

// init the idt_entry and load into cpu
void idt_init(void) {
    int i;

    // vectors 0-47, real stubs (exceptions + PIC lines) 
    for (i = 0; i < 48; i++) {
        idt_set_gate(i, (u32)stub_table[i]);
    }
    // vectors 48-255, bare-iret placeholder 
    for (; i < INT_VECTORS; i++) {
        idt_set_gate(i, (u32)stub_placeholder);
    }

    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (u32)&idt;

    __asm__ __volatile__("lidt %0" : : "m"(idtp));
}

// remap the PIC so hardware interrupts don't overwrite cpu exceptions
void pic_init(void) {
    outb(PIC1_CMD, 0x11);
    outb(PIC2_CMD, 0x11);
    outb(PIC1_DATA, IRQ_BASE);
    outb(PIC2_DATA, IRQ_BASE + 8);
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    // mask all interrupts by default until drivers request them
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

// registers an interrupt handler and opens its hardware line on the PIC
void irq_register(int irq, irq_handler_t h) {
    if (irq < 0 || irq >= IRQ_COUNT || !h) {
        return;
    }

    handlers[IRQ_BASE + irq] = h;

    u16 port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    outb(port, inb(port) & ~(u8)(1 << (irq & 7)));

    // slave needs cascade line open too
    if (irq >= 8) {                  
        outb(PIC1_DATA, inb(PIC1_DATA) & ~(u8)(1 << 2));
    }
}

// unregisters a handler and mutes its hardware line on the PIC
void irq_unregister(int irq) {
    if (irq < 0 || irq >= IRQ_COUNT) {
        return;
    }

    handlers[IRQ_BASE + irq] = 0;

    u16 port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    outb(port, inb(port) | (u8)(1 << (irq & 7)));
}



// handles cpu crashes, routes hardware signals, and resets the pic
void isr_dispatch(struct registers *r) {
    u32 vec = r->int_no;

    if (vec < IRQ_BASE) {
        // unhandled exception = crash out
        console_write("PANIC: unhandled exception, vector ");
        debug_print_hex32(vec);
        console_write(", err_code ");
        debug_print_hex32(r->err_code);

        if (vec == 14) {
            // cr2 has the bad address for page faults
            u32 fault_addr;
            __asm__ __volatile__("mov %%cr2, %0" : "=r"(fault_addr));
            console_write("\n  page fault at address: ");
            debug_print_hex32(fault_addr);
        }

        console_write("\nhalting\n");
        for (;;) {
            __asm__ __volatile__("hlt");
        }
    }

    int line = vec - IRQ_BASE;

    // ignore spurious irq7/15 floods
    if (line == 7 || line == 15) {
        u16 port = (line == 7) ? PIC1_CMD : PIC2_CMD;
        outb(port, 0x0B);
        if (!(inb(port) & 0x80)) {
            if (line == 15) {
                outb(PIC1_CMD, PIC_EOI);   
            }
            return;
        }
    }

    if (handlers[vec]) {
        handlers[vec](r);
    }

    // tell the pic we are done
    if (line >= 8) {
        outb(PIC2_CMD, PIC_EOI);
    }
    outb(PIC1_CMD, PIC_EOI);
}

void irq_disable(void) {
    __asm__ __volatile__("cli");
}

void irq_enable(void) {
    __asm__ __volatile__("sti");
}
