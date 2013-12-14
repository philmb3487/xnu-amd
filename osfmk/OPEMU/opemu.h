#pragma once

#include <stdint.h>
#include <mach/thread_state.h>

/** A saved state for the thread.
 ** This is needed because we support
 ** two different running modes.
 **/
struct op_saved_state {
	// one of either. order here matters.
	union {
		x86_saved_state64_t *state64;
		x86_saved_state32_t *state32;
	};

	enum {
		TYPE_64,
		TYPE_32,
	} type;
};
typedef struct op_saved_state op_saved_state_t;

/** XNU TRAP HANDLERS **/
void opemu_ktrap(x86_saved_state_t *state);
void opemu_utrap(x86_saved_state_t *state);

