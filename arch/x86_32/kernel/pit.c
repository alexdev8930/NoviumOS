#include <novium/timer.h>
#include <novium/sched.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/cpu.h>

#define PIT_OSC_FREQ   1193182u
#define PIT_CH0_DATA   0x40
#define PIT_CMD_REG    0x43
#define PIT_CMD_SQUARE 0x36

static volatile u32 tick_count = 0;
static u32 ticks_per_second = 0;

static void timer_callback(struct registers *regs) {
    tick_count++;
    SchedTick(regs);
}

void timer_init(int hz) {
    if (hz <= 0) {
        return;
    }
    u32 divisor = PIT_OSC_FREQ / (u32)hz;
    outb(PIT_CMD_REG, PIT_CMD_SQUARE);
    io_wait();
    outb(PIT_CH0_DATA, (u8)(divisor & 0xFF));
    io_wait();
    outb(PIT_CH0_DATA, (u8)((divisor >> 8) & 0xFF));
    ticks_per_second = (u32)hz;
    irq_register(0, timer_callback);
}

u32 timer_get_ticks(void) {
    return tick_count;
}

u64 timer_uptime_ms(void) {
    if (ticks_per_second == 0) {
        return 0;
    }
    u32 secs = tick_count / ticks_per_second;
    u32 rem  = tick_count % ticks_per_second;
    u32 frac = rem * 1000 / ticks_per_second;  
    u64 ms = (u64)secs * 1000ULL + frac;        
    return ms;
}

void timer_sleep_ms(u32 ms) {
    if (ticks_per_second == 0) {
        return;
    }
    u32 start = tick_count;
    u32 secs = ms / 1000U;
    u32 rem  = ms % 1000U;
    u32 wait_ticks = secs * ticks_per_second    
                   + rem * ticks_per_second / 1000U;
    while ((tick_count - start) < wait_ticks) {
        cpu_hlt();
    }
}
