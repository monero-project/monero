#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#if !(defined(_MSC_VER) || defined(__MINGW32__))
#include <sys/mman.h>
#endif

#include "int-util.h"
#include "hash-ops.h"
#include "variant4_random_math.h"
#include "CryptonightR_JIT.h"
#include "CryptonightR_template.h"

//x86 prologue and epilogue
#if defined __i386 || defined __x86_64__

static const uint8_t prologue[] = {
	0x4C, 0x8B, 0xD7,	// mov r10, rdi
	0x53,			// push rbx
	0x55,			// push rbp
	0x41, 0x57,		// push r15
	0x4C, 0x8B, 0xDC,	// mov r11, rsp
	0x41, 0x8B, 0x1A,	// mov ebx, DWORD PTR [r10]
	0x41, 0x8B, 0x72, 0x04,	// mov esi, DWORD PTR [r10+4]
	0x41, 0x8B, 0x7A, 0x08,	// mov edi, DWORD PTR [r10+8]
	0x41, 0x8B, 0x6A, 0x0C,	// mov ebp, DWORD PTR [r10+12]
	0x41, 0x8B, 0x62, 0x10,	// mov esp, DWORD PTR [r10+16]
	0x45, 0x8B, 0x7A, 0x14,	// mov r15d, DWORD PTR [r10+20]
	0x41, 0x8B, 0x42, 0x18,	// mov eax, DWORD PTR [r10+24]
	0x41, 0x8B, 0x52, 0x1C,	// mov edx, DWORD PTR [r10+28]
	0x45, 0x8B, 0x4A, 0x20,	// mov r9d, DWORD PTR [r10+32]
};

static const uint8_t epilogue[] = {
	0x49, 0x8B, 0xE3,	// mov rsp, r11
	0x41, 0x89, 0x1A,	// mov DWORD PTR [r10], ebx
	0x41, 0x89, 0x72, 0x04,	// mov DWORD PTR [r10+4], esi
	0x41, 0x89, 0x7A, 0x08,	// mov DWORD PTR [r10+8], edi
	0x41, 0x89, 0x6A, 0x0C,	// mov DWORD PTR [r10+12], ebp
	0x41, 0x5F,		// pop r15
	0x5D,			// pop rbp
	0x5B,			// pop rbx
	0xC3,			// ret
};
#endif

#if defined __PPC__ || defined __PPC64__
#include <endian.h>
static const uint32_t prologue[] = {
0x7c0802a6,   //mflr r0 save lr to r0
0xf8010010,  //std r0 16(r1) save r0 to stack
0x80830000,//      lwz 4,0(3) load values from data pointer at r3
0x80A30004,//      lwz 5,4(3)
0x80C30008,//      lwz 6,8(3)
0x80E3000C,//      lwz 7,12(3)
0x81030010,//      lwz 8,16(3)
0x81230014,//      lwz 9,20(3)
0x81430018,//      lwz 10,24(3)
0x8163001C,//      lwz 11,28(3)
0x81830020,//      lwz 12,32(3)
0x0000000 //end stream
};

static const uint32_t epilogue[] ={
0x90830000,//      stw 4,0(r3)  restore values to data pointer at r3
0x90A30004,//      stw 5,4(r3)
0x90C30008,//      stw 6,8(r3)
0x90E3000C,//      stw 7,12(r3)
0x91030010,//      stw 8,16(r3)
0x91230014,//     stw 9,20(r3)
0x91430018,//      stw 10,24(r3)
0x9163001C,//      stw 11,28(r3)
0x91830020,//      stw 12,32(r3)
0xe8010010,  //ld r0,16(r1) load lr from stack to r0
0x7c0803a6,  //restore link register
0x4e800020, //jump to lr
0x00000000 //end stream
};

#define ppcD(d)    (d << 21)
#define ppcS(s)        (s << 21)
#define ppcA(a)        (a << 16)
#define ppcB(b)        (b << 11)
#define ppcMC(c)       (c << 6)
#define ppcME(e)       (e << 1)
#define ppcIMM(imm)    ((imm) & 0xffff)
 
#define ppcHI(opcode)  ((opcode) << 26)
#define ppcLO(opcode)  ((opcode) << 1)

