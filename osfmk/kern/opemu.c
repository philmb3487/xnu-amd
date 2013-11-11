/*** SINETEK **
  * This is called the Opcode Emulator: it traps invalid opcode exceptions
  *   and modifies the state of the running thread accordingly.
  * There are two entry points, one for user space exceptions, and another for
  *   exceptions coming from kernel space.
  *
  * STATUS
  *  . SSSE3 is implemented.
  *  . SYSENTER is implemented.
  *
  *   ** AnV **
  * STATUS
  *  . SSE3 is implemented.
  *  . monitor/mwait is implemented.
  *
  * AnV:
  *  . Added SSE3 emulator
  *  . Changed SYSENTER to SYSCALL
  *  . Changed SYSEXIT to SYSRET
  */
#include <stdint.h>
#include "opemu.h"

#ifndef TESTCASE
#include <kern/sched_prim.h>

// forward declaration for syscall handlers of mach/bsd;
extern void mach_call_munger(x86_saved_state_t *state);
extern void mach_call_munger64(x86_saved_state_t *regs);
extern void unix_syscall(x86_saved_state_t *);
extern void unix_syscall64(x86_saved_state_t *);

#ifdef __i386__
// forward declaration of panic handler for kernel traps;
//extern void panic_trap(x86_saved_state32_t *saved_state);
void opemu_ktrap(x86_saved_state_t *state)
{
    x86_saved_state32_t *saved_state = saved_state32(state);
    uint8_t code_buffer[15];
    unsigned int bytes_skip = 0;

    sysexit_done = 0;

    memcpy(code_buffer, (const void*) saved_state->eip, 15);
    
    bytes_skip = ssse3_run(code_buffer, state, 0, 1);
    if (!bytes_skip)
        bytes_skip = sse3_run_a(code_buffer, state, 0, 1);
    if (!bytes_skip)
        bytes_skip = sse3_run_b(code_buffer, state, 0, 1);
    if (!bytes_skip)
        bytes_skip = sse3_run_c(code_buffer, state, 0, 1);
    if (!bytes_skip)
        bytes_skip = sysenter_run(code_buffer, state, 0);
    if (!bytes_skip)
    {
        bytes_skip = sysexit_run(code_buffer, state, 0, 1);
        if (bytes_skip == 0x7FFF)
        {
            return;
        }
    }
    if (!bytes_skip)
        bytes_skip = monitor_run(code_buffer);
    if (!bytes_skip)
        bytes_skip = fisttp_run((uint8_t **)&code_buffer);
    if (!bytes_skip)
    {
        printf ("Invalid kernel opcode: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n", code_buffer[0], code_buffer[1], code_buffer[2], code_buffer[3], code_buffer[4], code_buffer[5], code_buffer[6], code_buffer[7]);
        return;
    }
    
    if (bytes_skip != 0xFFFF)
        saved_state->eip += bytes_skip;
    else
        memcpy((void *) saved_state->eip, code_buffer, 15);
}

#else /* __x86_64__ */
// forward declaration of panic handler for kernel traps;
//extern void panic_trap(x86_saved_state64_t *regs);
void opemu_ktrap(x86_saved_state_t *state)
{
    x86_saved_state64_t *saved_state = saved_state64(state);
    uint8_t code_buffer[15];
    unsigned int bytes_skip = 0;
    
    memcpy(code_buffer, (const void*) saved_state->isf.rip, 15);
    
    bytes_skip = ssse3_run(code_buffer, state, 1, 1);
    if (!bytes_skip)
        bytes_skip = sse3_run_a(code_buffer, state, 1, 1);
    if (!bytes_skip)
        bytes_skip = sse3_run_b(code_buffer, state, 1, 1);
    if (!bytes_skip)
        bytes_skip = sse3_run_c(code_buffer, state, 1, 1);
    if (!bytes_skip)
        bytes_skip = sysenter_run(code_buffer, state, 1);
    if (!bytes_skip)
    {
        bytes_skip = sysexit_run(code_buffer, state, 1, 1);
        if (bytes_skip == 0x7FFF)
        {
            return;
        }
    }
    if (!bytes_skip)
        bytes_skip = monitor_run(code_buffer);
    if (!bytes_skip)
        bytes_skip = fisttp_run((uint8_t **)&code_buffer);
    if (!bytes_skip)
    {
        printf ("Opemu invalid kernel opcode: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n", code_buffer[0], code_buffer[1], code_buffer[2], code_buffer[3], code_buffer[4], code_buffer[5], code_buffer[6], code_buffer[7]);
        return;
    }
    if (bytes_skip != 0xFFFF)
        saved_state->isf.rip += bytes_skip;
    else
        memcpy((void *) saved_state->isf.rip, code_buffer, 15);
}
#endif


