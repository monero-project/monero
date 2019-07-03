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

static const uint8_t prologue[] = {
#if defined __i386 || defined __x86_64__
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
#endif
};

static const uint8_t epilogue[] = {
#if defined __i386 || defined __x86_64__
	0x49, 0x8B, 0xE3,	// mov rsp, r11
	0x41, 0x89, 0x1A,	// mov DWORD PTR [r10], ebx
	0x41, 0x89, 0x72, 0x04,	// mov DWORD PTR [r10+4], esi
	0x41, 0x89, 0x7A, 0x08,	// mov DWORD PTR [r10+8], edi
	0x41, 0x89, 0x6A, 0x0C,	// mov DWORD PTR [r10+12], ebp
	0x41, 0x5F,		// pop r15
	0x5D,			// pop rbp
	0x5B,			// pop rbx
	0xC3,			// ret
#endif
};

#define APPEND_CODE(src, size) \
	do { \
		if (JIT_code + (size) > JIT_code_end) \
			return -1; \
		memcpy(JIT_code, (src), (size)); \
		JIT_code += (size); \
	} while (0)

int v4_generate_JIT_code(const struct V4_Instruction* code, v4_random_math_JIT_func buf, const size_t buf_size)
{
#if defined __i386 || defined __x86_64__
	uint8_t* JIT_code = (uint8_t*) buf;
	const uint8_t* JIT_code_end = JIT_code + buf_size;

#if !(defined(_MSC_VER) || defined(__MINGW32__))
	if (mprotect((void*)buf, buf_size, PROT_READ | PROT_WRITE))
		return -1;
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
		return -1;
#endif

	__builtin___clear_cache((char*)buf, (char*)JIT_code);

	return 0;
#else
	return -1;
#endif
}
