#ifndef _DEBUG_H
#define _DEBUG_H

#include <types.h>
#include "stage2.h"

// architecture independant
int dbg_init(struct kernel_args *ka);
int dbg_init2(struct kernel_args *ka);
char dbg_putch(char c);
void dbg_puts(const char *s);
bool dbg_set_serial_debug(bool new_val);
int dprintf(const char *fmt, ...);
int panic(const char *fmt, ...);
void kernel_debugger();
int dbg_add_command(void (*func)(int, char **), const char *cmd, const char *desc);

#endif