void opemu_utrap(x86_saved_state_t *state)
{
    	uint8_t code_buffer[15];
    	int longmode;
    	unsigned int bytes_skip = 0;
    
    	if((longmode = is_saved_state64(state))) {
        		x86_saved_state64_t *regs;
        		regs = saved_state64(state);
        		copyin(regs->isf.rip, (char*) code_buffer, 15);
        		if (!bytes_skip)
            			bytes_skip = sysenter_run(code_buffer, state, longmode);
                if (!bytes_skip)
                        bytes_skip = sysexit_run(code_buffer, state, longmode, 0);
        		if (!bytes_skip)
            			bytes_skip = monitor_run(code_buffer);
        		if (bytes_skip != 0xFFFF)
            			regs->isf.rip += bytes_skip;
        		else
            			memcpy((void *) regs->isf.rip, code_buffer, 15);
        
        		if(!bytes_skip) printf ("Opemu invalid user 64-bit opcode: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n", code_buffer[0], code_buffer[1], code_buffer[2], code_buffer[3], code_buffer[4], code_buffer[5], code_buffer[6], code_buffer[7]);
        	} else {
            		x86_saved_state32_t *regs;
            		regs = saved_state32(state);
            		copyin(regs->eip, (char*) code_buffer, 15);
            		if (!bytes_skip)
                			bytes_skip = sysenter_run(code_buffer, state, longmode);
                    if (!bytes_skip)
                            bytes_skip = sysexit_run(code_buffer, state, longmode, 0);
            		if (bytes_skip != 0xFFFF)
                			regs->eip += bytes_skip;
            		else
                			memcpy((void *) regs->eip, code_buffer, 15);
            		if(!bytes_skip) printf ("Opemu invalid user 32-bit opcode: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n", code_buffer[0], code_buffer[1], code_buffer[2], code_buffer[3], code_buffer[4], code_buffer[5], code_buffer[6], code_buffer[7]);
            	}
    
    	thread_exception_return();
    	/*** NOTREACHED ***/
    }

/** Runs the sysenter emulator.
   **/
int sysenter_run(uint8_t *instruction, x86_saved_state_t *state, int longmode)
{
    	uint8_t *bytep = instruction;
    
        	if(bytep[0] == 0x0F && bytep[1] == 0x34) {
        	    // sysenter, TODO remove redundancy regs load
        	    //   edx/rdx        return address
        	    //   ecx/rcx        return stack
                if (longmode)
                {
                    x86_saved_state64_t *regs;
                    regs = saved_state64(state);
                    regs->isf.rip = regs->rdx;
                    regs->isf.rsp = regs->rcx;

                    if((signed int)regs->rax < 0) {
                        //      printf("mach call 64\n");
            		    mach_call_munger64(state);
            	    } else {
                	    //      printf("unix call 64\n");
                        unix_syscall64(state);
                    }
                } else {
                    x86_saved_state32_t *regs;
                    regs = saved_state32(state);
                    regs->eip = regs->edx;
                    regs->uesp = regs->ecx;

                    if((signed int)regs->eax < 0) {
                        //      printf("mach call 32\n");
            		    mach_call_munger(state);
            	    } else {
                	    //      printf("unix call 32\n");
                        unix_syscall(state);
                    }
                }

        	    /* NEVER REACHES */
        	    return 2;
        	}
    
    
    	return 0;
}

/** Runs the sysexit emulator.
 **/
