
# qhasm: int64 rp

# qhasm: int64 xp

# qhasm: input rp

# qhasm: input xp

# qhasm: int64 r0

# qhasm: int64 r1

# qhasm: int64 r2

# qhasm: int64 r3

# qhasm: int64 r4

# qhasm: int64 c1

# qhasm: int64 c2

# qhasm: int64 c3

# qhasm: int64 c4

# qhasm: int64 c5

# qhasm: int64 c6

# qhasm: int64 c7

# qhasm: caller c1

# qhasm: caller c2

# qhasm: caller c3

# qhasm: caller c4

# qhasm: caller c5

# qhasm: caller c6

# qhasm: caller c7

# qhasm: stack64 c1_stack

# qhasm: stack64 c2_stack

# qhasm: stack64 c3_stack

# qhasm: stack64 c4_stack

# qhasm: stack64 c5_stack

# qhasm: stack64 c6_stack

# qhasm: stack64 c7_stack

# qhasm: stack64 x119_stack

# qhasm: stack64 x219_stack

# qhasm: stack64 x319_stack

# qhasm: stack64 x419_stack

# qhasm: int64 squarer01

# qhasm: int64 squarer11

# qhasm: int64 squarer21

# qhasm: int64 squarer31

# qhasm: int64 squarer41

# qhasm: int64 squarerax

# qhasm: int64 squarerdx

# qhasm: int64 squaret

# qhasm: int64 squareredmask

# qhasm: enter crypto_sign_ed25519_amd64_51_30k_batch_fe25519_square
.text
.p2align 5
.globl _crypto_sign_ed25519_amd64_51_30k_batch_fe25519_square
.globl crypto_sign_ed25519_amd64_51_30k_batch_fe25519_square
_crypto_sign_ed25519_amd64_51_30k_batch_fe25519_square:
crypto_sign_ed25519_amd64_51_30k_batch_fe25519_square:
mov %rsp,%r11
and $31,%r11
add $64,%r11
sub %r11,%rsp

# qhasm: c1_stack = c1
# asm 1: movq <c1=int64#9,>c1_stack=stack64#1
# asm 2: movq <c1=%r11,>c1_stack=0(%rsp)
movq %r11,0(%rsp)

# qhasm: c2_stack = c2
# asm 1: movq <c2=int64#10,>c2_stack=stack64#2
# asm 2: movq <c2=%r12,>c2_stack=8(%rsp)
movq %r12,8(%rsp)

# qhasm: c3_stack = c3
# asm 1: movq <c3=int64#11,>c3_stack=stack64#3
# asm 2: movq <c3=%r13,>c3_stack=16(%rsp)
movq %r13,16(%rsp)

# qhasm: c4_stack = c4
# asm 1: movq <c4=int64#12,>c4_stack=stack64#4
# asm 2: movq <c4=%r14,>c4_stack=24(%rsp)
movq %r14,24(%rsp)

# qhasm: c5_stack = c5
# asm 1: movq <c5=int64#13,>c5_stack=stack64#5
# asm 2: movq <c5=%r15,>c5_stack=32(%rsp)
movq %r15,32(%rsp)

# qhasm: c6_stack = c6
# asm 1: movq <c6=int64#14,>c6_stack=stack64#6
# asm 2: movq <c6=%rbx,>c6_stack=40(%rsp)
movq %rbx,40(%rsp)

# qhasm: c7_stack = c7
# asm 1: movq <c7=int64#15,>c7_stack=stack64#7
# asm 2: movq <c7=%rbp,>c7_stack=48(%rsp)
movq %rbp,48(%rsp)

# qhasm:   squarerax = *(uint64 *)(xp + 0)
# asm 1: movq   0(<xp=int64#2),>squarerax=int64#7
# asm 2: movq   0(<xp=%rsi),>squarerax=%rax
movq   0(%rsi),%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(xp + 0)
# asm 1: mulq  0(<xp=int64#2)
# asm 2: mulq  0(<xp=%rsi)
mulq  0(%rsi)

# qhasm:   r0 = squarerax
# asm 1: mov  <squarerax=int64#7,>r0=int64#4
# asm 2: mov  <squarerax=%rax,>r0=%rcx
mov  %rax,%rcx

# qhasm:   squarer01 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer01=int64#5
# asm 2: mov  <squarerdx=%rdx,>squarer01=%r8
mov  %rdx,%r8

