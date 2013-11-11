#include <stdint.h>
#include "opemu.h"
/*   ** SINETEK **
* This is an emulator for the SSSE3 instruction set.
* It is executed as a part of the XNU kernel as a trap.
*
* Information about SSSE3: there are 32 instructions.
* A few of these use MMX registers and are therefore just like XMM.
*/

#ifndef TESTCASE
#include <kern/sched_prim.h>
#endif

#if 0
#define printf(...)
#endif

#ifndef TESTCASE
void print_buffer(uint8_t *buffer);
void print_buffer(uint8_t *buffer)
{
    int i;
    printf("DEBUG: emu: buffer data ");
    for(i = 0; i < 15; ++i) {
	    kprintf("%02x ", buffer[i]);
    }
    kprintf("\n");
}

void print_debug(uint8_t *buffer, x86_saved_state_t *saved_state);
void print_debug(uint8_t *buffer, x86_saved_state_t *saved_state)
{
    if(is_saved_state64(saved_state)) {
	    x86_saved_state64_t     *regs;
	    regs = saved_state64(saved_state);
	    kprintf("DEBUG: emu64:  eip=%016llx\n", regs->isf.rip);
    } else {
	    x86_saved_state32_t     *regs;
	    regs = saved_state32(saved_state);
	    kprintf("DEBUG: emu32:  eip=%08x\n", regs->eip);
    }
    print_buffer(buffer);
}

void opemu_trap(
    x86_saved_state_t *saved_state)
{
    /* instructions are at most 15ish bytes
     * can we just dereference something instead?
     */
    uint8_t buffered_code[15];

    if(is_saved_state64(saved_state)) {
	    x86_saved_state64_t     *regs;
	    regs = saved_state64(saved_state);

	//kprintf("DEBUG: emu64:  eip=%016llx\n", regs->isf.rip);
	    user_addr_t pc = regs->isf.rip;
	    copyin(pc, (char*)buffered_code, 15);
          //print_buffer(buffered_code);           
	    regs->isf.rip += opemu(buffered_code, saved_state);
	//kprintf("returning from opemu %eip=%08x\n", regs->isf.rip);
    } else {
	    x86_saved_state32_t     *regs;
	    regs = saved_state32(saved_state);

      	//kprintf("DEBUG: emu32:  eip=%08x\n", regs->eip);
	    user_addr_t pc = regs->eip;
	    copyin(pc, (char*)buffered_code, 15);
          //print_buffer(buffered_code);           
	    int opsize = (int) opemu(buffered_code, saved_state);          
	    regs->eip += opsize;
	//kprintf("returning from opemu %eip=%08x\n", regs->eip);
    }

    thread_exception_return();
    /* NOTREACHED */
}