int sysexit_run(uint8_t *instruction, x86_saved_state_t * state, int longmode, int kernel_trap)
{
    uint8_t *bytep = instruction;
    vm_offset_t addr;
    
    if(bytep[0] == 0x0F && bytep[1] == 0x35) {
        if (longmode)
        {
            x86_saved_state64_t *regs;
            regs = saved_state64(state);
            regs->isf.rip = regs->rdx;
            regs->isf.rsp = regs->rcx;
            if (kernel_trap)
            {
                addr = regs->rcx;
                return 0x7FFF;
            } else {
                thread_exception_return();
            }
        } else {
            x86_saved_state32_t *regs;
            regs = saved_state32(state);
            regs->eip = regs->edx;
            regs->uesp = regs->ecx;
            if (kernel_trap)
            {
                addr = regs->ecx;
                return 0x7FFF;
            } else {
                thread_exception_return();
            }
        }

        /* NEVER REACHES */
        return 2;
    }
    
    return 0;
}

/** Skips invalid monitor/mwait instructions. returns the number of bytes consumed.
 **/
int monitor_run(uint8_t *instruction)
{
    uint8_t *bytep = instruction;
    int ins_size = 0;
    
    switch (*bytep)
    {
        case 0xC8:
        case 0xC9:
            ins_size++;
            return(ins_size);
            break;
    }
    if (*bytep != 0x0F) return 0;
    bytep++;
    ins_size++;
    
    if (*bytep != 0x01) return 0;
    bytep++;
    ins_size++;
    
    return 0;
}

/** Runs the ssse3 emulator. returns the number of bytes consumed.
 **/
int ssse3_run(uint8_t *instruction, x86_saved_state_t *state, int longmode, int kernel_trap)
{
    // pointer to the current byte we're working on
    uint8_t *bytep = instruction;
    int ins_size = 0;
    int is_128 = 0, src_higher = 0, dst_higher = 0;
    XMM xmmsrc, xmmdst, xmmres;
    MM  mmsrc, mmdst, mmres;
    
    /** We can get a few prefixes, in any order:
     **  66 throws into 128-bit xmm mode.
     **  40->4f use higher xmm registers.
     **/
    if(*bytep == 0x66) {
        is_128 = 1;
        bytep++;
        ins_size++;
    }
    if((*bytep & 0xF0) == 0x40) {
        if(*bytep & 1) src_higher = 1;
        if(*bytep & 4) dst_higher = 1;
        bytep++;
        ins_size++;
    }
    
    if(*bytep != 0x0f) return 0;
    bytep++;
    ins_size++;
    
    /* Two SSSE3 instruction prefixes. */
    if((*bytep == 0x38 && bytep[1] != 0x0f) || (*bytep == 0x3a && bytep[1] == 0x0f)) {
        uint8_t opcode = bytep[1];
        uint8_t *modrm = &bytep[2];
        uint8_t operand;
        ins_size += 2; // not counting modRM byte or anything after.
        
        if(is_128) {
            int consumed = fetchoperands(modrm, src_higher, dst_higher, &xmmsrc, &xmmdst, longmode, state, kernel_trap, 1, ins_size);
            operand = bytep[2 + consumed];
            ins_size += consumed;
            
            // Bornzovka - add and modified XMM.
            
            XMM *xsrc, *xdst, *xres;
            xsrc = &xmmsrc;
            xdst = &xmmdst;
            xres = &xmmres;
            
            switch(opcode) {
                case 0x00: pshufb128(xsrc,xdst,xres); break;
                case 0x01: phaddw128(xsrc,xdst,xres); break;
                case 0x02: phaddd128(xsrc,xdst,xres); break;
                case 0x03: phaddsw128(xsrc,xdst,xres); break;
                case 0x04: pmaddubsw128(xsrc,xdst,xres); break;
                case 0x05: phsubw128(xsrc,xdst,xres); break;
                case 0x06: phsubd128(xsrc,xdst,xres); break;
                case 0x07: phsubsw128(xsrc,xdst,xres); break;
                case 0x08: psignb128(xsrc,xdst,xres); break;
                case 0x09: psignw128(xsrc,xdst,xres); break;
                case 0x0A: psignd128(xsrc,xdst,xres); break;
                case 0x0B: pmulhrsw128(xsrc,xdst,xres); break;
                case 0x0F: palignr128(xsrc, xdst, xres, operand);
                    ins_size++;
                    break;
                case 0x1C: pabsb128(xsrc,xres); break;
                case 0x1D: pabsw128(xsrc,xres); break;
                case 0x1E: pabsd128(xsrc,xres); break;
                default: return 0;
            }
            
            storeresult128(*modrm, dst_higher, xmmres);
        } else {
            int consumed = fetchoperands(modrm, src_higher, dst_higher, &mmsrc, &mmdst, longmode, state, kernel_trap, 0, ins_size);
            operand = bytep[2 + consumed];
            ins_size += consumed;
            
            // Bornzovka - add and modified MM.
            
            MM *msrc, *mdst, *mres;
            msrc = &mmsrc;
            mdst = &mmdst;
            mres = &mmres;
            
            
            switch(opcode) {
                case 0x00: pshufb64(msrc,mdst,mres); break;
                case 0x01: phaddw64(msrc,mdst,mres); break;
                case 0x02: phaddd64(msrc,mdst,mres); break;
                case 0x03: phaddsw64(msrc,mdst,mres); break;
                case 0x04: pmaddubsw64(msrc,mdst,mres); break;
                case 0x05: phsubw64(msrc,mdst,mres); break;
                case 0x06: phsubd64(msrc,mdst,mres); break;
                case 0x07: phsubsw64(msrc,mdst,mres); break;
                case 0x08: psignb64(msrc,mdst,mres); break;
                case 0x09: psignw64(msrc,mdst,mres); break;
                case 0x0A: psignd64(msrc,mdst,mres); break;
                case 0x0B: pmulhrsw64(msrc,mdst,mres); break;
                case 0x0F: palignr64(msrc,mdst,mres, operand);
                    ins_size++;
                    break;
                case 0x1C: pabsb64(msrc,mres); break;
                case 0x1D: pabsw64(msrc,mres); break;
                case 0x1E: pabsd64(msrc,mres); break;
                default: return 0;
            }
            
            storeresult64(*modrm, dst_higher, mmres);
        }
        
    } else {
        // opcode wasn't handled here
        return 0;
    }
    
    return ins_size;
}


