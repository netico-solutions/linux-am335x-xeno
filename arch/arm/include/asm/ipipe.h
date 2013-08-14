/* -*- linux-c -*-
 * arch/arm/include/asm/ipipe.h
 *
 * Copyright (C) 2002-2005 Philippe Gerum.
 * Copyright (C) 2005 Stelian Pop.
 * Copyright (C) 2006-2008 Gilles Chanteperdrix.
 * Copyright (C) 2010 Philippe Gerum (SMP port).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, Inc., 675 Mass Ave, Cambridge MA 02139,
 * USA; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __ARM_IPIPE_H
#define __ARM_IPIPE_H

#ifdef CONFIG_IPIPE

#define BROKEN_BUILTIN_RETURN_ADDRESS
#undef __BUILTIN_RETURN_ADDRESS0
#define __BUILTIN_RETURN_ADDRESS0 arm_return_addr(0)
#undef __BUILTIN_RETURN_ADDRESS1
#define __BUILTIN_RETURN_ADDRESS1 arm_return_addr(1)
extern unsigned long arm_return_addr(int level);

#include <linux/ipipe_trace.h>
#include <mach/irqs.h>

#define IPIPE_CORE_RELEASE	4

struct ipipe_domain;

#define IPIPE_TSC_TYPE_NONE	   		0
#define IPIPE_TSC_TYPE_FREERUNNING 		1
#define IPIPE_TSC_TYPE_DECREMENTER 		2
#define IPIPE_TSC_TYPE_FREERUNNING_COUNTDOWN	3
#define IPIPE_TSC_TYPE_FREERUNNING_TWICE	4

/* tscinfo, exported to user-space */
struct __ipipe_tscinfo {
	unsigned type;
	unsigned freq;
	unsigned long counter_vaddr;
	union {
		struct {
			unsigned long counter_paddr;
			unsigned long long mask;
		};
		struct {
			unsigned *counter; /* Hw counter physical address */
			unsigned long long mask; /* Significant bits in the hw counter. */
			unsigned long long *tsc; /* 64 bits tsc value. */
		} fr;
		struct {
			unsigned *counter; /* Hw counter physical address */
			unsigned long long mask; /* Significant bits in the hw counter. */
			unsigned *last_cnt; /* Counter value when updating
						tsc value. */
			unsigned long long *tsc; /* 64 bits tsc value. */
		} dec;
	} u;
};

struct ipipe_arch_sysinfo {
	struct __ipipe_tscinfo tsc;
};


/* arch specific stuff */
extern char __ipipe_tsc_area[];
void __ipipe_mach_get_tscinfo(struct __ipipe_tscinfo *info);

#ifdef CONFIG_IPIPE_ARM_KUSER_TSC
unsigned long long __ipipe_tsc_get(void) __attribute__((long_call));
void __ipipe_tsc_register(struct __ipipe_tscinfo *info);
void __ipipe_tsc_update(void);
extern unsigned long __ipipe_kuser_tsc_freq;
#define __ipipe_mach_hrclock_freq __ipipe_kuser_tsc_freq
#else /* ! generic tsc */
unsigned long long __ipipe_mach_get_tsc(void);
#define __ipipe_tsc_get() __ipipe_mach_get_tsc()
#ifndef __ipipe_mach_hrclock_freq
#define __ipipe_mach_hrclock_freq __ipipe_hrtimer_freq
#endif /* !__ipipe_mach_hrclock_freq */
#endif /* ! generic tsc */

#ifndef __ipipe_cpu_freq
#define __ipipe_cpu_freq __ipipe_mach_hrclock_freq
#endif
#ifndef __ipipe_mach_doirq
#define __ipipe_mach_doirq(irq) __ipipe_do_IRQ
#endif
#ifndef __ipipe_mach_ackirq
#define __ipipe_mach_ackirq(irq) __ipipe_ack_irq
#endif
#ifndef __ipipe_mach_hrtimer_debug
#define __ipipe_mach_hrtimer_debug(irq) do { } while(0)
#endif

#ifdef CONFIG_IPIPE_WANT_PREEMPTIBLE_SWITCH

#define ipipe_mm_switch_protect(flags)		\
	do {					\
		(void)(flags);			\
	} while(0)

#define ipipe_mm_switch_unprotect(flags)	\
	do {					\
		(void)(flags);			\
	} while(0)

#else /* !CONFIG_IPIPE_WANT_PREEMPTIBLE_SWITCH */

#define ipipe_mm_switch_protect(flags) \
	flags = hard_cond_local_irq_save()

#define ipipe_mm_switch_unprotect(flags) \
	hard_cond_local_irq_restore(flags)

#endif /* !CONFIG_IPIPE_WANT_PREEMPTIBLE_SWITCH */

#define ipipe_read_tsc(t)	do { t = __ipipe_tsc_get(); } while(0)
#define __ipipe_read_timebase()	__ipipe_tsc_get()

