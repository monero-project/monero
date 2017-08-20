
# qhasm: int64 rp

# qhasm: input rp

# qhasm: int64 r0

# qhasm: int64 r1

# qhasm: int64 r2

# qhasm: int64 r3

# qhasm: int64 r4

# qhasm: int64 t

# qhasm: int64 loop

# qhasm: int64 two51minus1

# qhasm: int64 two51minus19

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

# qhasm: enter crypto_sign_ed25519_amd64_51_30k_batch_fe25519_freeze
.text
.p2align 5
.globl _crypto_sign_ed25519_amd64_51_30k_batch_fe25519_freeze
.globl crypto_sign_ed25519_amd64_51_30k_batch_fe25519_freeze
_crypto_sign_ed25519_amd64_51_30k_batch_fe25519_freeze:
crypto_sign_ed25519_amd64_51_30k_batch_fe25519_freeze:
mov %rsp,%r11
and $31,%r11
add $64,%r11
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

# qhasm: r0 = *(uint64 *) (rp + 0)
# asm 1: movq   0(<rp=int64#1),>r0=int64#2
# asm 2: movq   0(<rp=%rdi),>r0=%rsi
movq   0(%rdi),%rsi

# qhasm: r1 = *(uint64 *) (rp + 8)
# asm 1: movq   8(<rp=int64#1),>r1=int64#3
# asm 2: movq   8(<rp=%rdi),>r1=%rdx
movq   8(%rdi),%rdx

# qhasm: r2 = *(uint64 *) (rp + 16)
# asm 1: movq   16(<rp=int64#1),>r2=int64#4
# asm 2: movq   16(<rp=%rdi),>r2=%rcx
movq   16(%rdi),%rcx

# qhasm: r3 = *(uint64 *) (rp + 24)
# asm 1: movq   24(<rp=int64#1),>r3=int64#5
# asm 2: movq   24(<rp=%rdi),>r3=%r8
movq   24(%rdi),%r8

# qhasm: r4 = *(uint64 *) (rp + 32)
# asm 1: movq   32(<rp=int64#1),>r4=int64#6
# asm 2: movq   32(<rp=%rdi),>r4=%r9
movq   32(%rdi),%r9

# qhasm: two51minus1 = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>two51minus1=int64#7
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>two51minus1=%rax
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51(%rip),%rax

# qhasm: two51minus19 = two51minus1
# asm 1: mov  <two51minus1=int64#7,>two51minus19=int64#8
# asm 2: mov  <two51minus1=%rax,>two51minus19=%r10
mov  %rax,%r10

# qhasm: two51minus19 -= 18
# asm 1: sub  $18,<two51minus19=int64#8
# asm 2: sub  $18,<two51minus19=%r10
sub  $18,%r10

# qhasm: loop = 3
# asm 1: mov  $3,>loop=int64#9
# asm 2: mov  $3,>loop=%r11
mov  $3,%r11

# qhasm: reduceloop:
._reduceloop:

# qhasm:   t = r0
# asm 1: mov  <r0=int64#2,>t=int64#10
# asm 2: mov  <r0=%rsi,>t=%r12
mov  %rsi,%r12

# qhasm:   (uint64) t >>= 51
# asm 1: shr  $51,<t=int64#10
# asm 2: shr  $51,<t=%r12
shr  $51,%r12

# qhasm:   r0 &= two51minus1
# asm 1: and  <two51minus1=int64#7,<r0=int64#2
# asm 2: and  <two51minus1=%rax,<r0=%rsi
and  %rax,%rsi

# qhasm:   r1 += t
# asm 1: add  <t=int64#10,<r1=int64#3
# asm 2: add  <t=%r12,<r1=%rdx
add  %r12,%rdx

# qhasm:   t = r1
# asm 1: mov  <r1=int64#3,>t=int64#10
# asm 2: mov  <r1=%rdx,>t=%r12
mov  %rdx,%r12

# qhasm:   (uint64) t >>= 51
# asm 1: shr  $51,<t=int64#10
# asm 2: shr  $51,<t=%r12
shr  $51,%r12