/** Runs the sse3 emulator. returns the number of bytes consumed.
 **/
int sse3_run_a(uint8_t *instruction, x86_saved_state_t *state, int longmode, int kernel_trap)
{
    uint8_t *bytep = instruction;
    int ins_size = 0;
    XMM xmmsrc, xmmdst, xmmres;
    int src_higher = 0, dst_higher = 0;
    
    if(*bytep != 0xF2) return 0;
    
    bytep++;
    ins_size++;
    
    if(*bytep != 0x0f) return 0;
    bytep++;
    ins_size++;
    
    uint8_t *modrm = &bytep[1];
    ins_size += 1;
    int consumed = fetchoperands(modrm, src_higher, dst_higher, &xmmsrc, &xmmdst, longmode, state, kernel_trap, 1, ins_size);
    ins_size += consumed;
    
    // Bornzovka - add and modified XMM.
    
    XMM *xsrc, *xdst, *xres;
    xsrc = &xmmsrc;
    xdst = &xmmdst;
    xres = &xmmres;
    
    switch (*bytep)
    {
        case 0x12:
            movddup(xsrc,xres);
            break;
        case 0x7C:
            haddps(xsrc,xdst,xres);
            break;
        case 0x7D:
            hsubps(xsrc,xdst,xres);
            break;
        case 0xD0:
            addsubps(xsrc,xdst,xres);
            break;
        case 0xF0:
            lddqu(xsrc,xres);
            break;
        default:
            return 0;
    }
    
    storeresult128(*modrm, dst_higher, xmmres);
    
    return ins_size;
}

