/*
             .d8888b.   .d8888b.   .d8888b.  8888888888 .d8888b.  
            d88P  Y88b d88P  Y88b d88P  Y88b 888       d88P  Y88b 
            Y88b.      Y88b.      Y88b.      888            .d88P 
             "Y888b.    "Y888b.    "Y888b.   8888888       8888"  
                "Y88b.     "Y88b.     "Y88b. 888            "Y8b. 
                  "888       "888       "888 888       888    888 
            Y88b  d88P Y88b  d88P Y88b  d88P 888       Y88b  d88P 
             "Y8888P"   "Y8888P"   "Y8888P"  8888888888 "Y8888P"  
*/

#include "opemu.h"
#include "ssse3_priv.h"

/**
 * Load operands from memory/register, store in obj.
 * @return: 0 if success
 */
int ssse3_grab_operands(ssse3_t *ssse3_obj)
{
	// TODO legacy mmx
	if (ssse3_obj->islegacy) goto bad;

	_store_xmm (ssse3_obj->udo_dst->base - UD_R_XMM0, (void*) &ssse3_obj->dst.uint128);
	if (ssse3_obj->udo_src->base == UD_OP_REG) {
		_store_xmm (ssse3_obj->udo_src->base - UD_R_XMM0, (void*) &ssse3_obj->src.uint128);
	} else {
		// m128 load
		int64_t disp;
		uint8_t disp_size = ssse3_obj->udo_src->offset;
		uint64_t address;
		
		if (ssse3_obj->udo_src->scale) goto bad; // TODO

		disp = ssse3_obj->udo_src->lval.uqword & ((2 ^ (disp_size)) - 1);
		if (retrieve_reg (ssse3_obj->op_obj->state,
			ssse3_obj->udo_src->base, &address) != 0) goto bad;

		// TODO this is good for kernel only
		ssse3_obj->src.uint128 = * (__uint128_t*) (address + disp);
	}

good:
	return 0;
bad:
	return -1;
}

/**
 * Store operands from obj to memory/register.
 * @return: 0 if success
 */
int ssse3_commit_results(const ssse3_t*);


/**
 * Main function for the ssse3 portion. Check if the offending
 * opcode is part of the ssse3 instruction set, if not, quit early.
 * if so, then we build the appropriate context, and jump to the right function.
 * @param op_obj: opemu object
 * @return: zero if an instruction was emulated properly
 */
int op_sse3x_run(const op_t *op_obj)
{
	LF

	ssse3_t ssse3_obj;
	ssse3_obj.op_obj = op_obj;
	const uint32_t mnemonic = ud_insn_mnemonic(ssse3_obj.op_obj->ud_obj);
	ssse3_func opf;

	switch (mnemonic) {
//	case UD_Ipsignb:	opf = psignb;
//	case UD_Ipsignw:	opf = psignw;
//	case UD_Ipsignd:	opf = psignd;
//
//	case UD_Ipabsb:		opf = pabsb;
//	case UD_Ipabsw:		opf = pabsw;
//	case UD_Ipabsd:		opf = pabsd;
//
//	case UD_Ipalignr:	opf = palignr;
//
	case UD_Ipshufb:	opf = pshufb;
//
//	case UD_Ipmulhrsw:	opf = pmulhrsw;
//
//	case UD_Ipmaddubsw:	opf = pmaddubsw;
//
//	case UD_Iphsubw:	opf = phsubw;
//	case UD_Iphsubd:	opf = phsubd;
//
//	case UD_Iphsubsw:	opf = phsubsw;
//
//	case UD_Iphaddw:	opf = phaddw;
//	case UD_Iphaddd:	opf = phaddd;
//
//	case UD_Iphaddsw:	opf = phaddsw;
sse3x_common:
	
	ssse3_obj.udo_src = ud_insn_opr (op_obj->ud_obj, 1);
	ssse3_obj.udo_dst = ud_insn_opr (op_obj->ud_obj, 0);
	ssse3_obj.udo_imm = ud_insn_opr (op_obj->ud_obj, 2);

	// run some sanity checks,
	if (ssse3_obj.udo_dst->type != UD_OP_REG) goto bad;
	if ((ssse3_obj.udo_src->type != UD_OP_REG)
		&& (ssse3_obj.udo_src->type != UD_OP_MEM))
		goto bad;

	// i'd like to know if this instruction is legacy mmx
	if ((ssse3_obj.udo_dst->base >= UD_R_MM0)
		&& (ssse3_obj.udo_dst->base <= UD_R_MM7)) {
		ssse3_obj.islegacy = 1;
		goto bad;
	} else ssse3_obj.islegacy = 0;
	
	if (ssse3_grab_operands(&ssse3_obj) != 0) goto bad;

	opf(&ssse3_obj);

	default: goto bad;
	}

good:
	return 0;
bad:
	return -1;
}

/**
 *
 */
void pshufb (ssse3_t *this)
{
	LF

	printf("src: "); print128(this->src.uint128); printf("\n");
	printf("dst: "); print128(this->dst.uint128); printf("\n");

	panic();

	for (int i = 0; i < (this->islegacy) ? 8 : 16; ++ i) {
	}
}