// forward declaration for sysenter handler of mach;
void mach_call_munger(x86_saved_state_t *state);
void unix_syscall(x86_saved_state_t *);
uint64_t opemu(uint8_t *code, x86_saved_state_t *saved_state)
{
    // size of the instruction in bytes. will serve for adjusting
    // the return address;
    int ins_size = 1;
    XMM Xsrc, Xdst, Xres;
    MM Msrc, Mdst, Mres;

    if(code[0] == 0x0F && code[1] == 0x34) {
	    // sysenter, TODO remove redundancy regs load
	    //   edx        return address
	    //   ecx        return stack
	    x86_saved_state32_t     *regs;
	    regs = saved_state32(saved_state);
	    regs->eip = regs->edx;
	    regs->uesp = regs->ecx;

	    if((signed int)regs->eax < 0) {
	    //      printf("mach call\n");
		    mach_call_munger(saved_state);
	    } else {
	    //      printf("unix call\n");
		    unix_syscall(saved_state);
	    }
	    /* NEVER REACHES */
    } else if(code[0] == 0xFF) {
	    // Instruction 0xFFFF, used as a debugging aid. (2-byte NOP)
	    if(code[1] != 0xFF) goto invalid;
	    ins_size = 2;
    } else if(
		(code[0] == 0x66 && code[1] == 0x0F && code[2] == 0x38) ||
		((code[0] == 0x66 && code[2] == 0x0F && code[3] == 0x38)) ) {
	    // Instruction would be of type XMM (128-bit).
	    XMM *src, *dst, *rs;
	    unsigned int NSRC, NDST;
	    src = &Xsrc;
	    dst = &Xdst;
	    rs = &Xres;
	    ins_size = 5;
		uint8_t opcode = code[3];

		/* In long mode, there is a possible 0x40->0x4f prefix
		 * used to handle the higher xmm registers.
		 */
	   
	    InterpretSSSE3Operands(&code[4], &NSRC, &NDST);
		if(code[1] & 0x40) {
			if(code[1] & 0x44) NDST += 8;
			if(code[1] & 0x41) NSRC += 8;
			opcode = code[4];
			ins_size += 1;
		}
	    getxmm(src, NSRC);
	    getxmm(dst, NDST);

	    switch(opcode) {
	    case 0x00: pshufb128(src,dst,rs); break;
	    case 0x01: phaddw128(src,dst,rs); break;
	    case 0x02: phaddd128(src,dst,rs); break;
	    case 0x03: phaddsw128(src,dst,rs); break;
	    case 0x04: pmaddubsw128(src,dst,rs); break;
	    case 0x05: phsubw128(src,dst,rs); break;
	    case 0x06: phsubd128(src,dst,rs); break;
	    case 0x07: phsubsw128(src,dst,rs); break;
	    case 0x08: psignb128(src,dst,rs); break;
	    case 0x09: psignw128(src,dst,rs); break;
	    case 0x0A: psignd128(src,dst,rs); break;
	    case 0x0B: pmulhrsw128(src,dst,rs); break;
	    case 0x1C: pabsb128(src,rs); break;
	    case 0x1D: pabsw128(src,rs); break;
	    case 0x1E: pabsd128(src,rs); break;
	    default: goto invalid; break;
	    }
	    movxmm(rs, NDST);

    } else if(code[0] == 0x0F && code[1] == 0x38) {
	    // Instruction would be of type MMX (64-bit).
	    MM *src, *dst, *rs;
	    unsigned int NSRC, NDST; // reg 0 to 7 possible.
	    src = &Msrc;
	    dst = &Mdst;
	    rs = &Mres;
	    ins_size = 4;

	    InterpretSSSE3Operands(&code[3], &NSRC, &NDST);
	    getmm(src, NSRC);
	    getmm(dst, NDST);

	    switch(code[2]) {
	    case 0x00: pshufb64(src,dst,rs); break;
	    case 0x01: phaddw64(src,dst,rs); break;
	    case 0x02: phaddd64(src,dst,rs); break;
	    case 0x03: phaddsw64(src,dst,rs); break;
	    case 0x04: pmaddubsw64(src,dst,rs); break;
	    case 0x05: phsubw64(src,dst,rs); break;
	    case 0x06: phsubd64(src,dst,rs); break;
	    case 0x07: phsubsw64(src,dst,rs); break;
	    case 0x08: psignb64(src,dst,rs); break;
	    case 0x09: psignw64(src,dst,rs); break;
	    case 0x0A: psignd64(src,dst,rs); break;
	    case 0x0B: pmulhrsw64(src,dst,rs); break;
	    case 0x1C: pabsb64(src,rs); break;
	    case 0x1D: pabsw64(src,rs); break;
	    case 0x1E: pabsd64(src,rs); break;
	    default: goto invalid; break;
	    }
	    movmm(rs, NDST);

    } else if(code[0] == 0x0F && code[1] == 0x3A &&
	      code[2] == 0x0F) {
	    // Not groupable with the other ones. (64-bit).
	    MM *src, *dst, *rs;
	    unsigned int NSRC, NDST; // reg 0 to 7 possible.
	    src = &Msrc;
	    dst = &Mdst;
	    rs = &Mres;
	    ins_size = 5;

	    InterpretSSSE3Operands(&code[3], &NSRC, &NDST);
	    getmm(src, NSRC);
	    getmm(dst, NDST);              
	    palignr64(src,dst,rs,code[4]);
	    movmm(rs, NDST);

    } else if(
	(code[0] == 0x66 && code[1] == 0x0F &&
	code[2] == 0x3A && code[3] == 0x0F) || 
	((code[0] == 0x66 && code[2] == 0x0F &&
	code[3] == 0x3A && code[4] == 0x0F) )
	) {
	    // Not groupable with the other ones. (128-bit).
	    XMM *src, *dst, *rs;
	    unsigned int NSRC, NDST;
	    src = &Xsrc;
	    dst = &Xdst;
	    rs = &Xres;
	    ins_size = 6;
		uint8_t operand = code[5];
		uint8_t modrm = code[4];

		/* In long mode, there is a possible 0x40->0x4f prefix
		 * used to handle the higher xmm registers.
		 */
	   
	    InterpretSSSE3Operands(&code[4], &NSRC, &NDST);
		if(code[1] & 0x40) {
			if(code[1] & 0x44) NDST += 8;
			if(code[1] & 0x41) NSRC += 8;
			modrm = code[5];
			operand = code[6];
			ins_size += 1;
		}

	    InterpretSSSE3Operands(&modrm, &NSRC, &NDST);
	    getxmm(src, NSRC);
	    getxmm(dst, NDST);
	    palignr128(src,dst,rs,operand);
	    movxmm(rs, NDST);

    } else {
invalid:
	    // Invalid opcode, report it
	kprintf("EMU: invop\n");
	printf("EMU: invop\n");
	    print_debug(code, saved_state);
    }


    return ins_size;
}
#endif

