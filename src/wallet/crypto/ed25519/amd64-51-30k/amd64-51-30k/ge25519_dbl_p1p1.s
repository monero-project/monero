
# qhasm: int64 rp

# qhasm: int64 pp

# qhasm: input rp

# qhasm: input pp

# qhasm: int64 a0

# qhasm: int64 a1

# qhasm: int64 a2

# qhasm: int64 a3

# qhasm: int64 a4

# qhasm: stack64 a0_stack

# qhasm: stack64 a1_stack

# qhasm: stack64 a2_stack

# qhasm: stack64 a3_stack

# qhasm: stack64 a4_stack

# qhasm: int64 b0

# qhasm: int64 b1

# qhasm: int64 b2

# qhasm: int64 b3

# qhasm: int64 b4

# qhasm: stack64 b0_stack

# qhasm: stack64 b1_stack

# qhasm: stack64 b2_stack

# qhasm: stack64 b3_stack

# qhasm: stack64 b4_stack

# qhasm: int64 c0

# qhasm: int64 c1

# qhasm: int64 c2

# qhasm: int64 c3

# qhasm: int64 c4

# qhasm: stack64 c0_stack

# qhasm: stack64 c1_stack

# qhasm: stack64 c2_stack

# qhasm: stack64 c3_stack

# qhasm: stack64 c4_stack

# qhasm: int64 d0

# qhasm: int64 d1

# qhasm: int64 d2

# qhasm: int64 d3

# qhasm: int64 d4

# qhasm: stack64 d0_stack

# qhasm: stack64 d1_stack

# qhasm: stack64 d2_stack

# qhasm: stack64 d3_stack

# qhasm: stack64 d4_stack

# qhasm: int64 e0

# qhasm: int64 e1

# qhasm: int64 e2

# qhasm: int64 e3

# qhasm: int64 e4

# qhasm: stack64 e0_stack

# qhasm: stack64 e1_stack

# qhasm: stack64 e2_stack

# qhasm: stack64 e3_stack

# qhasm: stack64 e4_stack

# qhasm: int64 rx0

# qhasm: int64 rx1

# qhasm: int64 rx2

# qhasm: int64 rx3

# qhasm: int64 rx4

# qhasm: stack64 rx0_stack

# qhasm: stack64 rx1_stack

# qhasm: stack64 rx2_stack

# qhasm: stack64 rx3_stack

# qhasm: stack64 rx4_stack

# qhasm: int64 ry0

# qhasm: int64 ry1

# qhasm: int64 ry2

# qhasm: int64 ry3

# qhasm: int64 ry4

# qhasm: int64 rz0

# qhasm: int64 rz1

# qhasm: int64 rz2

# qhasm: int64 rz3

# qhasm: int64 rz4

# qhasm: int64 rt0

# qhasm: int64 rt1

# qhasm: int64 rt2

# qhasm: int64 rt3

# qhasm: int64 rt4

# qhasm: int64 mulr01

# qhasm: int64 mulr11

# qhasm: int64 mulr21

# qhasm: int64 mulr31

# qhasm: int64 mulr41

# qhasm: int64 mulrax

# qhasm: int64 mulrdx

# qhasm: int64 mult

# qhasm: int64 mulredmask

# qhasm: stack64 mulx219_stack

# qhasm: stack64 mulx319_stack

# qhasm: stack64 mulx419_stack

# qhasm: int64 squarer01

# qhasm: int64 squarer11

# qhasm: int64 squarer21

# qhasm: int64 squarer31

# qhasm: int64 squarer41

# qhasm: int64 squarerax

# qhasm: int64 squarerdx

# qhasm: int64 squaret

# qhasm: int64 squareredmask

# qhasm:   int64 caller1

# qhasm:   int64 caller2

# qhasm:   int64 caller3

# qhasm:   int64 caller4

# qhasm:   int64 caller5

# qhasm:   int64 caller6

# qhasm:   int64 caller7

# qhasm:   caller caller1

# qhasm:   caller caller2

# qhasm:   caller caller3

# qhasm:   caller caller4

# qhasm:   caller caller5

# qhasm:   caller caller6

# qhasm:   caller caller7

# qhasm:   stack64 caller1_stack

# qhasm:   stack64 caller2_stack

# qhasm:   stack64 caller3_stack

# qhasm:   stack64 caller4_stack

# qhasm:   stack64 caller5_stack

# qhasm:   stack64 caller6_stack

# qhasm:   stack64 caller7_stack

# qhasm: enter crypto_sign_ed25519_amd64_51_30k_batch_ge25519_dbl_p1p1
.text
.p2align 5
.globl _crypto_sign_ed25519_amd64_51_30k_batch_ge25519_dbl_p1p1
.globl crypto_sign_ed25519_amd64_51_30k_batch_ge25519_dbl_p1p1
_crypto_sign_ed25519_amd64_51_30k_batch_ge25519_dbl_p1p1:
crypto_sign_ed25519_amd64_51_30k_batch_ge25519_dbl_p1p1:
mov %rsp,%r11
and $31,%r11
add $224,%r11
sub %r11,%rsp

# qhasm:   caller1_stack = caller1
# asm 1: movq <caller1=int64#9,>caller1_stack=stack64#1
# asm 2: movq <caller1=%r11,>caller1_stack=0(%rsp)
movq %r11,0(%rsp)

# qhasm:   caller2_stack = caller2
# asm 1: movq <caller2=int64#10,>caller2_stack=stack64#2
# asm 2: movq <caller2=%r12,>caller2_stack=8(%rsp)
movq %r12,8(%rsp)

# qhasm:   caller3_stack = caller3
# asm 1: movq <caller3=int64#11,>caller3_stack=stack64#3
# asm 2: movq <caller3=%r13,>caller3_stack=16(%rsp)
movq %r13,16(%rsp)

# qhasm:   caller4_stack = caller4
# asm 1: movq <caller4=int64#12,>caller4_stack=stack64#4
# asm 2: movq <caller4=%r14,>caller4_stack=24(%rsp)
movq %r14,24(%rsp)

# qhasm:   caller5_stack = caller5
# asm 1: movq <caller5=int64#13,>caller5_stack=stack64#5
# asm 2: movq <caller5=%r15,>caller5_stack=32(%rsp)
movq %r15,32(%rsp)

# qhasm:   caller6_stack = caller6
# asm 1: movq <caller6=int64#14,>caller6_stack=stack64#6
# asm 2: movq <caller6=%rbx,>caller6_stack=40(%rsp)
movq %rbx,40(%rsp)

# qhasm:   caller7_stack = caller7
# asm 1: movq <caller7=int64#15,>caller7_stack=stack64#7
# asm 2: movq <caller7=%rbp,>caller7_stack=48(%rsp)
movq %rbp,48(%rsp)

# qhasm:   squarerax = *(uint64 *)(pp + 0)
# asm 1: movq   0(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   0(<pp=%rsi),>squarerax=%rax
movq   0(%rsi),%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 0)
# asm 1: mulq  0(<pp=int64#2)
# asm 2: mulq  0(<pp=%rsi)
mulq  0(%rsi)

# qhasm:   a0 = squarerax
# asm 1: mov  <squarerax=int64#7,>a0=int64#4
# asm 2: mov  <squarerax=%rax,>a0=%rcx
mov  %rax,%rcx

# qhasm:   squarer01 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer01=int64#5
# asm 2: mov  <squarerdx=%rdx,>squarer01=%r8
mov  %rdx,%r8

# qhasm:   squarerax = *(uint64 *)(pp + 0)
# asm 1: movq   0(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   0(<pp=%rsi),>squarerax=%rax
movq   0(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 8)
# asm 1: mulq  8(<pp=int64#2)
# asm 2: mulq  8(<pp=%rsi)
mulq  8(%rsi)

# qhasm:   a1 = squarerax
# asm 1: mov  <squarerax=int64#7,>a1=int64#6
# asm 2: mov  <squarerax=%rax,>a1=%r9
mov  %rax,%r9

# qhasm:   squarer11 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer11=int64#8
# asm 2: mov  <squarerdx=%rdx,>squarer11=%r10
mov  %rdx,%r10

# qhasm:   squarerax = *(uint64 *)(pp + 0)
# asm 1: movq   0(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   0(<pp=%rsi),>squarerax=%rax
movq   0(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 16)
# asm 1: mulq  16(<pp=int64#2)
# asm 2: mulq  16(<pp=%rsi)
mulq  16(%rsi)

# qhasm:   a2 = squarerax
# asm 1: mov  <squarerax=int64#7,>a2=int64#9
# asm 2: mov  <squarerax=%rax,>a2=%r11
mov  %rax,%r11

# qhasm:   squarer21 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer21=int64#10
# asm 2: mov  <squarerdx=%rdx,>squarer21=%r12
mov  %rdx,%r12

# qhasm:   squarerax = *(uint64 *)(pp + 0)
# asm 1: movq   0(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   0(<pp=%rsi),>squarerax=%rax
movq   0(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 24)
# asm 1: mulq  24(<pp=int64#2)
# asm 2: mulq  24(<pp=%rsi)
mulq  24(%rsi)

# qhasm:   a3 = squarerax
# asm 1: mov  <squarerax=int64#7,>a3=int64#11
# asm 2: mov  <squarerax=%rax,>a3=%r13
mov  %rax,%r13

# qhasm:   squarer31 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer31=int64#12
# asm 2: mov  <squarerdx=%rdx,>squarer31=%r14
mov  %rdx,%r14

# qhasm:   squarerax = *(uint64 *)(pp + 0)
# asm 1: movq   0(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   0(<pp=%rsi),>squarerax=%rax
movq   0(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 32)
# asm 1: mulq  32(<pp=int64#2)
# asm 2: mulq  32(<pp=%rsi)
mulq  32(%rsi)

# qhasm:   a4 = squarerax
# asm 1: mov  <squarerax=int64#7,>a4=int64#13
# asm 2: mov  <squarerax=%rax,>a4=%r15
mov  %rax,%r15

# qhasm:   squarer41 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer41=int64#14
# asm 2: mov  <squarerdx=%rdx,>squarer41=%rbx
mov  %rdx,%rbx

# qhasm:   squarerax = *(uint64 *)(pp + 8)
# asm 1: movq   8(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   8(<pp=%rsi),>squarerax=%rax
movq   8(%rsi),%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 8)
# asm 1: mulq  8(<pp=int64#2)
# asm 2: mulq  8(<pp=%rsi)
mulq  8(%rsi)

# qhasm:   carry? a2 += squarerax
# asm 1: add  <squarerax=int64#7,<a2=int64#9
# asm 2: add  <squarerax=%rax,<a2=%r11
add  %rax,%r11

# qhasm:   squarer21 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer21=int64#10
# asm 2: adc <squarerdx=%rdx,<squarer21=%r12
adc %rdx,%r12

# qhasm:   squarerax = *(uint64 *)(pp + 8)
# asm 1: movq   8(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   8(<pp=%rsi),>squarerax=%rax
movq   8(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 16)
# asm 1: mulq  16(<pp=int64#2)
# asm 2: mulq  16(<pp=%rsi)
mulq  16(%rsi)

# qhasm:   carry? a3 += squarerax
# asm 1: add  <squarerax=int64#7,<a3=int64#11
# asm 2: add  <squarerax=%rax,<a3=%r13
add  %rax,%r13

# qhasm:   squarer31 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer31=int64#12
# asm 2: adc <squarerdx=%rdx,<squarer31=%r14
adc %rdx,%r14

# qhasm:   squarerax = *(uint64 *)(pp + 8)
# asm 1: movq   8(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   8(<pp=%rsi),>squarerax=%rax
movq   8(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 24)
# asm 1: mulq  24(<pp=int64#2)
# asm 2: mulq  24(<pp=%rsi)
mulq  24(%rsi)

# qhasm:   carry? a4 += squarerax
# asm 1: add  <squarerax=int64#7,<a4=int64#13
# asm 2: add  <squarerax=%rax,<a4=%r15
add  %rax,%r15

# qhasm:   squarer41 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer41=int64#14
# asm 2: adc <squarerdx=%rdx,<squarer41=%rbx
adc %rdx,%rbx

# qhasm:   squarerax = *(uint64 *)(pp + 8)
# asm 1: movq   8(<pp=int64#2),>squarerax=int64#3
# asm 2: movq   8(<pp=%rsi),>squarerax=%rdx
movq   8(%rsi),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 32)
# asm 1: mulq  32(<pp=int64#2)
# asm 2: mulq  32(<pp=%rsi)
mulq  32(%rsi)

# qhasm:   carry? a0 += squarerax
# asm 1: add  <squarerax=int64#7,<a0=int64#4
# asm 2: add  <squarerax=%rax,<a0=%rcx
add  %rax,%rcx

# qhasm:   squarer01 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer01=int64#5
# asm 2: adc <squarerdx=%rdx,<squarer01=%r8
adc %rdx,%r8

# qhasm:   squarerax = *(uint64 *)(pp + 16)
# asm 1: movq   16(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   16(<pp=%rsi),>squarerax=%rax
movq   16(%rsi),%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 16)
# asm 1: mulq  16(<pp=int64#2)
# asm 2: mulq  16(<pp=%rsi)
mulq  16(%rsi)

# qhasm:   carry? a4 += squarerax
# asm 1: add  <squarerax=int64#7,<a4=int64#13
# asm 2: add  <squarerax=%rax,<a4=%r15
add  %rax,%r15

