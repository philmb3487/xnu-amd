#pragma once

#include <stdint.h>
#include <kern/sched_prim.h>

struct op_saved_state {
	// one of either. order here matters.
	union {
		x86_saved_state64_t *state64;
		x86_saved_state32_t *state32;
	};

	enum {
		SAVEDSTATE_64,
		SAVEDSTATE_32,
	} type;
};
typedef struct op_saved_state op_saved_state_t;

/** XNU TRAP HANDLERS **/
void opemu_ktrap(x86_saved_state_t *state);
void opemu_utrap(x86_saved_state_t *state);


/**
 * Forward declarations of private xnu functions
 */
extern void mach_call_munger(x86_saved_state_t *state);
extern void unix_syscall(x86_saved_state_t *);