/** interpret ModRM byte
* [2:0] -- source operand
* [5:4] -- destination operand
*/
inline void InterpretSSSE3Operands(uint8_t *ModRM, unsigned int *src,
    unsigned int *dst)
{
    *src = *ModRM & 0x7;
    *dst = (*ModRM >> 3) & 0x7;
}

/* get value from the xmm register i */
inline void getxmm(XMM *v, unsigned int i)
{
    switch(i) {
    case 0:
      asm __volatile__ ("movdqu %%xmm0, %0" : "=m" (*v->a8));
    break;
    case 1:
      asm __volatile__ ("movdqu %%xmm1, %0" : "=m" (*v->a8));
    break;
    case 2:
      asm __volatile__ ("movdqu %%xmm2, %0" : "=m" (*v->a8));
    break;
    case 3:
      asm __volatile__ ("movdqu %%xmm3, %0" : "=m" (*v->a8));
    break;
    case 4:
      asm __volatile__ ("movdqu %%xmm4, %0" : "=m" (*v->a8));
    break;
    case 5:
      asm __volatile__ ("movdqu %%xmm5, %0" : "=m" (*v->a8));
    break;
    case 6:
      asm __volatile__ ("movdqu %%xmm6, %0" : "=m" (*v->a8));
    break;
    case 7:
      asm __volatile__ ("movdqu %%xmm7, %0" : "=m" (*v->a8));
    break;
    case 8:
      asm __volatile__ ("movdqu %%xmm8, %0" : "=m" (*v->a8));
    break;
    case 9:
      asm __volatile__ ("movdqu %%xmm9, %0" : "=m" (*v->a8));
    break;
    case 10:
      asm __volatile__ ("movdqu %%xmm10, %0" : "=m" (*v->a8));
    break;
    case 11:
      asm __volatile__ ("movdqu %%xmm11, %0" : "=m" (*v->a8));
    break;
    case 12:
      asm __volatile__ ("movdqu %%xmm12, %0" : "=m" (*v->a8));
    break;
    case 13:
      asm __volatile__ ("movdqu %%xmm13, %0" : "=m" (*v->a8));
    break;
    case 14:
      asm __volatile__ ("movdqu %%xmm14, %0" : "=m" (*v->a8));
    break;
    case 15:
      asm __volatile__ ("movdqu %%xmm15, %0" : "=m" (*v->a8));
    break;
    }
}

/* get value from the mm register i  */
inline void getmm(MM *v, unsigned int i)
{
    switch(i) {
    case 0:
      asm __volatile__ ("movq %%mm0, %0" : "=m" (*v->a8));
    break;
    case 1:
      asm __volatile__ ("movq %%mm1, %0" : "=m" (*v->a8));
    break;
    case 2:
      asm __volatile__ ("movq %%mm2, %0" : "=m" (*v->a8));
    break;
    case 3:
      asm __volatile__ ("movq %%mm3, %0" : "=m" (*v->a8));
    break;
    case 4:
      asm __volatile__ ("movq %%mm4, %0" : "=m" (*v->a8));
    break;
    case 5:
      asm __volatile__ ("movq %%mm5, %0" : "=m" (*v->a8));
    break;
    case 6:
      asm __volatile__ ("movq %%mm6, %0" : "=m" (*v->a8));
    break;
    case 7:
      asm __volatile__ ("movq %%mm7, %0" : "=m" (*v->a8));
    break;
    }
}