# qhasm:   squarer41 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer41=int64#14
# asm 2: adc <squarerdx=%rdx,<squarer41=%rbx
adc %rdx,%rbx

# qhasm:   squarerax = *(uint64 *)(pp + 16)
# asm 1: movq   16(<pp=int64#2),>squarerax=int64#3
# asm 2: movq   16(<pp=%rsi),>squarerax=%rdx
movq   16(%rsi),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 24)
# asm 1: mulq  24(<pp=int64#2)
# asm 2: mulq  24(<pp=%rsi)
mulq  24(%rsi)

# qhasm:   carry? a0 += squarerax
# asm 1: add  <squarerax=int64#7,<a0=int64#4
# asm 2: add  <squarerax=%rax,<a0=%rcx
add  %rax,%rcx

# qhasm:   squarer01 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer01=int64#5
# asm 2: adc <squarerdx=%rdx,<squarer01=%r8
adc %rdx,%r8

# qhasm:   squarerax = *(uint64 *)(pp + 16)
# asm 1: movq   16(<pp=int64#2),>squarerax=int64#3
# asm 2: movq   16(<pp=%rsi),>squarerax=%rdx
movq   16(%rsi),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 32)
# asm 1: mulq  32(<pp=int64#2)
# asm 2: mulq  32(<pp=%rsi)
mulq  32(%rsi)

# qhasm:   carry? a1 += squarerax
# asm 1: add  <squarerax=int64#7,<a1=int64#6
# asm 2: add  <squarerax=%rax,<a1=%r9
add  %rax,%r9

# qhasm:   squarer11 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer11=int64#8
# asm 2: adc <squarerdx=%rdx,<squarer11=%r10
adc %rdx,%r10

# qhasm:   squarerax = *(uint64 *)(pp + 24)
# asm 1: movq   24(<pp=int64#2),>squarerax=int64#3
# asm 2: movq   24(<pp=%rsi),>squarerax=%rdx
movq   24(%rsi),%rdx

# qhasm:   squarerax *= 19
# asm 1: imulq  $19,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $19,<squarerax=%rdx,>squarerax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 24)
# asm 1: mulq  24(<pp=int64#2)
# asm 2: mulq  24(<pp=%rsi)
mulq  24(%rsi)

# qhasm:   carry? a1 += squarerax
# asm 1: add  <squarerax=int64#7,<a1=int64#6
# asm 2: add  <squarerax=%rax,<a1=%r9
add  %rax,%r9

# qhasm:   squarer11 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer11=int64#8
# asm 2: adc <squarerdx=%rdx,<squarer11=%r10
adc %rdx,%r10

# qhasm:   squarerax = *(uint64 *)(pp + 24)
# asm 1: movq   24(<pp=int64#2),>squarerax=int64#3
# asm 2: movq   24(<pp=%rsi),>squarerax=%rdx
movq   24(%rsi),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 32)
# asm 1: mulq  32(<pp=int64#2)
# asm 2: mulq  32(<pp=%rsi)
mulq  32(%rsi)

# qhasm:   carry? a2 += squarerax
# asm 1: add  <squarerax=int64#7,<a2=int64#9
# asm 2: add  <squarerax=%rax,<a2=%r11
add  %rax,%r11

# qhasm:   squarer21 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer21=int64#10
# asm 2: adc <squarerdx=%rdx,<squarer21=%r12
adc %rdx,%r12

# qhasm:   squarerax = *(uint64 *)(pp + 32)
# asm 1: movq   32(<pp=int64#2),>squarerax=int64#3
# asm 2: movq   32(<pp=%rsi),>squarerax=%rdx
movq   32(%rsi),%rdx

# qhasm:   squarerax *= 19
# asm 1: imulq  $19,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $19,<squarerax=%rdx,>squarerax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 32)
# asm 1: mulq  32(<pp=int64#2)
# asm 2: mulq  32(<pp=%rsi)
mulq  32(%rsi)

# qhasm:   carry? a3 += squarerax
# asm 1: add  <squarerax=int64#7,<a3=int64#11
# asm 2: add  <squarerax=%rax,<a3=%r13
add  %rax,%r13

# qhasm:   squarer31 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer31=int64#12
# asm 2: adc <squarerdx=%rdx,<squarer31=%r14
adc %rdx,%r14

# qhasm:   squareredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>squareredmask=int64#3
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>squareredmask=%rdx
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,%rdx

# qhasm:   squarer01 = (squarer01.a0) << 13
# asm 1: shld $13,<a0=int64#4,<squarer01=int64#5
# asm 2: shld $13,<a0=%rcx,<squarer01=%r8
shld $13,%rcx,%r8

# qhasm:   a0 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<a0=int64#4
# asm 2: and  <squareredmask=%rdx,<a0=%rcx
and  %rdx,%rcx

# qhasm:   squarer11 = (squarer11.a1) << 13
# asm 1: shld $13,<a1=int64#6,<squarer11=int64#8
# asm 2: shld $13,<a1=%r9,<squarer11=%r10
shld $13,%r9,%r10

# qhasm:   a1 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<a1=int64#6
# asm 2: and  <squareredmask=%rdx,<a1=%r9
and  %rdx,%r9

# qhasm:   a1 += squarer01
# asm 1: add  <squarer01=int64#5,<a1=int64#6
# asm 2: add  <squarer01=%r8,<a1=%r9
add  %r8,%r9

# qhasm:   squarer21 = (squarer21.a2) << 13
# asm 1: shld $13,<a2=int64#9,<squarer21=int64#10
# asm 2: shld $13,<a2=%r11,<squarer21=%r12
shld $13,%r11,%r12

# qhasm:   a2 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<a2=int64#9
# asm 2: and  <squareredmask=%rdx,<a2=%r11
and  %rdx,%r11

# qhasm:   a2 += squarer11
# asm 1: add  <squarer11=int64#8,<a2=int64#9
# asm 2: add  <squarer11=%r10,<a2=%r11
add  %r10,%r11

# qhasm:   squarer31 = (squarer31.a3) << 13
# asm 1: shld $13,<a3=int64#11,<squarer31=int64#12
# asm 2: shld $13,<a3=%r13,<squarer31=%r14
shld $13,%r13,%r14

# qhasm:   a3 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<a3=int64#11
# asm 2: and  <squareredmask=%rdx,<a3=%r13
and  %rdx,%r13

# qhasm:   a3 += squarer21
# asm 1: add  <squarer21=int64#10,<a3=int64#11
# asm 2: add  <squarer21=%r12,<a3=%r13
add  %r12,%r13

# qhasm:   squarer41 = (squarer41.a4) << 13
# asm 1: shld $13,<a4=int64#13,<squarer41=int64#14
# asm 2: shld $13,<a4=%r15,<squarer41=%rbx
shld $13,%r15,%rbx

# qhasm:   a4 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<a4=int64#13
# asm 2: and  <squareredmask=%rdx,<a4=%r15
and  %rdx,%r15

# qhasm:   a4 += squarer31
# asm 1: add  <squarer31=int64#12,<a4=int64#13
# asm 2: add  <squarer31=%r14,<a4=%r15
add  %r14,%r15

# qhasm:   squarer41 = squarer41 * 19
# asm 1: imulq  $19,<squarer41=int64#14,>squarer41=int64#5
# asm 2: imulq  $19,<squarer41=%rbx,>squarer41=%r8
imulq  $19,%rbx,%r8

# qhasm:   a0 += squarer41
# asm 1: add  <squarer41=int64#5,<a0=int64#4
# asm 2: add  <squarer41=%r8,<a0=%rcx
add  %r8,%rcx

# qhasm:   squaret = a0
# asm 1: mov  <a0=int64#4,>squaret=int64#5
# asm 2: mov  <a0=%rcx,>squaret=%r8
mov  %rcx,%r8

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#5
# asm 2: shr  $51,<squaret=%r8
shr  $51,%r8

# qhasm:   squaret += a1
# asm 1: add  <a1=int64#6,<squaret=int64#5
# asm 2: add  <a1=%r9,<squaret=%r8
add  %r9,%r8

# qhasm:   a0 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<a0=int64#4
# asm 2: and  <squareredmask=%rdx,<a0=%rcx
and  %rdx,%rcx

# qhasm:   a1 = squaret
# asm 1: mov  <squaret=int64#5,>a1=int64#6
# asm 2: mov  <squaret=%r8,>a1=%r9
mov  %r8,%r9

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#5
# asm 2: shr  $51,<squaret=%r8
shr  $51,%r8

# qhasm:   squaret += a2
# asm 1: add  <a2=int64#9,<squaret=int64#5
# asm 2: add  <a2=%r11,<squaret=%r8
add  %r11,%r8

# qhasm:   a1 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<a1=int64#6
# asm 2: and  <squareredmask=%rdx,<a1=%r9
and  %rdx,%r9

# qhasm:   a2 = squaret
# asm 1: mov  <squaret=int64#5,>a2=int64#7
# asm 2: mov  <squaret=%r8,>a2=%rax
mov  %r8,%rax

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#5
# asm 2: shr  $51,<squaret=%r8
shr  $51,%r8

# qhasm:   squaret += a3
# asm 1: add  <a3=int64#11,<squaret=int64#5
# asm 2: add  <a3=%r13,<squaret=%r8
add  %r13,%r8

# qhasm:   a2 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<a2=int64#7
# asm 2: and  <squareredmask=%rdx,<a2=%rax
and  %rdx,%rax

# qhasm:   a3 = squaret
# asm 1: mov  <squaret=int64#5,>a3=int64#8
# asm 2: mov  <squaret=%r8,>a3=%r10
mov  %r8,%r10

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#5
# asm 2: shr  $51,<squaret=%r8
shr  $51,%r8

# qhasm:   squaret += a4
# asm 1: add  <a4=int64#13,<squaret=int64#5
# asm 2: add  <a4=%r15,<squaret=%r8
add  %r15,%r8

# qhasm:   a3 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<a3=int64#8
# asm 2: and  <squareredmask=%rdx,<a3=%r10
and  %rdx,%r10

# qhasm:   a4 = squaret
# asm 1: mov  <squaret=int64#5,>a4=int64#9
# asm 2: mov  <squaret=%r8,>a4=%r11
mov  %r8,%r11

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#5
# asm 2: shr  $51,<squaret=%r8
shr  $51,%r8

# qhasm:   squaret *= 19
# asm 1: imulq  $19,<squaret=int64#5,>squaret=int64#5
# asm 2: imulq  $19,<squaret=%r8,>squaret=%r8
imulq  $19,%r8,%r8

# qhasm:   a0 += squaret
# asm 1: add  <squaret=int64#5,<a0=int64#4
# asm 2: add  <squaret=%r8,<a0=%rcx
add  %r8,%rcx

# qhasm:   a4 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<a4=int64#9
# asm 2: and  <squareredmask=%rdx,<a4=%r11
and  %rdx,%r11

# qhasm: a0_stack = a0
# asm 1: movq <a0=int64#4,>a0_stack=stack64#8
# asm 2: movq <a0=%rcx,>a0_stack=56(%rsp)
movq %rcx,56(%rsp)

# qhasm: a1_stack = a1
# asm 1: movq <a1=int64#6,>a1_stack=stack64#9
# asm 2: movq <a1=%r9,>a1_stack=64(%rsp)
movq %r9,64(%rsp)

# qhasm: a2_stack = a2
# asm 1: movq <a2=int64#7,>a2_stack=stack64#10
# asm 2: movq <a2=%rax,>a2_stack=72(%rsp)
movq %rax,72(%rsp)

# qhasm: a3_stack = a3
# asm 1: movq <a3=int64#8,>a3_stack=stack64#11
# asm 2: movq <a3=%r10,>a3_stack=80(%rsp)
movq %r10,80(%rsp)

# qhasm: a4_stack = a4
# asm 1: movq <a4=int64#9,>a4_stack=stack64#12
# asm 2: movq <a4=%r11,>a4_stack=88(%rsp)
movq %r11,88(%rsp)

# qhasm:   squarerax = *(uint64 *)(pp + 40)
# asm 1: movq   40(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   40(<pp=%rsi),>squarerax=%rax
movq   40(%rsi),%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 40)
# asm 1: mulq  40(<pp=int64#2)
# asm 2: mulq  40(<pp=%rsi)
mulq  40(%rsi)

# qhasm:   b0 = squarerax
# asm 1: mov  <squarerax=int64#7,>b0=int64#4
# asm 2: mov  <squarerax=%rax,>b0=%rcx
mov  %rax,%rcx

# qhasm:   squarer01 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer01=int64#5
# asm 2: mov  <squarerdx=%rdx,>squarer01=%r8
mov  %rdx,%r8

# qhasm:   squarerax = *(uint64 *)(pp + 40)
# asm 1: movq   40(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   40(<pp=%rsi),>squarerax=%rax
movq   40(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 48)
# asm 1: mulq  48(<pp=int64#2)
# asm 2: mulq  48(<pp=%rsi)
mulq  48(%rsi)

# qhasm:   b1 = squarerax
# asm 1: mov  <squarerax=int64#7,>b1=int64#6
# asm 2: mov  <squarerax=%rax,>b1=%r9
mov  %rax,%r9

# qhasm:   squarer11 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer11=int64#8
# asm 2: mov  <squarerdx=%rdx,>squarer11=%r10
mov  %rdx,%r10

