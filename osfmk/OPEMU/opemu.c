/*   ** SINETEK **
 * This is called the Opcode Emulator: it traps invalid opcode exceptions
 *   and modifies the state of the running thread accordingly.
 * There are two entry points, one for user space exceptions, and another for
 *   exceptions coming from kernel space.
 *
 * STATUS
 *  . SSSE3 is implemented.
 *  . SYSENTER is implemented.
 *  . RDMSR is implemented.
 *
 */
#include <stdint.h>

#include "opemu.h"

/*
 * The KTRAP is only ever called from within the kernel,
 * and for now that is x86_64 only, so we simplify things,
 * and assume everything runs in x86_64.
 */
void opemu_ktrap(x86_saved_state_t *state)
{
	x86_saved_state64_t *saved_state = saved_state64(state);
	const uint8_t *code_buffer = (const uint8_t*) saved_state->isf.rip;
	uint8_t bytes_skip = 0;

	ud_t ud_obj;		// disassembler object
	op_t op_obj;
	

	ud_init(&ud_obj);
	ud_set_input_buffer(&ud_obj, code_buffer, 15);	// TODO dangerous
	ud_set_mode(&ud_obj, 64);
	ud_set_syntax(&ud_obj, UD_SYN_INTEL);
	ud_set_vendor(&ud_obj, UD_VENDOR_ANY);

	bytes_skip = ud_disassemble(&ud_obj);
	if ( bytes_skip == 0 ) goto bad;
	const uint32_t mnemonic = ud_insn_mnemonic(&ud_obj);

	/* since this is ring0, it could be an invalid MSR read.
	 * Instead of crashing the whole machine, report on it and keep running. */
	if (mnemonic == UD_Irdmsr) {
		printf("[MSR] unknown location 0x%016llx\r\n", saved_state->rcx);
		// best we can do is return 0;
		saved_state->rdx = saved_state->rax = 0;
		goto cleanexit;
	}

	/* Else run the full-blown opemu */

	int error = 0;

	// fill in the opemu object
	op_obj.state64 = saved_state;
	op_obj.state_flavor = SAVEDSTATE_64;
	op_obj.ud_obj = &ud_obj;

	error |= op_sse3x_run(op_obj);

	if (!error) goto cleanexit;

	/** fallthru **/
bad:
	{
		/* Well, now go in and get the asm text at least.. */
		const char *instruction_asm;
		instruction_asm = ud_insn_asm(&ud_obj);

		printf ( "OPEMU:  %s\n", instruction_asm) ;
		panic ( );
	}

cleanexit:
	saved_state->isf.rip += bytes_skip;
}

void opemu_utrap(x86_saved_state_t *state)
{
}