#define ppcADD     (uint32_t)(ppcHI(31) | ppcLO(266))
#define ppcADDI    (uint32_t)(ppcHI(14))
#define ppcADDIS   (uint32_t)(ppcHI(15))
#define ppcMULLW   (uint32_t)(ppcHI(31) | ppcLO(235))
#define ppcNEG     (uint32_t)(ppcHI(31) | ppcLO(104))
#define ppcSUBF    (uint32_t)(ppcHI(31) | ppcLO(40))
#define ppcXOR     (uint32_t)(ppcHI(31) | ppcLO(316))
#define ppcROTLW   (uint32_t)ppcHI(23)

uint32_t ppcgen_op(uint32_t op,uint32_t a0, uint32_t a1, uint32_t a2 ){
  switch (op){
    case(ppcADD):
      op = ppcADD | ppcD((uint8_t)a0) | ppcA((uint8_t)a1) | ppcB((uint8_t)a2);
      break;
    case(ppcSUBF):
      op = ppcSUBF | ppcD((uint8_t)a0) | ppcA((uint8_t)a1) | ppcB((uint8_t)a2);
      break;
    case(ppcMULLW):
      op = ppcMULLW | ppcD((uint8_t)a0) | ppcA((uint8_t)a1) | ppcB((uint8_t)a2);
      break;
    case(ppcXOR):
      op = ppcXOR | ppcD((uint8_t)a0) | ppcA((uint8_t)a1) | ppcB((uint8_t)a2);
      break;
    case(ppcNEG):
      op = ppcNEG | ppcD((uint8_t)a0) | ppcA((uint8_t)a1);
      break;
    case(ppcADDIS):
      op = ppcADDIS | ppcD((uint8_t)a0) | ppcA((uint8_t)a1) | ppcIMM((uint16_t)a2);
      break;
    case(ppcADDI):
      op = ppcADDI | ppcD((uint8_t)a0) | ppcA((uint8_t)a1) | ppcIMM((uint16_t)a2);
      break;
    case(ppcROTLW):
      op = ppcROTLW | ppcS((uint8_t)a1) | ppcA((uint8_t)a0) | ppcB((uint8_t)a2) | ppcMC((uint8_t)0) | ppcME((uint8_t)31);
      break;
  }
  return op;
}

int ppcJIT_load(void* execmem, uint32_t* code, uint64_t INST_LEN){
  uint64_t idx = 0;
  uint32_t* inst = (uint32_t*)execmem;
  for (;idx < INST_LEN; ++idx){
    if (inst[idx] == 0x00000000){
      inst = inst+idx;
      break;
    }
  }
  if(idx >= INST_LEN){
     return -1;
  }
  for (idx = 0; code[idx] != 0x00000000; ++idx){
    inst[idx] = code[idx];
  }
  return 0;
}

#endif //end ppc helper functions


#if defined __i386 || defined __x86_64__
#define APPEND_CODE(src, size) \
	do { \
		if (JIT_code + (size) > JIT_code_end) \
			return -1; \
		memcpy(JIT_code, (src), (size)); \
		JIT_code += (size); \
	} while (0)
