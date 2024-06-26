// Huge sections stolen from OpenBLT
// Will be redone later

#include <stage2.h>
#include <kernel.h>
#include <console.h>
#include <debug.h>
#include <vm.h>
#include <kernel.h>
#include <printf.h>
#include <string.h>
#include <int.h>
#include <smp.h>

#include <arch_cpu.h>
#include <arch_vm.h>
#include <arch_timer.h>
#include <arch_smp.h>
#include <arch_smp_priv.h>

static int num_cpus = 1;
static unsigned int *apic = NULL;
static unsigned int cpu_apic_id[SMP_MAX_CPUS] = { 0, 0};
static unsigned int cpu_os_id[SMP_MAX_CPUS] = { 0, 0};
static unsigned int cpu_apic_version[SMP_MAX_CPUS] = { 0, 0};
static unsigned int *ioapic = NULL; 
static unsigned int apic_timer_tics_per_sec = 0;

static int i386_timer_interrupt()
{
	arch_smp_ack_interrupt();
	
	return apic_timer_interrupt();
}

static int i386_ici_interrupt()
{
	// gin-u-wine inter-cpu interrupt
//	dprintf("inter-cpu interrupt on cpu %d\n", arch_smp_get_current_cpu());
	arch_smp_ack_interrupt();

	return smp_intercpu_int_handler();
}

static int i386_spurious_interrupt()
{
	// spurious interrupt
//	dprintf("spurious interrupt on cpu %d\n", arch_smp_get_current_cpu());
	arch_smp_ack_interrupt();
	return INT_NO_RESCHEDULE;
}

static int i386_smp_error_interrupt()
{
	// smp error interrupt
//	dprintf("smp error interrupt on cpu %d\n", arch_smp_get_current_cpu());
	arch_smp_ack_interrupt();
	return INT_NO_RESCHEDULE;
}

static unsigned int apic_read(unsigned int *addr)
{
	return *addr;
}

static void apic_write(unsigned int *addr, unsigned int data)
{
	*addr = data;
}

int arch_smp_init(kernel_args *ka)
{
	dprintf("arch_smp_init: entry\n");

	if(ka->num_cpus > 1) {
		// setup some globals
		num_cpus = ka->num_cpus;
		apic = ka->apic;
		ioapic = ka->ioapic;
		memcpy(cpu_apic_id, ka->cpu_apic_id, sizeof(ka->cpu_apic_id));
		memcpy(cpu_os_id, ka->cpu_os_id, sizeof(ka->cpu_os_id));
		memcpy(cpu_apic_version, ka->cpu_apic_version, sizeof(ka->cpu_apic_version));
		apic_timer_tics_per_sec = ka->apic_time_cv_factor;
	
		// setup areas that represent the apic & ioapic
		vm_create_area(vm_get_kernel_aspace(), "local_apic", (void *)&apic, AREA_ALREADY_MAPPED, PAGE_SIZE, 0);
		vm_create_area(vm_get_kernel_aspace(), "ioapic", (void *)&ioapic, AREA_ALREADY_MAPPED, PAGE_SIZE, 0);

		int_set_io_interrupt_handler(0xfb, &i386_timer_interrupt);
		int_set_io_interrupt_handler(0xfd, &i386_ici_interrupt);
		int_set_io_interrupt_handler(0xfe, &i386_smp_error_interrupt);
		int_set_io_interrupt_handler(0xff, &i386_spurious_interrupt);
	} else {
		num_cpus = 1;
	}
	return 0;
}

void arch_smp_send_broadcast_ici()
{
	int config;
	int state;
	
	state = int_disable_interrupts();
	
	config = apic_read(APIC_ICR1) & APIC_ICR1_WRITE_MASK;
	apic_write(APIC_ICR1, config | 0xfd | APIC_ICR1_DELMODE_FIXED | APIC_ICR1_DESTMODE_PHYS | APIC_ICR1_DEST_ALL_BUT_SELF);

	int_restore_interrupts(state);
}

void arch_smp_send_ici(int target_cpu)
{
	int config;
	int state;
	
	state = int_disable_interrupts();
	
	config = apic_read(APIC_ICR2) & APIC_ICR2_MASK;
	apic_write(APIC_ICR2, config | cpu_apic_id[target_cpu] << 24);

	config = apic_read(APIC_ICR1) & APIC_ICR1_WRITE_MASK;
	apic_write(APIC_ICR1, config | 0xfd | APIC_ICR1_DELMODE_FIXED | APIC_ICR1_DESTMODE_PHYS | APIC_ICR1_DEST_FIELD);

	int_restore_interrupts(state);
}

int arch_smp_get_current_cpu()
{
	if(apic == NULL)
		return 0;
	else
		return cpu_os_id[(apic_read(APIC_ID) & 0xffffffff) >> 24];
}

void arch_smp_ack_interrupt()
{
	apic_write(APIC_EOI, 0);
}

#define MIN_TIMEOUT 1000

int arch_smp_set_apic_timer(time_t relative_timeout)
{
	unsigned int config;
	int state;
	unsigned int ticks;

	if(apic == NULL)
		return -1;

	if(relative_timeout < MIN_TIMEOUT)
		relative_timeout = MIN_TIMEOUT;

	// calculation should be ok, since it's going to be 64-bit
	ticks = ((relative_timeout * apic_timer_tics_per_sec) / 1000000);

	state = int_disable_interrupts();

	config = apic_read(APIC_LVTT) | APIC_LVTT_M; // mask the timer
	apic_write(APIC_LVTT, config);
	
	apic_write(APIC_ICRT, 0); // zero out the timer
	
	config = apic_read(APIC_LVTT) & ~APIC_LVTT_M; // unmask the timer
	apic_write(APIC_LVTT, config);

	apic_write(APIC_ICRT, ticks); // start it up

	int_restore_interrupts(state);

	return 0;
}

int arch_smp_clear_apic_timer()
{
	unsigned int config;
	int state;
	
	if(apic == NULL)
		return -1;

	state = int_disable_interrupts();

	config = apic_read(APIC_LVTT) | APIC_LVTT_M; // mask the timer
	apic_write(APIC_LVTT, config);
	
	apic_write(APIC_ICRT, 0); // zero out the timer

	int_restore_interrupts(state);

	return 0;
}