# qhasm:   r1 &= two51minus1
# asm 1: and  <two51minus1=int64#7,<r1=int64#3
# asm 2: and  <two51minus1=%rax,<r1=%rdx
and  %rax,%rdx

# qhasm:   r2 += t
# asm 1: add  <t=int64#10,<r2=int64#4
# asm 2: add  <t=%r12,<r2=%rcx
add  %r12,%rcx

# qhasm:   t = r2
# asm 1: mov  <r2=int64#4,>t=int64#10
# asm 2: mov  <r2=%rcx,>t=%r12
mov  %rcx,%r12

# qhasm:   (uint64) t >>= 51
# asm 1: shr  $51,<t=int64#10
# asm 2: shr  $51,<t=%r12
shr  $51,%r12

# qhasm:   r2 &= two51minus1
# asm 1: and  <two51minus1=int64#7,<r2=int64#4
# asm 2: and  <two51minus1=%rax,<r2=%rcx
and  %rax,%rcx

# qhasm:   r3 += t
# asm 1: add  <t=int64#10,<r3=int64#5
# asm 2: add  <t=%r12,<r3=%r8
add  %r12,%r8

# qhasm:   t = r3
# asm 1: mov  <r3=int64#5,>t=int64#10
# asm 2: mov  <r3=%r8,>t=%r12
mov  %r8,%r12

# qhasm:   (uint64) t >>= 51
# asm 1: shr  $51,<t=int64#10
# asm 2: shr  $51,<t=%r12
shr  $51,%r12

# qhasm:   r3 &= two51minus1
# asm 1: and  <two51minus1=int64#7,<r3=int64#5
# asm 2: and  <two51minus1=%rax,<r3=%r8
and  %rax,%r8

# qhasm:   r4 += t
# asm 1: add  <t=int64#10,<r4=int64#6
# asm 2: add  <t=%r12,<r4=%r9
add  %r12,%r9

# qhasm:   t = r4
# asm 1: mov  <r4=int64#6,>t=int64#10
# asm 2: mov  <r4=%r9,>t=%r12
mov  %r9,%r12

# qhasm:   (uint64) t >>= 51
# asm 1: shr  $51,<t=int64#10
# asm 2: shr  $51,<t=%r12
shr  $51,%r12

# qhasm:   r4 &= two51minus1
# asm 1: and  <two51minus1=int64#7,<r4=int64#6
# asm 2: and  <two51minus1=%rax,<r4=%r9
and  %rax,%r9

# qhasm:   t *= 19
# asm 1: imulq  $19,<t=int64#10,>t=int64#10
# asm 2: imulq  $19,<t=%r12,>t=%r12
imulq  $19,%r12,%r12

# qhasm:   r0 += t
# asm 1: add  <t=int64#10,<r0=int64#2
# asm 2: add  <t=%r12,<r0=%rsi
add  %r12,%rsi

# qhasm:                    unsigned>? loop -= 1
# asm 1: sub  $1,<loop=int64#9
# asm 2: sub  $1,<loop=%r11
sub  $1,%r11
# comment:fp stack unchanged by jump

# qhasm: goto reduceloop if unsigned>
ja ._reduceloop

# qhasm: t = 1
# asm 1: mov  $1,>t=int64#10
# asm 2: mov  $1,>t=%r12
mov  $1,%r12

# qhasm:             signed<? r0 - two51minus19
# asm 1: cmp  <two51minus19=int64#8,<r0=int64#2
# asm 2: cmp  <two51minus19=%r10,<r0=%rsi
cmp  %r10,%rsi

# qhasm: t = loop if signed<
# asm 1: cmovl <loop=int64#9,<t=int64#10
# asm 2: cmovl <loop=%r11,<t=%r12
cmovl %r11,%r12

# qhasm:              =? r1 - two51minus1
# asm 1: cmp  <two51minus1=int64#7,<r1=int64#3
# asm 2: cmp  <two51minus1=%rax,<r1=%rdx
cmp  %rax,%rdx

# qhasm: t = loop if !=
# asm 1: cmovne <loop=int64#9,<t=int64#10
# asm 2: cmovne <loop=%r11,<t=%r12
cmovne %r11,%r12