#endif
int v4_generate_JIT_code(const struct V4_Instruction* code, v4_random_math_JIT_func buf, const size_t buf_size)
{
#if defined __i386 || defined __x86_64__
	uint8_t* JIT_code = (uint8_t*) buf;
	const uint8_t* JIT_code_end = JIT_code + buf_size;

#if !(defined(_MSC_VER) || defined(__MINGW32__))
	if (mprotect((void*)buf, buf_size, PROT_READ | PROT_WRITE))
		return 1;
#endif

	APPEND_CODE(prologue, sizeof(prologue));

	uint32_t prev_rot_src = 0xFFFFFFFFU;

	for (int i = 0;; ++i)
	{
		const struct V4_Instruction inst = code[i];
		if (inst.opcode == RET)
			break;

		const uint8_t opcode = (inst.opcode == MUL) ? inst.opcode : (inst.opcode + 2);

		const uint32_t a = inst.dst_index;
		const uint32_t b = inst.src_index;
		const uint8_t c = opcode | (inst.dst_index << V4_OPCODE_BITS) | (((inst.src_index == 8) ? inst.dst_index : inst.src_index) << (V4_OPCODE_BITS + V4_DST_INDEX_BITS));

		switch (inst.opcode)
		{
		case ROR:
		case ROL:
			if (b != prev_rot_src)
			{
				prev_rot_src = b;
				const uint8_t* p1 = (const uint8_t*) instructions_mov[c];
				const uint8_t* p2 = (const uint8_t*) instructions_mov[c + 1];
				APPEND_CODE(p1, p2 - p1);
			}
			break;
		}

		if (a == prev_rot_src)
			prev_rot_src = 0xFFFFFFFFU;

		const uint8_t* p1 = (const uint8_t*) instructions[c];
		const uint8_t* p2 = (const uint8_t*) instructions[c + 1];
		APPEND_CODE(p1, p2 - p1);

		if (inst.opcode == ADD)
			*(uint32_t*)(JIT_code - 4) = inst.C;
	}

	APPEND_CODE(epilogue, sizeof(epilogue));

#if !(defined(_MSC_VER) || defined(__MINGW32__))
	if (mprotect((void*)buf, buf_size, PROT_READ | PROT_EXEC))
		return 1;
#endif

	__builtin___clear_cache((char*)buf, (char*)JIT_code);

	return 0;
#elif defined __PPC__ || defined __PPC64__
    if(buf == NULL){
       return -1;
    }
    uint32_t* JIT_code = (uint32_t*) buf;
    uint64_t INST_LEN = (uint64_t)(buf_size / sizeof(uint32_t));
    
    #if __BYTE_ORDER == __BIG_ENDIAN
    //use end of buffer to store pointer to beginning of buffer (weirdness of the BE ppc abi)
    size_t l = buf_size/sizeof(void*);
    if (sizeof(void*) == sizeof(uint32_t)){
        *(JIT_code+l-1) = (uint32_t)JIT_code;
    }
    else if(sizeof(void*) == sizeof(uint64_t)){
        *((uint64_t*)JIT_code+l-1) = (uint64_t)JIT_code;
    }
    #endif

    static const uint8_t regN[] = {4,5,6,7,8,9,10,11,12};
    static const uint8_t r0 = 0;
    ppcJIT_load(JIT_code,prologue,INST_LEN); 
    for (uint32_t i = 0;; ++i)
    {
        uint8_t dst = code[i].dst_index;
        uint8_t src = code[i].src_index;
        uint32_t tmp[] = {0,0,0,0,0,0,0,0};
        int16_t* C;
        int retv = 0;
        switch (code[i].opcode)
        {
            case MUL:
                tmp[0] = ppcgen_op(ppcMULLW,regN[dst],regN[dst],regN[src]);
                retv = ppcJIT_load(JIT_code,tmp,INST_LEN);
                break;
            case ADD:
                C = (int16_t*)&code[i].C;
                #if __BYTE_ORDER == __BIG_ENDIAN
                uint16_t hi = (uint16_t)C[0];
                int16_t lo = C[1];
                #else
                uint16_t hi = (uint16_t)C[1];
                int16_t lo = C[0];
                #endif
                tmp[0] = ppcgen_op(ppcADD,regN[dst],regN[dst],regN[src]); 
                if(lo < 0) hi+=1;
                tmp[1] = ppcgen_op(ppcADDIS,regN[dst],regN[dst],hi); // load upper 16bits
                tmp[2] = ppcgen_op(ppcADDI,regN[dst],regN[dst],lo); // load lower 16bits
                retv = ppcJIT_load(JIT_code,tmp,INST_LEN);
                break;
            case SUB:
                tmp[0] = ppcgen_op(ppcSUBF,regN[dst],regN[src],regN[dst]);
                retv = ppcJIT_load(JIT_code,tmp,INST_LEN);
                break;
            case ROR:
                tmp[0] = ppcgen_op(ppcNEG,r0,regN[src],0);
                tmp[1] = ppcgen_op(ppcROTLW,regN[dst],regN[dst],r0);
                retv = ppcJIT_load(JIT_code,tmp,INST_LEN);
                break;
            case ROL:
                tmp[0] = ppcgen_op(ppcROTLW,regN[dst],regN[dst],regN[src]);
                retv = ppcJIT_load(JIT_code,tmp,INST_LEN);
                break;
            case XOR:
                tmp[0] = ppcgen_op(ppcXOR,regN[dst],regN[dst],regN[src]);
                retv = ppcJIT_load(JIT_code,tmp,INST_LEN);
                break;
            case RET:
                retv = ppcJIT_load(JIT_code,epilogue,INST_LEN);
                return 0;
            default:
                return -1;
        }
        if(retv != 0) return -1;
    }
//end ppc64
#else
	return 1;
#endif
  return 1;
}