/* move value over to xmm register i */
inline void movxmm(XMM *v, unsigned int i)
{
    switch(i) {
    case 0:
      asm __volatile__ ("movdqu %0, %%xmm0" :: "m" (*v->a8) );
    break;
    case 1:
      asm __volatile__ ("movdqu %0, %%xmm1" :: "m" (*v->a8) );
    break;
    case 2:
      asm __volatile__ ("movdqu %0, %%xmm2" :: "m" (*v->a8) );
    break;
    case 3:
      asm __volatile__ ("movdqu %0, %%xmm3" :: "m" (*v->a8) );
    break;
    case 4:
      asm __volatile__ ("movdqu %0, %%xmm4" :: "m" (*v->a8) );
    break;
    case 5:
      asm __volatile__ ("movdqu %0, %%xmm5" :: "m" (*v->a8) );
    break;
    case 6:
      asm __volatile__ ("movdqu %0, %%xmm6" :: "m" (*v->a8) );
    break;
    case 7:
      asm __volatile__ ("movdqu %0, %%xmm7" :: "m" (*v->a8) );
    break;
    case 8:
      asm __volatile__ ("movdqu %0, %%xmm8" :: "m" (*v->a8) );
    break;
    case 9:
      asm __volatile__ ("movdqu %0, %%xmm9" :: "m" (*v->a8) );
    break;
    case 10:
      asm __volatile__ ("movdqu %0, %%xmm10" :: "m" (*v->a8) );
    break;
    case 11:
      asm __volatile__ ("movdqu %0, %%xmm11" :: "m" (*v->a8) );
    break;
    case 12:
      asm __volatile__ ("movdqu %0, %%xmm12" :: "m" (*v->a8) );
    break;
    case 13:
      asm __volatile__ ("movdqu %0, %%xmm13" :: "m" (*v->a8) );
    break;
    case 14:
      asm __volatile__ ("movdqu %0, %%xmm14" :: "m" (*v->a8) );
    break;
    case 15:
      asm __volatile__ ("movdqu %0, %%xmm15" :: "m" (*v->a8) );
    break;
    }
}

/* move value over to mm register i */
inline void movmm(MM *v, unsigned int i)
{
    switch(i) {
    case 0:
      asm __volatile__ ("movq %0, %%mm0" :: "m" (*v->a8) );
    break;
    case 1:
      asm __volatile__ ("movq %0, %%mm1" :: "m" (*v->a8) );
    break;
    case 2:
      asm __volatile__ ("movq %0, %%mm2" :: "m" (*v->a8) );
    break;
    case 3:
      asm __volatile__ ("movq %0, %%mm3" :: "m" (*v->a8) );
    break;
    case 4:
      asm __volatile__ ("movq %0, %%mm4" :: "m" (*v->a8) );
    break;
    case 5:
      asm __volatile__ ("movq %0, %%mm5" :: "m" (*v->a8) );
    break;
    case 6:
      asm __volatile__ ("movq %0, %%mm6" :: "m" (*v->a8) );
    break;
    case 7:
      asm __volatile__ ("movq %0, %%mm7" :: "m" (*v->a8) );
    break;
    }
}

/***************************************/
/** SSSE3 instructions implementation **/
/***************************************/

#define SATSW(x) ((x > 32767)? 32767 : ((x < -32768)? -32768 : x) )



/** complex byte shuffle **/
void pshufb128(XMM *src, XMM *dst, XMM *res)
{
    int i;
    for(i = 0; i < 16; ++i)
	    res->a8[i] = (src->a8[i] < 0) ? 0 :
		    dst->a8[src->a8[i] & 0xF];
}

void pshufb64(MM *src, MM *dst, MM *res)
{
    int i;
    for(i = 0; i < 8; ++i)
	    res->a8[i] = (src->a8[i] < 0) ? 0 :
		    dst->a8[src->a8[i] & 0x7];
}

/** packed horizontal add word **/
void phaddw128(XMM *src, XMM *dst, XMM *res)
{
    int i;
    for(i = 0; i < 4; ++i)
	    res->a16[i] = dst->a16[2*i] + dst->a16[2*i+1];
    for(i = 0; i < 4; ++i)
	    res->a16[i+4] = src->a16[2*i] + src->a16[2*i+1];
}

void phaddw64(MM *src, MM *dst, MM *res)
{
    res->a16[0] = dst->a16[0] + dst->a16[1];
    res->a16[1] = dst->a16[2] + dst->a16[3];
    res->a16[2] = src->a16[0] + src->a16[1];
    res->a16[3] = src->a16[2] + src->a16[3];
}

/** packed horizontal add double **/
void phaddd128(XMM *src, XMM *dst, XMM *res)
{
    int i;
    for(i = 0; i < 2; ++i) {
	    res->a32[i  ] = dst->a32[2*i] + dst->a32[2*i+1];
    }
    for(i = 0; i < 2; ++i)
	    res->a32[i+2] = src->a32[2*i] + src->a32[2*i+1];
}