# qhasm:   squarerax = *(uint64 *)(pp + 40)
# asm 1: movq   40(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   40(<pp=%rsi),>squarerax=%rax
movq   40(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 56)
# asm 1: mulq  56(<pp=int64#2)
# asm 2: mulq  56(<pp=%rsi)
mulq  56(%rsi)

# qhasm:   b2 = squarerax
# asm 1: mov  <squarerax=int64#7,>b2=int64#9
# asm 2: mov  <squarerax=%rax,>b2=%r11
mov  %rax,%r11

# qhasm:   squarer21 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer21=int64#10
# asm 2: mov  <squarerdx=%rdx,>squarer21=%r12
mov  %rdx,%r12

# qhasm:   squarerax = *(uint64 *)(pp + 40)
# asm 1: movq   40(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   40(<pp=%rsi),>squarerax=%rax
movq   40(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 64)
# asm 1: mulq  64(<pp=int64#2)
# asm 2: mulq  64(<pp=%rsi)
mulq  64(%rsi)

# qhasm:   b3 = squarerax
# asm 1: mov  <squarerax=int64#7,>b3=int64#11
# asm 2: mov  <squarerax=%rax,>b3=%r13
mov  %rax,%r13

# qhasm:   squarer31 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer31=int64#12
# asm 2: mov  <squarerdx=%rdx,>squarer31=%r14
mov  %rdx,%r14

# qhasm:   squarerax = *(uint64 *)(pp + 40)
# asm 1: movq   40(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   40(<pp=%rsi),>squarerax=%rax
movq   40(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 72)
# asm 1: mulq  72(<pp=int64#2)
# asm 2: mulq  72(<pp=%rsi)
mulq  72(%rsi)

# qhasm:   b4 = squarerax
# asm 1: mov  <squarerax=int64#7,>b4=int64#13
# asm 2: mov  <squarerax=%rax,>b4=%r15
mov  %rax,%r15

# qhasm:   squarer41 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer41=int64#14
# asm 2: mov  <squarerdx=%rdx,>squarer41=%rbx
mov  %rdx,%rbx

# qhasm:   squarerax = *(uint64 *)(pp + 48)
# asm 1: movq   48(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   48(<pp=%rsi),>squarerax=%rax
movq   48(%rsi),%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 48)
# asm 1: mulq  48(<pp=int64#2)
# asm 2: mulq  48(<pp=%rsi)
mulq  48(%rsi)

# qhasm:   carry? b2 += squarerax
# asm 1: add  <squarerax=int64#7,<b2=int64#9
# asm 2: add  <squarerax=%rax,<b2=%r11
add  %rax,%r11

# qhasm:   squarer21 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer21=int64#10
# asm 2: adc <squarerdx=%rdx,<squarer21=%r12
adc %rdx,%r12

# qhasm:   squarerax = *(uint64 *)(pp + 48)
# asm 1: movq   48(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   48(<pp=%rsi),>squarerax=%rax
movq   48(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 56)
# asm 1: mulq  56(<pp=int64#2)
# asm 2: mulq  56(<pp=%rsi)
mulq  56(%rsi)

# qhasm:   carry? b3 += squarerax
# asm 1: add  <squarerax=int64#7,<b3=int64#11
# asm 2: add  <squarerax=%rax,<b3=%r13
add  %rax,%r13

# qhasm:   squarer31 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer31=int64#12
# asm 2: adc <squarerdx=%rdx,<squarer31=%r14
adc %rdx,%r14

# qhasm:   squarerax = *(uint64 *)(pp + 48)
# asm 1: movq   48(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   48(<pp=%rsi),>squarerax=%rax
movq   48(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 64)
# asm 1: mulq  64(<pp=int64#2)
# asm 2: mulq  64(<pp=%rsi)
mulq  64(%rsi)

# qhasm:   carry? b4 += squarerax
# asm 1: add  <squarerax=int64#7,<b4=int64#13
# asm 2: add  <squarerax=%rax,<b4=%r15
add  %rax,%r15

# qhasm:   squarer41 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer41=int64#14
# asm 2: adc <squarerdx=%rdx,<squarer41=%rbx
adc %rdx,%rbx

# qhasm:   squarerax = *(uint64 *)(pp + 48)
# asm 1: movq   48(<pp=int64#2),>squarerax=int64#3
# asm 2: movq   48(<pp=%rsi),>squarerax=%rdx
movq   48(%rsi),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 72)
# asm 1: mulq  72(<pp=int64#2)
# asm 2: mulq  72(<pp=%rsi)
mulq  72(%rsi)

# qhasm:   carry? b0 += squarerax
# asm 1: add  <squarerax=int64#7,<b0=int64#4
# asm 2: add  <squarerax=%rax,<b0=%rcx
add  %rax,%rcx

# qhasm:   squarer01 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer01=int64#5
# asm 2: adc <squarerdx=%rdx,<squarer01=%r8
adc %rdx,%r8

# qhasm:   squarerax = *(uint64 *)(pp + 56)
# asm 1: movq   56(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   56(<pp=%rsi),>squarerax=%rax
movq   56(%rsi),%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 56)
# asm 1: mulq  56(<pp=int64#2)
# asm 2: mulq  56(<pp=%rsi)
mulq  56(%rsi)

# qhasm:   carry? b4 += squarerax
# asm 1: add  <squarerax=int64#7,<b4=int64#13
# asm 2: add  <squarerax=%rax,<b4=%r15
add  %rax,%r15

# qhasm:   squarer41 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer41=int64#14
# asm 2: adc <squarerdx=%rdx,<squarer41=%rbx
adc %rdx,%rbx

# qhasm:   squarerax = *(uint64 *)(pp + 56)
# asm 1: movq   56(<pp=int64#2),>squarerax=int64#3
# asm 2: movq   56(<pp=%rsi),>squarerax=%rdx
movq   56(%rsi),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 64)
# asm 1: mulq  64(<pp=int64#2)
# asm 2: mulq  64(<pp=%rsi)
mulq  64(%rsi)

# qhasm:   carry? b0 += squarerax
# asm 1: add  <squarerax=int64#7,<b0=int64#4
# asm 2: add  <squarerax=%rax,<b0=%rcx
add  %rax,%rcx

# qhasm:   squarer01 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer01=int64#5
# asm 2: adc <squarerdx=%rdx,<squarer01=%r8
adc %rdx,%r8

# qhasm:   squarerax = *(uint64 *)(pp + 56)
# asm 1: movq   56(<pp=int64#2),>squarerax=int64#3
# asm 2: movq   56(<pp=%rsi),>squarerax=%rdx
movq   56(%rsi),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 72)
# asm 1: mulq  72(<pp=int64#2)
# asm 2: mulq  72(<pp=%rsi)
mulq  72(%rsi)

# qhasm:   carry? b1 += squarerax
# asm 1: add  <squarerax=int64#7,<b1=int64#6
# asm 2: add  <squarerax=%rax,<b1=%r9
add  %rax,%r9

# qhasm:   squarer11 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer11=int64#8
# asm 2: adc <squarerdx=%rdx,<squarer11=%r10
adc %rdx,%r10

# qhasm:   squarerax = *(uint64 *)(pp + 64)
# asm 1: movq   64(<pp=int64#2),>squarerax=int64#3
# asm 2: movq   64(<pp=%rsi),>squarerax=%rdx
movq   64(%rsi),%rdx

# qhasm:   squarerax *= 19
# asm 1: imulq  $19,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $19,<squarerax=%rdx,>squarerax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 64)
# asm 1: mulq  64(<pp=int64#2)
# asm 2: mulq  64(<pp=%rsi)
mulq  64(%rsi)

# qhasm:   carry? b1 += squarerax
# asm 1: add  <squarerax=int64#7,<b1=int64#6
# asm 2: add  <squarerax=%rax,<b1=%r9
add  %rax,%r9

# qhasm:   squarer11 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer11=int64#8
# asm 2: adc <squarerdx=%rdx,<squarer11=%r10
adc %rdx,%r10

# qhasm:   squarerax = *(uint64 *)(pp + 64)
# asm 1: movq   64(<pp=int64#2),>squarerax=int64#3
# asm 2: movq   64(<pp=%rsi),>squarerax=%rdx
movq   64(%rsi),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 72)
# asm 1: mulq  72(<pp=int64#2)
# asm 2: mulq  72(<pp=%rsi)
mulq  72(%rsi)

# qhasm:   carry? b2 += squarerax
# asm 1: add  <squarerax=int64#7,<b2=int64#9
# asm 2: add  <squarerax=%rax,<b2=%r11
add  %rax,%r11

# qhasm:   squarer21 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer21=int64#10
# asm 2: adc <squarerdx=%rdx,<squarer21=%r12
adc %rdx,%r12

# qhasm:   squarerax = *(uint64 *)(pp + 72)
# asm 1: movq   72(<pp=int64#2),>squarerax=int64#3
# asm 2: movq   72(<pp=%rsi),>squarerax=%rdx
movq   72(%rsi),%rdx

# qhasm:   squarerax *= 19
# asm 1: imulq  $19,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $19,<squarerax=%rdx,>squarerax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 72)
# asm 1: mulq  72(<pp=int64#2)
# asm 2: mulq  72(<pp=%rsi)
mulq  72(%rsi)

# qhasm:   carry? b3 += squarerax
# asm 1: add  <squarerax=int64#7,<b3=int64#11
# asm 2: add  <squarerax=%rax,<b3=%r13
add  %rax,%r13

# qhasm:   squarer31 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer31=int64#12
# asm 2: adc <squarerdx=%rdx,<squarer31=%r14
adc %rdx,%r14

# qhasm:   squareredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>squareredmask=int64#3
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>squareredmask=%rdx
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,%rdx

# qhasm:   squarer01 = (squarer01.b0) << 13
# asm 1: shld $13,<b0=int64#4,<squarer01=int64#5
# asm 2: shld $13,<b0=%rcx,<squarer01=%r8
shld $13,%rcx,%r8

# qhasm:   b0 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<b0=int64#4
# asm 2: and  <squareredmask=%rdx,<b0=%rcx
and  %rdx,%rcx

# qhasm:   squarer11 = (squarer11.b1) << 13
# asm 1: shld $13,<b1=int64#6,<squarer11=int64#8
# asm 2: shld $13,<b1=%r9,<squarer11=%r10
shld $13,%r9,%r10

# qhasm:   b1 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<b1=int64#6
# asm 2: and  <squareredmask=%rdx,<b1=%r9
and  %rdx,%r9

# qhasm:   b1 += squarer01
# asm 1: add  <squarer01=int64#5,<b1=int64#6
# asm 2: add  <squarer01=%r8,<b1=%r9
add  %r8,%r9

# qhasm:   squarer21 = (squarer21.b2) << 13
# asm 1: shld $13,<b2=int64#9,<squarer21=int64#10
# asm 2: shld $13,<b2=%r11,<squarer21=%r12
shld $13,%r11,%r12

# qhasm:   b2 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<b2=int64#9
# asm 2: and  <squareredmask=%rdx,<b2=%r11
and  %rdx,%r11

# qhasm:   b2 += squarer11
# asm 1: add  <squarer11=int64#8,<b2=int64#9
# asm 2: add  <squarer11=%r10,<b2=%r11
add  %r10,%r11

# qhasm:   squarer31 = (squarer31.b3) << 13
# asm 1: shld $13,<b3=int64#11,<squarer31=int64#12
# asm 2: shld $13,<b3=%r13,<squarer31=%r14
shld $13,%r13,%r14

# qhasm:   b3 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<b3=int64#11
# asm 2: and  <squareredmask=%rdx,<b3=%r13
and  %rdx,%r13

# qhasm:   b3 += squarer21
# asm 1: add  <squarer21=int64#10,<b3=int64#11
# asm 2: add  <squarer21=%r12,<b3=%r13
add  %r12,%r13

# qhasm:   squarer41 = (squarer41.b4) << 13
# asm 1: shld $13,<b4=int64#13,<squarer41=int64#14
# asm 2: shld $13,<b4=%r15,<squarer41=%rbx
shld $13,%r15,%rbx

# qhasm:   b4 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<b4=int64#13
# asm 2: and  <squareredmask=%rdx,<b4=%r15
and  %rdx,%r15

# qhasm:   b4 += squarer31
# asm 1: add  <squarer31=int64#12,<b4=int64#13
# asm 2: add  <squarer31=%r14,<b4=%r15
add  %r14,%r15

# qhasm:   squarer41 = squarer41 * 19
# asm 1: imulq  $19,<squarer41=int64#14,>squarer41=int64#5
# asm 2: imulq  $19,<squarer41=%rbx,>squarer41=%r8
imulq  $19,%rbx,%r8

# qhasm:   b0 += squarer41
# asm 1: add  <squarer41=int64#5,<b0=int64#4
# asm 2: add  <squarer41=%r8,<b0=%rcx
add  %r8,%rcx

# qhasm:   squaret = b0
# asm 1: mov  <b0=int64#4,>squaret=int64#5
# asm 2: mov  <b0=%rcx,>squaret=%r8
mov  %rcx,%r8

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#5
# asm 2: shr  $51,<squaret=%r8
shr  $51,%r8

# qhasm:   squaret += b1
# asm 1: add  <b1=int64#6,<squaret=int64#5
# asm 2: add  <b1=%r9,<squaret=%r8
add  %r9,%r8

# qhasm:   b0 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<b0=int64#4
# asm 2: and  <squareredmask=%rdx,<b0=%rcx
and  %rdx,%rcx

