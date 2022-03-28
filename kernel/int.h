#ifndef _INT_H
#define _INT_H

#include "stage2.h"

int int_init(struct kernel_args *ka);
int int_init2(struct kernel_args *ka);

#define _INT_ARCH_INLINE_CODE 1
#if _INT_ARCH_INLINE_CODE
#include "arch_int.h"
#define int_enable_interrupts	arch_int_enable_interrupts
#define int_disable_interrupts	arch_int_disable_interrupts
#define int_restore_interrupts	arch_int_restore_interrupts
#else
void int_enable_interrupts();
int int_disable_interrupts();
void int_restore_interrupts(int oldstate);
#endif

#endif

