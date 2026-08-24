#include <kernel/timer.h>                                 
#include <kernel/irq.h>                         
#include <asm/io.h>

#define PIT_OSC_FREQ  1193182u                            
#define PIT_CH0_DATA  0x40                             
#define PIT_CMD_REG   0x43             

static volatile u32 tick_count = 0;
static u32 ticks_per_second = 0;

static void timer_callback(struct registers *regs) {
    (void)regs;
    tick_count++;
}