# qhasm:   b1 = squaret
# asm 1: mov  <squaret=int64#5,>b1=int64#6
# asm 2: mov  <squaret=%r8,>b1=%r9
mov  %r8,%r9

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#5
# asm 2: shr  $51,<squaret=%r8
shr  $51,%r8

# qhasm:   squaret += b2
# asm 1: add  <b2=int64#9,<squaret=int64#5
# asm 2: add  <b2=%r11,<squaret=%r8
add  %r11,%r8

# qhasm:   b1 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<b1=int64#6
# asm 2: and  <squareredmask=%rdx,<b1=%r9
and  %rdx,%r9

# qhasm:   b2 = squaret
# asm 1: mov  <squaret=int64#5,>b2=int64#7
# asm 2: mov  <squaret=%r8,>b2=%rax
mov  %r8,%rax

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#5
# asm 2: shr  $51,<squaret=%r8
shr  $51,%r8

# qhasm:   squaret += b3
# asm 1: add  <b3=int64#11,<squaret=int64#5
# asm 2: add  <b3=%r13,<squaret=%r8
add  %r13,%r8

# qhasm:   b2 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<b2=int64#7
# asm 2: and  <squareredmask=%rdx,<b2=%rax
and  %rdx,%rax

# qhasm:   b3 = squaret
# asm 1: mov  <squaret=int64#5,>b3=int64#8
# asm 2: mov  <squaret=%r8,>b3=%r10
mov  %r8,%r10

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#5
# asm 2: shr  $51,<squaret=%r8
shr  $51,%r8

# qhasm:   squaret += b4
# asm 1: add  <b4=int64#13,<squaret=int64#5
# asm 2: add  <b4=%r15,<squaret=%r8
add  %r15,%r8

# qhasm:   b3 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<b3=int64#8
# asm 2: and  <squareredmask=%rdx,<b3=%r10
and  %rdx,%r10

# qhasm:   b4 = squaret
# asm 1: mov  <squaret=int64#5,>b4=int64#9
# asm 2: mov  <squaret=%r8,>b4=%r11
mov  %r8,%r11

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#5
# asm 2: shr  $51,<squaret=%r8
shr  $51,%r8

# qhasm:   squaret *= 19
# asm 1: imulq  $19,<squaret=int64#5,>squaret=int64#5
# asm 2: imulq  $19,<squaret=%r8,>squaret=%r8
imulq  $19,%r8,%r8

# qhasm:   b0 += squaret
# asm 1: add  <squaret=int64#5,<b0=int64#4
# asm 2: add  <squaret=%r8,<b0=%rcx
add  %r8,%rcx

# qhasm:   b4 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<b4=int64#9
# asm 2: and  <squareredmask=%rdx,<b4=%r11
and  %rdx,%r11

# qhasm: b0_stack = b0
# asm 1: movq <b0=int64#4,>b0_stack=stack64#13
# asm 2: movq <b0=%rcx,>b0_stack=96(%rsp)
movq %rcx,96(%rsp)

# qhasm: b1_stack = b1
# asm 1: movq <b1=int64#6,>b1_stack=stack64#14
# asm 2: movq <b1=%r9,>b1_stack=104(%rsp)
movq %r9,104(%rsp)

# qhasm: b2_stack = b2
# asm 1: movq <b2=int64#7,>b2_stack=stack64#15
# asm 2: movq <b2=%rax,>b2_stack=112(%rsp)
movq %rax,112(%rsp)

# qhasm: b3_stack = b3
# asm 1: movq <b3=int64#8,>b3_stack=stack64#16
# asm 2: movq <b3=%r10,>b3_stack=120(%rsp)
movq %r10,120(%rsp)

# qhasm: b4_stack = b4
# asm 1: movq <b4=int64#9,>b4_stack=stack64#17
# asm 2: movq <b4=%r11,>b4_stack=128(%rsp)
movq %r11,128(%rsp)

# qhasm:   squarerax = *(uint64 *)(pp + 80)
# asm 1: movq   80(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   80(<pp=%rsi),>squarerax=%rax
movq   80(%rsi),%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 80)
# asm 1: mulq  80(<pp=int64#2)
# asm 2: mulq  80(<pp=%rsi)
mulq  80(%rsi)

# qhasm:   c0 = squarerax
# asm 1: mov  <squarerax=int64#7,>c0=int64#4
# asm 2: mov  <squarerax=%rax,>c0=%rcx
mov  %rax,%rcx

# qhasm:   squarer01 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer01=int64#5
# asm 2: mov  <squarerdx=%rdx,>squarer01=%r8
mov  %rdx,%r8

# qhasm:   squarerax = *(uint64 *)(pp + 80)
# asm 1: movq   80(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   80(<pp=%rsi),>squarerax=%rax
movq   80(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 88)
# asm 1: mulq  88(<pp=int64#2)
# asm 2: mulq  88(<pp=%rsi)
mulq  88(%rsi)

# qhasm:   c1 = squarerax
# asm 1: mov  <squarerax=int64#7,>c1=int64#6
# asm 2: mov  <squarerax=%rax,>c1=%r9
mov  %rax,%r9

# qhasm:   squarer11 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer11=int64#8
# asm 2: mov  <squarerdx=%rdx,>squarer11=%r10
mov  %rdx,%r10

# qhasm:   squarerax = *(uint64 *)(pp + 80)
# asm 1: movq   80(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   80(<pp=%rsi),>squarerax=%rax
movq   80(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 96)
# asm 1: mulq  96(<pp=int64#2)
# asm 2: mulq  96(<pp=%rsi)
mulq  96(%rsi)

# qhasm:   c2 = squarerax
# asm 1: mov  <squarerax=int64#7,>c2=int64#9
# asm 2: mov  <squarerax=%rax,>c2=%r11
mov  %rax,%r11

# qhasm:   squarer21 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer21=int64#10
# asm 2: mov  <squarerdx=%rdx,>squarer21=%r12
mov  %rdx,%r12

# qhasm:   squarerax = *(uint64 *)(pp + 80)
# asm 1: movq   80(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   80(<pp=%rsi),>squarerax=%rax
movq   80(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 104)
# asm 1: mulq  104(<pp=int64#2)
# asm 2: mulq  104(<pp=%rsi)
mulq  104(%rsi)

# qhasm:   c3 = squarerax
# asm 1: mov  <squarerax=int64#7,>c3=int64#11
# asm 2: mov  <squarerax=%rax,>c3=%r13
mov  %rax,%r13

# qhasm:   squarer31 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer31=int64#12
# asm 2: mov  <squarerdx=%rdx,>squarer31=%r14
mov  %rdx,%r14

# qhasm:   squarerax = *(uint64 *)(pp + 80)
# asm 1: movq   80(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   80(<pp=%rsi),>squarerax=%rax
movq   80(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 112)
# asm 1: mulq  112(<pp=int64#2)
# asm 2: mulq  112(<pp=%rsi)
mulq  112(%rsi)

# qhasm:   c4 = squarerax
# asm 1: mov  <squarerax=int64#7,>c4=int64#13
# asm 2: mov  <squarerax=%rax,>c4=%r15
mov  %rax,%r15

# qhasm:   squarer41 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer41=int64#14
# asm 2: mov  <squarerdx=%rdx,>squarer41=%rbx
mov  %rdx,%rbx

# qhasm:   squarerax = *(uint64 *)(pp + 88)
# asm 1: movq   88(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   88(<pp=%rsi),>squarerax=%rax
movq   88(%rsi),%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 88)
# asm 1: mulq  88(<pp=int64#2)
# asm 2: mulq  88(<pp=%rsi)
mulq  88(%rsi)

# qhasm:   carry? c2 += squarerax
# asm 1: add  <squarerax=int64#7,<c2=int64#9
# asm 2: add  <squarerax=%rax,<c2=%r11
add  %rax,%r11

# qhasm:   squarer21 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer21=int64#10
# asm 2: adc <squarerdx=%rdx,<squarer21=%r12
adc %rdx,%r12

# qhasm:   squarerax = *(uint64 *)(pp + 88)
# asm 1: movq   88(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   88(<pp=%rsi),>squarerax=%rax
movq   88(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 96)
# asm 1: mulq  96(<pp=int64#2)
# asm 2: mulq  96(<pp=%rsi)
mulq  96(%rsi)

# qhasm:   carry? c3 += squarerax
# asm 1: add  <squarerax=int64#7,<c3=int64#11
# asm 2: add  <squarerax=%rax,<c3=%r13
add  %rax,%r13

# qhasm:   squarer31 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer31=int64#12
# asm 2: adc <squarerdx=%rdx,<squarer31=%r14
adc %rdx,%r14

# qhasm:   squarerax = *(uint64 *)(pp + 88)
# asm 1: movq   88(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   88(<pp=%rsi),>squarerax=%rax
movq   88(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 104)
# asm 1: mulq  104(<pp=int64#2)
# asm 2: mulq  104(<pp=%rsi)
mulq  104(%rsi)

# qhasm:   carry? c4 += squarerax
# asm 1: add  <squarerax=int64#7,<c4=int64#13
# asm 2: add  <squarerax=%rax,<c4=%r15
add  %rax,%r15

# qhasm:   squarer41 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer41=int64#14
# asm 2: adc <squarerdx=%rdx,<squarer41=%rbx
adc %rdx,%rbx

# qhasm:   squarerax = *(uint64 *)(pp + 88)
# asm 1: movq   88(<pp=int64#2),>squarerax=int64#3
# asm 2: movq   88(<pp=%rsi),>squarerax=%rdx
movq   88(%rsi),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 112)
# asm 1: mulq  112(<pp=int64#2)
# asm 2: mulq  112(<pp=%rsi)
mulq  112(%rsi)

# qhasm:   carry? c0 += squarerax
# asm 1: add  <squarerax=int64#7,<c0=int64#4
# asm 2: add  <squarerax=%rax,<c0=%rcx
add  %rax,%rcx

# qhasm:   squarer01 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer01=int64#5
# asm 2: adc <squarerdx=%rdx,<squarer01=%r8
adc %rdx,%r8

# qhasm:   squarerax = *(uint64 *)(pp + 96)
# asm 1: movq   96(<pp=int64#2),>squarerax=int64#7
# asm 2: movq   96(<pp=%rsi),>squarerax=%rax
movq   96(%rsi),%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 96)
# asm 1: mulq  96(<pp=int64#2)
# asm 2: mulq  96(<pp=%rsi)
mulq  96(%rsi)

# qhasm:   carry? c4 += squarerax
# asm 1: add  <squarerax=int64#7,<c4=int64#13
# asm 2: add  <squarerax=%rax,<c4=%r15
add  %rax,%r15

# qhasm:   squarer41 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer41=int64#14
# asm 2: adc <squarerdx=%rdx,<squarer41=%rbx
adc %rdx,%rbx

# qhasm:   squarerax = *(uint64 *)(pp + 96)
# asm 1: movq   96(<pp=int64#2),>squarerax=int64#3
# asm 2: movq   96(<pp=%rsi),>squarerax=%rdx
movq   96(%rsi),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 104)
# asm 1: mulq  104(<pp=int64#2)
# asm 2: mulq  104(<pp=%rsi)
mulq  104(%rsi)

# qhasm:   carry? c0 += squarerax
# asm 1: add  <squarerax=int64#7,<c0=int64#4
# asm 2: add  <squarerax=%rax,<c0=%rcx
add  %rax,%rcx

# qhasm:   squarer01 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer01=int64#5
# asm 2: adc <squarerdx=%rdx,<squarer01=%r8
adc %rdx,%r8

# qhasm:   squarerax = *(uint64 *)(pp + 96)
# asm 1: movq   96(<pp=int64#2),>squarerax=int64#3
# asm 2: movq   96(<pp=%rsi),>squarerax=%rdx
movq   96(%rsi),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 112)
# asm 1: mulq  112(<pp=int64#2)
# asm 2: mulq  112(<pp=%rsi)
mulq  112(%rsi)

# qhasm:   carry? c1 += squarerax
# asm 1: add  <squarerax=int64#7,<c1=int64#6
# asm 2: add  <squarerax=%rax,<c1=%r9
add  %rax,%r9

# qhasm:   squarer11 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer11=int64#8
# asm 2: adc <squarerdx=%rdx,<squarer11=%r10
adc %rdx,%r10

# qhasm:   squarerax = *(uint64 *)(pp + 104)
# asm 1: movq   104(<pp=int64#2),>squarerax=int64#3
# asm 2: movq   104(<pp=%rsi),>squarerax=%rdx
movq   104(%rsi),%rdx

# qhasm:   squarerax *= 19
# asm 1: imulq  $19,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $19,<squarerax=%rdx,>squarerax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 104)
# asm 1: mulq  104(<pp=int64#2)
# asm 2: mulq  104(<pp=%rsi)
mulq  104(%rsi)

# qhasm:   carry? c1 += squarerax
# asm 1: add  <squarerax=int64#7,<c1=int64#6
# asm 2: add  <squarerax=%rax,<c1=%r9
add  %rax,%r9

# qhasm:   squarer11 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer11=int64#8
# asm 2: adc <squarerdx=%rdx,<squarer11=%r10
adc %rdx,%r10

# qhasm:   squarerax = *(uint64 *)(pp + 104)
# asm 1: movq   104(<pp=int64#2),>squarerax=int64#3
# asm 2: movq   104(<pp=%rsi),>squarerax=%rdx
movq   104(%rsi),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 112)
# asm 1: mulq  112(<pp=int64#2)
# asm 2: mulq  112(<pp=%rsi)
mulq  112(%rsi)

