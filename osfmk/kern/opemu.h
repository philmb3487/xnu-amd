#ifndef OPEMU_H
#define OPEMU_H
#include <stdint.h>

#ifndef TESTCASE
#include <mach/thread_status.h>
#endif

union XMM_u {
    int8_t a8[16];
    int16_t a16[8];
    int32_t a32[4];
    int64_t a64[2];
    __int128_t a128;
    uint8_t ua8[16];
    uint16_t ua16[8];
    uint32_t ua32[4];
    uint64_t ua64[2];
    __uint128_t ua128;
    double fa64[2];
    float fa32[4];
};
typedef union XMM_u XMM;

union MM_u {
    int8_t a8[8];
    int16_t a16[4];
    int32_t a32[2];
    int64_t a64;
    uint8_t ua8[8];
    uint16_t ua16[4];
    uint32_t ua32[2];
    uint64_t ua64;
};
typedef union MM_u MM;

#ifndef TESTCASE
/** XNU TRAP HANDLERS **/
void opemu_ktrap(x86_saved_state_t *state);
void opemu_utrap(x86_saved_state_t *state);
int ssse3_run(uint8_t *instruction, x86_saved_state_t *state, int longmode, int kernel_trap);
int sse3_run_a(uint8_t *instruction, x86_saved_state_t *state, int longmode, int kernel_trap);
int sse3_run_b(uint8_t *instruction, x86_saved_state_t *state, int longmode, int kernel_trap);
int sse3_run_c(uint8_t *instruction, x86_saved_state_t *state, int longmode, int kernel_trap);
int sysenter_run(uint8_t *instruction, x86_saved_state_t *state, int longmode);
int sysexit_run(uint8_t *instruction, x86_saved_state_t *state, int longmode, int kernel_trap);
int monitor_run(uint8_t *instruction);
int fisttp_run(uint8_t **instruction);

int fetchoperands(uint8_t *ModRM, unsigned int hsrc, unsigned int hdst, void *src, void *dst,
                  unsigned int longmode, x86_saved_state_t *regs, int kernel_trap, int size_128, int ins_size);
void storeresult128(uint8_t ModRM, unsigned int hdst, XMM res);
void storeresult64(uint8_t ModRM, unsigned int hdst, MM res);
#endif

void print_bytes(uint8_t *from, int size);

void getxmm(XMM *v, unsigned int i);
void getmm(MM *v, unsigned int i);
void movxmm(XMM *v, unsigned int i);
void movmm(MM *v, unsigned int i);

/** All 32 SSSE3 instructions **/
void pshufb128	(XMM *src, XMM *dst, XMM *res);
void pshufb64	(MM *src, MM *dst, MM *res);
void phaddw128	(XMM *src, XMM *dst, XMM *res);
void phaddw64	(MM *src, MM *dst, MM *res);
void phaddd128	(XMM *src, XMM *dst, XMM *res);
void phaddd64	(MM *src, MM *dst, MM *res);
void phaddsw128	(XMM *src, XMM *dst, XMM *res);
void phaddsw64	(MM *src, MM *dst, MM *res);
void pmaddubsw128(XMM *src, XMM *dst, XMM *res);
void pmaddubsw64(MM *src, MM *dst, MM *res);
void phsubw128	(XMM *src, XMM *dst, XMM *res);
void phsubw64	(MM *src, MM *dst, MM *res);
void phsubd128	(XMM *src, XMM *dst, XMM *res);
void phsubd64	(MM *src, MM *dst, MM *res);
void phsubsw128	(XMM *src, XMM *dst, XMM *res);
void phsubsw64	(MM *src, MM *dst, MM *res);
void psignb128	(XMM *src, XMM *dst, XMM *res);
void psignb64	(MM *src, MM *dst, MM *res);
void psignw128	(XMM *src, XMM *dst, XMM *res);
void psignw64	(MM *src, MM *dst, MM *res);
void psignd128	(XMM *src, XMM *dst, XMM *res);
void psignd64	(MM *src, MM *dst, MM *res);
void pmulhrsw128(XMM *src, XMM *dst, XMM *res);
void pmulhrsw64	(MM *src, MM *dst, MM *res);
void pabsb128	(XMM *src, XMM *res);
void pabsb64	(MM *src, MM *res);
void pabsw128	(XMM *src, XMM *res);
void pabsw64	(MM *src, MM *res);
void pabsd128	(XMM *src, XMM *res);
void pabsd64	(MM *src, MM *res);
void palignr128	(XMM *src, XMM *dst, XMM *res, uint8_t IMM);
void palignr64	(MM *src, MM *dst, MM *res, uint8_t IMM);
void addsubpd(XMM *src, XMM *dst, XMM *res);
void addsubps(XMM *src, XMM *dst, XMM *res);
void haddpd(XMM *src, XMM *dst, XMM *res);
void haddps(XMM *src, XMM *dst, XMM *res);
void hsubpd(XMM *src, XMM *dst, XMM *res);
void hsubps(XMM *src, XMM *dst, XMM *res);
void lddqu(XMM *src, XMM *res);
void movddup(XMM *src, XMM *res);
void movshdup(XMM *src, XMM *res);
void movsldup(XMM *src, XMM *res);

#endif

