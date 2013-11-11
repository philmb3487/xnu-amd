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
void opemu_trap(x86_saved_state_t *saved_state);
uint64_t opemu(uint8_t *code, x86_saved_state_t *saved_state);
#endif
inline void getxmm(XMM *v, unsigned int i);
inline void getmm(MM *v, unsigned int i);
inline void movxmm(XMM *v, unsigned int i);
inline void movmm(MM *v, unsigned int i);
inline void InterpretSSSE3Operands(uint8_t *ModRM,
	unsigned int *src, unsigned int *dst);

/** All 32 SSSE3 instructions **/
void pshufb128(XMM *src, XMM *dst, XMM *res);
void pshufb64(MM *src, MM *dst, MM *res);
void phaddw128(XMM *src, XMM *dst, XMM *res);
void phaddw64(MM *src, MM *dst, MM *res);
void phaddd128(XMM *src, XMM *dst, XMM *res);
void phaddd64(MM *src, MM *dst, MM *res);
void phaddsw128(XMM *src, XMM *dst, XMM *res);
void phaddsw64(MM *src, MM *dst, MM *res);
void pmaddubsw128(XMM *src, XMM *dst, XMM *res);
void pmaddubsw64(MM *src, MM *dst, MM *res);
void phsubw128(XMM *src, XMM *dst, XMM *res);
void phsubw64(MM *src, MM *dst, MM *res);
void phsubd128(XMM *src, XMM *dst, XMM *res);
void phsubd64(MM *src, MM *dst, MM *res);
void phsubsw128(XMM *src, XMM *dst, XMM *res);
void phsubsw64(MM *src, MM *dst, MM *res);
void psignb128(XMM *src, XMM *dst, XMM *res);
void psignb64(MM *src, MM *dst, MM *res);
void psignw128(XMM *src, XMM *dst, XMM *res);
void psignw64(MM *src, MM *dst, MM *res);
void psignd128(XMM *src, XMM *dst, XMM *res);
void psignd64(MM *src, MM *dst, MM *res);
void pmulhrsw128(XMM *src, XMM *dst, XMM *res);
void pmulhrsw64(MM *src, MM *dst, MM *res);
void pabsb128(XMM *src, XMM *res);
void pabsb64(MM *src, MM *res);
void pabsw128(XMM *src, XMM *res);
void pabsw64(MM *src, MM *res);
void pabsd128(XMM *src, XMM *res);
void pabsd64(MM *src, MM *res);
void palignr128(XMM *src, XMM *dst, XMM *res, uint8_t IMM);
void palignr64(MM *src, MM *dst, MM *res, uint8_t IMM);

#endif