# qhasm:   carry? c2 += squarerax
# asm 1: add  <squarerax=int64#7,<c2=int64#9
# asm 2: add  <squarerax=%rax,<c2=%r11
add  %rax,%r11

# qhasm:   squarer21 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer21=int64#10
# asm 2: adc <squarerdx=%rdx,<squarer21=%r12
adc %rdx,%r12

# qhasm:   squarerax = *(uint64 *)(pp + 112)
# asm 1: movq   112(<pp=int64#2),>squarerax=int64#3
# asm 2: movq   112(<pp=%rsi),>squarerax=%rdx
movq   112(%rsi),%rdx

# qhasm:   squarerax *= 19
# asm 1: imulq  $19,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $19,<squarerax=%rdx,>squarerax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(pp + 112)
# asm 1: mulq  112(<pp=int64#2)
# asm 2: mulq  112(<pp=%rsi)
mulq  112(%rsi)

# qhasm:   carry? c3 += squarerax
# asm 1: add  <squarerax=int64#7,<c3=int64#11
# asm 2: add  <squarerax=%rax,<c3=%r13
add  %rax,%r13

# qhasm:   squarer31 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer31=int64#12
# asm 2: adc <squarerdx=%rdx,<squarer31=%r14
adc %rdx,%r14

# qhasm:   squareredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>squareredmask=int64#3
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>squareredmask=%rdx
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,%rdx

# qhasm:   squarer01 = (squarer01.c0) << 13
# asm 1: shld $13,<c0=int64#4,<squarer01=int64#5
# asm 2: shld $13,<c0=%rcx,<squarer01=%r8
shld $13,%rcx,%r8

# qhasm:   c0 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<c0=int64#4
# asm 2: and  <squareredmask=%rdx,<c0=%rcx
and  %rdx,%rcx

# qhasm:   squarer11 = (squarer11.c1) << 13
# asm 1: shld $13,<c1=int64#6,<squarer11=int64#8
# asm 2: shld $13,<c1=%r9,<squarer11=%r10
shld $13,%r9,%r10

# qhasm:   c1 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<c1=int64#6
# asm 2: and  <squareredmask=%rdx,<c1=%r9
and  %rdx,%r9

# qhasm:   c1 += squarer01
# asm 1: add  <squarer01=int64#5,<c1=int64#6
# asm 2: add  <squarer01=%r8,<c1=%r9
add  %r8,%r9

# qhasm:   squarer21 = (squarer21.c2) << 13
# asm 1: shld $13,<c2=int64#9,<squarer21=int64#10
# asm 2: shld $13,<c2=%r11,<squarer21=%r12
shld $13,%r11,%r12

# qhasm:   c2 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<c2=int64#9
# asm 2: and  <squareredmask=%rdx,<c2=%r11
and  %rdx,%r11

# qhasm:   c2 += squarer11
# asm 1: add  <squarer11=int64#8,<c2=int64#9
# asm 2: add  <squarer11=%r10,<c2=%r11
add  %r10,%r11

# qhasm:   squarer31 = (squarer31.c3) << 13
# asm 1: shld $13,<c3=int64#11,<squarer31=int64#12
# asm 2: shld $13,<c3=%r13,<squarer31=%r14
shld $13,%r13,%r14

# qhasm:   c3 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<c3=int64#11
# asm 2: and  <squareredmask=%rdx,<c3=%r13
and  %rdx,%r13

# qhasm:   c3 += squarer21
# asm 1: add  <squarer21=int64#10,<c3=int64#11
# asm 2: add  <squarer21=%r12,<c3=%r13
add  %r12,%r13

# qhasm:   squarer41 = (squarer41.c4) << 13
# asm 1: shld $13,<c4=int64#13,<squarer41=int64#14
# asm 2: shld $13,<c4=%r15,<squarer41=%rbx
shld $13,%r15,%rbx

# qhasm:   c4 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<c4=int64#13
# asm 2: and  <squareredmask=%rdx,<c4=%r15
and  %rdx,%r15

# qhasm:   c4 += squarer31
# asm 1: add  <squarer31=int64#12,<c4=int64#13
# asm 2: add  <squarer31=%r14,<c4=%r15
add  %r14,%r15

# qhasm:   squarer41 = squarer41 * 19
# asm 1: imulq  $19,<squarer41=int64#14,>squarer41=int64#5
# asm 2: imulq  $19,<squarer41=%rbx,>squarer41=%r8
imulq  $19,%rbx,%r8

# qhasm:   c0 += squarer41
# asm 1: add  <squarer41=int64#5,<c0=int64#4
# asm 2: add  <squarer41=%r8,<c0=%rcx
add  %r8,%rcx

# qhasm:   squaret = c0
# asm 1: mov  <c0=int64#4,>squaret=int64#5
# asm 2: mov  <c0=%rcx,>squaret=%r8
mov  %rcx,%r8

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#5
# asm 2: shr  $51,<squaret=%r8
shr  $51,%r8

# qhasm:   squaret += c1
# asm 1: add  <c1=int64#6,<squaret=int64#5
# asm 2: add  <c1=%r9,<squaret=%r8
add  %r9,%r8

# qhasm:   c0 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<c0=int64#4
# asm 2: and  <squareredmask=%rdx,<c0=%rcx
and  %rdx,%rcx

# qhasm:   c1 = squaret
# asm 1: mov  <squaret=int64#5,>c1=int64#6
# asm 2: mov  <squaret=%r8,>c1=%r9
mov  %r8,%r9

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#5
# asm 2: shr  $51,<squaret=%r8
shr  $51,%r8

# qhasm:   squaret += c2
# asm 1: add  <c2=int64#9,<squaret=int64#5
# asm 2: add  <c2=%r11,<squaret=%r8
add  %r11,%r8

# qhasm:   c1 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<c1=int64#6
# asm 2: and  <squareredmask=%rdx,<c1=%r9
and  %rdx,%r9

# qhasm:   c2 = squaret
# asm 1: mov  <squaret=int64#5,>c2=int64#7
# asm 2: mov  <squaret=%r8,>c2=%rax
mov  %r8,%rax

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#5
# asm 2: shr  $51,<squaret=%r8
shr  $51,%r8

# qhasm:   squaret += c3
# asm 1: add  <c3=int64#11,<squaret=int64#5
# asm 2: add  <c3=%r13,<squaret=%r8
add  %r13,%r8

# qhasm:   c2 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<c2=int64#7
# asm 2: and  <squareredmask=%rdx,<c2=%rax
and  %rdx,%rax

# qhasm:   c3 = squaret
# asm 1: mov  <squaret=int64#5,>c3=int64#8
# asm 2: mov  <squaret=%r8,>c3=%r10
mov  %r8,%r10

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#5
# asm 2: shr  $51,<squaret=%r8
shr  $51,%r8

# qhasm:   squaret += c4
# asm 1: add  <c4=int64#13,<squaret=int64#5
# asm 2: add  <c4=%r15,<squaret=%r8
add  %r15,%r8

# qhasm:   c3 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<c3=int64#8
# asm 2: and  <squareredmask=%rdx,<c3=%r10
and  %rdx,%r10

# qhasm:   c4 = squaret
# asm 1: mov  <squaret=int64#5,>c4=int64#9
# asm 2: mov  <squaret=%r8,>c4=%r11
mov  %r8,%r11

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#5
# asm 2: shr  $51,<squaret=%r8
shr  $51,%r8

# qhasm:   squaret *= 19
# asm 1: imulq  $19,<squaret=int64#5,>squaret=int64#5
# asm 2: imulq  $19,<squaret=%r8,>squaret=%r8
imulq  $19,%r8,%r8

# qhasm:   c0 += squaret
# asm 1: add  <squaret=int64#5,<c0=int64#4
# asm 2: add  <squaret=%r8,<c0=%rcx
add  %r8,%rcx

# qhasm:   c4 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<c4=int64#9
# asm 2: and  <squareredmask=%rdx,<c4=%r11
and  %rdx,%r11

# qhasm: c0 += c0
# asm 1: add  <c0=int64#4,<c0=int64#4
# asm 2: add  <c0=%rcx,<c0=%rcx
add  %rcx,%rcx

# qhasm: c1 += c1
# asm 1: add  <c1=int64#6,<c1=int64#6
# asm 2: add  <c1=%r9,<c1=%r9
add  %r9,%r9

# qhasm: c2 += c2
# asm 1: add  <c2=int64#7,<c2=int64#7
# asm 2: add  <c2=%rax,<c2=%rax
add  %rax,%rax

# qhasm: c3 += c3
# asm 1: add  <c3=int64#8,<c3=int64#8
# asm 2: add  <c3=%r10,<c3=%r10
add  %r10,%r10

# qhasm: c4 += c4
# asm 1: add  <c4=int64#9,<c4=int64#9
# asm 2: add  <c4=%r11,<c4=%r11
add  %r11,%r11

# qhasm: c0_stack = c0
# asm 1: movq <c0=int64#4,>c0_stack=stack64#18
# asm 2: movq <c0=%rcx,>c0_stack=136(%rsp)
movq %rcx,136(%rsp)

# qhasm: c1_stack = c1
# asm 1: movq <c1=int64#6,>c1_stack=stack64#19
# asm 2: movq <c1=%r9,>c1_stack=144(%rsp)
movq %r9,144(%rsp)

# qhasm: c2_stack = c2
# asm 1: movq <c2=int64#7,>c2_stack=stack64#20
# asm 2: movq <c2=%rax,>c2_stack=152(%rsp)
movq %rax,152(%rsp)

# qhasm: c3_stack = c3
# asm 1: movq <c3=int64#8,>c3_stack=stack64#21
# asm 2: movq <c3=%r10,>c3_stack=160(%rsp)
movq %r10,160(%rsp)

# qhasm: c4_stack = c4
# asm 1: movq <c4=int64#9,>c4_stack=stack64#22
# asm 2: movq <c4=%r11,>c4_stack=168(%rsp)
movq %r11,168(%rsp)

# qhasm: d0 = *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P0
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_2P0,>d0=int64#3
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_2P0,>d0=%rdx
movq crypto_sign_ed25519_amd64_51_30k_batch_2P0,%rdx

# qhasm: d1 = *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,>d1=int64#4
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,>d1=%rcx
movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%rcx

# qhasm: d2 = *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,>d2=int64#5
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,>d2=%r8
movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r8

# qhasm: d3 = *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,>d3=int64#6
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,>d3=%r9
movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r9

# qhasm: d4 = *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,>d4=int64#7
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,>d4=%rax
movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%rax

# qhasm: e0 = d0
# asm 1: mov  <d0=int64#3,>e0=int64#8
# asm 2: mov  <d0=%rdx,>e0=%r10
mov  %rdx,%r10

# qhasm: e1 = d1
# asm 1: mov  <d1=int64#4,>e1=int64#9
# asm 2: mov  <d1=%rcx,>e1=%r11
mov  %rcx,%r11

# qhasm: e2 = d2
# asm 1: mov  <d2=int64#5,>e2=int64#10
# asm 2: mov  <d2=%r8,>e2=%r12
mov  %r8,%r12

# qhasm: e3 = d3
# asm 1: mov  <d3=int64#6,>e3=int64#11
# asm 2: mov  <d3=%r9,>e3=%r13
mov  %r9,%r13

# qhasm: e4 = d4
# asm 1: mov  <d4=int64#7,>e4=int64#12
# asm 2: mov  <d4=%rax,>e4=%r14
mov  %rax,%r14

# qhasm: d0 -= a0_stack
# asm 1: subq <a0_stack=stack64#8,<d0=int64#3
# asm 2: subq <a0_stack=56(%rsp),<d0=%rdx
subq 56(%rsp),%rdx

# qhasm: d1 -= a1_stack
# asm 1: subq <a1_stack=stack64#9,<d1=int64#4
# asm 2: subq <a1_stack=64(%rsp),<d1=%rcx
subq 64(%rsp),%rcx

# qhasm: d2 -= a2_stack
# asm 1: subq <a2_stack=stack64#10,<d2=int64#5
# asm 2: subq <a2_stack=72(%rsp),<d2=%r8
subq 72(%rsp),%r8

# qhasm: d3 -= a3_stack
# asm 1: subq <a3_stack=stack64#11,<d3=int64#6
# asm 2: subq <a3_stack=80(%rsp),<d3=%r9
subq 80(%rsp),%r9

# qhasm: d4 -= a4_stack
# asm 1: subq <a4_stack=stack64#12,<d4=int64#7
# asm 2: subq <a4_stack=88(%rsp),<d4=%rax
subq 88(%rsp),%rax

# qhasm: e0 -= b0_stack
# asm 1: subq <b0_stack=stack64#13,<e0=int64#8
# asm 2: subq <b0_stack=96(%rsp),<e0=%r10
subq 96(%rsp),%r10

# qhasm: e1 -= b1_stack
# asm 1: subq <b1_stack=stack64#14,<e1=int64#9
# asm 2: subq <b1_stack=104(%rsp),<e1=%r11
subq 104(%rsp),%r11

# qhasm: e2 -= b2_stack
# asm 1: subq <b2_stack=stack64#15,<e2=int64#10
# asm 2: subq <b2_stack=112(%rsp),<e2=%r12
subq 112(%rsp),%r12

