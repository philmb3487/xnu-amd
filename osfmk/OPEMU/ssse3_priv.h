#pragma once

#include "opemu.h"
#include "libudis86/extern.h"

// log function debug
#define LF	D("%s\n", __PRETTY_FUNCTION__);
#define D	printf

/**
 * 128-bit Register proper for ssse3
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

/**
 * Print said register to screen. Useful for debugging
 */
#define print128(x)	printf("0x%016llx%016llx", ((uint64_t*)(&(x)))[1], ((uint64_t*)(&(x)))[0] );



/**
 * ssse3 object
 */
struct ssse3 {
	uint8_t	extended;	// bool type

	sse_reg_t		dst, src;
	sse_reg_t		res;

	// operands
	const ud_operand_t	*udo_src, *udo_dst, *udo_imm;

	// objects
	const op_t		*op_obj;
};
typedef struct ssse3 ssse3_t;



/**
 * Instruction emulation function type.
 */
typedef void (*ssse3_func)(ssse3_t*);


#define movdqu_template(n, where)					\
	do {								\
	asm __volatile__ ("movdqu %%xmm" #n ", %0" : "=m" (*(where)));	\
	} while (0); break;

/**
 * Store xmm register somewhere in memory
 */
void _store_xmm (const uint8_t n, uint8_t *where)
{
	switch (n) {
case 0:  movdqu_template(0, where);
case 1:  movdqu_template(1, where);
case 2:  movdqu_template(2, where);
case 3:  movdqu_template(3, where);
case 4:  movdqu_template(4, where);
case 5:  movdqu_template(5, where);
case 6:  movdqu_template(6, where);
case 7:  movdqu_template(7, where);
case 8:  movdqu_template(8, where);
case 9:  movdqu_template(9, where);
case 10: movdqu_template(10, where);
case 11: movdqu_template(11, where);
case 12: movdqu_template(12, where);
case 13: movdqu_template(13, where);
case 14: movdqu_template(14, where);
case 15: movdqu_template(15, where);
}}

/**
 * Load xmm register from memory
 */
void _load_xmm (const uint8_t n, const uint8_t *where)
{
	switch (n) {
case 0:  movdqu_template(0, where);
case 1:  movdqu_template(1, where);
case 2:  movdqu_template(2, where);
case 3:  movdqu_template(3, where);
case 4:  movdqu_template(4, where);
case 5:  movdqu_template(5, where);
case 6:  movdqu_template(6, where);
case 7:  movdqu_template(7, where);
case 8:  movdqu_template(8, where);
case 9:  movdqu_template(9, where);
case 10: movdqu_template(10, where);
case 11: movdqu_template(11, where);
case 12: movdqu_template(12, where);
case 13: movdqu_template(13, where);
case 14: movdqu_template(14, where);
case 15: movdqu_template(15, where);
}}

int ssse3_grab_operands(ssse3_t*);
int ssse3_commit_results(const ssse3_t*);
int op_sse3x_run(const op_t*);

void psignb	(ssse3_t*);
void psignw	(ssse3_t*);
void psignd	(ssse3_t*);
void pabsb	(ssse3_t*);
void pabsw	(ssse3_t*);
void pabsd	(ssse3_t*);
void palignr	(ssse3_t*);
void pshufb	(ssse3_t*);
void pmulhrsw	(ssse3_t*);
void pmaddubsw	(ssse3_t*);
void phsubw	(ssse3_t*);
void phsubd	(ssse3_t*);
void phsubsw	(ssse3_t*);
void phaddw	(ssse3_t*);
void phaddd	(ssse3_t*);
void phaddsw	(ssse3_t*);


