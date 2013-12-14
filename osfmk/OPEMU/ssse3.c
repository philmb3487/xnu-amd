#include "opemu.h"
#include "ssse3_priv.h"

/**
 * Load operands from memory/register, store in obj.
 * @return: 0 if success
 */
int ssse3_grab_operands(ssse3_t*);

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
	ssse3_obj.udo_src = ud_insn_opr (op_obj->ud_obj, 1);
	ssse3_obj.udo_dst = ud_insn_opr (op_obj->ud_obj, 0);
	ssse3_obj.udo_imm = ud_insn_opr (op_obj->ud_obj, 2);

	const uint32_t mnemonic = ud_insn_mnemonic(ssse3_obj.op_obj->ud_obj);
	ssse3_func opf;

	switch (mnemonic) {
	
	case UD_Ipsignb:	opf = psignb;
	case UD_Ipsignw:	opf = psignw;
	case UD_Ipsignd:	opf = psignd;

	case UD_Ipabsb:		opf = pabsb;
	case UD_Ipabsw:		opf = pabsw;
	case UD_Ipabsd:		opf = pabsd;

	case UD_Ipalignr:	opf = palignr;

	case UD_Ipshufb:	opf = pshufb;

	case UD_Ipmulhrsw:	opf = pmulhrsw;

	case UD_Ipmaddubsw:	opf = pmaddubsw;

	case UD_Iphsubw:	opf = phsubw;
	case UD_Iphsubd:	opf = phsubd;

	case UD_Iphsubsw:	opf = phsubsw;

	case UD_Iphaddw:	opf = phaddw;
	case UD_Iphaddd:	opf = phaddd;

	case UD_Iphaddsw:	opf = phaddsw;
sse3x_common:
	
	default: goto bad;
	}

good:
	return 0;
bad:
	return -1;
}