# qhasm:   squarerax = *(uint64 *)(xp + 0)
# asm 1: movq   0(<xp=int64#2),>squarerax=int64#7
# asm 2: movq   0(<xp=%rsi),>squarerax=%rax
movq   0(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(xp + 8)
# asm 1: mulq  8(<xp=int64#2)
# asm 2: mulq  8(<xp=%rsi)
mulq  8(%rsi)

# qhasm:   r1 = squarerax
# asm 1: mov  <squarerax=int64#7,>r1=int64#6
# asm 2: mov  <squarerax=%rax,>r1=%r9
mov  %rax,%r9

# qhasm:   squarer11 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer11=int64#8
# asm 2: mov  <squarerdx=%rdx,>squarer11=%r10
mov  %rdx,%r10

# qhasm:   squarerax = *(uint64 *)(xp + 0)
# asm 1: movq   0(<xp=int64#2),>squarerax=int64#7
# asm 2: movq   0(<xp=%rsi),>squarerax=%rax
movq   0(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(xp + 16)
# asm 1: mulq  16(<xp=int64#2)
# asm 2: mulq  16(<xp=%rsi)
mulq  16(%rsi)

# qhasm:   r2 = squarerax
# asm 1: mov  <squarerax=int64#7,>r2=int64#9
# asm 2: mov  <squarerax=%rax,>r2=%r11
mov  %rax,%r11

# qhasm:   squarer21 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer21=int64#10
# asm 2: mov  <squarerdx=%rdx,>squarer21=%r12
mov  %rdx,%r12

# qhasm:   squarerax = *(uint64 *)(xp + 0)
# asm 1: movq   0(<xp=int64#2),>squarerax=int64#7
# asm 2: movq   0(<xp=%rsi),>squarerax=%rax
movq   0(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(xp + 24)
# asm 1: mulq  24(<xp=int64#2)
# asm 2: mulq  24(<xp=%rsi)
mulq  24(%rsi)

# qhasm:   r3 = squarerax
# asm 1: mov  <squarerax=int64#7,>r3=int64#11
# asm 2: mov  <squarerax=%rax,>r3=%r13
mov  %rax,%r13

# qhasm:   squarer31 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer31=int64#12
# asm 2: mov  <squarerdx=%rdx,>squarer31=%r14
mov  %rdx,%r14

# qhasm:   squarerax = *(uint64 *)(xp + 0)
# asm 1: movq   0(<xp=int64#2),>squarerax=int64#7
# asm 2: movq   0(<xp=%rsi),>squarerax=%rax
movq   0(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(xp + 32)
# asm 1: mulq  32(<xp=int64#2)
# asm 2: mulq  32(<xp=%rsi)
mulq  32(%rsi)

# qhasm:   r4 = squarerax
# asm 1: mov  <squarerax=int64#7,>r4=int64#13
# asm 2: mov  <squarerax=%rax,>r4=%r15
mov  %rax,%r15

# qhasm:   squarer41 = squarerdx
# asm 1: mov  <squarerdx=int64#3,>squarer41=int64#14
# asm 2: mov  <squarerdx=%rdx,>squarer41=%rbx
mov  %rdx,%rbx

# qhasm:   squarerax = *(uint64 *)(xp + 8)
# asm 1: movq   8(<xp=int64#2),>squarerax=int64#7
# asm 2: movq   8(<xp=%rsi),>squarerax=%rax
movq   8(%rsi),%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(xp + 8)
# asm 1: mulq  8(<xp=int64#2)
# asm 2: mulq  8(<xp=%rsi)
mulq  8(%rsi)

# qhasm:   carry? r2 += squarerax
# asm 1: add  <squarerax=int64#7,<r2=int64#9
# asm 2: add  <squarerax=%rax,<r2=%r11
add  %rax,%r11

# qhasm:   squarer21 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer21=int64#10
# asm 2: adc <squarerdx=%rdx,<squarer21=%r12
adc %rdx,%r12

# qhasm:   squarerax = *(uint64 *)(xp + 8)
# asm 1: movq   8(<xp=int64#2),>squarerax=int64#7
# asm 2: movq   8(<xp=%rsi),>squarerax=%rax
movq   8(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(xp + 16)
# asm 1: mulq  16(<xp=int64#2)
# asm 2: mulq  16(<xp=%rsi)
mulq  16(%rsi)

# qhasm:   carry? r3 += squarerax
# asm 1: add  <squarerax=int64#7,<r3=int64#11
# asm 2: add  <squarerax=%rax,<r3=%r13
add  %rax,%r13

# qhasm:   squarer31 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer31=int64#12
# asm 2: adc <squarerdx=%rdx,<squarer31=%r14
adc %rdx,%r14

# qhasm:   squarerax = *(uint64 *)(xp + 8)
# asm 1: movq   8(<xp=int64#2),>squarerax=int64#7
# asm 2: movq   8(<xp=%rsi),>squarerax=%rax
movq   8(%rsi),%rax

# qhasm:   squarerax <<= 1
# asm 1: shl  $1,<squarerax=int64#7
# asm 2: shl  $1,<squarerax=%rax
shl  $1,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(xp + 24)
# asm 1: mulq  24(<xp=int64#2)
# asm 2: mulq  24(<xp=%rsi)
mulq  24(%rsi)

# qhasm:   carry? r4 += squarerax
# asm 1: add  <squarerax=int64#7,<r4=int64#13
# asm 2: add  <squarerax=%rax,<r4=%r15
add  %rax,%r15

# qhasm:   squarer41 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer41=int64#14
# asm 2: adc <squarerdx=%rdx,<squarer41=%rbx
adc %rdx,%rbx

# qhasm:   squarerax = *(uint64 *)(xp + 8)
# asm 1: movq   8(<xp=int64#2),>squarerax=int64#3
# asm 2: movq   8(<xp=%rsi),>squarerax=%rdx
movq   8(%rsi),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(xp + 32)
# asm 1: mulq  32(<xp=int64#2)
# asm 2: mulq  32(<xp=%rsi)
mulq  32(%rsi)

# qhasm:   carry? r0 += squarerax
# asm 1: add  <squarerax=int64#7,<r0=int64#4
# asm 2: add  <squarerax=%rax,<r0=%rcx
add  %rax,%rcx

# qhasm:   squarer01 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer01=int64#5
# asm 2: adc <squarerdx=%rdx,<squarer01=%r8
adc %rdx,%r8

# qhasm:   squarerax = *(uint64 *)(xp + 16)
# asm 1: movq   16(<xp=int64#2),>squarerax=int64#7
# asm 2: movq   16(<xp=%rsi),>squarerax=%rax
movq   16(%rsi),%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(xp + 16)
# asm 1: mulq  16(<xp=int64#2)
# asm 2: mulq  16(<xp=%rsi)
mulq  16(%rsi)

# qhasm:   carry? r4 += squarerax
# asm 1: add  <squarerax=int64#7,<r4=int64#13
# asm 2: add  <squarerax=%rax,<r4=%r15
add  %rax,%r15

# qhasm:   squarer41 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer41=int64#14
# asm 2: adc <squarerdx=%rdx,<squarer41=%rbx
adc %rdx,%rbx

# qhasm:   squarerax = *(uint64 *)(xp + 16)
# asm 1: movq   16(<xp=int64#2),>squarerax=int64#3
# asm 2: movq   16(<xp=%rsi),>squarerax=%rdx
movq   16(%rsi),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(xp + 24)
# asm 1: mulq  24(<xp=int64#2)
# asm 2: mulq  24(<xp=%rsi)
mulq  24(%rsi)

# qhasm:   carry? r0 += squarerax
# asm 1: add  <squarerax=int64#7,<r0=int64#4
# asm 2: add  <squarerax=%rax,<r0=%rcx
add  %rax,%rcx

# qhasm:   squarer01 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer01=int64#5
# asm 2: adc <squarerdx=%rdx,<squarer01=%r8
adc %rdx,%r8

# qhasm:   squarerax = *(uint64 *)(xp + 16)
# asm 1: movq   16(<xp=int64#2),>squarerax=int64#3
# asm 2: movq   16(<xp=%rsi),>squarerax=%rdx
movq   16(%rsi),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(xp + 32)
# asm 1: mulq  32(<xp=int64#2)
# asm 2: mulq  32(<xp=%rsi)
mulq  32(%rsi)

# qhasm:   carry? r1 += squarerax
# asm 1: add  <squarerax=int64#7,<r1=int64#6
# asm 2: add  <squarerax=%rax,<r1=%r9
add  %rax,%r9

# qhasm:   squarer11 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer11=int64#8
# asm 2: adc <squarerdx=%rdx,<squarer11=%r10
adc %rdx,%r10

# qhasm:   squarerax = *(uint64 *)(xp + 24)
# asm 1: movq   24(<xp=int64#2),>squarerax=int64#3
# asm 2: movq   24(<xp=%rsi),>squarerax=%rdx
movq   24(%rsi),%rdx

# qhasm:   squarerax *= 19
# asm 1: imulq  $19,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $19,<squarerax=%rdx,>squarerax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(xp + 24)
# asm 1: mulq  24(<xp=int64#2)
# asm 2: mulq  24(<xp=%rsi)
mulq  24(%rsi)

# qhasm:   carry? r1 += squarerax
# asm 1: add  <squarerax=int64#7,<r1=int64#6
# asm 2: add  <squarerax=%rax,<r1=%r9
add  %rax,%r9

# qhasm:   squarer11 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer11=int64#8
# asm 2: adc <squarerdx=%rdx,<squarer11=%r10
adc %rdx,%r10

# qhasm:   squarerax = *(uint64 *)(xp + 24)
# asm 1: movq   24(<xp=int64#2),>squarerax=int64#3
# asm 2: movq   24(<xp=%rsi),>squarerax=%rdx
movq   24(%rsi),%rdx

# qhasm:   squarerax *= 38
# asm 1: imulq  $38,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $38,<squarerax=%rdx,>squarerax=%rax
imulq  $38,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(xp + 32)
# asm 1: mulq  32(<xp=int64#2)
# asm 2: mulq  32(<xp=%rsi)
mulq  32(%rsi)

# qhasm:   carry? r2 += squarerax
# asm 1: add  <squarerax=int64#7,<r2=int64#9
# asm 2: add  <squarerax=%rax,<r2=%r11
add  %rax,%r11

# qhasm:   squarer21 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer21=int64#10
# asm 2: adc <squarerdx=%rdx,<squarer21=%r12
adc %rdx,%r12

# qhasm:   squarerax = *(uint64 *)(xp + 32)
# asm 1: movq   32(<xp=int64#2),>squarerax=int64#3
# asm 2: movq   32(<xp=%rsi),>squarerax=%rdx
movq   32(%rsi),%rdx

# qhasm:   squarerax *= 19
# asm 1: imulq  $19,<squarerax=int64#3,>squarerax=int64#7
# asm 2: imulq  $19,<squarerax=%rdx,>squarerax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) squarerdx squarerax = squarerax * *(uint64 *)(xp + 32)
# asm 1: mulq  32(<xp=int64#2)
# asm 2: mulq  32(<xp=%rsi)
mulq  32(%rsi)

# qhasm:   carry? r3 += squarerax
# asm 1: add  <squarerax=int64#7,<r3=int64#11
# asm 2: add  <squarerax=%rax,<r3=%r13
add  %rax,%r13

# qhasm:   squarer31 += squarerdx + carry
# asm 1: adc <squarerdx=int64#3,<squarer31=int64#12
# asm 2: adc <squarerdx=%rdx,<squarer31=%r14
adc %rdx,%r14

# qhasm:   squareredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>squareredmask=int64#2
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>squareredmask=%rsi
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,%rsi

# qhasm:   squarer01 = (squarer01.r0) << 13
# asm 1: shld $13,<r0=int64#4,<squarer01=int64#5
# asm 2: shld $13,<r0=%rcx,<squarer01=%r8
shld $13,%rcx,%r8

# qhasm:   r0 &= squareredmask
# asm 1: and  <squareredmask=int64#2,<r0=int64#4
# asm 2: and  <squareredmask=%rsi,<r0=%rcx
and  %rsi,%rcx

# qhasm:   squarer11 = (squarer11.r1) << 13
# asm 1: shld $13,<r1=int64#6,<squarer11=int64#8
# asm 2: shld $13,<r1=%r9,<squarer11=%r10
shld $13,%r9,%r10

# qhasm:   r1 &= squareredmask
# asm 1: and  <squareredmask=int64#2,<r1=int64#6
# asm 2: and  <squareredmask=%rsi,<r1=%r9
and  %rsi,%r9

# qhasm:   r1 += squarer01
# asm 1: add  <squarer01=int64#5,<r1=int64#6
# asm 2: add  <squarer01=%r8,<r1=%r9
add  %r8,%r9

# qhasm:   squarer21 = (squarer21.r2) << 13
# asm 1: shld $13,<r2=int64#9,<squarer21=int64#10
# asm 2: shld $13,<r2=%r11,<squarer21=%r12
shld $13,%r11,%r12

# qhasm:   r2 &= squareredmask
# asm 1: and  <squareredmask=int64#2,<r2=int64#9
# asm 2: and  <squareredmask=%rsi,<r2=%r11
and  %rsi,%r11

# qhasm:   r2 += squarer11
# asm 1: add  <squarer11=int64#8,<r2=int64#9
# asm 2: add  <squarer11=%r10,<r2=%r11
add  %r10,%r11

# qhasm:   squarer31 = (squarer31.r3) << 13
# asm 1: shld $13,<r3=int64#11,<squarer31=int64#12
# asm 2: shld $13,<r3=%r13,<squarer31=%r14
shld $13,%r13,%r14

# qhasm:   r3 &= squareredmask
# asm 1: and  <squareredmask=int64#2,<r3=int64#11
# asm 2: and  <squareredmask=%rsi,<r3=%r13
and  %rsi,%r13

# qhasm:   r3 += squarer21
# asm 1: add  <squarer21=int64#10,<r3=int64#11
# asm 2: add  <squarer21=%r12,<r3=%r13
add  %r12,%r13

# qhasm:   squarer41 = (squarer41.r4) << 13
# asm 1: shld $13,<r4=int64#13,<squarer41=int64#14
# asm 2: shld $13,<r4=%r15,<squarer41=%rbx
shld $13,%r15,%rbx

# qhasm:   r4 &= squareredmask
# asm 1: and  <squareredmask=int64#2,<r4=int64#13
# asm 2: and  <squareredmask=%rsi,<r4=%r15
and  %rsi,%r15

# qhasm:   r4 += squarer31
# asm 1: add  <squarer31=int64#12,<r4=int64#13
# asm 2: add  <squarer31=%r14,<r4=%r15
add  %r14,%r15

# qhasm:   squarer41 = squarer41 * 19
# asm 1: imulq  $19,<squarer41=int64#14,>squarer41=int64#3
# asm 2: imulq  $19,<squarer41=%rbx,>squarer41=%rdx
imulq  $19,%rbx,%rdx

# qhasm:   r0 += squarer41
# asm 1: add  <squarer41=int64#3,<r0=int64#4
# asm 2: add  <squarer41=%rdx,<r0=%rcx
add  %rdx,%rcx

# qhasm:   squaret = r0
# asm 1: mov  <r0=int64#4,>squaret=int64#3
# asm 2: mov  <r0=%rcx,>squaret=%rdx
mov  %rcx,%rdx

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#3
# asm 2: shr  $51,<squaret=%rdx
shr  $51,%rdx

# qhasm:   squaret += r1
# asm 1: add  <r1=int64#6,<squaret=int64#3
# asm 2: add  <r1=%r9,<squaret=%rdx
add  %r9,%rdx

# qhasm:   r0 &= squareredmask
# asm 1: and  <squareredmask=int64#2,<r0=int64#4
# asm 2: and  <squareredmask=%rsi,<r0=%rcx
and  %rsi,%rcx

# qhasm:   r1 = squaret
# asm 1: mov  <squaret=int64#3,>r1=int64#5
# asm 2: mov  <squaret=%rdx,>r1=%r8
mov  %rdx,%r8

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#3
# asm 2: shr  $51,<squaret=%rdx
shr  $51,%rdx

# qhasm:   squaret += r2
# asm 1: add  <r2=int64#9,<squaret=int64#3
# asm 2: add  <r2=%r11,<squaret=%rdx
add  %r11,%rdx

# qhasm:   r1 &= squareredmask
# asm 1: and  <squareredmask=int64#2,<r1=int64#5
# asm 2: and  <squareredmask=%rsi,<r1=%r8
and  %rsi,%r8

# qhasm:   r2 = squaret
# asm 1: mov  <squaret=int64#3,>r2=int64#6
# asm 2: mov  <squaret=%rdx,>r2=%r9
mov  %rdx,%r9

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#3
# asm 2: shr  $51,<squaret=%rdx
shr  $51,%rdx

# qhasm:   squaret += r3
# asm 1: add  <r3=int64#11,<squaret=int64#3
# asm 2: add  <r3=%r13,<squaret=%rdx
add  %r13,%rdx

# qhasm:   r2 &= squareredmask
# asm 1: and  <squareredmask=int64#2,<r2=int64#6
# asm 2: and  <squareredmask=%rsi,<r2=%r9
and  %rsi,%r9

# qhasm:   r3 = squaret
# asm 1: mov  <squaret=int64#3,>r3=int64#7
# asm 2: mov  <squaret=%rdx,>r3=%rax
mov  %rdx,%rax

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#3
# asm 2: shr  $51,<squaret=%rdx
shr  $51,%rdx

# qhasm:   squaret += r4
# asm 1: add  <r4=int64#13,<squaret=int64#3
# asm 2: add  <r4=%r15,<squaret=%rdx
add  %r15,%rdx

# qhasm:   r3 &= squareredmask
# asm 1: and  <squareredmask=int64#2,<r3=int64#7
# asm 2: and  <squareredmask=%rsi,<r3=%rax
and  %rsi,%rax

# qhasm:   r4 = squaret
# asm 1: mov  <squaret=int64#3,>r4=int64#8
# asm 2: mov  <squaret=%rdx,>r4=%r10
mov  %rdx,%r10

# qhasm:   (uint64) squaret >>= 51
# asm 1: shr  $51,<squaret=int64#3
# asm 2: shr  $51,<squaret=%rdx
shr  $51,%rdx

# qhasm:   squaret *= 19
# asm 1: imulq  $19,<squaret=int64#3,>squaret=int64#3
# asm 2: imulq  $19,<squaret=%rdx,>squaret=%rdx
imulq  $19,%rdx,%rdx

# qhasm:   r0 += squaret
# asm 1: add  <squaret=int64#3,<r0=int64#4
# asm 2: add  <squaret=%rdx,<r0=%rcx
add  %rdx,%rcx

# qhasm:   r4 &= squareredmask
# asm 1: and  <squareredmask=int64#2,<r4=int64#8
# asm 2: and  <squareredmask=%rsi,<r4=%r10
and  %rsi,%r10

# qhasm: *(uint64 *)(rp + 0) = r0
# asm 1: movq   <r0=int64#4,0(<rp=int64#1)
# asm 2: movq   <r0=%rcx,0(<rp=%rdi)
movq   %rcx,0(%rdi)

# qhasm: *(uint64 *)(rp + 8) = r1
# asm 1: movq   <r1=int64#5,8(<rp=int64#1)
# asm 2: movq   <r1=%r8,8(<rp=%rdi)
movq   %r8,8(%rdi)

# qhasm: *(uint64 *)(rp + 16) = r2
# asm 1: movq   <r2=int64#6,16(<rp=int64#1)
# asm 2: movq   <r2=%r9,16(<rp=%rdi)
movq   %r9,16(%rdi)

# qhasm: *(uint64 *)(rp + 24) = r3
# asm 1: movq   <r3=int64#7,24(<rp=int64#1)
# asm 2: movq   <r3=%rax,24(<rp=%rdi)
movq   %rax,24(%rdi)

# qhasm: *(uint64 *)(rp + 32) = r4
# asm 1: movq   <r4=int64#8,32(<rp=int64#1)
# asm 2: movq   <r4=%r10,32(<rp=%rdi)
movq   %r10,32(%rdi)

# qhasm: c1 =c1_stack
# asm 1: movq <c1_stack=stack64#1,>c1=int64#9
# asm 2: movq <c1_stack=0(%rsp),>c1=%r11
movq 0(%rsp),%r11

# qhasm: c2 =c2_stack
# asm 1: movq <c2_stack=stack64#2,>c2=int64#10
# asm 2: movq <c2_stack=8(%rsp),>c2=%r12
movq 8(%rsp),%r12

# qhasm: c3 =c3_stack
# asm 1: movq <c3_stack=stack64#3,>c3=int64#11
# asm 2: movq <c3_stack=16(%rsp),>c3=%r13
movq 16(%rsp),%r13

# qhasm: c4 =c4_stack
# asm 1: movq <c4_stack=stack64#4,>c4=int64#12
# asm 2: movq <c4_stack=24(%rsp),>c4=%r14
movq 24(%rsp),%r14

# qhasm: c5 =c5_stack
# asm 1: movq <c5_stack=stack64#5,>c5=int64#13
# asm 2: movq <c5_stack=32(%rsp),>c5=%r15
movq 32(%rsp),%r15

# qhasm: c6 =c6_stack
# asm 1: movq <c6_stack=stack64#6,>c6=int64#14
# asm 2: movq <c6_stack=40(%rsp),>c6=%rbx
movq 40(%rsp),%rbx

# qhasm: c7 =c7_stack
# asm 1: movq <c7_stack=stack64#7,>c7=int64#15
# asm 2: movq <c7_stack=48(%rsp),>c7=%rbp
movq 48(%rsp),%rbp

# qhasm: leave
add %r11,%rsp
mov %rdi,%rax
mov %rsi,%rdx
ret