void phaddd64(MM *src, MM *dst, MM *res)
{
    res->a32[0] = dst->a32[0] + dst->a32[1];
    res->a32[1] = src->a32[0] + src->a32[1];
}

/** packed horizontal add and saturate word **/
void phaddsw128(XMM *src, XMM *dst, XMM *res)
{
    int i;
    for(i = 0; i < 4; ++i)
	    res->a16[i] = SATSW( dst->a16[2*i] + dst->a16[2*i+1] );
    for(i = 0; i < 4; ++i)
	    res->a16[i+4] = SATSW( src->a16[2*i] + src->a16[2*i+1] );
}

void phaddsw64(MM *src, MM *dst, MM *res)
{
    res->a16[0] = SATSW( dst->a16[0] + dst->a16[1] );
    res->a16[1] = SATSW( dst->a16[2] + dst->a16[3] );
    res->a16[2] = SATSW( src->a16[0] + src->a16[1] );
    res->a16[3] = SATSW( src->a16[2] + src->a16[3] );
}

/** multiply and add packed signed and unsigned bytes **/
void pmaddubsw128(XMM *src, XMM *dst, XMM *res)
{
    int i;
    int16_t tmp[16];
    for(i=0; i<16; ++i) {
	    tmp[i] = src->a8[i] * dst->ua8[i];
    }
    for(i=0; i<8; ++i) {
	    res->a16[i] = SATSW( tmp[2*i] + tmp[2*i+1] );
    }
}

void pmaddubsw64(MM *src, MM *dst, MM *res)
{
    int i;
    int16_t tmp[8];
    for(i=0; i<8; ++i) {
	    tmp[i] = src->a8[i] * dst->ua8[i];
    }
    for(i=0; i<4; ++i) {
	    res->a16[i] = SATSW( tmp[2*i] + tmp[2*i+1] );
    }
}

/** packed horizontal subtract word **/
void phsubw128(XMM *src, XMM *dst, XMM *res)
{
    int i;
    for(i = 0; i < 4; ++i)
	    res->a16[i] = dst->a16[2*i] - dst->a16[2*i+1];
    for(i = 0; i < 4; ++i)
	    res->a16[i+4] = src->a16[2*i] - src->a16[2*i+1];
}

void phsubw64(MM *src, MM *dst, MM *res)
{
    res->a16[0] = dst->a16[0] - dst->a16[1];
    res->a16[1] = dst->a16[2] - dst->a16[3];
    res->a16[2] = src->a16[0] - src->a16[1];
    res->a16[3] = src->a16[2] - src->a16[3];
}

/** packed horizontal subtract double **/
void phsubd128(XMM *src, XMM *dst, XMM *res)
{
    int i;
    for(i = 0; i < 2; ++i)
	    res->a32[i  ] = dst->a32[2*i] - dst->a32[2*i+1];
    for(i = 0; i < 2; ++i)
	    res->a32[i+2] = src->a32[2*i] - src->a32[2*i+1];
}

void phsubd64(MM *src, MM *dst, MM *res)
{
    res->a32[0] = dst->a32[0] - dst->a32[1];
    res->a32[1] = src->a32[0] - src->a32[1];
}

/** packed horizontal subtract and saturate word **/
void phsubsw128(XMM *src, XMM *dst, XMM *res)
{
    int i;
    for(i = 0; i < 4; ++i)
	    res->a16[i] = SATSW( dst->a16[2*i] - dst->a16[2*i+1] );
    for(i = 0; i < 4; ++i)
	    res->a16[i+4] = SATSW( src->a16[2*i] - src->a16[2*i+1] );
}

void phsubsw64(MM *src, MM *dst, MM *res)
{
    res->a16[0] = SATSW( dst->a16[0] - dst->a16[1] );
    res->a16[1] = SATSW( dst->a16[2] - dst->a16[3] );
    res->a16[2] = SATSW( src->a16[0] - src->a16[1] );
    res->a16[3] = SATSW( src->a16[2] - src->a16[3] );
}

/** packed sign byte **/
void psignb128(XMM *src, XMM *dst, XMM *res)
{
    int i;
    for(i = 0; i < 16; ++i) {
	    if(src->a8[i] < 0) res->a8[i] = -dst->a8[i];
	    else if(src->a8[i] == 0) res->a8[i] = 0;
	    else res->a8[i] = dst->a8[i];
    }
}

