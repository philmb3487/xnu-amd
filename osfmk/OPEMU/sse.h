#pragma once

#include <stdint.h>
#include "opemu.h"

#include "libudis86/types.h"

/*
 * 128-bit Register proper for SSE-n
 * For 64-bit operations, use the same register type, and ignore the high values
 */
union __attribute__((__packed__)) sse_reg {
int8_t		int8[16];
int16_t		int16[8];
int32_t		int32[4];
int64_t		int64[2];
__int128_t	int128;
uint8_t		uint8[16];
uint16_t	uint16[8];
uint32_t	uint32[4];
uint64_t	uint64[2];
__uint128_t	uint128;
};
typedef union sse_reg sse_reg_t;

/*
 * Print said register to screen. Useful for debugging
 */
#define print128(x)	printf("0x%016llx%016llx", ((uint64_t*)(&(x)))[1], ((uint64_t*)(&(x)))[0] );

/*
 * SSEEMU Context
 */
struct op_ctx_sse {
	uint8_t	extended;	// bool type

	sse_reg_t		dst, src;
	sse_reg_t		res;

	// operands

	const ud_operand_t	*operand_source;
	const ud_operand_t	*operand_destination;
	const ud_operand_t	*operand_immediate;
};
typedef struct op_ctx_sse op_ctx_sse_t;

/*
 * Instruction emulation function type.
 */
typedef void (*op_sse_function_t)(op_ctx_sse_t*);

/*
 * Read the operands from memory, store them in ctx
 */
void _store_xmm (uint8_t, uint8_t *);
void _load_xmm (uint8_t n, const uint8_t *where);
int sse3x_grab_operands(op_ctx_sse_t*, op_saved_state_t*);
void sse3x_commit_results(op_ctx_sse_t*);

/******************************************/
/** SSSE3 instruction set implementation **/
/******************************************/

/* Main 'run' functions */
int op_sse3x_run(ud_t*, op_saved_state_t*);

/** All 32 SSSE3 instructions            **/

void psignb	(op_ctx_sse_t*);
void psignw	(op_ctx_sse_t*);
void psignd	(op_ctx_sse_t*);
void pabsb	(op_ctx_sse_t*);
void pabsw	(op_ctx_sse_t*);
void pabsd	(op_ctx_sse_t*);
void palignr	(op_ctx_sse_t*);
void pshufb	(op_ctx_sse_t*);
void pmulhrsw	(op_ctx_sse_t*);
void pmaddubsw	(op_ctx_sse_t*);
void phsubw	(op_ctx_sse_t*);
void phsubd	(op_ctx_sse_t*);
void phsubsw	(op_ctx_sse_t*);
void phaddw	(op_ctx_sse_t*);
void phaddd	(op_ctx_sse_t*);
void phaddsw	(op_ctx_sse_t*);


