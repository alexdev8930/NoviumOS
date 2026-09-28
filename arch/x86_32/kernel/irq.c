#include <asm/irq.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <novium/sched.h>
#include <novium/stdio.h>

/* programmable interrupt controller port and command definitions */
#define PIC1_CMD    0x20
#define PIC1_DATA   0x21
#define PIC2_CMD    0xA0
#define PIC2_DATA   0xA1
#define PIC_EOI     0x20
#define IRQ_BASE    0x20
#define IRQ_COUNT   16
#define INT_VECTORS 256

/* OCW3 command selects which register the command port returns on the next read */
#define PIC_OCW3_IRR    0x0A
#define PIC_OCW3_ISR    0x0B
#define PIC_IN_SERVICE  0x80


/* IDT entry tracks the handler address, selector, and gate attributes */
struct idt_entry {                        
    u16 offset_low;                      
    u16 selector;                         
    u8  zero;                           
    u8  type_attr;             
    u16 offset_high;                      
} __attribute__((packed));  

/* IDTR tells the location and size of the idt_entry array */
struct idtr {
    u16 limit;
    u32 base;
} __attribute__((packed));  

static struct idt_entry idt[INT_VECTORS];
static struct idtr idtp;
static irq_handler_t handlers[INT_VECTORS];
static u32 spurious_count[IRQ_COUNT];
static u32 spurious_reported = 0;

/* sets up an interrupt gate in the idt_entry */
static void idt_set_gate(int n, u32 fn) {  
    idt[n].offset_low  = fn & 0xFFFF;    
    idt[n].selector    = 0x10;
    idt[n].zero        = 0;              
    idt[n].type_attr   = 0x8E;             
    idt[n].offset_high = (fn >> 16) & 0xFFFF;
}       

/* fallback to prevent crashes */
__attribute__((naked)) static void stub_placeholder(void) { 
    __asm__ __volatile__("iret");           
}                

/* I mean like im not gonna write 48+ lines when i can just use a macro table */
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

/* init the idt_entry and load into cpu */
void idt_init(void) {
    int i;

    /* vectors 0-47, real stubs (exceptions + PIC lines) */
    for (i = 0; i < 48; i++) {
        idt_set_gate(i, (u32)stub_table[i]);
    }
    /* vectors 48-255, bare-iret placeholder */
    for (; i < INT_VECTORS; i++) {
        idt_set_gate(i, (u32)stub_placeholder);
    }

    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (u32)&idt;

    __asm__ __volatile__("lidt %0" : : "m"(idtp));
}

/* remap the PIC so hardware interrupts don't overwrite cpu exceptions */
void pic_init(void) {
    outb(PIC1_CMD, 0x11);
    outb(PIC2_CMD, 0x11);
    outb(PIC1_DATA, IRQ_BASE);
    outb(PIC2_DATA, IRQ_BASE + 8);
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    /* mask all interrupts by default until drivers request them */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

/* registers an interrupt handler and opens its hardware line on the PIC */
void irq_register(int irq, irq_handler_t h) {
    if (irq < 0 || irq >= IRQ_COUNT || !h) {
        return;
    }

    handlers[IRQ_BASE + irq] = h;

    u16 port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    outb(port, inb(port) & ~(u8)(1 << (irq & 7)));

    /* slave needs cascade line open too */
    if (irq >= 8) {                  
        outb(PIC1_DATA, inb(PIC1_DATA) & ~(u8)(1 << 2));
    }
}

/* unregisters a handler and mutes its hardware line on the PIC */
void irq_unregister(int irq) {
    if (irq < 0 || irq >= IRQ_COUNT) {
        return;
    }

    handlers[IRQ_BASE + irq] = 0;

    u16 port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    outb(port, inb(port) | (u8)(1 << (irq & 7)));
}

/* number of spurious interrupts seen on a line */
u32 irq_spurious_count(int irq) {
    if (irq < 0 || irq >= IRQ_COUNT) {
        return 0;
    }

    return spurious_count[irq];
}


/* handles cpu crashes, routes hardware signals, and resets the pic */
void isr_dispatch(struct registers *r) {
    u32 vec = r->int_no;

    if (vec < IRQ_BASE) {
        /* unhandled exception = crash out */
        kprintf("PANIC: unhandled exception, vector 0x%x, err_code 0x%x\n",
                vec, r->err_code);

        if (vec == 14) {
            /* cr2 has the bad address for page faults */
            u32 fault_addr;
            __asm__ __volatile__("mov %%cr2, %0" : "=r"(fault_addr));
            kprintf("  page fault at address 0x%x\n", fault_addr);
        }

        kprintf("halting\n");

        for (;;) {
            cpu_hlt();
        }
    }

    int line = vec - IRQ_BASE;

    /*
     * Spurious IRQ7/15 handling: If a hardware line drops prematurely, the 
     * 8259 PIC raises a ghost interrupt. We read the In-Service Register (ISR) 
     * to verify if the interrupt is real. If the ISR bit is clear, it's 
     * spurious. For IRQ15, we must still send an EOI to the master PIC only.
     */
    if (line == 7 || line == 15) {
        u16 port = (line == 7) ? PIC1_CMD : PIC2_CMD;

        outb(port, PIC_OCW3_ISR); /* read In-Service Register */
        u8 in_service = inb(port);
        
        outb(port, PIC_OCW3_IRR); /* reset sticky register to IRR */

        if (!(in_service & PIC_IN_SERVICE)) {
            spurious_count[line]++;

            if (line == 15) {
                outb(PIC1_CMD, PIC_EOI);
            }

            if ((spurious_reported & (1u << line)) == 0) {
                spurious_reported |= 1u << line;
                kprintf("WARNING: spurious IRQ %u (ISR 0x%x), check PIC/EOI\n",
                        (u32)line, (u32)in_service);
            }

            return;
        }
    }

    if (handlers[vec]) {
        handlers[vec](r);
    }

    /* tell the pic we are done */
    if (line >= 8) {
        outb(PIC2_CMD, PIC_EOI);
    }
    outb(PIC1_CMD, PIC_EOI);

    /*
     * Only now is it safe to act on a preemption the handler asked for. The
     * scheduler must not switch tasks while the line is still in service,
     * because the interrupted handler would be suspended with its eoi pending
     * and the controller would stop delivering interrupts.
     */
    SchedPreempt();
}

void irq_disable(void) {
    cpu_cli();
}

void irq_enable(void) {
    cpu_sti();
}