void psignb64(MM *src, MM *dst, MM *res)
{
    int i;
    for(i = 0; i < 8; ++i) {
	    if(src->a8[i] < 0) res->a8[i] = -dst->a8[i];
	    else if(src->a8[i] == 0) res->a8[i] = 0;
	    else res->a8[i] = dst->a8[i];
    }
}

/** packed sign word **/
void psignw128(XMM *src, XMM *dst, XMM *res)
{
    int i;
    for(i = 0; i < 8; ++i) {
	    if(src->a16[i] < 0) res->a16[i] = -dst->a16[i];
	    else if(src->a16[i] == 0) res->a16[i] = 0;
	    else res->a16[i] = dst->a16[i];
    }
}

void psignw64(MM *src, MM *dst, MM *res)
{
    int i;
    for(i = 0; i < 4; ++i) {
	    if(src->a16[i] < 0) res->a16[i] = -dst->a16[i];
	    else if(src->a16[i] == 0) res->a16[i] = 0;
	    else res->a16[i] = dst->a16[i];
    }
}

/** packed sign double **/
void psignd128(XMM *src, XMM *dst, XMM *res)
{
    int i;
    for(i = 0; i < 4; ++i) {
	    if(src->a32[i] < 0) res->a32[i] = -dst->a32[i];
	    else if(src->a32[i] == 0) res->a32[i] = 0;
	    else res->a32[i] = dst->a32[i];
    }
}

void psignd64(MM *src, MM *dst, MM *res)
{
    int i;
    for(i = 0; i < 2; ++i) {
	    if(src->a32[i] < 0) res->a32[i] = -dst->a32[i];
	    else if(src->a32[i] == 0) res->a32[i] = 0;
	    else res->a32[i] = dst->a32[i];
    }
}

/** packed multiply high with round and scale word **/
void pmulhrsw128(XMM *src, XMM *dst, XMM *res)
{
    int i;
    for(i = 0; i < 8; ++i)
	    res->a16[i] = (((dst->a16[i] * src->a16[i] >> 14) + 1) >> 1);
}

void pmulhrsw64(MM *src, MM *dst, MM *res)
{
    int i;
    for(i = 0; i < 4; ++i)
	    res->a16[i] = (((dst->a16[i] * src->a16[i] >> 14) + 1) >> 1);
}

/** packed absolute value byte **/
void pabsb128(XMM *src, XMM *res)
{
    int i;
    for(i = 0; i < 16; ++i)
	    if(src->a8[i] < 0) res->a8[i] = -src->a8[i];
	    else res->a8[i] = src->a8[i];
}

void pabsb64(MM *src, MM *res)
{
    int i;
    for(i = 0; i < 8; ++i)
	    if(src->a8[i] < 0) res->a8[i] = -src->a8[i];
	    else res->a8[i] = src->a8[i];
}

/** packed absolute value word **/
void pabsw128(XMM *src, XMM *res)
{
    int i;
    for(i = 0; i < 8; ++i)
	    if(src->a16[i] < 0) res->a16[i] = -src->a16[i];
	    else res->a16[i] = src->a16[i];
}

void pabsw64(MM *src, MM *res)
{
    int i;
    for(i = 0; i < 4; ++i)
	    if(src->a16[i] < 0) res->a16[i] = -src->a16[i];
	    else res->a16[i] = src->a16[i];
}

/** packed absolute value double **/
void pabsd128(XMM *src, XMM *res)
{
    int i;
    for(i = 0; i < 4; ++i)
	    if(src->a32[i] < 0) res->a32[i] = -src->a32[i];
	    else res->a32[i] = src->a32[i];
}

void pabsd64(MM *src, MM *res)
{
    int i;
    for(i = 0; i < 2; ++i)
	    if(src->a32[i] < 0) res->a32[i] = -src->a32[i];
	    else res->a32[i] = src->a32[i];
}

/** packed align right **/
void palignr128(XMM *src, XMM *dst, XMM *res, uint8_t IMM)
{
	int n = IMM * 8;
	__uint128_t low, high;
	low = src->ua128;
	high = dst->ua128;
	res->ua128 = (low >> n) + (high << (128-n));
}

void palignr64(MM *src, MM *dst, MM *res, uint8_t IMM)
{
	int n = IMM * 8;
	__uint128_t t;
	t = src->ua64 | ((__uint128_t)dst->ua64 << 64);
	t >>= n;
	res->ua64 = t;
}


