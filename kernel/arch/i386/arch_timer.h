#ifndef _ARCH_TIMER_H
#define _ARCH_TIMER_H

#include <stage2.h>

int apic_timer_interrupt();

void arch_timer_set_hardware_timer(time_t timeout);
void arch_timer_clear_hardware_timer();
int arch_init_timer(kernel_args *ka);

#endif