# qhasm: e3 -= b3_stack
# asm 1: subq <b3_stack=stack64#16,<e3=int64#11
# asm 2: subq <b3_stack=120(%rsp),<e3=%r13
subq 120(%rsp),%r13

# qhasm: e4 -= b4_stack
# asm 1: subq <b4_stack=stack64#17,<e4=int64#12
# asm 2: subq <b4_stack=128(%rsp),<e4=%r14
subq 128(%rsp),%r14

# qhasm: d0_stack = d0
# asm 1: movq <d0=int64#3,>d0_stack=stack64#8
# asm 2: movq <d0=%rdx,>d0_stack=56(%rsp)
movq %rdx,56(%rsp)

# qhasm: d1_stack = d1
# asm 1: movq <d1=int64#4,>d1_stack=stack64#9
# asm 2: movq <d1=%rcx,>d1_stack=64(%rsp)
movq %rcx,64(%rsp)

# qhasm: d2_stack = d2
# asm 1: movq <d2=int64#5,>d2_stack=stack64#10
# asm 2: movq <d2=%r8,>d2_stack=72(%rsp)
movq %r8,72(%rsp)

# qhasm: d3_stack = d3
# asm 1: movq <d3=int64#6,>d3_stack=stack64#11
# asm 2: movq <d3=%r9,>d3_stack=80(%rsp)
movq %r9,80(%rsp)

# qhasm: d4_stack = d4
# asm 1: movq <d4=int64#7,>d4_stack=stack64#12
# asm 2: movq <d4=%rax,>d4_stack=88(%rsp)
movq %rax,88(%rsp)

# qhasm: e0_stack = e0
# asm 1: movq <e0=int64#8,>e0_stack=stack64#23
# asm 2: movq <e0=%r10,>e0_stack=176(%rsp)
movq %r10,176(%rsp)

# qhasm: e1_stack = e1
# asm 1: movq <e1=int64#9,>e1_stack=stack64#24
# asm 2: movq <e1=%r11,>e1_stack=184(%rsp)
movq %r11,184(%rsp)

# qhasm: e2_stack = e2
# asm 1: movq <e2=int64#10,>e2_stack=stack64#25
# asm 2: movq <e2=%r12,>e2_stack=192(%rsp)
movq %r12,192(%rsp)

# qhasm: e3_stack = e3
# asm 1: movq <e3=int64#11,>e3_stack=stack64#26
# asm 2: movq <e3=%r13,>e3_stack=200(%rsp)
movq %r13,200(%rsp)

# qhasm: e4_stack = e4
# asm 1: movq <e4=int64#12,>e4_stack=stack64#27
# asm 2: movq <e4=%r14,>e4_stack=208(%rsp)
movq %r14,208(%rsp)

# qhasm: rz0 = d0
# asm 1: mov  <d0=int64#3,>rz0=int64#8
# asm 2: mov  <d0=%rdx,>rz0=%r10
mov  %rdx,%r10

# qhasm: rz1 = d1
# asm 1: mov  <d1=int64#4,>rz1=int64#9
# asm 2: mov  <d1=%rcx,>rz1=%r11
mov  %rcx,%r11

# qhasm: rz2 = d2
# asm 1: mov  <d2=int64#5,>rz2=int64#10
# asm 2: mov  <d2=%r8,>rz2=%r12
mov  %r8,%r12

# qhasm: rz3 = d3
# asm 1: mov  <d3=int64#6,>rz3=int64#11
# asm 2: mov  <d3=%r9,>rz3=%r13
mov  %r9,%r13

# qhasm: rz4 = d4
# asm 1: mov  <d4=int64#7,>rz4=int64#12
# asm 2: mov  <d4=%rax,>rz4=%r14
mov  %rax,%r14

# qhasm: rz0 += b0_stack
# asm 1: addq <b0_stack=stack64#13,<rz0=int64#8
# asm 2: addq <b0_stack=96(%rsp),<rz0=%r10
addq 96(%rsp),%r10

# qhasm: rz1 += b1_stack
# asm 1: addq <b1_stack=stack64#14,<rz1=int64#9
# asm 2: addq <b1_stack=104(%rsp),<rz1=%r11
addq 104(%rsp),%r11

# qhasm: rz2 += b2_stack
# asm 1: addq <b2_stack=stack64#15,<rz2=int64#10
# asm 2: addq <b2_stack=112(%rsp),<rz2=%r12
addq 112(%rsp),%r12

# qhasm: rz3 += b3_stack
# asm 1: addq <b3_stack=stack64#16,<rz3=int64#11
# asm 2: addq <b3_stack=120(%rsp),<rz3=%r13
addq 120(%rsp),%r13

# qhasm: rz4 += b4_stack
# asm 1: addq <b4_stack=stack64#17,<rz4=int64#12
# asm 2: addq <b4_stack=128(%rsp),<rz4=%r14
addq 128(%rsp),%r14

# qhasm: *(uint64 *) (rp + 40) = rz0
# asm 1: movq   <rz0=int64#8,40(<rp=int64#1)
# asm 2: movq   <rz0=%r10,40(<rp=%rdi)
movq   %r10,40(%rdi)

# qhasm: *(uint64 *) (rp + 48) = rz1
# asm 1: movq   <rz1=int64#9,48(<rp=int64#1)
# asm 2: movq   <rz1=%r11,48(<rp=%rdi)
movq   %r11,48(%rdi)

# qhasm: *(uint64 *) (rp + 56) = rz2
# asm 1: movq   <rz2=int64#10,56(<rp=int64#1)
# asm 2: movq   <rz2=%r12,56(<rp=%rdi)
movq   %r12,56(%rdi)

# qhasm: *(uint64 *) (rp + 64) = rz3
# asm 1: movq   <rz3=int64#11,64(<rp=int64#1)
# asm 2: movq   <rz3=%r13,64(<rp=%rdi)
movq   %r13,64(%rdi)

# qhasm: *(uint64 *) (rp + 72) = rz4
# asm 1: movq   <rz4=int64#12,72(<rp=int64#1)
# asm 2: movq   <rz4=%r14,72(<rp=%rdi)
movq   %r14,72(%rdi)

# qhasm: d0 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P0
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P0,<d0=int64#3
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P0,<d0=%rdx
add  crypto_sign_ed25519_amd64_51_30k_batch_2P0,%rdx

# qhasm: d1 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<d1=int64#4
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<d1=%rcx
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%rcx

# qhasm: d2 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<d2=int64#5
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<d2=%r8
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r8

# qhasm: d3 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<d3=int64#6
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<d3=%r9
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r9

# qhasm: d4 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<d4=int64#7
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<d4=%rax
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%rax

# qhasm: d0 -= b0_stack
# asm 1: subq <b0_stack=stack64#13,<d0=int64#3
# asm 2: subq <b0_stack=96(%rsp),<d0=%rdx
subq 96(%rsp),%rdx

# qhasm: d1 -= b1_stack
# asm 1: subq <b1_stack=stack64#14,<d1=int64#4
# asm 2: subq <b1_stack=104(%rsp),<d1=%rcx
subq 104(%rsp),%rcx

# qhasm: d2 -= b2_stack
# asm 1: subq <b2_stack=stack64#15,<d2=int64#5
# asm 2: subq <b2_stack=112(%rsp),<d2=%r8
subq 112(%rsp),%r8

# qhasm: d3 -= b3_stack
# asm 1: subq <b3_stack=stack64#16,<d3=int64#6
# asm 2: subq <b3_stack=120(%rsp),<d3=%r9
subq 120(%rsp),%r9

# qhasm: d4 -= b4_stack
# asm 1: subq <b4_stack=stack64#17,<d4=int64#7
# asm 2: subq <b4_stack=128(%rsp),<d4=%rax
subq 128(%rsp),%rax

# qhasm: *(uint64 *)(rp + 80) = d0
# asm 1: movq   <d0=int64#3,80(<rp=int64#1)
# asm 2: movq   <d0=%rdx,80(<rp=%rdi)
movq   %rdx,80(%rdi)

# qhasm: *(uint64 *)(rp + 88) = d1
# asm 1: movq   <d1=int64#4,88(<rp=int64#1)
# asm 2: movq   <d1=%rcx,88(<rp=%rdi)
movq   %rcx,88(%rdi)

# qhasm: *(uint64 *)(rp + 96) = d2
# asm 1: movq   <d2=int64#5,96(<rp=int64#1)
# asm 2: movq   <d2=%r8,96(<rp=%rdi)
movq   %r8,96(%rdi)

# qhasm: *(uint64 *)(rp + 104) = d3
# asm 1: movq   <d3=int64#6,104(<rp=int64#1)
# asm 2: movq   <d3=%r9,104(<rp=%rdi)
movq   %r9,104(%rdi)

# qhasm: *(uint64 *)(rp + 112) = d4
# asm 1: movq   <d4=int64#7,112(<rp=int64#1)
# asm 2: movq   <d4=%rax,112(<rp=%rdi)
movq   %rax,112(%rdi)

# qhasm: rz0 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_4P0
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_4P0,<rz0=int64#8
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_4P0,<rz0=%r10
add  crypto_sign_ed25519_amd64_51_30k_batch_4P0,%r10

# qhasm: rz1 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_4P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_4P1234,<rz1=int64#9
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_4P1234,<rz1=%r11
add  crypto_sign_ed25519_amd64_51_30k_batch_4P1234,%r11

# qhasm: rz2 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_4P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_4P1234,<rz2=int64#10
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_4P1234,<rz2=%r12
add  crypto_sign_ed25519_amd64_51_30k_batch_4P1234,%r12

# qhasm: rz3 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_4P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_4P1234,<rz3=int64#11
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_4P1234,<rz3=%r13
add  crypto_sign_ed25519_amd64_51_30k_batch_4P1234,%r13

# qhasm: rz4 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_4P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_4P1234,<rz4=int64#12
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_4P1234,<rz4=%r14
add  crypto_sign_ed25519_amd64_51_30k_batch_4P1234,%r14

# qhasm: rz0 -= c0_stack
# asm 1: subq <c0_stack=stack64#18,<rz0=int64#8
# asm 2: subq <c0_stack=136(%rsp),<rz0=%r10
subq 136(%rsp),%r10

# qhasm: rz1 -= c1_stack
# asm 1: subq <c1_stack=stack64#19,<rz1=int64#9
# asm 2: subq <c1_stack=144(%rsp),<rz1=%r11
subq 144(%rsp),%r11

# qhasm: rz2 -= c2_stack
# asm 1: subq <c2_stack=stack64#20,<rz2=int64#10
# asm 2: subq <c2_stack=152(%rsp),<rz2=%r12
subq 152(%rsp),%r12

# qhasm: rz3 -= c3_stack
# asm 1: subq <c3_stack=stack64#21,<rz3=int64#11
# asm 2: subq <c3_stack=160(%rsp),<rz3=%r13
subq 160(%rsp),%r13

# qhasm: rz4 -= c4_stack
# asm 1: subq <c4_stack=stack64#22,<rz4=int64#12
# asm 2: subq <c4_stack=168(%rsp),<rz4=%r14
subq 168(%rsp),%r14

# qhasm: *(uint64 *) (rp + 120) = rz0
# asm 1: movq   <rz0=int64#8,120(<rp=int64#1)
# asm 2: movq   <rz0=%r10,120(<rp=%rdi)
movq   %r10,120(%rdi)

# qhasm: *(uint64 *) (rp + 128) = rz1
# asm 1: movq   <rz1=int64#9,128(<rp=int64#1)
# asm 2: movq   <rz1=%r11,128(<rp=%rdi)
movq   %r11,128(%rdi)

# qhasm: *(uint64 *) (rp + 136) = rz2
# asm 1: movq   <rz2=int64#10,136(<rp=int64#1)
# asm 2: movq   <rz2=%r12,136(<rp=%rdi)
movq   %r12,136(%rdi)

# qhasm: *(uint64 *) (rp + 144) = rz3
# asm 1: movq   <rz3=int64#11,144(<rp=int64#1)
# asm 2: movq   <rz3=%r13,144(<rp=%rdi)
movq   %r13,144(%rdi)

# qhasm: *(uint64 *) (rp + 152) = rz4
# asm 1: movq   <rz4=int64#12,152(<rp=int64#1)
# asm 2: movq   <rz4=%r14,152(<rp=%rdi)
movq   %r14,152(%rdi)

# qhasm: rx0 = *(uint64 *)(pp + 0)
# asm 1: movq   0(<pp=int64#2),>rx0=int64#3
# asm 2: movq   0(<pp=%rsi),>rx0=%rdx
movq   0(%rsi),%rdx

# qhasm: rx1 = *(uint64 *)(pp + 8)
# asm 1: movq   8(<pp=int64#2),>rx1=int64#4
# asm 2: movq   8(<pp=%rsi),>rx1=%rcx
movq   8(%rsi),%rcx

# qhasm: rx2 = *(uint64 *)(pp + 16)
# asm 1: movq   16(<pp=int64#2),>rx2=int64#5
# asm 2: movq   16(<pp=%rsi),>rx2=%r8
movq   16(%rsi),%r8

# qhasm: rx3 = *(uint64 *)(pp + 24)
# asm 1: movq   24(<pp=int64#2),>rx3=int64#6
# asm 2: movq   24(<pp=%rsi),>rx3=%r9
movq   24(%rsi),%r9

# qhasm: rx4 = *(uint64 *)(pp + 32)
# asm 1: movq   32(<pp=int64#2),>rx4=int64#7
# asm 2: movq   32(<pp=%rsi),>rx4=%rax
movq   32(%rsi),%rax

