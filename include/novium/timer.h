#ifndef NOVIUM_TIMER_H                                     
#define NOVIUM_TIMER_H                                   

#include <novium/types.h>                                 

void timer_init(int hz);                                   
u32  timer_get_ticks(void);                              
u64  timer_uptime_ms(void);                               
void timer_sleep_ms(u32 ms);                             

#endif                                                   
