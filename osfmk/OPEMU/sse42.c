#include "ssse3_priv.h"

/**
 * Compare and index string
 */
void pcmpistri	(ssse3_t *this)
{
	const int imm = this->udo_imm->lval.ubyte;
	const int comparison_mode = (imm >> 2) & 0b11;
	const int polarity = (imm >> 4) & 0b11;
	const int isword = imm & 0b1;
	const int issigned = imm & 0b10;
	const int isMSB = imm & 0b1000000;

	const int bounds = isword ? 8 : 16;

	uint16_t *srcw = this->src.uint16;
	uint16_t *dstw = this->dst.uint16;

	uint8_t *srcb = this->src.uint8;
	uint8_t *dstb = this->dst.uint8;

	uint16_t res1 = 0, res2;
	unsigned int index;
	unsigned int MSsetbit = 0;
	unsigned int LSsetbit = 0;

	/* thanks for excusing me the nesting */

	switch (comparison_mode) {
	case 0:
		/* EQUAL comparison */
		if (isword) {
			for (int j = 0; j < bounds; ++j) {
				this->res.uint16[j] = dstw[j];
				res1 <<= 1;

				for (int i = 0; i < bounds; ++i) {
					if (dstw[j] == srcw[i]) {
						res1 ++;
						MSsetbit = j;
						if (!LSsetbit) LSsetbit = j + 1;
					}
				}
			}
		} else {
			for (int j = 0; j < bounds; ++j) {
				this->res.uint8[j] = dstb[j];
				res1 <<= 1;

				for (int i = 0; i < bounds; ++i) {
					if (dstb[j] == srcb[i]) {
						res1 ++;
						MSsetbit = j;
						if (!LSsetbit) LSsetbit = j + 1;
					}
				}
			}
		}
		break;

	default: panic();
	}

	switch (polarity) {
	case 1: panic();
		break;

	case 3: panic();
		break;

	default: res2 = res1;
	}

	// bring the index back to zero-based
	--MSsetbit;
	--LSsetbit;

#if 0
	printf("src: ");
	print128(this->src);
	printf("\n");

	printf("dst: ");
	print128(this->dst);
	printf("\n");

	printf("res: ");
	print128(this->res);
	printf("\n");

	printf("and the int2 is %02x\n", res2);
	printf("and the index  is %d\n", isMSB ? MSsetbit : LSsetbit );
#endif

	// TODO document and add support for i386
	this->op_obj->state64->rcx = isMSB ? MSsetbit : LSsetbit;

	this->op_obj->state64->isf.rflags &= ~ 0b100011010101;
	this->op_obj->state64->isf.rflags |= (res2) ? 1 : 0; // C

	// check for null in src, dst
	for (int i = 0; i < 16; ++i) {
		if (srcb[i] == 0) this->op_obj->state64->isf.rflags |= 1 << 6; // Z
		if (dstb[i] == 0) this->op_obj->state64->isf.rflags |= 1 << 7; // S
	}

	if (res2 & 0b1) this->op_obj->state64->isf.rflags |= 1 << 11;
}