# qhasm: rx0 += *(uint64 *)(pp + 40)
# asm 1: addq 40(<pp=int64#2),<rx0=int64#3
# asm 2: addq 40(<pp=%rsi),<rx0=%rdx
addq 40(%rsi),%rdx

# qhasm: rx1 += *(uint64 *)(pp + 48)
# asm 1: addq 48(<pp=int64#2),<rx1=int64#4
# asm 2: addq 48(<pp=%rsi),<rx1=%rcx
addq 48(%rsi),%rcx

# qhasm: rx2 += *(uint64 *)(pp + 56)
# asm 1: addq 56(<pp=int64#2),<rx2=int64#5
# asm 2: addq 56(<pp=%rsi),<rx2=%r8
addq 56(%rsi),%r8

# qhasm: rx3 += *(uint64 *)(pp + 64)
# asm 1: addq 64(<pp=int64#2),<rx3=int64#6
# asm 2: addq 64(<pp=%rsi),<rx3=%r9
addq 64(%rsi),%r9

# qhasm: rx4 += *(uint64 *)(pp + 72)
# asm 1: addq 72(<pp=int64#2),<rx4=int64#7
# asm 2: addq 72(<pp=%rsi),<rx4=%rax
addq 72(%rsi),%rax

# qhasm: rx0_stack = rx0
# asm 1: movq <rx0=int64#3,>rx0_stack=stack64#13
# asm 2: movq <rx0=%rdx,>rx0_stack=96(%rsp)
movq %rdx,96(%rsp)

# qhasm: rx1_stack = rx1
# asm 1: movq <rx1=int64#4,>rx1_stack=stack64#14
# asm 2: movq <rx1=%rcx,>rx1_stack=104(%rsp)
movq %rcx,104(%rsp)

# qhasm: rx2_stack = rx2
# asm 1: movq <rx2=int64#5,>rx2_stack=stack64#15
# asm 2: movq <rx2=%r8,>rx2_stack=112(%rsp)
movq %r8,112(%rsp)

# qhasm: rx3_stack = rx3
# asm 1: movq <rx3=int64#6,>rx3_stack=stack64#16
# asm 2: movq <rx3=%r9,>rx3_stack=120(%rsp)
movq %r9,120(%rsp)

# qhasm: rx4_stack = rx4
# asm 1: movq <rx4=int64#7,>rx4_stack=stack64#17
# asm 2: movq <rx4=%rax,>rx4_stack=128(%rsp)
movq %rax,128(%rsp)

# qhasm:   squarerax = rx0_stack
# asm 1: movq <rx0_stack=stack64#13,>squarerax=int64#7
# asm 2: movq <rx0_stack=96(%rsp),>squarerax=%rax
movq 96(%rsp),%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * rx0_stack
# asm 1: mulq  <rx0_stack=stack64#13
# asm 2: mulq  <rx0_stack=96(%rsp)
mulq  96(%rsp)

# qhasm:   rx0 = squarerax
# asm 1: mov  <squarerax=int64#7,>rx0=int64#2
# asm 2: mov  <squarerax=%rax,>rx0=%rsi
mov  %rax,%rsi

# qhasm:   squarer01 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer01=int64#4
# asm 2: mov  <squarerdx=%rdx,>squarer01=%rcx
mov  %rdx,%rcx

# qhasm:   squarerax = rx0_stack
# asm 1: movq <rx0_stack=stack64#13,>squarerax=int64#7
# asm 2: movq <rx0_stack=96(%rsp),>squarerax=%rax
movq 96(%rsp),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * rx1_stack
# asm 1: mulq  <rx1_stack=stack64#14
# asm 2: mulq  <rx1_stack=104(%rsp)
mulq  104(%rsp)

# qhasm:   rx1 = squarerax
# asm 1: mov  <squarerax=int64#7,>rx1=int64#5
# asm 2: mov  <squarerax=%rax,>rx1=%r8
mov  %rax,%r8

# qhasm:   squarer11 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer11=int64#6
# asm 2: mov  <squarerdx=%rdx,>squarer11=%r9
mov  %rdx,%r9

# qhasm:   squarerax = rx0_stack
# asm 1: movq <rx0_stack=stack64#13,>squarerax=int64#7
# asm 2: movq <rx0_stack=96(%rsp),>squarerax=%rax
movq 96(%rsp),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * rx2_stack
# asm 1: mulq  <rx2_stack=stack64#15
# asm 2: mulq  <rx2_stack=112(%rsp)
mulq  112(%rsp)

# qhasm:   rx2 = squarerax
# asm 1: mov  <squarerax=int64#7,>rx2=int64#8
# asm 2: mov  <squarerax=%rax,>rx2=%r10
mov  %rax,%r10

# qhasm:   squarer21 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer21=int64#9
# asm 2: mov  <squarerdx=%rdx,>squarer21=%r11
mov  %rdx,%r11

# qhasm:   squarerax = rx0_stack
# asm 1: movq <rx0_stack=stack64#13,>squarerax=int64#7
# asm 2: movq <rx0_stack=96(%rsp),>squarerax=%rax
movq 96(%rsp),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * rx3_stack
# asm 1: mulq  <rx3_stack=stack64#16
# asm 2: mulq  <rx3_stack=120(%rsp)
mulq  120(%rsp)

# qhasm:   rx3 = squarerax
# asm 1: mov  <squarerax=int64#7,>rx3=int64#10
# asm 2: mov  <squarerax=%rax,>rx3=%r12
mov  %rax,%r12

# qhasm:   squarer31 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer31=int64#11
# asm 2: mov  <squarerdx=%rdx,>squarer31=%r13
mov  %rdx,%r13

# qhasm:   squarerax = rx0_stack
# asm 1: movq <rx0_stack=stack64#13,>squarerax=int64#7
# asm 2: movq <rx0_stack=96(%rsp),>squarerax=%rax
movq 96(%rsp),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * rx4_stack
# asm 1: mulq  <rx4_stack=stack64#17
# asm 2: mulq  <rx4_stack=128(%rsp)
mulq  128(%rsp)

# qhasm:   rx4 = squarerax
# asm 1: mov  <squarerax=int64#7,>rx4=int64#12
# asm 2: mov  <squarerax=%rax,>rx4=%r14
mov  %rax,%r14

# qhasm:   squarer41 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer41=int64#13
# asm 2: mov  <squarerdx=%rdx,>squarer41=%r15
mov  %rdx,%r15

# qhasm:   squarerax = rx1_stack
# asm 1: movq <rx1_stack=stack64#14,>squarerax=int64#7
# asm 2: movq <rx1_stack=104(%rsp),>squarerax=%rax
movq 104(%rsp),%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * rx1_stack
# asm 1: mulq  <rx1_stack=stack64#14
# asm 2: mulq  <rx1_stack=104(%rsp)
mulq  104(%rsp)

# qhasm:   carry? rx2 += squarerax
# asm 1: add  <squarerax=int64#7,<rx2=int64#8
# asm 2: add  <squarerax=%rax,<rx2=%r10
add  %rax,%r10

# qhasm:   squarer21 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer21=int64#9
# asm 2: adc <squarerdx=%rdx,<squarer21=%r11
adc %rdx,%r11

# qhasm:   squarerax = rx1_stack
# asm 1: movq <rx1_stack=stack64#14,>squarerax=int64#7
# asm 2: movq <rx1_stack=104(%rsp),>squarerax=%rax
movq 104(%rsp),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * rx2_stack
# asm 1: mulq  <rx2_stack=stack64#15
# asm 2: mulq  <rx2_stack=112(%rsp)
mulq  112(%rsp)

# qhasm:   carry? rx3 += squarerax
# asm 1: add  <squarerax=int64#7,<rx3=int64#10
# asm 2: add  <squarerax=%rax,<rx3=%r12
add  %rax,%r12

# qhasm:   squarer31 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer31=int64#11
# asm 2: adc <squarerdx=%rdx,<squarer31=%r13
adc %rdx,%r13

# qhasm:   squarerax = rx1_stack
# asm 1: movq <rx1_stack=stack64#14,>squarerax=int64#7
# asm 2: movq <rx1_stack=104(%rsp),>squarerax=%rax
movq 104(%rsp),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * rx3_stack
# asm 1: mulq  <rx3_stack=stack64#16
# asm 2: mulq  <rx3_stack=120(%rsp)
mulq  120(%rsp)

# qhasm:   carry? rx4 += squarerax
# asm 1: add  <squarerax=int64#7,<rx4=int64#12
# asm 2: add  <squarerax=%rax,<rx4=%r14
add  %rax,%r14

# qhasm:   squarer41 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer41=int64#13
# asm 2: adc <squarerdx=%rdx,<squarer41=%r15
adc %rdx,%r15

# qhasm:   squarerax = rx1_stack
# asm 1: movq <rx1_stack=stack64#14,>squarerax=int64#3
# asm 2: movq <rx1_stack=104(%rsp),>squarerax=%rdx
movq 104(%rsp),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * rx4_stack
# asm 1: mulq  <rx4_stack=stack64#17
# asm 2: mulq  <rx4_stack=128(%rsp)
mulq  128(%rsp)

# qhasm:   carry? rx0 += squarerax
# asm 1: add  <squarerax=int64#7,<rx0=int64#2
# asm 2: add  <squarerax=%rax,<rx0=%rsi
add  %rax,%rsi

# qhasm:   squarer01 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer01=int64#4
# asm 2: adc <squarerdx=%rdx,<squarer01=%rcx
adc %rdx,%rcx

# qhasm:   squarerax = rx2_stack
# asm 1: movq <rx2_stack=stack64#15,>squarerax=int64#7
# asm 2: movq <rx2_stack=112(%rsp),>squarerax=%rax
movq 112(%rsp),%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * rx2_stack
# asm 1: mulq  <rx2_stack=stack64#15
# asm 2: mulq  <rx2_stack=112(%rsp)
mulq  112(%rsp)

# qhasm:   carry? rx4 += squarerax
# asm 1: add  <squarerax=int64#7,<rx4=int64#12
# asm 2: add  <squarerax=%rax,<rx4=%r14
add  %rax,%r14

# qhasm:   squarer41 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer41=int64#13
# asm 2: adc <squarerdx=%rdx,<squarer41=%r15
adc %rdx,%r15

# qhasm:   squarerax = rx2_stack
# asm 1: movq <rx2_stack=stack64#15,>squarerax=int64#3
# asm 2: movq <rx2_stack=112(%rsp),>squarerax=%rdx
movq 112(%rsp),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * rx3_stack
# asm 1: mulq  <rx3_stack=stack64#16
# asm 2: mulq  <rx3_stack=120(%rsp)
mulq  120(%rsp)

# qhasm:   carry? rx0 += squarerax
# asm 1: add  <squarerax=int64#7,<rx0=int64#2
# asm 2: add  <squarerax=%rax,<rx0=%rsi
add  %rax,%rsi

# qhasm:   squarer01 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer01=int64#4
# asm 2: adc <squarerdx=%rdx,<squarer01=%rcx
adc %rdx,%rcx

# qhasm:   squarerax = rx2_stack
# asm 1: movq <rx2_stack=stack64#15,>squarerax=int64#3
# asm 2: movq <rx2_stack=112(%rsp),>squarerax=%rdx
movq 112(%rsp),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * rx4_stack
# asm 1: mulq  <rx4_stack=stack64#17
# asm 2: mulq  <rx4_stack=128(%rsp)
mulq  128(%rsp)

# qhasm:   carry? rx1 += squarerax
# asm 1: add  <squarerax=int64#7,<rx1=int64#5
# asm 2: add  <squarerax=%rax,<rx1=%r8
add  %rax,%r8

# qhasm:   squarer11 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer11=int64#6
# asm 2: adc <squarerdx=%rdx,<squarer11=%r9
adc %rdx,%r9

# qhasm:   squarerax = rx3_stack
# asm 1: movq <rx3_stack=stack64#16,>squarerax=int64#3
# asm 2: movq <rx3_stack=120(%rsp),>squarerax=%rdx
movq 120(%rsp),%rdx

# qhasm:   squarerax *= 19
# asm 1: imulq  $19,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $19,<squarerax=%rdx,>squarerax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * rx3_stack
# asm 1: mulq  <rx3_stack=stack64#16
# asm 2: mulq  <rx3_stack=120(%rsp)
mulq  120(%rsp)

# qhasm:   carry? rx1 += squarerax
# asm 1: add  <squarerax=int64#7,<rx1=int64#5
# asm 2: add  <squarerax=%rax,<rx1=%r8
add  %rax,%r8

# qhasm:   squarer11 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer11=int64#6
# asm 2: adc <squarerdx=%rdx,<squarer11=%r9
adc %rdx,%r9

# qhasm:   squarerax = rx3_stack
# asm 1: movq <rx3_stack=stack64#16,>squarerax=int64#3
# asm 2: movq <rx3_stack=120(%rsp),>squarerax=%rdx
movq 120(%rsp),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * rx4_stack
# asm 1: mulq  <rx4_stack=stack64#17
# asm 2: mulq  <rx4_stack=128(%rsp)
mulq  128(%rsp)

# qhasm:   carry? rx2 += squarerax
# asm 1: add  <squarerax=int64#7,<rx2=int64#8
# asm 2: add  <squarerax=%rax,<rx2=%r10
add  %rax,%r10

