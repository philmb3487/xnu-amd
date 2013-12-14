#pragma once

#include <stdint.h>
#include <kern/sched_prim.h>
#include "libudis86/extern.h"

struct op {
	// one of either. order here matters.
	union {
		x86_saved_state64_t *state64;
		x86_saved_state32_t *state32;
	};

	enum {
		SAVEDSTATE_64,
		SAVEDSTATE_32,
	} state_flavor;

	// disassembly object
	ud_t		*ud_obj;
};
typedef struct op op_t;

/**
 * Trap handlers, analogous to a program's entry point
 * @param state: xnu's trap.c saved thread state
 */
void opemu_ktrap(x86_saved_state_t *state);
void opemu_utrap(x86_saved_state_t *state);


/**
 * Forward declarations of private xnu functions
 */
extern void mach_call_munger(x86_saved_state_t *state);
extern void unix_syscall(x86_saved_state_t *);