int sse3_run_b(uint8_t *instruction, x86_saved_state_t *state, int longmode, int kernel_trap)
{
    uint8_t *bytep = instruction;
    int ins_size = 0;
    XMM xmmsrc, xmmdst, xmmres;
    int src_higher = 0, dst_higher = 0;
    
    if(*bytep != 0xF3) return 0;
    
    bytep++;
    ins_size++;
    
    if(*bytep != 0x0f) return 0;
    bytep++;
    ins_size++;
    
    uint8_t *modrm = &bytep[1];
    ins_size += 1;
    int consumed = fetchoperands(modrm, src_higher, dst_higher, &xmmsrc, &xmmdst, longmode, state, kernel_trap, 1, ins_size);
    ins_size += consumed;
    
    // Bornzovka - add and modified XMM.
    
    XMM *xsrc, *xdst, *xres;
    xsrc = &xmmsrc;
    xdst = &xmmdst;
    xres = &xmmres;
    
    switch (*bytep)
    {
        case 0x12:
            movsldup(xsrc,xres);
            break;
        case 0x16:
            movshdup(xsrc,xres);
            break;
        default:
            return 0;
    }
    
    storeresult128(*modrm, dst_higher, xmmres);
    
    return ins_size;
}

int sse3_run_c(uint8_t *instruction, x86_saved_state_t *state, int longmode, int kernel_trap)
{
    uint8_t *bytep = instruction;
    int ins_size = 0;
    XMM xmmsrc, xmmdst, xmmres;
    int src_higher = 0, dst_higher = 0;
    
    if(*bytep != 0x66) return 0;
    
    bytep++;
    ins_size++;
    
    if(*bytep != 0x0f) return 0;
    bytep++;
    ins_size++;
    
    uint8_t *modrm = &bytep[1];
    ins_size += 1;
    int consumed = fetchoperands(modrm, src_higher, dst_higher, &xmmsrc, &xmmdst, longmode, state, kernel_trap, 1, ins_size);
    ins_size += consumed;
    
    // Bornzovka - add and modified XMM.
    
    XMM *xsrc, *xdst, *xres;
    xsrc = &xmmsrc;
    xdst = &xmmdst;
    xres = &xmmres;
    
    switch (*bytep)
    {
        case 0x7C:
            haddpd(xsrc,xdst,xres);
            break;
        case 0x7D:
            hsubpd(xsrc,xdst,xres);
            break;
        case 0xD0:
            addsubpd(xsrc,xdst,xres);
            break;
        default:
            return 0;
    }
    
    storeresult128(*modrm, dst_higher, xmmres);
    
    return ins_size;
}

int fisttp_run(uint8_t **instruction)
{
    uint8_t *bytep = *instruction;
    
    if (((bytep[0] & 0xf0) == 0xd0) &&
        (((bytep[1] >> 3) & 7) == 1)) {
        
        switch (bytep[0]) {
            case 0xdf: /* word */
            case 0xdb: /* dword */
                bytep[1] |= (3 << 3);
                break;
            case 0xdd: /* qword */
                bytep[0] = 0xdf;
                bytep[1] |= (7 << 3);
                break;
            default:
                return 0;
        }
        
        memcpy(*instruction, bytep, 15);
        
        return 0xFFFF;
    }
    
    return 0;
}

void print_bytes(uint8_t *from, int size)
{
    int i;
    for(i = 0; i < size; ++i)
    {
        printf("%02x ", from[i]);
    }
    printf("\n");
}

/** Fetch SSSE3 operands (except immediate values, which are fetched elsewhere).
   * We depend on memory copies, which differs depending on whether we're in kernel space
   * or not. For this reason, need to pass in a lot of information, including the state of
   * registers.
   *
   * The return value is the number of bytes used, including the ModRM byte,
   * and displacement values, as well as SIB if used.
   */
