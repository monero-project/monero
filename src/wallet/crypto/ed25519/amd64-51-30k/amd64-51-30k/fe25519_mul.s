
# qhasm: int64 rp

# qhasm: int64 xp

# qhasm: int64 yp

# qhasm: input rp

# qhasm: input xp

# qhasm: input yp

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

# qhasm: stack64 rp_stack

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

# qhasm: enter crypto_sign_ed25519_amd64_51_30k_batch_fe25519_mul
.text
.p2align 5
.globl _crypto_sign_ed25519_amd64_51_30k_batch_fe25519_mul
.globl crypto_sign_ed25519_amd64_51_30k_batch_fe25519_mul
_crypto_sign_ed25519_amd64_51_30k_batch_fe25519_mul:
crypto_sign_ed25519_amd64_51_30k_batch_fe25519_mul:
mov %rsp,%r11
and $31,%r11
add $96,%r11
sub %r11,%rsp

# qhasm:   c1_stack = c1
# asm 1: movq <c1=int64#9,>c1_stack=stack64#1
# asm 2: movq <c1=%r11,>c1_stack=0(%rsp)
movq %r11,0(%rsp)

# qhasm:   c2_stack = c2
# asm 1: movq <c2=int64#10,>c2_stack=stack64#2
# asm 2: movq <c2=%r12,>c2_stack=8(%rsp)
movq %r12,8(%rsp)

# qhasm:   c3_stack = c3
# asm 1: movq <c3=int64#11,>c3_stack=stack64#3
# asm 2: movq <c3=%r13,>c3_stack=16(%rsp)
movq %r13,16(%rsp)

# qhasm:   c4_stack = c4
# asm 1: movq <c4=int64#12,>c4_stack=stack64#4
# asm 2: movq <c4=%r14,>c4_stack=24(%rsp)
movq %r14,24(%rsp)

# qhasm:   c5_stack = c5
# asm 1: movq <c5=int64#13,>c5_stack=stack64#5
# asm 2: movq <c5=%r15,>c5_stack=32(%rsp)
movq %r15,32(%rsp)

# qhasm:   c6_stack = c6
# asm 1: movq <c6=int64#14,>c6_stack=stack64#6
# asm 2: movq <c6=%rbx,>c6_stack=40(%rsp)
movq %rbx,40(%rsp)

# qhasm:   c7_stack = c7
# asm 1: movq <c7=int64#15,>c7_stack=stack64#7
# asm 2: movq <c7=%rbp,>c7_stack=48(%rsp)
movq %rbp,48(%rsp)

# qhasm:   rp_stack = rp
# asm 1: movq <rp=int64#1,>rp_stack=stack64#8
# asm 2: movq <rp=%rdi,>rp_stack=56(%rsp)
movq %rdi,56(%rsp)

# qhasm: yp = yp
# asm 1: mov  <yp=int64#3,>yp=int64#4
# asm 2: mov  <yp=%rdx,>yp=%rcx
mov  %rdx,%rcx

