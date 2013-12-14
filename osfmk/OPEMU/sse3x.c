#include <stdint.h>
#include <mach/thread_status.h>

#include "sse.h"
#include "libudis86/types.h"

// log function debug
#define LF	D("%s\n", __PRETTY_FUNCTION__);
//#define D	printf
#define D	//

/*
 * Run an intruction. Returns non-zero on failure.
 */
int op_sse3x_run(ud_t *ud_obj, op_saved_state_t *saved_state)
{
	op_ctx_sse_t	ctx;
	const uint32_t mnemonic = ud_insn_mnemonic(ud_obj);
	op_sse_function_t	opf	= NULL;

	switch (mnemonic) {
		case UD_Ipsignb:	opf = psignb;
//		case UD_Ipsignw:	opf = psignw;
//		case UD_Ipsignd:	opf = psignd;
//
//		case UD_Ipabsb:		opf = pabsb;
//		case UD_Ipabsw:		opf = pabsw;
//		case UD_Ipabsd:		opf = pabsd;
//
		case UD_Ipalignr:	opf = palignr;

		case UD_Ipshufb:	opf = pshufb;
//
//		case UD_Ipmulhrsw:	opf = pmulhrsw;
//
//		case UD_Ipmaddubsw:	opf = pmaddubsw;
//
//		case UD_Iphsubw:	opf = phsubw;
//		case UD_Iphsubd:	opf = phsubd;
//
//		case UD_Iphsubsw:	opf = phsubsw;
//
//		case UD_Iphaddw:	opf = phaddw;
//		case UD_Iphaddd:	opf = phaddd;
//
//		case UD_Iphaddsw:	opf = phaddsw;
sse3x_common:
		
		ctx.operand_source = ud_insn_opr (ud_obj, 1);
		ctx.operand_destination = ud_insn_opr (ud_obj, 0);
		ctx.operand_immediate = ud_insn_opr (ud_obj, 2);

	// TODO this is wrong
		ctx.extended = saved_state->type == TYPE_64;

		sse3x_grab_operands(&ctx, saved_state);

		opf(&ctx);

		/** push back in the results **/

    sse_store();
		if () {
		} else {
		}

		goto good;

		/** fallthru **/
		default: goto bad;
	}

bad:
	return -1;

good:
	return 0;
}