# qhasm:              =? r2 - two51minus1
# asm 1: cmp  <two51minus1=int64#7,<r2=int64#4
# asm 2: cmp  <two51minus1=%rax,<r2=%rcx
cmp  %rax,%rcx

# qhasm: t = loop if !=
# asm 1: cmovne <loop=int64#9,<t=int64#10
# asm 2: cmovne <loop=%r11,<t=%r12
cmovne %r11,%r12

# qhasm:              =? r3 - two51minus1
# asm 1: cmp  <two51minus1=int64#7,<r3=int64#5
# asm 2: cmp  <two51minus1=%rax,<r3=%r8
cmp  %rax,%r8

# qhasm: t = loop if !=
# asm 1: cmovne <loop=int64#9,<t=int64#10
# asm 2: cmovne <loop=%r11,<t=%r12
cmovne %r11,%r12

# qhasm:              =? r4 - two51minus1
# asm 1: cmp  <two51minus1=int64#7,<r4=int64#6
# asm 2: cmp  <two51minus1=%rax,<r4=%r9
cmp  %rax,%r9

# qhasm: t = loop if !=
# asm 1: cmovne <loop=int64#9,<t=int64#10
# asm 2: cmovne <loop=%r11,<t=%r12
cmovne %r11,%r12

# qhasm: t = -t
# asm 1: neg  <t=int64#10
# asm 2: neg  <t=%r12
neg  %r12

# qhasm: two51minus1 &= t
# asm 1: and  <t=int64#10,<two51minus1=int64#7
# asm 2: and  <t=%r12,<two51minus1=%rax
and  %r12,%rax

# qhasm: two51minus19 &= t
# asm 1: and  <t=int64#10,<two51minus19=int64#8
# asm 2: and  <t=%r12,<two51minus19=%r10
and  %r12,%r10

# qhasm: r0 -= two51minus19
# asm 1: sub  <two51minus19=int64#8,<r0=int64#2
# asm 2: sub  <two51minus19=%r10,<r0=%rsi
sub  %r10,%rsi

# qhasm: r1 -= two51minus1
# asm 1: sub  <two51minus1=int64#7,<r1=int64#3
# asm 2: sub  <two51minus1=%rax,<r1=%rdx
sub  %rax,%rdx

# qhasm: r2 -= two51minus1
# asm 1: sub  <two51minus1=int64#7,<r2=int64#4
# asm 2: sub  <two51minus1=%rax,<r2=%rcx
sub  %rax,%rcx

# qhasm: r3 -= two51minus1
# asm 1: sub  <two51minus1=int64#7,<r3=int64#5
# asm 2: sub  <two51minus1=%rax,<r3=%r8
sub  %rax,%r8

# qhasm: r4 -= two51minus1
# asm 1: sub  <two51minus1=int64#7,<r4=int64#6
# asm 2: sub  <two51minus1=%rax,<r4=%r9
sub  %rax,%r9

# qhasm: *(uint64 *)(rp + 0) = r0
# asm 1: movq   <r0=int64#2,0(<rp=int64#1)
# asm 2: movq   <r0=%rsi,0(<rp=%rdi)
movq   %rsi,0(%rdi)

# qhasm: *(uint64 *)(rp + 8) = r1
# asm 1: movq   <r1=int64#3,8(<rp=int64#1)
# asm 2: movq   <r1=%rdx,8(<rp=%rdi)
movq   %rdx,8(%rdi)

# qhasm: *(uint64 *)(rp + 16) = r2
# asm 1: movq   <r2=int64#4,16(<rp=int64#1)
# asm 2: movq   <r2=%rcx,16(<rp=%rdi)
movq   %rcx,16(%rdi)

# qhasm: *(uint64 *)(rp + 24) = r3
# asm 1: movq   <r3=int64#5,24(<rp=int64#1)
# asm 2: movq   <r3=%r8,24(<rp=%rdi)
movq   %r8,24(%rdi)

# qhasm: *(uint64 *)(rp + 32) = r4
# asm 1: movq   <r4=int64#6,32(<rp=int64#1)
# asm 2: movq   <r4=%r9,32(<rp=%rdi)
movq   %r9,32(%rdi)

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