# qhasm:   mulrax = *(uint64 *)(xp + 24)
# asm 1: movq   24(<xp=int64#2),>mulrax=int64#3
# asm 2: movq   24(<xp=%rsi),>mulrax=%rdx
movq   24(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx319_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx319_stack=stack64#9
# asm 2: movq <mulrax=%rax,>mulx319_stack=64(%rsp)
movq %rax,64(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 16)
# asm 1: mulq  16(<yp=int64#4)
# asm 2: mulq  16(<yp=%rcx)
mulq  16(%rcx)

# qhasm:   r0 = mulrax
# asm 1: mov  <mulrax=int64#7,>r0=int64#5
# asm 2: mov  <mulrax=%rax,>r0=%r8
mov  %rax,%r8

# qhasm:   mulr01 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr01=int64#6
# asm 2: mov  <mulrdx=%rdx,>mulr01=%r9
mov  %rdx,%r9

# qhasm:   mulrax = *(uint64 *)(xp + 32)
# asm 1: movq   32(<xp=int64#2),>mulrax=int64#3
# asm 2: movq   32(<xp=%rsi),>mulrax=%rdx
movq   32(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx419_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx419_stack=stack64#10
# asm 2: movq <mulrax=%rax,>mulx419_stack=72(%rsp)
movq %rax,72(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 8)
# asm 1: mulq  8(<yp=int64#4)
# asm 2: mulq  8(<yp=%rcx)
mulq  8(%rcx)

# qhasm:   carry? r0 += mulrax
# asm 1: add  <mulrax=int64#7,<r0=int64#5
# asm 2: add  <mulrax=%rax,<r0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = *(uint64 *)(xp + 0)
# asm 1: movq   0(<xp=int64#2),>mulrax=int64#7
# asm 2: movq   0(<xp=%rsi),>mulrax=%rax
movq   0(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 0)
# asm 1: mulq  0(<yp=int64#4)
# asm 2: mulq  0(<yp=%rcx)
mulq  0(%rcx)

# qhasm:   carry? r0 += mulrax
# asm 1: add  <mulrax=int64#7,<r0=int64#5
# asm 2: add  <mulrax=%rax,<r0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = *(uint64 *)(xp + 0)
# asm 1: movq   0(<xp=int64#2),>mulrax=int64#7
# asm 2: movq   0(<xp=%rsi),>mulrax=%rax
movq   0(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 8)
# asm 1: mulq  8(<yp=int64#4)
# asm 2: mulq  8(<yp=%rcx)
mulq  8(%rcx)

# qhasm:   r1 = mulrax
# asm 1: mov  <mulrax=int64#7,>r1=int64#8
# asm 2: mov  <mulrax=%rax,>r1=%r10
mov  %rax,%r10

# qhasm:   mulr11 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr11=int64#9
# asm 2: mov  <mulrdx=%rdx,>mulr11=%r11
mov  %rdx,%r11

# qhasm:   mulrax = *(uint64 *)(xp + 0)
# asm 1: movq   0(<xp=int64#2),>mulrax=int64#7
# asm 2: movq   0(<xp=%rsi),>mulrax=%rax
movq   0(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 16)
# asm 1: mulq  16(<yp=int64#4)
# asm 2: mulq  16(<yp=%rcx)
mulq  16(%rcx)

# qhasm:   r2 = mulrax
# asm 1: mov  <mulrax=int64#7,>r2=int64#10
# asm 2: mov  <mulrax=%rax,>r2=%r12
mov  %rax,%r12

# qhasm:   mulr21 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr21=int64#11
# asm 2: mov  <mulrdx=%rdx,>mulr21=%r13
mov  %rdx,%r13

# qhasm:   mulrax = *(uint64 *)(xp + 0)
# asm 1: movq   0(<xp=int64#2),>mulrax=int64#7
# asm 2: movq   0(<xp=%rsi),>mulrax=%rax
movq   0(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 24)
# asm 1: mulq  24(<yp=int64#4)
# asm 2: mulq  24(<yp=%rcx)
mulq  24(%rcx)

# qhasm:   r3 = mulrax
# asm 1: mov  <mulrax=int64#7,>r3=int64#12
# asm 2: mov  <mulrax=%rax,>r3=%r14
mov  %rax,%r14

# qhasm:   mulr31 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr31=int64#13
# asm 2: mov  <mulrdx=%rdx,>mulr31=%r15
mov  %rdx,%r15

# qhasm:   mulrax = *(uint64 *)(xp + 0)
# asm 1: movq   0(<xp=int64#2),>mulrax=int64#7
# asm 2: movq   0(<xp=%rsi),>mulrax=%rax
movq   0(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 32)
# asm 1: mulq  32(<yp=int64#4)
# asm 2: mulq  32(<yp=%rcx)
mulq  32(%rcx)

# qhasm:   r4 = mulrax
# asm 1: mov  <mulrax=int64#7,>r4=int64#14
# asm 2: mov  <mulrax=%rax,>r4=%rbx
mov  %rax,%rbx

# qhasm:   mulr41 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr41=int64#15
# asm 2: mov  <mulrdx=%rdx,>mulr41=%rbp
mov  %rdx,%rbp

# qhasm:   mulrax = *(uint64 *)(xp + 8)
# asm 1: movq   8(<xp=int64#2),>mulrax=int64#7
# asm 2: movq   8(<xp=%rsi),>mulrax=%rax
movq   8(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 0)
# asm 1: mulq  0(<yp=int64#4)
# asm 2: mulq  0(<yp=%rcx)
mulq  0(%rcx)

# qhasm:   carry? r1 += mulrax
# asm 1: add  <mulrax=int64#7,<r1=int64#8
# asm 2: add  <mulrax=%rax,<r1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = *(uint64 *)(xp + 8)
# asm 1: movq   8(<xp=int64#2),>mulrax=int64#7
# asm 2: movq   8(<xp=%rsi),>mulrax=%rax
movq   8(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 8)
# asm 1: mulq  8(<yp=int64#4)
# asm 2: mulq  8(<yp=%rcx)
mulq  8(%rcx)

# qhasm:   carry? r2 += mulrax
# asm 1: add  <mulrax=int64#7,<r2=int64#10
# asm 2: add  <mulrax=%rax,<r2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = *(uint64 *)(xp + 8)
# asm 1: movq   8(<xp=int64#2),>mulrax=int64#7
# asm 2: movq   8(<xp=%rsi),>mulrax=%rax
movq   8(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 16)
# asm 1: mulq  16(<yp=int64#4)
# asm 2: mulq  16(<yp=%rcx)
mulq  16(%rcx)

# qhasm:   carry? r3 += mulrax
# asm 1: add  <mulrax=int64#7,<r3=int64#12
# asm 2: add  <mulrax=%rax,<r3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulrax = *(uint64 *)(xp + 8)
# asm 1: movq   8(<xp=int64#2),>mulrax=int64#7
# asm 2: movq   8(<xp=%rsi),>mulrax=%rax
movq   8(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 24)
# asm 1: mulq  24(<yp=int64#4)
# asm 2: mulq  24(<yp=%rcx)
mulq  24(%rcx)

# qhasm:   carry? r4 += mulrax
# asm 1: add  <mulrax=int64#7,<r4=int64#14
# asm 2: add  <mulrax=%rax,<r4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = *(uint64 *)(xp + 8)
# asm 1: movq   8(<xp=int64#2),>mulrax=int64#3
# asm 2: movq   8(<xp=%rsi),>mulrax=%rdx
movq   8(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 32)
# asm 1: mulq  32(<yp=int64#4)
# asm 2: mulq  32(<yp=%rcx)
mulq  32(%rcx)

# qhasm:   carry? r0 += mulrax
# asm 1: add  <mulrax=int64#7,<r0=int64#5
# asm 2: add  <mulrax=%rax,<r0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = *(uint64 *)(xp + 16)
# asm 1: movq   16(<xp=int64#2),>mulrax=int64#7
# asm 2: movq   16(<xp=%rsi),>mulrax=%rax
movq   16(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 0)
# asm 1: mulq  0(<yp=int64#4)
# asm 2: mulq  0(<yp=%rcx)
mulq  0(%rcx)

# qhasm:   carry? r2 += mulrax
# asm 1: add  <mulrax=int64#7,<r2=int64#10
# asm 2: add  <mulrax=%rax,<r2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = *(uint64 *)(xp + 16)
# asm 1: movq   16(<xp=int64#2),>mulrax=int64#7
# asm 2: movq   16(<xp=%rsi),>mulrax=%rax
movq   16(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 8)
# asm 1: mulq  8(<yp=int64#4)
# asm 2: mulq  8(<yp=%rcx)
mulq  8(%rcx)

# qhasm:   carry? r3 += mulrax
# asm 1: add  <mulrax=int64#7,<r3=int64#12
# asm 2: add  <mulrax=%rax,<r3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulrax = *(uint64 *)(xp + 16)
# asm 1: movq   16(<xp=int64#2),>mulrax=int64#7
# asm 2: movq   16(<xp=%rsi),>mulrax=%rax
movq   16(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 16)
# asm 1: mulq  16(<yp=int64#4)
# asm 2: mulq  16(<yp=%rcx)
mulq  16(%rcx)

# qhasm:   carry? r4 += mulrax
# asm 1: add  <mulrax=int64#7,<r4=int64#14
# asm 2: add  <mulrax=%rax,<r4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = *(uint64 *)(xp + 16)
# asm 1: movq   16(<xp=int64#2),>mulrax=int64#3
# asm 2: movq   16(<xp=%rsi),>mulrax=%rdx
movq   16(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 24)
# asm 1: mulq  24(<yp=int64#4)
# asm 2: mulq  24(<yp=%rcx)
mulq  24(%rcx)

# qhasm:   carry? r0 += mulrax
# asm 1: add  <mulrax=int64#7,<r0=int64#5
# asm 2: add  <mulrax=%rax,<r0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = *(uint64 *)(xp + 16)
# asm 1: movq   16(<xp=int64#2),>mulrax=int64#3
# asm 2: movq   16(<xp=%rsi),>mulrax=%rdx
movq   16(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 32)
# asm 1: mulq  32(<yp=int64#4)
# asm 2: mulq  32(<yp=%rcx)
mulq  32(%rcx)

# qhasm:   carry? r1 += mulrax
# asm 1: add  <mulrax=int64#7,<r1=int64#8
# asm 2: add  <mulrax=%rax,<r1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = *(uint64 *)(xp + 24)
# asm 1: movq   24(<xp=int64#2),>mulrax=int64#7
# asm 2: movq   24(<xp=%rsi),>mulrax=%rax
movq   24(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 0)
# asm 1: mulq  0(<yp=int64#4)
# asm 2: mulq  0(<yp=%rcx)
mulq  0(%rcx)

# qhasm:   carry? r3 += mulrax
# asm 1: add  <mulrax=int64#7,<r3=int64#12
# asm 2: add  <mulrax=%rax,<r3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulrax = *(uint64 *)(xp + 24)
# asm 1: movq   24(<xp=int64#2),>mulrax=int64#7
# asm 2: movq   24(<xp=%rsi),>mulrax=%rax
movq   24(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 8)
# asm 1: mulq  8(<yp=int64#4)
# asm 2: mulq  8(<yp=%rcx)
mulq  8(%rcx)

# qhasm:   carry? r4 += mulrax
# asm 1: add  <mulrax=int64#7,<r4=int64#14
# asm 2: add  <mulrax=%rax,<r4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <mulx319_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 24)
# asm 1: mulq  24(<yp=int64#4)
# asm 2: mulq  24(<yp=%rcx)
mulq  24(%rcx)

# qhasm:   carry? r1 += mulrax
# asm 1: add  <mulrax=int64#7,<r1=int64#8
# asm 2: add  <mulrax=%rax,<r1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <mulx319_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 32)
# asm 1: mulq  32(<yp=int64#4)
# asm 2: mulq  32(<yp=%rcx)
mulq  32(%rcx)

# qhasm:   carry? r2 += mulrax
# asm 1: add  <mulrax=int64#7,<r2=int64#10
# asm 2: add  <mulrax=%rax,<r2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = *(uint64 *)(xp + 32)
# asm 1: movq   32(<xp=int64#2),>mulrax=int64#7
# asm 2: movq   32(<xp=%rsi),>mulrax=%rax
movq   32(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 0)
# asm 1: mulq  0(<yp=int64#4)
# asm 2: mulq  0(<yp=%rcx)
mulq  0(%rcx)

# qhasm:   carry? r4 += mulrax
# asm 1: add  <mulrax=int64#7,<r4=int64#14
# asm 2: add  <mulrax=%rax,<r4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#10,>mulrax=int64#7
# asm 2: movq <mulx419_stack=72(%rsp),>mulrax=%rax
movq 72(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 16)
# asm 1: mulq  16(<yp=int64#4)
# asm 2: mulq  16(<yp=%rcx)
mulq  16(%rcx)

# qhasm:   carry? r1 += mulrax
# asm 1: add  <mulrax=int64#7,<r1=int64#8
# asm 2: add  <mulrax=%rax,<r1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#10,>mulrax=int64#7
# asm 2: movq <mulx419_stack=72(%rsp),>mulrax=%rax
movq 72(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 24)
# asm 1: mulq  24(<yp=int64#4)
# asm 2: mulq  24(<yp=%rcx)
mulq  24(%rcx)

# qhasm:   carry? r2 += mulrax
# asm 1: add  <mulrax=int64#7,<r2=int64#10
# asm 2: add  <mulrax=%rax,<r2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#10,>mulrax=int64#7
# asm 2: movq <mulx419_stack=72(%rsp),>mulrax=%rax
movq 72(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(yp + 32)
# asm 1: mulq  32(<yp=int64#4)
# asm 2: mulq  32(<yp=%rcx)
mulq  32(%rcx)

# qhasm:   carry? r3 += mulrax
# asm 1: add  <mulrax=int64#7,<r3=int64#12
# asm 2: add  <mulrax=%rax,<r3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=int64#2
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=%rsi
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,%rsi

# qhasm:   mulr01 = (mulr01.r0) << 13
# asm 1: shld $13,<r0=int64#5,<mulr01=int64#6
# asm 2: shld $13,<r0=%r8,<mulr01=%r9
shld $13,%r8,%r9

# qhasm:   r0 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<r0=int64#5
# asm 2: and  <mulredmask=%rsi,<r0=%r8
and  %rsi,%r8

# qhasm:   mulr11 = (mulr11.r1) << 13
# asm 1: shld $13,<r1=int64#8,<mulr11=int64#9
# asm 2: shld $13,<r1=%r10,<mulr11=%r11
shld $13,%r10,%r11

# qhasm:   r1 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<r1=int64#8
# asm 2: and  <mulredmask=%rsi,<r1=%r10
and  %rsi,%r10

# qhasm:   r1 += mulr01
# asm 1: add  <mulr01=int64#6,<r1=int64#8
# asm 2: add  <mulr01=%r9,<r1=%r10
add  %r9,%r10

# qhasm:   mulr21 = (mulr21.r2) << 13
# asm 1: shld $13,<r2=int64#10,<mulr21=int64#11
# asm 2: shld $13,<r2=%r12,<mulr21=%r13
shld $13,%r12,%r13

# qhasm:   r2 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<r2=int64#10
# asm 2: and  <mulredmask=%rsi,<r2=%r12
and  %rsi,%r12

# qhasm:   r2 += mulr11
# asm 1: add  <mulr11=int64#9,<r2=int64#10
# asm 2: add  <mulr11=%r11,<r2=%r12
add  %r11,%r12

# qhasm:   mulr31 = (mulr31.r3) << 13
# asm 1: shld $13,<r3=int64#12,<mulr31=int64#13
# asm 2: shld $13,<r3=%r14,<mulr31=%r15
shld $13,%r14,%r15

# qhasm:   r3 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<r3=int64#12
# asm 2: and  <mulredmask=%rsi,<r3=%r14
and  %rsi,%r14

# qhasm:   r3 += mulr21
# asm 1: add  <mulr21=int64#11,<r3=int64#12
# asm 2: add  <mulr21=%r13,<r3=%r14
add  %r13,%r14

# qhasm:   mulr41 = (mulr41.r4) << 13
# asm 1: shld $13,<r4=int64#14,<mulr41=int64#15
# asm 2: shld $13,<r4=%rbx,<mulr41=%rbp
shld $13,%rbx,%rbp

# qhasm:   r4 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<r4=int64#14
# asm 2: and  <mulredmask=%rsi,<r4=%rbx
and  %rsi,%rbx

# qhasm:   r4 += mulr31
# asm 1: add  <mulr31=int64#13,<r4=int64#14
# asm 2: add  <mulr31=%r15,<r4=%rbx
add  %r15,%rbx

# qhasm:   mulr41 = mulr41 * 19
# asm 1: imulq  $19,<mulr41=int64#15,>mulr41=int64#3
# asm 2: imulq  $19,<mulr41=%rbp,>mulr41=%rdx
imulq  $19,%rbp,%rdx

# qhasm:   r0 += mulr41
# asm 1: add  <mulr41=int64#3,<r0=int64#5
# asm 2: add  <mulr41=%rdx,<r0=%r8
add  %rdx,%r8

# qhasm:   mult = r0
# asm 1: mov  <r0=int64#5,>mult=int64#3
# asm 2: mov  <r0=%r8,>mult=%rdx
mov  %r8,%rdx

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#3
# asm 2: shr  $51,<mult=%rdx
shr  $51,%rdx

# qhasm:   mult += r1
# asm 1: add  <r1=int64#8,<mult=int64#3
# asm 2: add  <r1=%r10,<mult=%rdx
add  %r10,%rdx

# qhasm:   r1 = mult
# asm 1: mov  <mult=int64#3,>r1=int64#4
# asm 2: mov  <mult=%rdx,>r1=%rcx
mov  %rdx,%rcx

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#3
# asm 2: shr  $51,<mult=%rdx
shr  $51,%rdx

# qhasm:   r0 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<r0=int64#5
# asm 2: and  <mulredmask=%rsi,<r0=%r8
and  %rsi,%r8

# qhasm:   mult += r2
# asm 1: add  <r2=int64#10,<mult=int64#3
# asm 2: add  <r2=%r12,<mult=%rdx
add  %r12,%rdx

# qhasm:   r2 = mult
# asm 1: mov  <mult=int64#3,>r2=int64#6
# asm 2: mov  <mult=%rdx,>r2=%r9
mov  %rdx,%r9

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#3
# asm 2: shr  $51,<mult=%rdx
shr  $51,%rdx

# qhasm:   r1 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<r1=int64#4
# asm 2: and  <mulredmask=%rsi,<r1=%rcx
and  %rsi,%rcx

# qhasm:   mult += r3
# asm 1: add  <r3=int64#12,<mult=int64#3
# asm 2: add  <r3=%r14,<mult=%rdx
add  %r14,%rdx

# qhasm:   r3 = mult
# asm 1: mov  <mult=int64#3,>r3=int64#7
# asm 2: mov  <mult=%rdx,>r3=%rax
mov  %rdx,%rax

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#3
# asm 2: shr  $51,<mult=%rdx
shr  $51,%rdx

# qhasm:   r2 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<r2=int64#6
# asm 2: and  <mulredmask=%rsi,<r2=%r9
and  %rsi,%r9

# qhasm:   mult += r4
# asm 1: add  <r4=int64#14,<mult=int64#3
# asm 2: add  <r4=%rbx,<mult=%rdx
add  %rbx,%rdx

# qhasm:   r4 = mult
# asm 1: mov  <mult=int64#3,>r4=int64#8
# asm 2: mov  <mult=%rdx,>r4=%r10
mov  %rdx,%r10

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#3
# asm 2: shr  $51,<mult=%rdx
shr  $51,%rdx

# qhasm:   r3 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<r3=int64#7
# asm 2: and  <mulredmask=%rsi,<r3=%rax
and  %rsi,%rax

# qhasm:   mult *= 19
# asm 1: imulq  $19,<mult=int64#3,>mult=int64#3
# asm 2: imulq  $19,<mult=%rdx,>mult=%rdx
imulq  $19,%rdx,%rdx

# qhasm:   r0 += mult
# asm 1: add  <mult=int64#3,<r0=int64#5
# asm 2: add  <mult=%rdx,<r0=%r8
add  %rdx,%r8

# qhasm:   r4 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<r4=int64#8
# asm 2: and  <mulredmask=%rsi,<r4=%r10
and  %rsi,%r10

# qhasm: *(uint64 *)(rp + 0) = r0
# asm 1: movq   <r0=int64#5,0(<rp=int64#1)
# asm 2: movq   <r0=%r8,0(<rp=%rdi)
movq   %r8,0(%rdi)

# qhasm: *(uint64 *)(rp + 8) = r1
# asm 1: movq   <r1=int64#4,8(<rp=int64#1)
# asm 2: movq   <r1=%rcx,8(<rp=%rdi)
movq   %rcx,8(%rdi)

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
