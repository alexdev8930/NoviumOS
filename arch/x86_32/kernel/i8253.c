#include <novium/timer.h>
#include <novium/sched.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/cpu.h>

#define PIT_OSC_FREQ   1193182u
#define PIT_CH0_DATA   0x40
#define PIT_CMD_REG    0x43
#define PIT_CMD_SQUARE 0x36
#define PIT_MAX_DIVISOR 65535u

static volatile u32 tick_count = 0;
static u32 ticks_per_second = 0;

static void timer_callback(struct registers *regs) {
    tick_count++;
    SchedTick(regs);
    SchedWakeExpired(tick_count);   /* wake any expired sleepers */
}

void timer_init(int hz) {
    if (hz <= 0) {
        return;
    }

    /* 16-bit reload, and a divisor of 0 means 65536 */
    u32 divisor = PIT_OSC_FREQ / (u32)hz;

    if (divisor == 0) {
        divisor = 1;
    } else if (divisor > PIT_MAX_DIVISOR) {
        divisor = PIT_MAX_DIVISOR;
    }

    outb(PIT_CMD_REG, PIT_CMD_SQUARE);
    io_wait();
    outb(PIT_CH0_DATA, (u8)(divisor & 0xFF));
    io_wait();
    outb(PIT_CH0_DATA, (u8)((divisor >> 8) & 0xFF));

    /* the real rate, not the requested one */
    ticks_per_second = PIT_OSC_FREQ / divisor;
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

/* sleeps for at least ms milliseconds, blocking until the timer wakes us */
void timer_sleep_ms(u32 ms) {
    if (ticks_per_second == 0 || ms == 0) {
        return;
    }

    if (!cpu_irq_enabled()) {
        return;   /* no timer irq would arrive, so we would halt forever */
    }

    /* split the seconds off first so no 64-bit division is needed, and round up */
    u32 secs = ms / 1000u;
    u32 rem  = ms % 1000u;
    u32 deadline = tick_count + secs * ticks_per_second
                 + (rem * ticks_per_second + 999u) / 1000u;

    Task *Self = SchedCurrent();

    if (Self == 0 || Self->Id == 0) {
        /* idle task, so yield and halt instead of blocking */
        while ((s32)(tick_count - deadline) < 0) {
            SchedYield();
            cpu_hlt();
        }
        return;
    }

    if (!SchedHasRunnable()) {
        /* nothing else to run, so there is nothing to switch to */
        while ((s32)(tick_count - deadline) < 0) {
            cpu_hlt();
        }
        return;
    }

    SchedSleepUntil(deadline);
}