int fetchoperands(uint8_t *ModRM, unsigned int hsrc, unsigned int hdst, void *src, void *dst, unsigned int longmode, x86_saved_state_t *saved_state, int kernel_trap, int size_128, int __unused ins_size)
{
    unsigned int num_src = *ModRM & 0x7;
    unsigned int num_dst = (*ModRM >> 3) & 0x7;
    unsigned int mod = *ModRM >> 6;
    int consumed = 1;
    
    if(hsrc) num_src += 8;
    if(hdst) num_dst += 8;
    if(size_128) getxmm((XMM*)dst, num_dst);
    else getmm((MM*)dst, num_dst);
    
    if(mod == 3) {
        if(size_128) getxmm((XMM*)src, num_src);
        else getmm((MM*)src, num_src);
    } else if (longmode) {
        uint64_t address;
        
        // DST is always an XMM register. decode for SRC.
        x86_saved_state64_t* r64 = saved_state64(saved_state);
        uint64_t reg_sel[8] = {r64->rax, r64->rcx, r64->rdx,
            r64->rbx, r64->isf.rsp, r64->rbp,
            r64->rsi, r64->rdi};
        //if(hsrc) printf("opemu error: high reg ssse");
        if(num_src == 4) {
            /* Special case: SIB byte used TODO fix r8-r15? */
            uint8_t scale = ModRM[1] >> 6;
            uint8_t base = ModRM[1] & 0x7;
            uint8_t index = (ModRM[1] >> 3) & 0x7;
            consumed++;
            
            // meaning of SIB depends on mod
            if(mod == 0) {
                //if(base == 5) printf("opemu error: mod0 disp32 not implemented\n");
                if(index == 4) address = reg_sel[base];
                else address = reg_sel[base] + (reg_sel[index] * (1<<scale));
            } else {
                if(index == 4) address = reg_sel[base];
                else address = reg_sel[base] + (reg_sel[index] * (1<<scale));
            }
        } else {
            address = reg_sel[num_src];
        }
        
        if((mod == 0) && (num_src == 5)) {
            // special case, RIP-relative dword displacement
            // TODO fix
            //printf("opemu error: no rip yet");
            address = r64->isf.rip + *((int32_t*)&ModRM[consumed]);
            consumed += 4;
        } if(mod == 1) {
            // byte displacement
            address += (int8_t)ModRM[consumed];//-----------
            consumed++;
        } else if(mod == 2) {
            // dword displacement. can it be qword?
            address += *((int32_t*)&ModRM[consumed]);
            consumed += 4;
        }
        
        // address is good now, do read and store operands.
        if(kernel_trap) {
            if(size_128) ((XMM*)src)->ua128 = *((__uint128_t*)address);
            else ((MM*)src)->ua64 = *((uint64_t*)address);
        } else {
            //printf("xnu: da = %llx, rsp=%llx,  rip=%llx\n", address, reg_sel[4], r64->isf.rip);
            if(size_128) copyin(address, (char*) &((XMM*)src)->ua128, 16);
            else copyin(address, (char*) &((MM*)src)->ua64, 8);
        }
    } else { /* AnV - modified Sinetek's source for 32-bit */

        uint32_t address;
        
        // DST is always an XMM register. decode for SRC.
        x86_saved_state32_t* r32 = saved_state32(saved_state);
        uint32_t reg_sel[8] = {r32->eax, r32->ecx, r32->edx,
            r32->ebx, r32->uesp, r32->ebp,
            r32->esi, r32->edi};
        //if(hsrc) printf("opemu error: high reg ssse");
        if(num_src == 4) {
            /* Special case: SIB byte used TODO fix r8-r15? */
            uint8_t scale = ModRM[1] >> 6;
            uint8_t base = ModRM[1] & 0x7;
            uint8_t index = (ModRM[1] >> 3) & 0x7;
            consumed++;
            
            // meaning of SIB depends on mod
            if(mod == 0) {
                //if(base == 5) printf("opemu error: mod0 disp32 not implemented\n");
                if(index == 4) address = reg_sel[base];
                else address = reg_sel[base] + (reg_sel[index] * (1<<scale));
            } else {
                if(index == 4) address = reg_sel[base];
                else address = reg_sel[base] + (reg_sel[index] * (1<<scale));
            }
        } else {
            address = reg_sel[num_src];
        }
        
        if((mod == 0) && (num_src == 5)) {
            // special case, RIP-relative dword displacement
            // TODO fix
            //printf("opemu error: no rip yet");
            address = r32->eip + *((int32_t*)&ModRM[consumed]);
            consumed += 4;
        } if(mod == 1) {
            // byte displacement
            address += (int8_t)ModRM[consumed];//------------
            consumed++;
        } else if(mod == 2) {
            // dword displacement. can it be qword?
            address += *((int32_t*)&ModRM[consumed]);
            consumed += 4;
        }
        
        // address is good now, do read and store operands.
        if(kernel_trap) {
            if(size_128) ((XMM*)src)->ua128 = *((__uint128_t*)address);
            else ((MM*)src)->ua64 = *((uint64_t*)address);
        } else {
            //printf("xnu: da = %llx, rsp=%llx,  rip=%llx\n", address, reg_sel[4], r32->eip);
            if(size_128) copyin(address, (char*) &((XMM*)src)->ua128, 16);
            else copyin(address, (char*) &((MM*)src)->ua64, 8);
        }
    }
    
    return consumed;
}