# qhasm:   squarer21 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer21=int64#9
# asm 2: adc <squarerdx=%rdx,<squarer21=%r11
adc %rdx,%r11

# qhasm:   squarerax = rx4_stack
# asm 1: movq <rx4_stack=stack64#17,>squarerax=int64#3
# asm 2: movq <rx4_stack=128(%rsp),>squarerax=%rdx
movq 128(%rsp),%rdx

# qhasm:   squarerax *= 19
# asm 1: imulq  $19,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $19,<squarerax=%rdx,>squarerax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * rx4_stack
# asm 1: mulq  <rx4_stack=stack64#17
# asm 2: mulq  <rx4_stack=128(%rsp)
mulq  128(%rsp)

# qhasm:   carry? rx3 += squarerax
# asm 1: add  <squarerax=int64#7,<rx3=int64#10
# asm 2: add  <squarerax=%rax,<rx3=%r12
add  %rax,%r12

# qhasm:   squarer31 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer31=int64#11
# asm 2: adc <squarerdx=%rdx,<squarer31=%r13
adc %rdx,%r13

# qhasm:   squareredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>squareredmask=int64#3
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>squareredmask=%rdx
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,%rdx

# qhasm:   squarer01 = (squarer01.rx0) << 13
# asm 1: shld $13,<rx0=int64#2,<squarer01=int64#4
# asm 2: shld $13,<rx0=%rsi,<squarer01=%rcx
shld $13,%rsi,%rcx

# qhasm:   rx0 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<rx0=int64#2
# asm 2: and  <squareredmask=%rdx,<rx0=%rsi
and  %rdx,%rsi

# qhasm:   squarer11 = (squarer11.rx1) << 13
# asm 1: shld $13,<rx1=int64#5,<squarer11=int64#6
# asm 2: shld $13,<rx1=%r8,<squarer11=%r9
shld $13,%r8,%r9

# qhasm:   rx1 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<rx1=int64#5
# asm 2: and  <squareredmask=%rdx,<rx1=%r8
and  %rdx,%r8

# qhasm:   rx1 += squarer01
# asm 1: add  <squarer01=int64#4,<rx1=int64#5
# asm 2: add  <squarer01=%rcx,<rx1=%r8
add  %rcx,%r8

# qhasm:   squarer21 = (squarer21.rx2) << 13
# asm 1: shld $13,<rx2=int64#8,<squarer21=int64#9
# asm 2: shld $13,<rx2=%r10,<squarer21=%r11
shld $13,%r10,%r11

# qhasm:   rx2 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<rx2=int64#8
# asm 2: and  <squareredmask=%rdx,<rx2=%r10
and  %rdx,%r10

# qhasm:   rx2 += squarer11
# asm 1: add  <squarer11=int64#6,<rx2=int64#8
# asm 2: add  <squarer11=%r9,<rx2=%r10
add  %r9,%r10

# qhasm:   squarer31 = (squarer31.rx3) << 13
# asm 1: shld $13,<rx3=int64#10,<squarer31=int64#11
# asm 2: shld $13,<rx3=%r12,<squarer31=%r13
shld $13,%r12,%r13

# qhasm:   rx3 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<rx3=int64#10
# asm 2: and  <squareredmask=%rdx,<rx3=%r12
and  %rdx,%r12

# qhasm:   rx3 += squarer21
# asm 1: add  <squarer21=int64#9,<rx3=int64#10
# asm 2: add  <squarer21=%r11,<rx3=%r12
add  %r11,%r12

# qhasm:   squarer41 = (squarer41.rx4) << 13
# asm 1: shld $13,<rx4=int64#12,<squarer41=int64#13
# asm 2: shld $13,<rx4=%r14,<squarer41=%r15
shld $13,%r14,%r15

# qhasm:   rx4 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<rx4=int64#12
# asm 2: and  <squareredmask=%rdx,<rx4=%r14
and  %rdx,%r14

# qhasm:   rx4 += squarer31
# asm 1: add  <squarer31=int64#11,<rx4=int64#12
# asm 2: add  <squarer31=%r13,<rx4=%r14
add  %r13,%r14

# qhasm:   squarer41 = squarer41 * 19
# asm 1: imulq  $19,<squarer41=int64#13,>squarer41=int64#4
# asm 2: imulq  $19,<squarer41=%r15,>squarer41=%rcx
imulq  $19,%r15,%rcx

# qhasm:   rx0 += squarer41
# asm 1: add  <squarer41=int64#4,<rx0=int64#2
# asm 2: add  <squarer41=%rcx,<rx0=%rsi
add  %rcx,%rsi

# qhasm:   squaret = rx0
# asm 1: mov  <rx0=int64#2,>squaret=int64#4
# asm 2: mov  <rx0=%rsi,>squaret=%rcx
mov  %rsi,%rcx

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#4
# asm 2: shr  $51,<squaret=%rcx
shr  $51,%rcx

# qhasm:   squaret += rx1
# asm 1: add  <rx1=int64#5,<squaret=int64#4
# asm 2: add  <rx1=%r8,<squaret=%rcx
add  %r8,%rcx

# qhasm:   rx0 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<rx0=int64#2
# asm 2: and  <squareredmask=%rdx,<rx0=%rsi
and  %rdx,%rsi

# qhasm:   rx1 = squaret
# asm 1: mov  <squaret=int64#4,>rx1=int64#5
# asm 2: mov  <squaret=%rcx,>rx1=%r8
mov  %rcx,%r8

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#4
# asm 2: shr  $51,<squaret=%rcx
shr  $51,%rcx

# qhasm:   squaret += rx2
# asm 1: add  <rx2=int64#8,<squaret=int64#4
# asm 2: add  <rx2=%r10,<squaret=%rcx
add  %r10,%rcx

# qhasm:   rx1 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<rx1=int64#5
# asm 2: and  <squareredmask=%rdx,<rx1=%r8
and  %rdx,%r8

# qhasm:   rx2 = squaret
# asm 1: mov  <squaret=int64#4,>rx2=int64#6
# asm 2: mov  <squaret=%rcx,>rx2=%r9
mov  %rcx,%r9

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#4
# asm 2: shr  $51,<squaret=%rcx
shr  $51,%rcx

# qhasm:   squaret += rx3
# asm 1: add  <rx3=int64#10,<squaret=int64#4
# asm 2: add  <rx3=%r12,<squaret=%rcx
add  %r12,%rcx

# qhasm:   rx2 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<rx2=int64#6
# asm 2: and  <squareredmask=%rdx,<rx2=%r9
and  %rdx,%r9

# qhasm:   rx3 = squaret
# asm 1: mov  <squaret=int64#4,>rx3=int64#7
# asm 2: mov  <squaret=%rcx,>rx3=%rax
mov  %rcx,%rax

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#4
# asm 2: shr  $51,<squaret=%rcx
shr  $51,%rcx

# qhasm:   squaret += rx4
# asm 1: add  <rx4=int64#12,<squaret=int64#4
# asm 2: add  <rx4=%r14,<squaret=%rcx
add  %r14,%rcx

# qhasm:   rx3 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<rx3=int64#7
# asm 2: and  <squareredmask=%rdx,<rx3=%rax
and  %rdx,%rax

# qhasm:   rx4 = squaret
# asm 1: mov  <squaret=int64#4,>rx4=int64#8
# asm 2: mov  <squaret=%rcx,>rx4=%r10
mov  %rcx,%r10

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#4
# asm 2: shr  $51,<squaret=%rcx
shr  $51,%rcx

# qhasm:   squaret *= 19
# asm 1: imulq  $19,<squaret=int64#4,>squaret=int64#4
# asm 2: imulq  $19,<squaret=%rcx,>squaret=%rcx
imulq  $19,%rcx,%rcx

# qhasm:   rx0 += squaret
# asm 1: add  <squaret=int64#4,<rx0=int64#2
# asm 2: add  <squaret=%rcx,<rx0=%rsi
add  %rcx,%rsi

# qhasm:   rx4 &= squareredmask
# asm 1: and  <squareredmask=int64#3,<rx4=int64#8
# asm 2: and  <squareredmask=%rdx,<rx4=%r10
and  %rdx,%r10

# qhasm: rx0 += d0_stack
# asm 1: addq <d0_stack=stack64#8,<rx0=int64#2
# asm 2: addq <d0_stack=56(%rsp),<rx0=%rsi
addq 56(%rsp),%rsi

# qhasm: rx1 += d1_stack
# asm 1: addq <d1_stack=stack64#9,<rx1=int64#5
# asm 2: addq <d1_stack=64(%rsp),<rx1=%r8
addq 64(%rsp),%r8

# qhasm: rx2 += d2_stack
# asm 1: addq <d2_stack=stack64#10,<rx2=int64#6
# asm 2: addq <d2_stack=72(%rsp),<rx2=%r9
addq 72(%rsp),%r9

# qhasm: rx3 += d3_stack
# asm 1: addq <d3_stack=stack64#11,<rx3=int64#7
# asm 2: addq <d3_stack=80(%rsp),<rx3=%rax
addq 80(%rsp),%rax

# qhasm: rx4 += d4_stack
# asm 1: addq <d4_stack=stack64#12,<rx4=int64#8
# asm 2: addq <d4_stack=88(%rsp),<rx4=%r10
addq 88(%rsp),%r10

# qhasm: rx0 += e0_stack
# asm 1: addq <e0_stack=stack64#23,<rx0=int64#2
# asm 2: addq <e0_stack=176(%rsp),<rx0=%rsi
addq 176(%rsp),%rsi

# qhasm: rx1 += e1_stack
# asm 1: addq <e1_stack=stack64#24,<rx1=int64#5
# asm 2: addq <e1_stack=184(%rsp),<rx1=%r8
addq 184(%rsp),%r8

# qhasm: rx2 += e2_stack
# asm 1: addq <e2_stack=stack64#25,<rx2=int64#6
# asm 2: addq <e2_stack=192(%rsp),<rx2=%r9
addq 192(%rsp),%r9

# qhasm: rx3 += e3_stack
# asm 1: addq <e3_stack=stack64#26,<rx3=int64#7
# asm 2: addq <e3_stack=200(%rsp),<rx3=%rax
addq 200(%rsp),%rax

# qhasm: rx4 += e4_stack
# asm 1: addq <e4_stack=stack64#27,<rx4=int64#8
# asm 2: addq <e4_stack=208(%rsp),<rx4=%r10
addq 208(%rsp),%r10

# qhasm: *(uint64 *)(rp + 0) = rx0
# asm 1: movq   <rx0=int64#2,0(<rp=int64#1)
# asm 2: movq   <rx0=%rsi,0(<rp=%rdi)
movq   %rsi,0(%rdi)

# qhasm: *(uint64 *)(rp + 8) = rx1
# asm 1: movq   <rx1=int64#5,8(<rp=int64#1)
# asm 2: movq   <rx1=%r8,8(<rp=%rdi)
movq   %r8,8(%rdi)

# qhasm: *(uint64 *)(rp + 16) = rx2
# asm 1: movq   <rx2=int64#6,16(<rp=int64#1)
# asm 2: movq   <rx2=%r9,16(<rp=%rdi)
movq   %r9,16(%rdi)

# qhasm: *(uint64 *)(rp + 24) = rx3
# asm 1: movq   <rx3=int64#7,24(<rp=int64#1)
# asm 2: movq   <rx3=%rax,24(<rp=%rdi)
movq   %rax,24(%rdi)

# qhasm: *(uint64 *)(rp + 32) = rx4
# asm 1: movq   <rx4=int64#8,32(<rp=int64#1)
# asm 2: movq   <rx4=%r10,32(<rp=%rdi)
movq   %r10,32(%rdi)

# qhasm:   caller1 = caller1_stack
# asm 1: movq <caller1_stack=stack64#1,>caller1=int64#9
# asm 2: movq <caller1_stack=0(%rsp),>caller1=%r11
movq 0(%rsp),%r11

# qhasm:   caller2 = caller2_stack
# asm 1: movq <caller2_stack=stack64#2,>caller2=int64#10
# asm 2: movq <caller2_stack=8(%rsp),>caller2=%r12
movq 8(%rsp),%r12

# qhasm:   caller3 = caller3_stack
# asm 1: movq <caller3_stack=stack64#3,>caller3=int64#11
# asm 2: movq <caller3_stack=16(%rsp),>caller3=%r13
movq 16(%rsp),%r13

# qhasm:   caller4 = caller4_stack
# asm 1: movq <caller4_stack=stack64#4,>caller4=int64#12
# asm 2: movq <caller4_stack=24(%rsp),>caller4=%r14
movq 24(%rsp),%r14

# qhasm:   caller5 = caller5_stack
# asm 1: movq <caller5_stack=stack64#5,>caller5=int64#13
# asm 2: movq <caller5_stack=32(%rsp),>caller5=%r15
movq 32(%rsp),%r15

# qhasm:   caller6 = caller6_stack
# asm 1: movq <caller6_stack=stack64#6,>caller6=int64#14
# asm 2: movq <caller6_stack=40(%rsp),>caller6=%rbx
movq 40(%rsp),%rbx

# qhasm:   caller7 = caller7_stack
# asm 1: movq <caller7_stack=stack64#7,>caller7=int64#15
# asm 2: movq <caller7_stack=48(%rsp),>caller7=%rbp
movq 48(%rsp),%rbp

# qhasm: leave
add %r11,%rsp
mov %rdi,%rax
mov %rsi,%rdx
ret