#define ipipe_tsc2ns(t) \
({ \
	unsigned long long delta = (t)*1000; \
	do_div(delta, __ipipe_cpu_freq / 1000000 + 1); \
	(unsigned long)delta; \
})
#define ipipe_tsc2us(t) \
({ \
	unsigned long long delta = (t); \
	do_div(delta, __ipipe_cpu_freq / 1000000 + 1); \
	(unsigned long)delta; \
})

/* Private interface -- Internal use only */

#define __ipipe_enable_irq(irq)		enable_irq(irq)
#define __ipipe_disable_irq(irq)	disable_irq(irq)

/* PIC muting */
struct ipipe_mach_pic_muter {
	void (*enable_irqdesc)(struct ipipe_domain *ipd, unsigned irq);
	void (*disable_irqdesc)(struct ipipe_domain *ipd, unsigned irq);
	void (*mute)(void);
	void (*unmute)(void);
};

typedef unsigned long
__ipipe_irqbits_t[(NR_IRQS + BITS_PER_LONG - 1) / BITS_PER_LONG];

extern struct ipipe_mach_pic_muter ipipe_pic_muter;
extern __ipipe_irqbits_t __ipipe_irqbits;

void ipipe_pic_muter_register(struct ipipe_mach_pic_muter *muter);

void __ipipe_enable_irqdesc(struct ipipe_domain *ipd, unsigned irq);

void __ipipe_disable_irqdesc(struct ipipe_domain *ipd, unsigned irq);

static inline void ipipe_mute_pic(void)
{
	if (ipipe_pic_muter.mute)
		ipipe_pic_muter.mute();
}

static inline void ipipe_unmute_pic(void)
{
	if (ipipe_pic_muter.unmute)
		ipipe_pic_muter.unmute();
}

#define ipipe_notify_root_preemption() do { } while(0)

#ifdef CONFIG_SMP
void __ipipe_early_core_setup(void);
void __ipipe_hook_critical_ipi(struct ipipe_domain *ipd);
void __ipipe_root_ipi(unsigned int irq, void *cookie);
void __ipipe_root_localtimer(unsigned int irq, void *cookie);
void __ipipe_send_vnmi(void (*fn)(void *), cpumask_t cpumask, void *arg);
void __ipipe_do_vnmi(unsigned int irq, void *cookie);
void __ipipe_grab_ipi(unsigned svc, struct pt_regs *regs);
#else /* !CONFIG_SMP */
#define __ipipe_early_core_setup()	do { } while(0)
#define __ipipe_hook_critical_ipi(ipd)	do { } while(0)
#endif /* !CONFIG_SMP */
#ifndef __ipipe_mach_init_platform
#define __ipipe_mach_init_platform()	do { } while(0)
#endif

void __ipipe_enable_pipeline(void);

void __ipipe_do_critical_sync(unsigned irq, void *cookie);

void __ipipe_grab_irq(int irq, struct pt_regs *regs);

void __ipipe_exit_irq(struct pt_regs *regs);

static inline void ipipe_handle_multi_irq(int irq, struct pt_regs *regs)
{
	__ipipe_grab_irq(irq, regs);
}

#ifdef CONFIG_SMP
static inline void ipipe_handle_multi_ipi(int irq, struct pt_regs *regs)
{
	__ipipe_grab_ipi(irq, regs);
}
#endif /* CONFIG_SMP */

static inline unsigned long __ipipe_ffnz(unsigned long ul)
{
	return ffs(ul) - 1;
}

#define __ipipe_syscall_watched_p(p, sc)				\
	(ipipe_notifier_enabled_p(p) || (unsigned long)sc >= __ARM_NR_BASE + 64)

#define __ipipe_root_tick_p(regs) (!arch_irqs_disabled_flags(regs->ARM_cpsr))

#else /* !CONFIG_IPIPE */

#define __ipipe_tsc_update()	do { } while(0)

#define hard_smp_processor_id()		smp_processor_id()

#define ipipe_mm_switch_protect(flags) \
	do {					\
		(void) (flags);			\
	} while(0)

#define ipipe_mm_switch_unprotect(flags)	\
	do {					\
		(void) (flags);			\
	} while(0)

static inline void ipipe_handle_multi_irq(int irq, struct pt_regs *regs)
{
	handle_IRQ(irq, regs);
}

#ifdef CONFIG_SMP
static inline void ipipe_handle_multi_ipi(int irq, struct pt_regs *regs)
{
	handle_IPI(irq, regs);
}
#endif /* CONFIG_SMP */
#endif /* CONFIG_IPIPE */

#if defined (CONFIG_IPIPE_DEBUG) &&		\
	(defined(CONFIG_DEBUG_LL) || defined(CONFIG_SERIAL_8250_CONSOLE))
void __ipipe_serial_debug(const char *fmt, ...);
#else
#define __ipipe_serial_debug(fmt, args...)	do { } while (0)
#endif

#endif	/* !__ARM_IPIPE_H */