void storeresult128(uint8_t ModRM, unsigned int hdst, XMM res)
{
    	unsigned int num_dst = (ModRM >> 3) & 0x7;
    	if(hdst) num_dst += 8;
        	movxmm(&res, num_dst);
}
void storeresult64(uint8_t ModRM, unsigned int __unused hdst, MM res)
{
    	unsigned int num_dst = (ModRM >> 3) & 0x7;
    	movmm(&res, num_dst);
}

#endif /* TESTCASE */

/* get value from the xmm register i */
void getxmm(XMM *v, unsigned int i)
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
#ifdef __x86_64__
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
#endif
        	}
    }

/* get value from the mm register i  */
void getmm(MM *v, unsigned int i)
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
void movxmm(XMM *v, unsigned int i)
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
#ifdef __x86_64__
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
#endif
        	}
    }

/* move value over to mm register i */
void movmm(MM *v, unsigned int i)
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


/****************************************/
/* Bronzovka: Correcting for old amd    */
/****************************************/
 
/***************************************/
/** SSE3 instructions implementation  **/
/***************************************/

void addsubpd(XMM *src, XMM *dst, XMM *res)
{
    res->fa64[0] = src->fa64[0] - dst->fa64[0];
    res->fa64[1] = src->fa64[1] + dst->fa64[1];
}

void addsubps(XMM *src, XMM *dst, XMM *res)
{
    res->fa32[0] = src->fa32[0] - dst->fa32[0];
    res->fa32[1] = src->fa32[1] + dst->fa32[1];
    res->fa32[2] = src->fa32[2] - dst->fa32[2];
    res->fa32[3] = src->fa32[3] + dst->fa32[3];
}

void haddpd(XMM *src, XMM *dst, XMM *res)
{
    res->fa64[0] = src->fa64[0] + src->fa64[1];
    res->fa64[1] = dst->fa64[0] + dst->fa64[1];
}

void haddps(XMM *src, XMM *dst, XMM *res)
{
    res->fa32[0] = src->fa32[0] + src->fa32[1];
    res->fa32[1] = src->fa32[2] + src->fa32[3];
    res->fa32[2] = dst->fa32[0] + dst->fa32[1];
    res->fa32[3] = dst->fa32[2] + dst->fa32[3];
}

void hsubpd(XMM *src, XMM *dst, XMM *res)
{
    res->fa64[0] = src->fa64[0] - src->fa64[1];
    res->fa64[1] = dst->fa64[0] - dst->fa64[1];
}

void hsubps(XMM *src, XMM *dst, XMM *res)
{
    res->fa32[0] = src->fa32[0] - src->fa32[1];
    res->fa32[1] = src->fa32[2] - src->fa32[3];
    res->fa32[2] = dst->fa32[0] - dst->fa32[1];
    res->fa32[3] = dst->fa32[2] - dst->fa32[3];
}

void lddqu(XMM *src, XMM *res)
{
    res->fa64[0] = src->fa64[0];
    res->fa64[1] = src->fa64[1];
}

void movddup(XMM *src, XMM *res)
{
    res->fa64[0] = src->fa64[0];
    res->fa64[1] = src->fa64[0];
}

void movshdup(XMM *src, XMM *res)
{
    res->fa32[0] = src->fa32[1];
    res->fa32[1] = src->fa32[1];
    res->fa32[2] = src->fa32[3];
    res->fa32[3] = src->fa32[3];
}

void movsldup(XMM *src, XMM *res)
{
    res->fa32[0] = src->fa32[0];
    res->fa32[1] = src->fa32[0];
    res->fa32[2] = src->fa32[2];
    res->fa32[3] = src->fa32[2];
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