#define movdqu_template(n, where)					\
	do {								\
	asm __volatile__ ("movdqu %%xmm" #n ", %0" : "=m" (*(where)));	\
	} while (0); break;

/** Store xmm register somewhere in memory.
 **/
void _store_xmm (uint8_t n, uint8_t *where)
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

/** Load xmm register from memory.
 **/
void _load_xmm (uint8_t n, const uint8_t *where)
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


/*
 * Read the operands from memory, store them in ctx
 */
int sse3x_grab_operands(op_ctx_sse_t *ctx, op_saved_state_t *sp)
{
	// sanity check on operands,
	// dest has to be either XMM or MM
	// src can be one of XMM, MM, MEM

	if (ctx->operand_destination->type != UD_OP_REG) goto bad;
	if ((ctx->operand_source->type != UD_OP_REG) && (ctx->operand_source->type != UD_OP_MEM)) goto bad;	

	if (ctx->extended) {

		_store_xmm(ctx->operand_destination->base - UD_R_XMM0, (void*) &ctx->dst.uint128);
		if (ctx->operand_source->type == UD_OP_REG)
			_store_xmm(ctx->operand_source->base - UD_R_XMM0, (void*) &ctx->src.uint128);
		else {
			/* memory load */

			switch (ctx->operand_source->base) {
			case UD_R_RSP:
				if (ctx->operand_source->scale > 0) goto bad;

				__uint128_t *srcp = (void*) sp->state64->isf.rsp + ctx->operand_source->lval.sword;
				ctx->src.uint128 = *srcp;
				break;

			default: goto bad;
			}
		}

	} else {
	}

	goto good;

bad:
	return -1;
good:
	return 0;
}

/*
 * Take the results stored inside the context, and commit them
 * to machine registers.
 */
void sse3x_commit_results(op_ctx_sse_t*)
{}

// TODO document this
#define vector_block(context, count, block, stepblock, type)		\
        do {								\
		const type *src = & (context)-> src.uint8[0];		\
		const type *dst = & (context)-> dst.uint8[0];		\
		      type *res = & (context)-> res.uint8[0];		\
									\
		switch ((count)) {					\
case 16:	(block); (stepblock);					\
		(block); (stepblock);					\
		(block); (stepblock);					\
		(block); (stepblock);					\
		(block); (stepblock);					\
		(block); (stepblock);					\
		(block); (stepblock);					\
		(block); (stepblock);					\
case 8:		(block); (stepblock);					\
		(block); (stepblock);					\
		(block); (stepblock);					\
		(block); (stepblock);					\
case 4:		(block); (stepblock);					\
		(block); (stepblock);					\
case 2:		(block); (stepblock);					\
		(block); (stepblock);					\
		}							\
        } while (0);

// TODO document this
#define vector_block_for_byte(context, count, block)			\
	do {								\
		vector_block ((context), (count), (block), {		\
			++res; ++dst; ++src;				\
		}, uint8_t);						\
	} while (0);							\


/******************************************/
/** SSSE3 instruction set implementation **/
/******************************************/
//// TODO  unrolling?? inlining??

#define SATSW(x) ((x > 32767)? 32767 : ((x < -32768)? -32768 : x) )

/*
	Negate/zero/preserve packed byte integers
 */
void psignb	(op_ctx_sse_t	*ctx)
{
	LF

	vector_block_for_byte (ctx, (ctx->extended) ? 16 : 8, {
		if ( (signed) *src < 0 ) *res = - *dst;
		else if ( *src == 0 ) *res = 0;
		else *res = *dst;
	});
}

/*
	Negate/zero/preserve packed word integers
 */
void psignw	(op_ctx_sse_t	*ctx)
{
	LF


}

/*
	Negate/zero/preserve packed doubleword
 */
void psignd	(op_ctx_sse_t	*ctx)
{
	LF
}

/*
	Compute the absolute value of bytes
 */
void pabsb	(op_ctx_sse_t	*ctx)
{
	LF
}


void pabsw	(op_ctx_sse_t	*ctx);
void pabsd	(op_ctx_sse_t	*ctx);

/*
	Concatenate destination and source operands,
	extract byte-aligned result shifted to the right by constant value
 */
void palignr	(op_ctx_sse_t	*ctx)
{
	LF

	printf ("\nimm  :  %d\n", ctx->operand_immediate->lval.ubyte);
}

/*
 	Shuffle Bytes
 */
void pshufb	(op_ctx_sse_t	*ctx)
{
	LF

	vector_block_for_byte (ctx, (ctx->extended) ? 16 : 8, {
		if (*src & 0x80) *res = 0;
		else *res = ctx->dst.uint8 [*src & 0xF]; 
	});
}

void pmulhrsw	(op_ctx_sse_t	*ctx);
void pmaddubsw	(op_ctx_sse_t	*ctx);
void phsubw	(op_ctx_sse_t	*ctx);
void phsubd	(op_ctx_sse_t	*ctx);
void phsubsw	(op_ctx_sse_t	*ctx);
void phaddw	(op_ctx_sse_t	*ctx);
void phaddd	(op_ctx_sse_t	*ctx);
void phaddsw	(op_ctx_sse_t	*ctx);




#if 0
/** packed horizontal add word **/
void phaddw128(XMM src, XMM dst, XMM *res)
{
	int i;
	for(i = 0; i < 4; ++i)
		res->a16[i] = dst.a16[2*i] + dst.a16[2*i+1];
	for(i = 0; i < 4; ++i)
		res->a16[i+4] = src.a16[2*i] + src.a16[2*i+1];
}

void phaddw64(MM src, MM dst, MM *res)
{
	res->a16[0] = dst.a16[0] + dst.a16[1];
	res->a16[1] = dst.a16[2] + dst.a16[3];
	res->a16[2] = src.a16[0] + src.a16[1];
	res->a16[3] = src.a16[2] + src.a16[3];
}

/** packed horizontal add double **/
void phaddd128(XMM src, XMM dst, XMM *res)
{
	int i;
	for(i = 0; i < 2; ++i) {
		res->a32[i  ] = dst.a32[2*i] + dst.a32[2*i+1];
	}
	for(i = 0; i < 2; ++i)
		res->a32[i+2] = src.a32[2*i] + src.a32[2*i+1];
}

void phaddd64(MM src, MM dst, MM *res)
{
	res->a32[0] = dst.a32[0] + dst.a32[1];
	res->a32[1] = src.a32[0] + src.a32[1];
}

/** packed horizontal add and saturate word **/
void phaddsw128(XMM src, XMM dst, XMM *res)
{
	int i;
	for(i = 0; i < 4; ++i)
		res->a16[i] = SATSW( dst.a16[2*i] + dst.a16[2*i+1] );
	for(i = 0; i < 4; ++i)
		res->a16[i+4] = SATSW( src.a16[2*i] + src.a16[2*i+1] );
}

void phaddsw64(MM src, MM dst, MM *res)
{
	res->a16[0] = SATSW( dst.a16[0] + dst.a16[1] );
	res->a16[1] = SATSW( dst.a16[2] + dst.a16[3] );
	res->a16[2] = SATSW( src.a16[0] + src.a16[1] );
	res->a16[3] = SATSW( src.a16[2] + src.a16[3] );
}

/** multiply and add packed signed and unsigned bytes **/
void pmaddubsw128(XMM src, XMM dst, XMM *res)
{
	int i;
	int16_t tmp[16];
	for(i=0; i<16; ++i) {
		tmp[i] = src.a8[i] * dst.ua8[i];
	}
	for(i=0; i<8; ++i) {
		res->a16[i] = SATSW( tmp[2*i] + tmp[2*i+1] );
	}
}

void pmaddubsw64(MM src, MM dst, MM *res)
{
	int i;
	int16_t tmp[8];
	for(i=0; i<8; ++i) {
		tmp[i] = src.a8[i] * dst.ua8[i];
	}
	for(i=0; i<4; ++i) {
		res->a16[i] = SATSW( tmp[2*i] + tmp[2*i+1] );
	}
}

/** packed horizontal subtract word **/
void phsubw128(XMM src, XMM dst, XMM *res)
{
	int i;
	for(i = 0; i < 4; ++i)
		res->a16[i] = dst.a16[2*i] - dst.a16[2*i+1];
	for(i = 0; i < 4; ++i)
		res->a16[i+4] = src.a16[2*i] - src.a16[2*i+1];
}

void phsubw64(MM src, MM dst, MM *res)
{
	res->a16[0] = dst.a16[0] - dst.a16[1];
	res->a16[1] = dst.a16[2] - dst.a16[3];
	res->a16[2] = src.a16[0] - src.a16[1];
	res->a16[3] = src.a16[2] - src.a16[3];
}

/** packed horizontal subtract double **/
void phsubd128(XMM src, XMM dst, XMM *res)
{
	int i;
	for(i = 0; i < 2; ++i)
		res->a32[i  ] = dst.a32[2*i] - dst.a32[2*i+1];
	for(i = 0; i < 2; ++i)
		res->a32[i+2] = src.a32[2*i] - src.a32[2*i+1];
}

void phsubd64(MM src, MM dst, MM *res)
{
	res->a32[0] = dst.a32[0] - dst.a32[1];
	res->a32[1] = src.a32[0] - src.a32[1];
}

/** packed horizontal subtract and saturate word **/
void phsubsw128(XMM src, XMM dst, XMM *res)
{
	int i;
	for(i = 0; i < 4; ++i)
		res->a16[i] = SATSW( dst.a16[2*i] - dst.a16[2*i+1] );
	for(i = 0; i < 4; ++i)
		res->a16[i+4] = SATSW( src.a16[2*i] - src.a16[2*i+1] );
}

void phsubsw64(MM src, MM dst, MM *res)
{
	res->a16[0] = SATSW( dst.a16[0] - dst.a16[1] );
	res->a16[1] = SATSW( dst.a16[2] - dst.a16[3] );
	res->a16[2] = SATSW( src.a16[0] - src.a16[1] );
	res->a16[3] = SATSW( src.a16[2] - src.a16[3] );
}

/** packed sign byte **/
void psignb128(XMM src, XMM dst, XMM *res)
{
	int i;
	for(i = 0; i < 16; ++i) {
		if(src.a8[i] < 0) res->a8[i] = -dst.a8[i];
		else if(src.a8[i] == 0) res->a8[i] = 0;
		else res->a8[i] = dst.a8[i];
	}
}

void psignb64(MM src, MM dst, MM *res)
{
	int i;
	for(i = 0; i < 8; ++i) {
		if(src.a8[i] < 0) res->a8[i] = -dst.a8[i];
		else if(src.a8[i] == 0) res->a8[i] = 0;
		else res->a8[i] = dst.a8[i];
	}
}

/** packed sign word **/
void psignw128(XMM src, XMM dst, XMM *res)
{
	int i;
	for(i = 0; i < 8; ++i) {
		if(src.a16[i] < 0) res->a16[i] = -dst.a16[i];
		else if(src.a16[i] == 0) res->a16[i] = 0;
		else res->a16[i] = dst.a16[i];
	}
}

void psignw64(MM src, MM dst, MM *res)
{
	int i;
	for(i = 0; i < 4; ++i) {
		if(src.a16[i] < 0) res->a16[i] = -dst.a16[i];
		else if(src.a16[i] == 0) res->a16[i] = 0;
		else res->a16[i] = dst.a16[i];
	}
}

/** packed sign double **/
void psignd128(XMM src, XMM dst, XMM *res)
{
	int i;
	for(i = 0; i < 4; ++i) {
		if(src.a32[i] < 0) res->a32[i] = -dst.a32[i];
		else if(src.a32[i] == 0) res->a32[i] = 0;
		else res->a32[i] = dst.a32[i];
	}
}

void psignd64(MM src, MM dst, MM *res)
{
	int i;
	for(i = 0; i < 2; ++i) {
		if(src.a32[i] < 0) res->a32[i] = -dst.a32[i];
		else if(src.a32[i] == 0) res->a32[i] = 0;
		else res->a32[i] = dst.a32[i];
	}
}

/** packed multiply high with round and scale word **/
void pmulhrsw128(XMM src, XMM dst, XMM *res)
{
	int i;
	for(i = 0; i < 8; ++i)
		res->a16[i] = (((dst.a16[i] * src.a16[i] >> 14) + 1) >> 1);
}

void pmulhrsw64(MM src, MM dst, MM *res)
{
	int i;
	for(i = 0; i < 4; ++i)
		res->a16[i] = (((dst.a16[i] * src.a16[i] >> 14) + 1) >> 1);
}

/** packed absolute value byte **/
void pabsb128(XMM src, XMM *res)
{
	int i;
	for(i = 0; i < 16; ++i)
		if(src.a8[i] < 0) res->a8[i] = -src.a8[i];
		else res->a8[i] = src.a8[i];
}

void pabsb64(MM src, MM *res)
{
	int i;
	for(i = 0; i < 8; ++i)
		if(src.a8[i] < 0) res->a8[i] = -src.a8[i];
		else res->a8[i] = src.a8[i];
}

/** packed absolute value word **/
void pabsw128(XMM src, XMM *res)
{
	int i;
	for(i = 0; i < 8; ++i)
		if(src.a16[i] < 0) res->a16[i] = -src.a16[i];
		else res->a16[i] = src.a16[i];
}

void pabsw64(MM src, MM *res)
{
	int i;
	for(i = 0; i < 4; ++i)
		if(src.a16[i] < 0) res->a16[i] = -src.a16[i];
		else res->a16[i] = src.a16[i];
}

/** packed absolute value double **/
void pabsd128(XMM src, XMM *res)
{
	int i;
	for(i = 0; i < 4; ++i)
		if(src.a32[i] < 0) res->a32[i] = -src.a32[i];
		else res->a32[i] = src.a32[i];
}

void pabsd64(MM src, MM *res)
{
	int i;
	for(i = 0; i < 2; ++i)
		if(src.a32[i] < 0) res->a32[i] = -src.a32[i];
		else res->a32[i] = src.a32[i];
}

/** packed align right **/
void palignr128(XMM src, XMM dst, XMM *res, uint8_t IMM)
{
	int j, index;
	uint8_t tmp[32];
	if(IMM > 31) {
		res->ua128 = 0;
	} else {
		((__uint128_t*)tmp)[0] = src.ua128;
		((__uint128_t*)tmp)[1] = dst.ua128;
		for (j = 0; j < 16; j++) {
			index = j + IMM;
			if(index > 31) res->ua8[j] = 0;
			else res->ua8[j] = tmp[index];
		}
	}
}

void palignr64(MM src, MM dst, MM *res, uint8_t IMM)
{
	int n = IMM * 8;
	if(IMM > 15) {
		res->ua64 = 0;
	} else {
		__uint128_t t;
		t = src.ua64 | ((__uint128_t)dst.ua64 << 64);
		t >>= n;
		res->ua64 = t;
	}
}

#endif

