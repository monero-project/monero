
# qhasm: int64 tp

# qhasm: int64 pos

# qhasm: int64 b

# qhasm: int64 basep

# qhasm: input tp

# qhasm: input pos

# qhasm: input b

# qhasm: input basep

# qhasm: int64 mask

# qhasm: int64 u

# qhasm: int64 tysubx0

# qhasm: int64 tysubx1

# qhasm: int64 tysubx2

# qhasm: int64 tysubx3

# qhasm: int64 tysubx4

# qhasm: int64 txaddy0

# qhasm: int64 txaddy1

# qhasm: int64 txaddy2

# qhasm: int64 txaddy3

# qhasm: int64 txaddy4

# qhasm: int64 tt2d0

# qhasm: int64 tt2d1

# qhasm: int64 tt2d2

# qhasm: int64 tt2d3

# qhasm: int64 tt2d4

# qhasm: int64 tt0

# qhasm: int64 tt1

# qhasm: int64 tt2

# qhasm: int64 tt3

# qhasm: int64 tt4

# qhasm: int64 t

# qhasm: stack64 tp_stack

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

# qhasm: enter crypto_sign_ed25519_amd64_51_30k_batch_choose_t
.text
.p2align 5
.globl _crypto_sign_ed25519_amd64_51_30k_batch_choose_tp
.globl crypto_sign_ed25519_amd64_51_30k_batch_choose_tp
_crypto_sign_ed25519_amd64_51_30k_batch_choose_tp:
crypto_sign_ed25519_amd64_51_30k_batch_choose_tp:
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

# qhasm: tp_stack = tp
# asm 1: movq <tp=int64#1,>tp_stack=stack64#8
# asm 2: movq <tp=%rdi,>tp_stack=56(%rsp)
movq %rdi,56(%rsp)

# qhasm: pos *= 960
# asm 1: imulq  $960,<pos=int64#2,>pos=int64#1
# asm 2: imulq  $960,<pos=%rsi,>pos=%rdi
imulq  $960,%rsi,%rdi

# qhasm: mask = b
# asm 1: mov  <b=int64#3,>mask=int64#2
# asm 2: mov  <b=%rdx,>mask=%rsi
mov  %rdx,%rsi

# qhasm: (int64) mask >>= 7
# asm 1: sar  $7,<mask=int64#2
# asm 2: sar  $7,<mask=%rsi
sar  $7,%rsi

# qhasm: u = b
# asm 1: mov  <b=int64#3,>u=int64#5
# asm 2: mov  <b=%rdx,>u=%r8
mov  %rdx,%r8

# qhasm: u += mask
# asm 1: add  <mask=int64#2,<u=int64#5
# asm 2: add  <mask=%rsi,<u=%r8
add  %rsi,%r8

# qhasm: u ^= mask
# asm 1: xor  <mask=int64#2,<u=int64#5
# asm 2: xor  <mask=%rsi,<u=%r8
xor  %rsi,%r8

# qhasm: tysubx0 = 1
# asm 1: mov  $1,>tysubx0=int64#2
# asm 2: mov  $1,>tysubx0=%rsi
mov  $1,%rsi

# qhasm: tysubx1 = 0
# asm 1: mov  $0,>tysubx1=int64#6
# asm 2: mov  $0,>tysubx1=%r9
mov  $0,%r9

# qhasm: tysubx2 = 0
# asm 1: mov  $0,>tysubx2=int64#7
# asm 2: mov  $0,>tysubx2=%rax
mov  $0,%rax

# qhasm: tysubx3 = 0
# asm 1: mov  $0,>tysubx3=int64#8
# asm 2: mov  $0,>tysubx3=%r10
mov  $0,%r10

# qhasm: tysubx4 = 0
# asm 1: mov  $0,>tysubx4=int64#9
# asm 2: mov  $0,>tysubx4=%r11
mov  $0,%r11

# qhasm: txaddy0 = 1
# asm 1: mov  $1,>txaddy0=int64#10
# asm 2: mov  $1,>txaddy0=%r12
mov  $1,%r12

# qhasm: txaddy1 = 0
# asm 1: mov  $0,>txaddy1=int64#11
# asm 2: mov  $0,>txaddy1=%r13
mov  $0,%r13

# qhasm: txaddy2 = 0
# asm 1: mov  $0,>txaddy2=int64#12
# asm 2: mov  $0,>txaddy2=%r14
mov  $0,%r14

# qhasm: txaddy3 = 0
# asm 1: mov  $0,>txaddy3=int64#13
# asm 2: mov  $0,>txaddy3=%r15
mov  $0,%r15

# qhasm: txaddy4 = 0
# asm 1: mov  $0,>txaddy4=int64#14
# asm 2: mov  $0,>txaddy4=%rbx
mov  $0,%rbx

# qhasm: =? u - 1
# asm 1: cmp  $1,<u=int64#5
# asm 2: cmp  $1,<u=%r8
cmp  $1,%r8

# qhasm: t = *(uint64 *)(basep + 0 + pos)
# asm 1: movq   0(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   0(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   0(%rcx,%rdi),%rbp

# qhasm: tysubx0 = t if =
# asm 1: cmove <t=int64#15,<tysubx0=int64#2
# asm 2: cmove <t=%rbp,<tysubx0=%rsi
cmove %rbp,%rsi

# qhasm: t = *(uint64 *)(basep + 8 + pos)
# asm 1: movq   8(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   8(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   8(%rcx,%rdi),%rbp

# qhasm: tysubx1 = t if =
# asm 1: cmove <t=int64#15,<tysubx1=int64#6
# asm 2: cmove <t=%rbp,<tysubx1=%r9
cmove %rbp,%r9

# qhasm: t = *(uint64 *)(basep + 16 + pos)
# asm 1: movq   16(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   16(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   16(%rcx,%rdi),%rbp

# qhasm: tysubx2 = t if =
# asm 1: cmove <t=int64#15,<tysubx2=int64#7
# asm 2: cmove <t=%rbp,<tysubx2=%rax
cmove %rbp,%rax

# qhasm: t = *(uint64 *)(basep + 24 + pos)
# asm 1: movq   24(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   24(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   24(%rcx,%rdi),%rbp

# qhasm: tysubx3 = t if =
# asm 1: cmove <t=int64#15,<tysubx3=int64#8
# asm 2: cmove <t=%rbp,<tysubx3=%r10
cmove %rbp,%r10

# qhasm: t = *(uint64 *)(basep + 32 + pos)
# asm 1: movq   32(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   32(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   32(%rcx,%rdi),%rbp

# qhasm: tysubx4 = t if =
# asm 1: cmove <t=int64#15,<tysubx4=int64#9
# asm 2: cmove <t=%rbp,<tysubx4=%r11
cmove %rbp,%r11

# qhasm: t = *(uint64 *)(basep + 40 + pos)
# asm 1: movq   40(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   40(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   40(%rcx,%rdi),%rbp

# qhasm: txaddy0 = t if =
# asm 1: cmove <t=int64#15,<txaddy0=int64#10
# asm 2: cmove <t=%rbp,<txaddy0=%r12
cmove %rbp,%r12

# qhasm: t = *(uint64 *)(basep + 48 + pos)
# asm 1: movq   48(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   48(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   48(%rcx,%rdi),%rbp

# qhasm: txaddy1 = t if =
# asm 1: cmove <t=int64#15,<txaddy1=int64#11
# asm 2: cmove <t=%rbp,<txaddy1=%r13
cmove %rbp,%r13

# qhasm: t = *(uint64 *)(basep + 56 + pos)
# asm 1: movq   56(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   56(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   56(%rcx,%rdi),%rbp

# qhasm: txaddy2 = t if =
# asm 1: cmove <t=int64#15,<txaddy2=int64#12
# asm 2: cmove <t=%rbp,<txaddy2=%r14
cmove %rbp,%r14

# qhasm: t = *(uint64 *)(basep + 64 + pos)
# asm 1: movq   64(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   64(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   64(%rcx,%rdi),%rbp

# qhasm: txaddy3 = t if =
# asm 1: cmove <t=int64#15,<txaddy3=int64#13
# asm 2: cmove <t=%rbp,<txaddy3=%r15
cmove %rbp,%r15

# qhasm: t = *(uint64 *)(basep + 72 + pos)
# asm 1: movq   72(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   72(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   72(%rcx,%rdi),%rbp

# qhasm: txaddy4 = t if =
# asm 1: cmove <t=int64#15,<txaddy4=int64#14
# asm 2: cmove <t=%rbp,<txaddy4=%rbx
cmove %rbp,%rbx

# qhasm: =? u - 2
# asm 1: cmp  $2,<u=int64#5
# asm 2: cmp  $2,<u=%r8
cmp  $2,%r8

# qhasm: t = *(uint64 *)(basep + 160 + pos)
# asm 1: movq   160(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   160(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   160(%rcx,%rdi),%rbp

# qhasm: tysubx0 = t if =
# asm 1: cmove <t=int64#15,<tysubx0=int64#2
# asm 2: cmove <t=%rbp,<tysubx0=%rsi
cmove %rbp,%rsi

# qhasm: t = *(uint64 *)(basep + 168 + pos)
# asm 1: movq   168(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   168(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   168(%rcx,%rdi),%rbp

# qhasm: tysubx1 = t if =
# asm 1: cmove <t=int64#15,<tysubx1=int64#6
# asm 2: cmove <t=%rbp,<tysubx1=%r9
cmove %rbp,%r9

# qhasm: t = *(uint64 *)(basep + 176 + pos)
# asm 1: movq   176(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   176(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   176(%rcx,%rdi),%rbp

# qhasm: tysubx2 = t if =
# asm 1: cmove <t=int64#15,<tysubx2=int64#7
# asm 2: cmove <t=%rbp,<tysubx2=%rax
cmove %rbp,%rax

# qhasm: t = *(uint64 *)(basep + 184 + pos)
# asm 1: movq   184(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   184(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   184(%rcx,%rdi),%rbp

# qhasm: tysubx3 = t if =
# asm 1: cmove <t=int64#15,<tysubx3=int64#8
# asm 2: cmove <t=%rbp,<tysubx3=%r10
cmove %rbp,%r10

# qhasm: t = *(uint64 *)(basep + 192 + pos)
# asm 1: movq   192(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   192(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   192(%rcx,%rdi),%rbp

# qhasm: tysubx4 = t if =
# asm 1: cmove <t=int64#15,<tysubx4=int64#9
# asm 2: cmove <t=%rbp,<tysubx4=%r11
cmove %rbp,%r11

# qhasm: t = *(uint64 *)(basep + 200 + pos)
# asm 1: movq   200(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   200(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   200(%rcx,%rdi),%rbp

# qhasm: txaddy0 = t if =
# asm 1: cmove <t=int64#15,<txaddy0=int64#10
# asm 2: cmove <t=%rbp,<txaddy0=%r12
cmove %rbp,%r12

# qhasm: t = *(uint64 *)(basep + 208 + pos)
# asm 1: movq   208(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   208(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   208(%rcx,%rdi),%rbp

# qhasm: txaddy1 = t if =
# asm 1: cmove <t=int64#15,<txaddy1=int64#11
# asm 2: cmove <t=%rbp,<txaddy1=%r13
cmove %rbp,%r13

# qhasm: t = *(uint64 *)(basep + 216 + pos)
# asm 1: movq   216(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   216(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   216(%rcx,%rdi),%rbp

# qhasm: txaddy2 = t if =
# asm 1: cmove <t=int64#15,<txaddy2=int64#12
# asm 2: cmove <t=%rbp,<txaddy2=%r14
cmove %rbp,%r14

# qhasm: t = *(uint64 *)(basep + 184 + pos)
# asm 1: movq   184(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   184(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   224(%rcx,%rdi),%rbp

# qhasm: txaddy3 = t if =
# asm 1: cmove <t=int64#15,<txaddy3=int64#13
# asm 2: cmove <t=%rbp,<txaddy3=%r15
cmove %rbp,%r15

# qhasm: t = *(uint64 *)(basep + 232 + pos)
# asm 1: movq   232(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   232(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   232(%rcx,%rdi),%rbp

# qhasm: txaddy4 = t if =
# asm 1: cmove <t=int64#15,<txaddy4=int64#14
# asm 2: cmove <t=%rbp,<txaddy4=%rbx
cmove %rbp,%rbx

# qhasm: =? u - 3
# asm 1: cmp  $3,<u=int64#5
# asm 2: cmp  $3,<u=%r8
cmp  $3,%r8

# qhasm: t = *(uint64 *)(basep + 320 + pos)
# asm 1: movq   320(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   320(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   320(%rcx,%rdi),%rbp

# qhasm: tysubx0 = t if =
# asm 1: cmove <t=int64#15,<tysubx0=int64#2
# asm 2: cmove <t=%rbp,<tysubx0=%rsi
cmove %rbp,%rsi

# qhasm: t = *(uint64 *)(basep + 328 + pos)
# asm 1: movq   328(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   328(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   328(%rcx,%rdi),%rbp

# qhasm: tysubx1 = t if =
# asm 1: cmove <t=int64#15,<tysubx1=int64#6
# asm 2: cmove <t=%rbp,<tysubx1=%r9
cmove %rbp,%r9

# qhasm: t = *(uint64 *)(basep + 336 + pos)
# asm 1: movq   336(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   336(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   336(%rcx,%rdi),%rbp

# qhasm: tysubx2 = t if =
# asm 1: cmove <t=int64#15,<tysubx2=int64#7
# asm 2: cmove <t=%rbp,<tysubx2=%rax
cmove %rbp,%rax

# qhasm: t = *(uint64 *)(basep + 344 + pos)
# asm 1: movq   344(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   344(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   344(%rcx,%rdi),%rbp

# qhasm: tysubx3 = t if =
# asm 1: cmove <t=int64#15,<tysubx3=int64#8
# asm 2: cmove <t=%rbp,<tysubx3=%r10
cmove %rbp,%r10

# qhasm: t = *(uint64 *)(basep + 272 + pos)
# asm 1: movq   272(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   272(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   352(%rcx,%rdi),%rbp

# qhasm: tysubx4 = t if =
# asm 1: cmove <t=int64#15,<tysubx4=int64#9
# asm 2: cmove <t=%rbp,<tysubx4=%r11
cmove %rbp,%r11

# qhasm: t = *(uint64 *)(basep + 360 + pos)
# asm 1: movq   360(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   360(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   360(%rcx,%rdi),%rbp

# qhasm: txaddy0 = t if =
# asm 1: cmove <t=int64#15,<txaddy0=int64#10
# asm 2: cmove <t=%rbp,<txaddy0=%r12
cmove %rbp,%r12

# qhasm: t = *(uint64 *)(basep + 368 + pos)
# asm 1: movq   368(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   368(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   368(%rcx,%rdi),%rbp

# qhasm: txaddy1 = t if =
# asm 1: cmove <t=int64#15,<txaddy1=int64#11
# asm 2: cmove <t=%rbp,<txaddy1=%r13
cmove %rbp,%r13

# qhasm: t = *(uint64 *)(basep + 376 + pos)
# asm 1: movq   376(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   376(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   376(%rcx,%rdi),%rbp

# qhasm: txaddy2 = t if =
# asm 1: cmove <t=int64#15,<txaddy2=int64#12
# asm 2: cmove <t=%rbp,<txaddy2=%r14
cmove %rbp,%r14

# qhasm: t = *(uint64 *)(basep + 384 + pos)
# asm 1: movq   384(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   384(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   384(%rcx,%rdi),%rbp

# qhasm: txaddy3 = t if =
# asm 1: cmove <t=int64#15,<txaddy3=int64#13
# asm 2: cmove <t=%rbp,<txaddy3=%r15
cmove %rbp,%r15

# qhasm: t = *(uint64 *)(basep + 392 + pos)
# asm 1: movq   392(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   392(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   392(%rcx,%rdi),%rbp

# qhasm: txaddy4 = t if =
# asm 1: cmove <t=int64#15,<txaddy4=int64#14
# asm 2: cmove <t=%rbp,<txaddy4=%rbx
cmove %rbp,%rbx

# qhasm: =? u - 4
# asm 1: cmp  $4,<u=int64#5
# asm 2: cmp  $4,<u=%r8
cmp  $4,%r8

# qhasm: t = *(uint64 *)(basep + 480 + pos)
# asm 1: movq   480(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   480(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   480(%rcx,%rdi),%rbp

# qhasm: tysubx0 = t if =
# asm 1: cmove <t=int64#15,<tysubx0=int64#2
# asm 2: cmove <t=%rbp,<tysubx0=%rsi
cmove %rbp,%rsi

# qhasm: t = *(uint64 *)(basep + 488 + pos)
# asm 1: movq   488(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   488(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   488(%rcx,%rdi),%rbp

# qhasm: tysubx1 = t if =
# asm 1: cmove <t=int64#15,<tysubx1=int64#6
# asm 2: cmove <t=%rbp,<tysubx1=%r9
cmove %rbp,%r9

# qhasm: t = *(uint64 *)(basep + 496 + pos)
# asm 1: movq   496(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   496(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   496(%rcx,%rdi),%rbp

# qhasm: tysubx2 = t if =
# asm 1: cmove <t=int64#15,<tysubx2=int64#7
# asm 2: cmove <t=%rbp,<tysubx2=%rax
cmove %rbp,%rax

# qhasm: t = *(uint64 *)(basep + 384 + pos)
# asm 1: movq   384(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   384(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   504(%rcx,%rdi),%rbp

# qhasm: tysubx3 = t if =
# asm 1: cmove <t=int64#15,<tysubx3=int64#8
# asm 2: cmove <t=%rbp,<tysubx3=%r10
cmove %rbp,%r10

# qhasm: t = *(uint64 *)(basep + 512 + pos)
# asm 1: movq   512(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   512(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   512(%rcx,%rdi),%rbp

# qhasm: tysubx4 = t if =
# asm 1: cmove <t=int64#15,<tysubx4=int64#9
# asm 2: cmove <t=%rbp,<tysubx4=%r11
cmove %rbp,%r11

# qhasm: t = *(uint64 *)(basep + 400 + pos)
# asm 1: movq   400(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   400(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   520(%rcx,%rdi),%rbp

# qhasm: txaddy0 = t if =
# asm 1: cmove <t=int64#15,<txaddy0=int64#10
# asm 2: cmove <t=%rbp,<txaddy0=%r12
cmove %rbp,%r12

# qhasm: t = *(uint64 *)(basep + 528 + pos)
# asm 1: movq   528(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   528(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   528(%rcx,%rdi),%rbp

# qhasm: txaddy1 = t if =
# asm 1: cmove <t=int64#15,<txaddy1=int64#11
# asm 2: cmove <t=%rbp,<txaddy1=%r13
cmove %rbp,%r13

# qhasm: t = *(uint64 *)(basep + 536 + pos)
# asm 1: movq   536(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   536(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   536(%rcx,%rdi),%rbp

# qhasm: txaddy2 = t if =
# asm 1: cmove <t=int64#15,<txaddy2=int64#12
# asm 2: cmove <t=%rbp,<txaddy2=%r14
cmove %rbp,%r14

# qhasm: t = *(uint64 *)(basep + 424 + pos)
# asm 1: movq   424(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   424(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   544(%rcx,%rdi),%rbp

# qhasm: txaddy3 = t if =
# asm 1: cmove <t=int64#15,<txaddy3=int64#13
# asm 2: cmove <t=%rbp,<txaddy3=%r15
cmove %rbp,%r15

# qhasm: t = *(uint64 *)(basep + 552 + pos)
# asm 1: movq   552(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   552(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   552(%rcx,%rdi),%rbp

# qhasm: txaddy4 = t if =
# asm 1: cmove <t=int64#15,<txaddy4=int64#14
# asm 2: cmove <t=%rbp,<txaddy4=%rbx
cmove %rbp,%rbx

# qhasm: =? u - 5
# asm 1: cmp  $5,<u=int64#5
# asm 2: cmp  $5,<u=%r8
cmp  $5,%r8

# qhasm: t = *(uint64 *)(basep + 640 + pos)
# asm 1: movq   640(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   640(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   640(%rcx,%rdi),%rbp

# qhasm: tysubx0 = t if =
# asm 1: cmove <t=int64#15,<tysubx0=int64#2
# asm 2: cmove <t=%rbp,<tysubx0=%rsi
cmove %rbp,%rsi

# qhasm: t = *(uint64 *)(basep + 648 + pos)
# asm 1: movq   648(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   648(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   648(%rcx,%rdi),%rbp

# qhasm: tysubx1 = t if =
# asm 1: cmove <t=int64#15,<tysubx1=int64#6
# asm 2: cmove <t=%rbp,<tysubx1=%r9
cmove %rbp,%r9

# qhasm: t = *(uint64 *)(basep + 656 + pos)
# asm 1: movq   656(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   656(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   656(%rcx,%rdi),%rbp

# qhasm: tysubx2 = t if =
# asm 1: cmove <t=int64#15,<tysubx2=int64#7
# asm 2: cmove <t=%rbp,<tysubx2=%rax
cmove %rbp,%rax

# qhasm: t = *(uint64 *)(basep + 664 + pos)
# asm 1: movq   664(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   664(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   664(%rcx,%rdi),%rbp

# qhasm: tysubx3 = t if =
# asm 1: cmove <t=int64#15,<tysubx3=int64#8
# asm 2: cmove <t=%rbp,<tysubx3=%r10
cmove %rbp,%r10

# qhasm: t = *(uint64 *)(basep + 512 + pos)
# asm 1: movq   512(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   512(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   672(%rcx,%rdi),%rbp

# qhasm: tysubx4 = t if =
# asm 1: cmove <t=int64#15,<tysubx4=int64#9
# asm 2: cmove <t=%rbp,<tysubx4=%r11
cmove %rbp,%r11

# qhasm: t = *(uint64 *)(basep + 680 + pos)
# asm 1: movq   680(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   680(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   680(%rcx,%rdi),%rbp

# qhasm: txaddy0 = t if =
# asm 1: cmove <t=int64#15,<txaddy0=int64#10
# asm 2: cmove <t=%rbp,<txaddy0=%r12
cmove %rbp,%r12

# qhasm: t = *(uint64 *)(basep + 688 + pos)
# asm 1: movq   688(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   688(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   688(%rcx,%rdi),%rbp

# qhasm: txaddy1 = t if =
# asm 1: cmove <t=int64#15,<txaddy1=int64#11
# asm 2: cmove <t=%rbp,<txaddy1=%r13
cmove %rbp,%r13

# qhasm: t = *(uint64 *)(basep + 696 + pos)
# asm 1: movq   696(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   696(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   696(%rcx,%rdi),%rbp

# qhasm: txaddy2 = t if =
# asm 1: cmove <t=int64#15,<txaddy2=int64#12
# asm 2: cmove <t=%rbp,<txaddy2=%r14
cmove %rbp,%r14

# qhasm: t = *(uint64 *)(basep + 704 + pos)
# asm 1: movq   704(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   704(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   704(%rcx,%rdi),%rbp

# qhasm: txaddy3 = t if =
# asm 1: cmove <t=int64#15,<txaddy3=int64#13
# asm 2: cmove <t=%rbp,<txaddy3=%r15
cmove %rbp,%r15

# qhasm: t = *(uint64 *)(basep + 712 + pos)
# asm 1: movq   712(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   712(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   712(%rcx,%rdi),%rbp

# qhasm: txaddy4 = t if =
# asm 1: cmove <t=int64#15,<txaddy4=int64#14
# asm 2: cmove <t=%rbp,<txaddy4=%rbx
cmove %rbp,%rbx

# qhasm: =? u - 6
# asm 1: cmp  $6,<u=int64#5
# asm 2: cmp  $6,<u=%r8
cmp  $6,%r8

# qhasm: t = *(uint64 *)(basep + 800 + pos)
# asm 1: movq   800(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   800(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   800(%rcx,%rdi),%rbp

# qhasm: tysubx0 = t if =
# asm 1: cmove <t=int64#15,<tysubx0=int64#2
# asm 2: cmove <t=%rbp,<tysubx0=%rsi
cmove %rbp,%rsi

# qhasm: t = *(uint64 *)(basep + 808 + pos)
# asm 1: movq   808(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   808(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   808(%rcx,%rdi),%rbp

# qhasm: tysubx1 = t if =
# asm 1: cmove <t=int64#15,<tysubx1=int64#6
# asm 2: cmove <t=%rbp,<tysubx1=%r9
cmove %rbp,%r9

# qhasm: t = *(uint64 *)(basep + 816 + pos)
# asm 1: movq   816(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   816(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   816(%rcx,%rdi),%rbp

# qhasm: tysubx2 = t if =
# asm 1: cmove <t=int64#15,<tysubx2=int64#7
# asm 2: cmove <t=%rbp,<tysubx2=%rax
cmove %rbp,%rax

# qhasm: t = *(uint64 *)(basep + 824 + pos)
# asm 1: movq   824(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   824(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   824(%rcx,%rdi),%rbp

# qhasm: tysubx3 = t if =
# asm 1: cmove <t=int64#15,<tysubx3=int64#8
# asm 2: cmove <t=%rbp,<tysubx3=%r10
cmove %rbp,%r10

# qhasm: t = *(uint64 *)(basep + 832 + pos)
# asm 1: movq   832(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   832(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   832(%rcx,%rdi),%rbp

# qhasm: tysubx4 = t if =
# asm 1: cmove <t=int64#15,<tysubx4=int64#9
# asm 2: cmove <t=%rbp,<tysubx4=%r11
cmove %rbp,%r11

# qhasm: t = *(uint64 *)(basep + 640 + pos)
# asm 1: movq   640(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   640(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   840(%rcx,%rdi),%rbp

# qhasm: txaddy0 = t if =
# asm 1: cmove <t=int64#15,<txaddy0=int64#10
# asm 2: cmove <t=%rbp,<txaddy0=%r12
cmove %rbp,%r12

# qhasm: t = *(uint64 *)(basep + 848 + pos)
# asm 1: movq   848(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   848(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   848(%rcx,%rdi),%rbp

# qhasm: txaddy1 = t if =
# asm 1: cmove <t=int64#15,<txaddy1=int64#11
# asm 2: cmove <t=%rbp,<txaddy1=%r13
cmove %rbp,%r13

# qhasm: t = *(uint64 *)(basep + 856 + pos)
# asm 1: movq   856(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   856(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   856(%rcx,%rdi),%rbp

# qhasm: txaddy2 = t if =
# asm 1: cmove <t=int64#15,<txaddy2=int64#12
# asm 2: cmove <t=%rbp,<txaddy2=%r14
cmove %rbp,%r14

# qhasm: t = *(uint64 *)(basep + 864 + pos)
# asm 1: movq   864(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   864(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   864(%rcx,%rdi),%rbp

# qhasm: txaddy3 = t if =
# asm 1: cmove <t=int64#15,<txaddy3=int64#13
# asm 2: cmove <t=%rbp,<txaddy3=%r15
cmove %rbp,%r15

# qhasm: t = *(uint64 *)(basep + 872 + pos)
# asm 1: movq   872(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   872(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   872(%rcx,%rdi),%rbp

# qhasm: txaddy4 = t if =
# asm 1: cmove <t=int64#15,<txaddy4=int64#14
# asm 2: cmove <t=%rbp,<txaddy4=%rbx
cmove %rbp,%rbx

# qhasm: =? u - 7
# asm 1: cmp  $7,<u=int64#5
# asm 2: cmp  $7,<u=%r8
cmp  $7,%r8

# qhasm: t = *(uint64 *)(basep + 960 + pos)
# asm 1: movq   960(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   960(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   960(%rcx,%rdi),%rbp

# qhasm: tysubx0 = t if =
# asm 1: cmove <t=int64#15,<tysubx0=int64#2
# asm 2: cmove <t=%rbp,<tysubx0=%rsi
cmove %rbp,%rsi

# qhasm: t = *(uint64 *)(basep + 968 + pos)
# asm 1: movq   968(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   968(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   968(%rcx,%rdi),%rbp

# qhasm: tysubx1 = t if =
# asm 1: cmove <t=int64#15,<tysubx1=int64#6
# asm 2: cmove <t=%rbp,<tysubx1=%r9
cmove %rbp,%r9

# qhasm: t = *(uint64 *)(basep + 976 + pos)
# asm 1: movq   976(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   976(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   976(%rcx,%rdi),%rbp

# qhasm: tysubx2 = t if =
# asm 1: cmove <t=int64#15,<tysubx2=int64#7
# asm 2: cmove <t=%rbp,<tysubx2=%rax
cmove %rbp,%rax

# qhasm: t = *(uint64 *)(basep + 984 + pos)
# asm 1: movq   984(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   984(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   984(%rcx,%rdi),%rbp

# qhasm: tysubx3 = t if =
# asm 1: cmove <t=int64#15,<tysubx3=int64#8
# asm 2: cmove <t=%rbp,<tysubx3=%r10
cmove %rbp,%r10

# qhasm: t = *(uint64 *)(basep + 992 + pos)
# asm 1: movq   992(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   992(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   992(%rcx,%rdi),%rbp

# qhasm: tysubx4 = t if =
# asm 1: cmove <t=int64#15,<tysubx4=int64#9
# asm 2: cmove <t=%rbp,<tysubx4=%r11
cmove %rbp,%r11

# qhasm: t = *(uint64 *)(basep + 1000 + pos)
# asm 1: movq   1000(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   1000(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   1000(%rcx,%rdi),%rbp

# qhasm: txaddy0 = t if =
# asm 1: cmove <t=int64#15,<txaddy0=int64#10
# asm 2: cmove <t=%rbp,<txaddy0=%r12
cmove %rbp,%r12

# qhasm: t = *(uint64 *)(basep + 1008 + pos)
# asm 1: movq   1008(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   1008(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   1008(%rcx,%rdi),%rbp

# qhasm: txaddy1 = t if =
# asm 1: cmove <t=int64#15,<txaddy1=int64#11
# asm 2: cmove <t=%rbp,<txaddy1=%r13
cmove %rbp,%r13

# qhasm: t = *(uint64 *)(basep + 1016 + pos)
# asm 1: movq   1016(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   1016(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   1016(%rcx,%rdi),%rbp

# qhasm: txaddy2 = t if =
# asm 1: cmove <t=int64#15,<txaddy2=int64#12
# asm 2: cmove <t=%rbp,<txaddy2=%r14
cmove %rbp,%r14

# qhasm: t = *(uint64 *)(basep + 1024 + pos)
# asm 1: movq   1024(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   1024(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   1024(%rcx,%rdi),%rbp

# qhasm: txaddy3 = t if =
# asm 1: cmove <t=int64#15,<txaddy3=int64#13
# asm 2: cmove <t=%rbp,<txaddy3=%r15
cmove %rbp,%r15

# qhasm: t = *(uint64 *)(basep + 1032 + pos)
# asm 1: movq   1032(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   1032(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   1032(%rcx,%rdi),%rbp

# qhasm: txaddy4 = t if =
# asm 1: cmove <t=int64#15,<txaddy4=int64#14
# asm 2: cmove <t=%rbp,<txaddy4=%rbx
cmove %rbp,%rbx

# qhasm: =? u - 8
# asm 1: cmp  $8,<u=int64#5
# asm 2: cmp  $8,<u=%r8
cmp  $8,%r8

# qhasm: t = *(uint64 *)(basep + 1120 + pos)
# asm 1: movq   1120(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   1120(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   1120(%rcx,%rdi),%rbp

# qhasm: tysubx0 = t if =
# asm 1: cmove <t=int64#15,<tysubx0=int64#2
# asm 2: cmove <t=%rbp,<tysubx0=%rsi
cmove %rbp,%rsi

# qhasm: t = *(uint64 *)(basep + 1128 + pos)
# asm 1: movq   1128(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   1128(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   1128(%rcx,%rdi),%rbp

# qhasm: tysubx1 = t if =
# asm 1: cmove <t=int64#15,<tysubx1=int64#6
# asm 2: cmove <t=%rbp,<tysubx1=%r9
cmove %rbp,%r9

# qhasm: t = *(uint64 *)(basep + 1136 + pos)
# asm 1: movq   1136(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   1136(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   1136(%rcx,%rdi),%rbp

# qhasm: tysubx2 = t if =
# asm 1: cmove <t=int64#15,<tysubx2=int64#7
# asm 2: cmove <t=%rbp,<tysubx2=%rax
cmove %rbp,%rax

# qhasm: t = *(uint64 *)(basep + 1144 + pos)
# asm 1: movq   1144(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   1144(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   1144(%rcx,%rdi),%rbp

# qhasm: tysubx3 = t if =
# asm 1: cmove <t=int64#15,<tysubx3=int64#8
# asm 2: cmove <t=%rbp,<tysubx3=%r10
cmove %rbp,%r10

# qhasm: t = *(uint64 *)(basep + 1152 + pos)
# asm 1: movq   1152(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   1152(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   1152(%rcx,%rdi),%rbp

# qhasm: tysubx4 = t if =
# asm 1: cmove <t=int64#15,<tysubx4=int64#9
# asm 2: cmove <t=%rbp,<tysubx4=%r11
cmove %rbp,%r11

# qhasm: t = *(uint64 *)(basep + 1160 + pos)
# asm 1: movq   1160(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   1160(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   1160(%rcx,%rdi),%rbp

# qhasm: txaddy0 = t if =
# asm 1: cmove <t=int64#15,<txaddy0=int64#10
# asm 2: cmove <t=%rbp,<txaddy0=%r12
cmove %rbp,%r12

# qhasm: t = *(uint64 *)(basep + 1168 + pos)
# asm 1: movq   1168(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   1168(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   1168(%rcx,%rdi),%rbp

# qhasm: txaddy1 = t if =
# asm 1: cmove <t=int64#15,<txaddy1=int64#11
# asm 2: cmove <t=%rbp,<txaddy1=%r13
cmove %rbp,%r13

# qhasm: t = *(uint64 *)(basep + 1176 + pos)
# asm 1: movq   1176(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   1176(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   1176(%rcx,%rdi),%rbp

# qhasm: txaddy2 = t if =
# asm 1: cmove <t=int64#15,<txaddy2=int64#12
# asm 2: cmove <t=%rbp,<txaddy2=%r14
cmove %rbp,%r14

# qhasm: t = *(uint64 *)(basep + 1184 + pos)
# asm 1: movq   1184(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   1184(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   1184(%rcx,%rdi),%rbp

# qhasm: txaddy3 = t if =
# asm 1: cmove <t=int64#15,<txaddy3=int64#13
# asm 2: cmove <t=%rbp,<txaddy3=%r15
cmove %rbp,%r15

# qhasm: t = *(uint64 *)(basep + 1192 + pos)
# asm 1: movq   1192(<basep=int64#4,<pos=int64#1),>t=int64#15
# asm 2: movq   1192(<basep=%rcx,<pos=%rdi),>t=%rbp
movq   1192(%rcx,%rdi),%rbp

# qhasm: txaddy4 = t if =
# asm 1: cmove <t=int64#15,<txaddy4=int64#14
# asm 2: cmove <t=%rbp,<txaddy4=%rbx
cmove %rbp,%rbx

# qhasm: signed<? b - 0
# asm 1: cmp  $0,<b=int64#3
# asm 2: cmp  $0,<b=%rdx
cmp  $0,%rdx

# qhasm: t = tysubx0
# asm 1: mov  <tysubx0=int64#2,>t=int64#15
# asm 2: mov  <tysubx0=%rsi,>t=%rbp
mov  %rsi,%rbp

# qhasm: tysubx0 = txaddy0 if signed<
# asm 1: cmovl <txaddy0=int64#10,<tysubx0=int64#2
# asm 2: cmovl <txaddy0=%r12,<tysubx0=%rsi
cmovl %r12,%rsi

# qhasm: txaddy0 = t if signed<
# asm 1: cmovl <t=int64#15,<txaddy0=int64#10
# asm 2: cmovl <t=%rbp,<txaddy0=%r12
cmovl %rbp,%r12

# qhasm: t = tysubx1
# asm 1: mov  <tysubx1=int64#6,>t=int64#15
# asm 2: mov  <tysubx1=%r9,>t=%rbp
mov  %r9,%rbp

# qhasm: tysubx1 = txaddy1 if signed<
# asm 1: cmovl <txaddy1=int64#11,<tysubx1=int64#6
# asm 2: cmovl <txaddy1=%r13,<tysubx1=%r9
cmovl %r13,%r9

# qhasm: txaddy1 = t if signed<
# asm 1: cmovl <t=int64#15,<txaddy1=int64#11
# asm 2: cmovl <t=%rbp,<txaddy1=%r13
cmovl %rbp,%r13

# qhasm: t = tysubx2
# asm 1: mov  <tysubx2=int64#7,>t=int64#15
# asm 2: mov  <tysubx2=%rax,>t=%rbp
mov  %rax,%rbp

# qhasm: tysubx2 = txaddy2 if signed<
# asm 1: cmovl <txaddy2=int64#12,<tysubx2=int64#7
# asm 2: cmovl <txaddy2=%r14,<tysubx2=%rax
cmovl %r14,%rax

# qhasm: txaddy2 = t if signed<
# asm 1: cmovl <t=int64#15,<txaddy2=int64#12
# asm 2: cmovl <t=%rbp,<txaddy2=%r14
cmovl %rbp,%r14

# qhasm: t = tysubx3
# asm 1: mov  <tysubx3=int64#8,>t=int64#15
# asm 2: mov  <tysubx3=%r10,>t=%rbp
mov  %r10,%rbp

# qhasm: tysubx3 = txaddy3 if signed<
# asm 1: cmovl <txaddy3=int64#13,<tysubx3=int64#8
# asm 2: cmovl <txaddy3=%r15,<tysubx3=%r10
cmovl %r15,%r10

# qhasm: txaddy3 = t if signed<
# asm 1: cmovl <t=int64#15,<txaddy3=int64#13
# asm 2: cmovl <t=%rbp,<txaddy3=%r15
cmovl %rbp,%r15

# qhasm: t = tysubx4
# asm 1: mov  <tysubx4=int64#9,>t=int64#15
# asm 2: mov  <tysubx4=%r11,>t=%rbp
mov  %r11,%rbp

# qhasm: tysubx4 = txaddy4 if signed<
# asm 1: cmovl <txaddy4=int64#14,<tysubx4=int64#9
# asm 2: cmovl <txaddy4=%rbx,<tysubx4=%r11
cmovl %rbx,%r11

# qhasm: txaddy4 = t if signed<
# asm 1: cmovl <t=int64#15,<txaddy4=int64#14
# asm 2: cmovl <t=%rbp,<txaddy4=%rbx
cmovl %rbp,%rbx

# qhasm: tp = tp_stack
# asm 1: movq <tp_stack=stack64#8,>tp=int64#15
# asm 2: movq <tp_stack=56(%rsp),>tp=%rbp
movq 56(%rsp),%rbp

# qhasm: *(uint64 *)(tp + 0) = tysubx0
# asm 1: movq   <tysubx0=int64#2,0(<tp=int64#15)
# asm 2: movq   <tysubx0=%rsi,0(<tp=%rbp)
movq   %rsi,0(%rbp)

# qhasm: *(uint64 *)(tp + 8) = tysubx1
# asm 1: movq   <tysubx1=int64#6,8(<tp=int64#15)
# asm 2: movq   <tysubx1=%r9,8(<tp=%rbp)
movq   %r9,8(%rbp)

# qhasm: *(uint64 *)(tp + 16) = tysubx2
# asm 1: movq   <tysubx2=int64#7,16(<tp=int64#15)
# asm 2: movq   <tysubx2=%rax,16(<tp=%rbp)
movq   %rax,16(%rbp)

# qhasm: *(uint64 *)(tp + 24) = tysubx3
# asm 1: movq   <tysubx3=int64#8,24(<tp=int64#15)
# asm 2: movq   <tysubx3=%r10,24(<tp=%rbp)
movq   %r10,24(%rbp)

# qhasm: *(uint64 *)(tp + 32) = tysubx4
# asm 1: movq   <tysubx4=int64#9,32(<tp=int64#15)
# asm 2: movq   <tysubx4=%r11,32(<tp=%rbp)
movq   %r11,32(%rbp)

# qhasm: *(uint64 *)(tp + 40) = txaddy0
# asm 1: movq   <txaddy0=int64#10,40(<tp=int64#15)
# asm 2: movq   <txaddy0=%r12,40(<tp=%rbp)
movq   %r12,40(%rbp)

# qhasm: *(uint64 *)(tp + 48) = txaddy1
# asm 1: movq   <txaddy1=int64#11,48(<tp=int64#15)
# asm 2: movq   <txaddy1=%r13,48(<tp=%rbp)
movq   %r13,48(%rbp)

# qhasm: *(uint64 *)(tp + 56) = txaddy2
# asm 1: movq   <txaddy2=int64#12,56(<tp=int64#15)
# asm 2: movq   <txaddy2=%r14,56(<tp=%rbp)
movq   %r14,56(%rbp)

# qhasm: *(uint64 *)(tp + 64) = txaddy3
# asm 1: movq   <txaddy3=int64#13,64(<tp=int64#15)
# asm 2: movq   <txaddy3=%r15,64(<tp=%rbp)
movq   %r15,64(%rbp)

# qhasm: *(uint64 *)(tp + 72) = txaddy4
# asm 1: movq   <txaddy4=int64#14,72(<tp=int64#15)
# asm 2: movq   <txaddy4=%rbx,72(<tp=%rbp)
movq   %rbx,72(%rbp)

# qhasm: tz0 = 1
mov  $1,%r12

# qhasm: tz1 = 0
mov  $0,%r13

# qhasm: tz2 = 0
mov  $0,%r14

# qhasm: tz3 = 0
mov  $0,%r15

# qhasm: tz4 = 0
mov  $0,%rbx

# qhasm: tt2d0 = 0
# asm 1: mov  $0,>tt2d0=int64#2
# asm 2: mov  $0,>tt2d0=%rsi
mov  $0,%rsi

# qhasm: tt2d1 = 0
# asm 1: mov  $0,>tt2d1=int64#6
# asm 2: mov  $0,>tt2d1=%r9
mov  $0,%r9

# qhasm: tt2d2 = 0
# asm 1: mov  $0,>tt2d2=int64#7
# asm 2: mov  $0,>tt2d2=%rax
mov  $0,%rax

# qhasm: tt2d3 = 0
# asm 1: mov  $0,>tt2d3=int64#8
# asm 2: mov  $0,>tt2d3=%r10
mov  $0,%r10

# qhasm: tt2d4 = 0
# asm 1: mov  $0,>tt2d4=int64#9
# asm 2: mov  $0,>tt2d4=%r11
mov  $0,%r11

# qhasm: =? u - 1
# asm 1: cmp  $1,<u=int64#5
# asm 2: cmp  $1,<u=%r8
cmp  $1,%r8

# qhasm: t = *(uint64 *)(basep + 80 + pos)
# asm 1: movq   80(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   80(<basep=%rcx,<pos=%rdi),>t=%r12
movq   80(%rcx,%rdi),%rbp

# qhasm: tz0 = t if =
cmove %rbp,%r12

# qhasm: t = *(uint64 *)(basep + 88 + pos)
# asm 1: movq   80(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   80(<basep=%rcx,<pos=%rdi),>t=%r12
movq   88(%rcx,%rdi),%rbp

# qhasm: tz1 = t if =
cmove %rbp,%r13

# qhasm: t = *(uint64 *)(basep + 96 + pos)
# asm 1: movq   96(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   96(<basep=%rcx,<pos=%rdi),>t=%r12
movq   96(%rcx,%rdi),%rbp

# qhasm: tz2 = t if =
cmove %rbp,%r14

# qhasm: t = *(uint64 *)(basep + 104 + pos)
# asm 1: movq   104(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   104(<basep=%rcx,<pos=%rdi),>t=%r12
movq   104(%rcx,%rdi),%rbp

# qhasm: tz3 = t if =
cmove %rbp,%r15

# qhasm: t = *(uint64 *)(basep + 112 + pos)
# asm 1: movq   112(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   112(<basep=%rcx,<pos=%rdi),>t=%r12
movq   112(%rcx,%rdi),%rbp

# qhasm: tz4 = t if =
cmove %rbp,%rbx

# qhasm: t = *(uint64 *)(basep + 120 + pos)
# asm 1: movq   120(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   120(<basep=%rcx,<pos=%rdi),>t=%r12
movq   120(%rcx,%rdi),%rbp

# qhasm: tt2d0 = t if =
# asm 1: cmove <t=int64#10,<tt2d0=int64#2
# asm 2: cmove <t=%r12,<tt2d0=%rsi
cmove %rbp,%rsi

# qhasm: t = *(uint64 *)(basep + 128 + pos)
# asm 1: movq   128(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   128(<basep=%rcx,<pos=%rdi),>t=%r12
movq   128(%rcx,%rdi),%rbp

# qhasm: tt2d1 = t if =
# asm 1: cmove <t=int64#10,<tt2d1=int64#6
# asm 2: cmove <t=%r12,<tt2d1=%r9
cmove %rbp,%r9

# qhasm: t = *(uint64 *)(basep + 136 + pos)
# asm 1: movq   136(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   136(<basep=%rcx,<pos=%rdi),>t=%r12
movq   136(%rcx,%rdi),%rbp

# qhasm: tt2d2 = t if =
# asm 1: cmove <t=int64#10,<tt2d2=int64#7
# asm 2: cmove <t=%r12,<tt2d2=%rax
cmove %rbp,%rax

# qhasm: t = *(uint64 *)(basep + 144 + pos)
# asm 1: movq   144(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   144(<basep=%rcx,<pos=%rdi),>t=%r12
movq   144(%rcx,%rdi),%rbp

# qhasm: tt2d3 = t if =
# asm 1: cmove <t=int64#10,<tt2d3=int64#8
# asm 2: cmove <t=%r12,<tt2d3=%r10
cmove %rbp,%r10

# qhasm: t = *(uint64 *)(basep + 152 + pos)
# asm 1: movq   152(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   152(<basep=%rcx,<pos=%rdi),>t=%r12
movq   152(%rcx,%rdi),%rbp

# qhasm: tt2d4 = t if =
# asm 1: cmove <t=int64#10,<tt2d4=int64#9
# asm 2: cmove <t=%r12,<tt2d4=%r11
cmove %rbp,%r11

# qhasm: =? u - 2
# asm 1: cmp  $2,<u=int64#5
# asm 2: cmp  $2,<u=%r8
cmp  $2,%r8

# qhasm: t = *(uint64 *)(basep + 240 + pos)
# asm 1: movq   240(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   240(<basep=%rcx,<pos=%rdi),>t=%r12
movq   240(%rcx,%rdi),%rbp

# qhasm: tz0 = t if =
cmove %rbp,%r12

# qhasm: t = *(uint64 *)(basep + 248 + pos)
# asm 1: movq   248(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   248(<basep=%rcx,<pos=%rdi),>t=%r12
movq   248(%rcx,%rdi),%rbp

# qhasm: tz1 = t if =
cmove %rbp,%r13

# qhasm: t = *(uint64 *)(basep + 256 + pos)
# asm 1: movq   256(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   256(<basep=%rcx,<pos=%rdi),>t=%r12
movq   256(%rcx,%rdi),%rbp

# qhasm: tz2 = t if =
cmove %rbp,%r14

# qhasm: t = *(uint64 *)(basep + 264 + pos)
# asm 1: movq   264(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   264(<basep=%rcx,<pos=%rdi),>t=%r12
movq   264(%rcx,%rdi),%rbp

# qhasm: tz3 = t if =
cmove %rbp,%r15

# qhasm: t = *(uint64 *)(basep + 272 + pos)
# asm 1: movq   272(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   272(<basep=%rcx,<pos=%rdi),>t=%r12
movq   272(%rcx,%rdi),%rbp

# qhasm: tz4 = t if =
cmove %rbp,%rbx

# qhasm: t = *(uint64 *)(basep + 280 + pos)
# asm 1: movq   280(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   280(<basep=%rcx,<pos=%rdi),>t=%r12
movq   280(%rcx,%rdi),%rbp

# qhasm: tt2d0 = t if =
# asm 1: cmove <t=int64#10,<tt2d0=int64#2
# asm 2: cmove <t=%r12,<tt2d0=%rsi
cmove %rbp,%rsi

# qhasm: t = *(uint64 *)(basep + 288 + pos)
# asm 1: movq   288(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   288(<basep=%rcx,<pos=%rdi),>t=%r12
movq   288(%rcx,%rdi),%rbp

# qhasm: tt2d1 = t if =
# asm 1: cmove <t=int64#10,<tt2d1=int64#6
# asm 2: cmove <t=%r12,<tt2d1=%r9
cmove %rbp,%r9

# qhasm: t = *(uint64 *)(basep + 296 + pos)
# asm 1: movq   296(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   296(<basep=%rcx,<pos=%rdi),>t=%r12
movq   296(%rcx,%rdi),%rbp

# qhasm: tt2d2 = t if =
# asm 1: cmove <t=int64#10,<tt2d2=int64#7
# asm 2: cmove <t=%r12,<tt2d2=%rax
cmove %rbp,%rax

# qhasm: t = *(uint64 *)(basep + 304 + pos)
# asm 1: movq   304(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   304(<basep=%rcx,<pos=%rdi),>t=%r12
movq   304(%rcx,%rdi),%rbp

# qhasm: tt2d3 = t if =
# asm 1: cmove <t=int64#10,<tt2d3=int64#8
# asm 2: cmove <t=%r12,<tt2d3=%r10
cmove %rbp,%r10

# qhasm: t = *(uint64 *)(basep + 312 + pos)
# asm 1: movq   312(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   312(<basep=%rcx,<pos=%rdi),>t=%r12
movq   312(%rcx,%rdi),%rbp

# qhasm: tt2d4 = t if =
# asm 1: cmove <t=int64#10,<tt2d4=int64#9
# asm 2: cmove <t=%r12,<tt2d4=%r11
cmove %rbp,%r11

# qhasm: =? u - 3
# asm 1: cmp  $3,<u=int64#5
# asm 2: cmp  $3,<u=%r8
cmp  $3,%r8

# qhasm: t = *(uint64 *)(basep + 400 + pos)
# asm 1: movq   400(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   400(<basep=%rcx,<pos=%rdi),>t=%r12
movq   400(%rcx,%rdi),%rbp

# qhasm: tz0 = t if =
cmove %rbp,%r12

# qhasm: t = *(uint64 *)(basep + 408 + pos)
# asm 1: movq   408(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   408(<basep=%rcx,<pos=%rdi),>t=%r12
movq   408(%rcx,%rdi),%rbp

# qhasm: tz1 = t if =
cmove %rbp,%r13

# qhasm: t = *(uint64 *)(basep + 416 + pos)
# asm 1: movq   416(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   416(<basep=%rcx,<pos=%rdi),>t=%r12
movq   416(%rcx,%rdi),%rbp

# qhasm: tz2 = t if =
cmove %rbp,%r14

# qhasm: t = *(uint64 *)(basep + 424 + pos)
# asm 1: movq   424(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   424(<basep=%rcx,<pos=%rdi),>t=%r12
movq   424(%rcx,%rdi),%rbp

# qhasm: tz3 = t if =
cmove %rbp,%r15

# qhasm: t = *(uint64 *)(basep + 432 + pos)
# asm 1: movq   432(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   432(<basep=%rcx,<pos=%rdi),>t=%r12
movq   432(%rcx,%rdi),%rbp

# qhasm: tz4 = t if =
cmove %rbp,%rbx

# qhasm: t = *(uint64 *)(basep + 440 + pos)
# asm 1: movq   440(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   440(<basep=%rcx,<pos=%rdi),>t=%r12
movq   440(%rcx,%rdi),%rbp

# qhasm: tt2d0 = t if =
# asm 1: cmove <t=int64#10,<tt2d0=int64#2
# asm 2: cmove <t=%r12,<tt2d0=%rsi
cmove %rbp,%rsi

# qhasm: t = *(uint64 *)(basep + 448 + pos)
# asm 1: movq   448(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   448(<basep=%rcx,<pos=%rdi),>t=%r12
movq   448(%rcx,%rdi),%rbp

# qhasm: tt2d1 = t if =
# asm 1: cmove <t=int64#10,<tt2d1=int64#6
# asm 2: cmove <t=%r12,<tt2d1=%r9
cmove %rbp,%r9

# qhasm: t = *(uint64 *)(basep + 336 + pos)
# asm 1: movq   336(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   336(<basep=%rcx,<pos=%rdi),>t=%r12
movq   456(%rcx,%rdi),%rbp

# qhasm: tt2d2 = t if =
# asm 1: cmove <t=int64#10,<tt2d2=int64#7
# asm 2: cmove <t=%r12,<tt2d2=%rax
cmove %rbp,%rax

# qhasm: t = *(uint64 *)(basep + 344 + pos)
# asm 1: movq   344(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   344(<basep=%rcx,<pos=%rdi),>t=%r12
movq   464(%rcx,%rdi),%rbp

# qhasm: tt2d3 = t if =
# asm 1: cmove <t=int64#10,<tt2d3=int64#8
# asm 2: cmove <t=%r12,<tt2d3=%r10
cmove %rbp,%r10

# qhasm: t = *(uint64 *)(basep + 472 + pos)
# asm 1: movq   472(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   472(<basep=%rcx,<pos=%rdi),>t=%r12
movq   472(%rcx,%rdi),%rbp

# qhasm: tt2d4 = t if =
# asm 1: cmove <t=int64#10,<tt2d4=int64#9
# asm 2: cmove <t=%r12,<tt2d4=%r11
cmove %rbp,%r11

# qhasm: =? u - 4
# asm 1: cmp  $4,<u=int64#5
# asm 2: cmp  $4,<u=%r8
cmp  $4,%r8

# qhasm: t = *(uint64 *)(basep + 560 + pos)
# asm 1: movq   560(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   560(<basep=%rcx,<pos=%rdi),>t=%r12
movq   560(%rcx,%rdi),%rbp

# qhasm: tz0 = t if =
cmove %rbp,%r12

# qhasm: t = *(uint64 *)(basep + 568 + pos)
# asm 1: movq   568(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   568(<basep=%rcx,<pos=%rdi),>t=%r12
movq   568(%rcx,%rdi),%rbp

# qhasm: tz1 = t if =
cmove %rbp,%r13

# qhasm: t = *(uint64 *)(basep + 576 + pos)
# asm 1: movq   576(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   576(<basep=%rcx,<pos=%rdi),>t=%r12
movq   576(%rcx,%rdi),%rbp

# qhasm: tz2 = t if =
cmove %rbp,%r14

# qhasm: t = *(uint64 *)(basep + 584 + pos)
# asm 1: movq   584(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   584(<basep=%rcx,<pos=%rdi),>t=%r12
movq   584(%rcx,%rdi),%rbp

# qhasm: tz3 = t if =
cmove %rbp,%r15

# qhasm: t = *(uint64 *)(basep + 592 + pos)
# asm 1: movq   592(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   592(<basep=%rcx,<pos=%rdi),>t=%r12
movq   592(%rcx,%rdi),%rbp

# qhasm: tz4 = t if =
cmove %rbp,%rbx

# qhasm: t = *(uint64 *)(basep + 600 + pos)
# asm 1: movq   600(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   600(<basep=%rcx,<pos=%rdi),>t=%r12
movq   600(%rcx,%rdi),%rbp

# qhasm: tt2d0 = t if =
# asm 1: cmove <t=int64#10,<tt2d0=int64#2
# asm 2: cmove <t=%r12,<tt2d0=%rsi
cmove %rbp,%rsi

# qhasm: t = *(uint64 *)(basep + 608 + pos)
# asm 1: movq   608(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   608(<basep=%rcx,<pos=%rdi),>t=%r12
movq   608(%rcx,%rdi),%rbp

# qhasm: tt2d1 = t if =
# asm 1: cmove <t=int64#10,<tt2d1=int64#6
# asm 2: cmove <t=%r12,<tt2d1=%r9
cmove %rbp,%r9

# qhasm: t = *(uint64 *)(basep + 616 + pos)
# asm 1: movq   616(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   616(<basep=%rcx,<pos=%rdi),>t=%r12
movq   616(%rcx,%rdi),%rbp

# qhasm: tt2d2 = t if =
# asm 1: cmove <t=int64#10,<tt2d2=int64#7
# asm 2: cmove <t=%r12,<tt2d2=%rax
cmove %rbp,%rax

# qhasm: t = *(uint64 *)(basep + 624 + pos)
# asm 1: movq   624(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   624(<basep=%rcx,<pos=%rdi),>t=%r12
movq   624(%rcx,%rdi),%rbp

# qhasm: tt2d3 = t if =
# asm 1: cmove <t=int64#10,<tt2d3=int64#8
# asm 2: cmove <t=%r12,<tt2d3=%r10
cmove %rbp,%r10

# qhasm: t = *(uint64 *)(basep + 632 + pos)
# asm 1: movq   632(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   632(<basep=%rcx,<pos=%rdi),>t=%r12
movq   632(%rcx,%rdi),%rbp

# qhasm: tt2d4 = t if =
# asm 1: cmove <t=int64#10,<tt2d4=int64#9
# asm 2: cmove <t=%r12,<tt2d4=%r11
cmove %rbp,%r11

# qhasm: =? u - 5
# asm 1: cmp  $5,<u=int64#5
# asm 2: cmp  $5,<u=%r8
cmp  $5,%r8

# qhasm: t = *(uint64 *)(basep + 720 + pos)
# asm 1: movq   720(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   720(<basep=%rcx,<pos=%rdi),>t=%r12
movq   720(%rcx,%rdi),%rbp

# qhasm: tz0 = t if =
cmove %rbp,%r12

# qhasm: t = *(uint64 *)(basep + 728 + pos)
# asm 1: movq   728(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   728(<basep=%rcx,<pos=%rdi),>t=%r12
movq   728(%rcx,%rdi),%rbp

# qhasm: tz1 = t if =
cmove %rbp,%r13

# qhasm: t = *(uint64 *)(basep + 736 + pos)
# asm 1: movq   736(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   736(<basep=%rcx,<pos=%rdi),>t=%r12
movq   736(%rcx,%rdi),%rbp

# qhasm: tz2 = t if =
cmove %rbp,%r14

# qhasm: t = *(uint64 *)(basep + 744 + pos)
# asm 1: movq   744(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   744(<basep=%rcx,<pos=%rdi),>t=%r12
movq   744(%rcx,%rdi),%rbp

# qhasm: tz3 = t if =
cmove %rbp,%r15

# qhasm: t = *(uint64 *)(basep + 752 + pos)
# asm 1: movq   752(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   752(<basep=%rcx,<pos=%rdi),>t=%r12
movq   752(%rcx,%rdi),%rbp

# qhasm: tz4 = t if =
cmove %rbp,%rbx

# qhasm: t = *(uint64 *)(basep + 760 + pos)
# asm 1: movq   760(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   760(<basep=%rcx,<pos=%rdi),>t=%r12
movq   760(%rcx,%rdi),%rbp

# qhasm: tt2d0 = t if =
# asm 1: cmove <t=int64#10,<tt2d0=int64#2
# asm 2: cmove <t=%r12,<tt2d0=%rsi
cmove %rbp,%rsi

# qhasm: t = *(uint64 *)(basep + 768 + pos)
# asm 1: movq   768(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   768(<basep=%rcx,<pos=%rdi),>t=%r12
movq   768(%rcx,%rdi),%rbp

# qhasm: tt2d1 = t if =
# asm 1: cmove <t=int64#10,<tt2d1=int64#6
# asm 2: cmove <t=%r12,<tt2d1=%r9
cmove %rbp,%r9

# qhasm: t = *(uint64 *)(basep + 776 + pos)
# asm 1: movq   776(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   776(<basep=%rcx,<pos=%rdi),>t=%r12
movq   776(%rcx,%rdi),%rbp

# qhasm: tt2d2 = t if =
# asm 1: cmove <t=int64#10,<tt2d2=int64#7
# asm 2: cmove <t=%r12,<tt2d2=%rax
cmove %rbp,%rax

# qhasm: t = *(uint64 *)(basep + 784 + pos)
# asm 1: movq   784(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   784(<basep=%rcx,<pos=%rdi),>t=%r12
movq   784(%rcx,%rdi),%rbp

# qhasm: tt2d3 = t if =
# asm 1: cmove <t=int64#10,<tt2d3=int64#8
# asm 2: cmove <t=%r12,<tt2d3=%r10
cmove %rbp,%r10

# qhasm: t = *(uint64 *)(basep + 792 + pos)
# asm 1: movq   792(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   792(<basep=%rcx,<pos=%rdi),>t=%r12
movq   792(%rcx,%rdi),%rbp

# qhasm: tt2d4 = t if =
# asm 1: cmove <t=int64#10,<tt2d4=int64#9
# asm 2: cmove <t=%r12,<tt2d4=%r11
cmove %rbp,%r11

# qhasm: =? u - 6
# asm 1: cmp  $6,<u=int64#5
# asm 2: cmp  $6,<u=%r8
cmp  $6,%r8

# qhasm: t = *(uint64 *)(basep + 880 + pos)
# asm 1: movq   880(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   880(<basep=%rcx,<pos=%rdi),>t=%r12
movq   880(%rcx,%rdi),%rbp

# qhasm: tz0 = t if =
cmove %rbp,%r12

# qhasm: t = *(uint64 *)(basep + 888 + pos)
# asm 1: movq   888(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   888(<basep=%rcx,<pos=%rdi),>t=%r12
movq   888(%rcx,%rdi),%rbp

# qhasm: tz1 = t if =
cmove %rbp,%r13

# qhasm: t = *(uint64 *)(basep + 896 + pos)
# asm 1: movq   896(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   896(<basep=%rcx,<pos=%rdi),>t=%r12
movq   896(%rcx,%rdi),%rbp

# qhasm: tz1 = t if =
cmove %rbp,%r14

# qhasm: t = *(uint64 *)(basep + 904 + pos)
# asm 1: movq   904(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   904(<basep=%rcx,<pos=%rdi),>t=%r12
movq   904(%rcx,%rdi),%rbp

# qhasm: tz2 = t if =
cmove %rbp,%r15

# qhasm: t = *(uint64 *)(basep + 912 + pos)
# asm 1: movq   912(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   912(<basep=%rcx,<pos=%rdi),>t=%r12
movq   912(%rcx,%rdi),%rbp

# qhasm: tz3 = t if =
cmove %rbp,%rbx

# qhasm: t = *(uint64 *)(basep + 920 + pos)
# asm 1: movq   920(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   920(<basep=%rcx,<pos=%rdi),>t=%r12
movq   920(%rcx,%rdi),%rbp

# qhasm: tt2d0 = t if =
# asm 1: cmove <t=int64#10,<tt2d0=int64#2
# asm 2: cmove <t=%r12,<tt2d0=%rsi
cmove %rbp,%rsi

# qhasm: t = *(uint64 *)(basep + 928 + pos)
# asm 1: movq   928(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   928(<basep=%rcx,<pos=%rdi),>t=%r12
movq   928(%rcx,%rdi),%rbp

# qhasm: tt2d1 = t if =
# asm 1: cmove <t=int64#10,<tt2d1=int64#6
# asm 2: cmove <t=%r12,<tt2d1=%r9
cmove %rbp,%r9

# qhasm: t = *(uint64 *)(basep + 936 + pos)
# asm 1: movq   936(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   936(<basep=%rcx,<pos=%rdi),>t=%r12
movq   936(%rcx,%rdi),%rbp

# qhasm: tt2d2 = t if =
# asm 1: cmove <t=int64#10,<tt2d2=int64#7
# asm 2: cmove <t=%r12,<tt2d2=%rax
cmove %rbp,%rax

# qhasm: t = *(uint64 *)(basep + 944 + pos)
# asm 1: movq   944(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   944(<basep=%rcx,<pos=%rdi),>t=%r12
movq   944(%rcx,%rdi),%rbp

# qhasm: tt2d3 = t if =
# asm 1: cmove <t=int64#10,<tt2d3=int64#8
# asm 2: cmove <t=%r12,<tt2d3=%r10
cmove %rbp,%r10

# qhasm: t = *(uint64 *)(basep + 952 + pos)
# asm 1: movq   952(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   952(<basep=%rcx,<pos=%rdi),>t=%r12
movq   952(%rcx,%rdi),%rbp

# qhasm: tt2d4 = t if =
# asm 1: cmove <t=int64#10,<tt2d4=int64#9
# asm 2: cmove <t=%r12,<tt2d4=%r11
cmove %rbp,%r11

# qhasm: =? u - 7
# asm 1: cmp  $7,<u=int64#5
# asm 2: cmp  $7,<u=%r8
cmp  $7,%r8

# qhasm: t = *(uint64 *)(basep + 1040 + pos)
# asm 1: movq   1040(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   1040(<basep=%rcx,<pos=%rdi),>t=%r12
movq   1040(%rcx,%rdi),%rbp

# qhasm: tz0 = t if =
cmove %rbp,%r12

# qhasm: t = *(uint64 *)(basep + 1048 + pos)
# asm 1: movq   1048(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   1048(<basep=%rcx,<pos=%rdi),>t=%r12
movq   1048(%rcx,%rdi),%rbp

# qhasm: tz1 = t if =
cmove %rbp,%r13

# qhasm: t = *(uint64 *)(basep + 1056 + pos)
# asm 1: movq   1056(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   1056(<basep=%rcx,<pos=%rdi),>t=%r12
movq   1056(%rcx,%rdi),%rbp

# qhasm: tz2 = t if =
cmove %rbp,%r14

# qhasm: t = *(uint64 *)(basep + 1064 + pos)
# asm 1: movq   1064(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   1064(<basep=%rcx,<pos=%rdi),>t=%r12
movq   1064(%rcx,%rdi),%rbp

# qhasm: tz3 = t if =
cmove %rbp,%r15

# qhasm: t = *(uint64 *)(basep + 1072 + pos)
# asm 1: movq   1072(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   1072(<basep=%rcx,<pos=%rdi),>t=%r12
movq   1072(%rcx,%rdi),%rbp

# qhasm: tz4 = t if =
cmove %rbp,%rbx

# qhasm: t = *(uint64 *)(basep + 1080 + pos)
# asm 1: movq   1080(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   1080(<basep=%rcx,<pos=%rdi),>t=%r12
movq   1080(%rcx,%rdi),%rbp

# qhasm: tt2d0 = t if =
# asm 1: cmove <t=int64#10,<tt2d0=int64#2
# asm 2: cmove <t=%r12,<tt2d0=%rsi
cmove %rbp,%rsi

# qhasm: t = *(uint64 *)(basep + 1088 + pos)
# asm 1: movq   1088(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   1088(<basep=%rcx,<pos=%rdi),>t=%r12
movq   1088(%rcx,%rdi),%rbp

# qhasm: tt2d1 = t if =
# asm 1: cmove <t=int64#10,<tt2d1=int64#6
# asm 2: cmove <t=%r12,<tt2d1=%r9
cmove %rbp,%r9

# qhasm: t = *(uint64 *)(basep + 1096 + pos)
# asm 1: movq   1096(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   1096(<basep=%rcx,<pos=%rdi),>t=%r12
movq   1096(%rcx,%rdi),%rbp

# qhasm: tt2d2 = t if =
# asm 1: cmove <t=int64#10,<tt2d2=int64#7
# asm 2: cmove <t=%r12,<tt2d2=%rax
cmove %rbp,%rax

# qhasm: t = *(uint64 *)(basep + 1104 + pos)
# asm 1: movq   1104(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   1104(<basep=%rcx,<pos=%rdi),>t=%r12
movq   1104(%rcx,%rdi),%rbp

# qhasm: tt2d3 = t if =
# asm 1: cmove <t=int64#10,<tt2d3=int64#8
# asm 2: cmove <t=%r12,<tt2d3=%r10
cmove %rbp,%r10

# qhasm: t = *(uint64 *)(basep + 1112 + pos)
# asm 1: movq   1112(<basep=int64#4,<pos=int64#1),>t=int64#10
# asm 2: movq   1112(<basep=%rcx,<pos=%rdi),>t=%r12
movq   1112(%rcx,%rdi),%rbp

# qhasm: tt2d4 = t if =
# asm 1: cmove <t=int64#10,<tt2d4=int64#9
# asm 2: cmove <t=%r12,<tt2d4=%r11
cmove %rbp,%r11

# qhasm: =? u - 8
# asm 1: cmp  $8,<u=int64#5
# asm 2: cmp  $8,<u=%r8
cmp  $8,%r8

# qhasm: t = *(uint64 *)(basep + 1200 + pos)
# asm 1: movq   1200(<basep=int64#4,<pos=int64#1),>t=int64#5
# asm 2: movq   1200(<basep=%rcx,<pos=%rdi),>t=%r8
movq   1200(%rcx,%rdi),%r8

# qhasm: tz0 = t if =
cmove %r8,%r12

# qhasm: t = *(uint64 *)(basep + 1208 + pos)
# asm 1: movq   1208(<basep=int64#4,<pos=int64#1),>t=int64#5
# asm 2: movq   1208(<basep=%rcx,<pos=%rdi),>t=%r8
movq   1208(%rcx,%rdi),%r8

# qhasm: tz1 = t if =
cmove %r8,%r13

# qhasm: t = *(uint64 *)(basep + 1216 + pos)
# asm 1: movq   1216(<basep=int64#4,<pos=int64#1),>t=int64#5
# asm 2: movq   1216(<basep=%rcx,<pos=%rdi),>t=%r8
movq   1216(%rcx,%rdi),%r8

# qhasm: tz2 = t if =
cmove %r8,%r14

# qhasm: t = *(uint64 *)(basep + 1224 + pos)
# asm 1: movq   1224(<basep=int64#4,<pos=int64#1),>t=int64#5
# asm 2: movq   1224(<basep=%rcx,<pos=%rdi),>t=%r8
movq   1224(%rcx,%rdi),%r8

# qhasm: tz3 = t if =
cmove %r8,%r15

# qhasm: t = *(uint64 *)(basep + 1232 + pos)
# asm 1: movq   1232(<basep=int64#4,<pos=int64#1),>t=int64#5
# asm 2: movq   1232(<basep=%rcx,<pos=%rdi),>t=%r8
movq   1232(%rcx,%rdi),%r8

# qhasm: tz4 = t if =
cmove %r8,%rbx

# qhasm: t = *(uint64 *)(basep + 1240 + pos)
# asm 1: movq   1240(<basep=int64#4,<pos=int64#1),>t=int64#5
# asm 2: movq   1240(<basep=%rcx,<pos=%rdi),>t=%r8
movq   1240(%rcx,%rdi),%r8

# qhasm: tt2d0 = t if =
# asm 1: cmove <t=int64#5,<tt2d0=int64#2
# asm 2: cmove <t=%r8,<tt2d0=%rsi
cmove %r8,%rsi

# qhasm: t = *(uint64 *)(basep + 1248 + pos)
# asm 1: movq   1248(<basep=int64#4,<pos=int64#1),>t=int64#5
# asm 2: movq   1248(<basep=%rcx,<pos=%rdi),>t=%r8
movq   1248(%rcx,%rdi),%r8

# qhasm: tt2d1 = t if =
# asm 1: cmove <t=int64#5,<tt2d1=int64#6
# asm 2: cmove <t=%r8,<tt2d1=%r9
cmove %r8,%r9

# qhasm: t = *(uint64 *)(basep + 1256 + pos)
# asm 1: movq   1256(<basep=int64#4,<pos=int64#1),>t=int64#5
# asm 2: movq   1256(<basep=%rcx,<pos=%rdi),>t=%r8
movq   1256(%rcx,%rdi),%r8

# qhasm: tt2d2 = t if =
# asm 1: cmove <t=int64#5,<tt2d2=int64#7
# asm 2: cmove <t=%r8,<tt2d2=%rax
cmove %r8,%rax

# qhasm: t = *(uint64 *)(basep + 1264 + pos)
# asm 1: movq   1264(<basep=int64#4,<pos=int64#1),>t=int64#5
# asm 2: movq   1264(<basep=%rcx,<pos=%rdi),>t=%r8
movq   1264(%rcx,%rdi),%r8

# qhasm: tt2d3 = t if =
# asm 1: cmove <t=int64#5,<tt2d3=int64#8
# asm 2: cmove <t=%r8,<tt2d3=%r10
cmove %r8,%r10

# qhasm: t = *(uint64 *)(basep + 1272 + pos)
# asm 1: movq   1272(<basep=int64#4,<pos=int64#1),>t=int64#1
# asm 2: movq   1272(<basep=%rcx,<pos=%rdi),>t=%rdi
movq   1272(%rcx,%rdi),%rdi

# qhasm: tt2d4 = t if =
# asm 1: cmove <t=int64#1,<tt2d4=int64#9
# asm 2: cmove <t=%rdi,<tt2d4=%r11
cmove %rdi,%r11

# qhasm: tp = tp_stack
# asm 1: movq <tp_stack=stack64#8,>tp=int64#15
# asm 2: movq <tp_stack=56(%rsp),>tp=%rbp
movq 56(%rsp),%rbp

# qhasm: *(uint64 *)(tp + 80) = tz0
# asm 1: movq   <tysubx0=int64#2,0(<tp=int64#15)
# asm 2: movq   <tysubx0=%rsi,0(<tp=%rbp)
movq   %r12,80(%rbp)

# qhasm: *(uint64 *)(tp + 88) = tz0
# asm 1: movq   <tysubx0=int64#2,0(<tp=int64#15)
# asm 2: movq   <tysubx0=%rsi,0(<tp=%rbp)
movq   %r13,88(%rbp)

# qhasm: *(uint64 *)(tp + 96) = tz0
# asm 1: movq   <tysubx0=int64#2,0(<tp=int64#15)
# asm 2: movq   <tysubx0=%rsi,0(<tp=%rbp)
movq   %r14,96(%rbp)

# qhasm: *(uint64 *)(tp + 104) = tz0
# asm 1: movq   <tysubx0=int64#2,0(<tp=int64#15)
# asm 2: movq   <tysubx0=%rsi,0(<tp=%rbp)
movq   %r15,104(%rbp)

# qhasm: *(uint64 *)(tp + 112) = tz0
# asm 1: movq   <tysubx0=int64#2,0(<tp=int64#15)
# asm 2: movq   <tysubx0=%rsi,0(<tp=%rbp)
movq   %rbx,112(%rbp)

# qhasm: tt0 = *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P0
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_2P0,>tt0=int64#1
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_2P0,>tt0=%rdi
movq crypto_sign_ed25519_amd64_51_30k_batch_2P0(%rip),%rdi

# qhasm: tt1 = *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,>tt1=int64#4
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,>tt1=%rcx
movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234(%rip),%rcx

# qhasm: tt2 = *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,>tt2=int64#5
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,>tt2=%r8
movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234(%rip),%r8

# qhasm: tt3 = *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,>tt3=int64#10
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,>tt3=%r12
movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234(%rip),%r12

# qhasm: tt4 = *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,>tt4=int64#11
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234,>tt4=%r13
movq crypto_sign_ed25519_amd64_51_30k_batch_2P1234(%rip),%r13

# qhasm: tt0 -= tt2d0
# asm 1: sub  <tt2d0=int64#2,<tt0=int64#1
# asm 2: sub  <tt2d0=%rsi,<tt0=%rdi
sub  %rsi,%rdi

# qhasm: tt1 -= tt2d1
# asm 1: sub  <tt2d1=int64#6,<tt1=int64#4
# asm 2: sub  <tt2d1=%r9,<tt1=%rcx
sub  %r9,%rcx

# qhasm: tt2 -= tt2d2
# asm 1: sub  <tt2d2=int64#7,<tt2=int64#5
# asm 2: sub  <tt2d2=%rax,<tt2=%r8
sub  %rax,%r8

# qhasm: tt3 -= tt2d3
# asm 1: sub  <tt2d3=int64#8,<tt3=int64#10
# asm 2: sub  <tt2d3=%r10,<tt3=%r12
sub  %r10,%r12

# qhasm: tt4 -= tt2d4
# asm 1: sub  <tt2d4=int64#9,<tt4=int64#11
# asm 2: sub  <tt2d4=%r11,<tt4=%r13
sub  %r11,%r13

# qhasm: signed<? b - 0
# asm 1: cmp  $0,<b=int64#3
# asm 2: cmp  $0,<b=%rdx
cmp  $0,%rdx

# qhasm: tt2d0 = tt0 if signed<
# asm 1: cmovl <tt0=int64#1,<tt2d0=int64#2
# asm 2: cmovl <tt0=%rdi,<tt2d0=%rsi
cmovl %rdi,%rsi

# qhasm: tt2d1 = tt1 if signed<
# asm 1: cmovl <tt1=int64#4,<tt2d1=int64#6
# asm 2: cmovl <tt1=%rcx,<tt2d1=%r9
cmovl %rcx,%r9

# qhasm: tt2d2 = tt2 if signed<
# asm 1: cmovl <tt2=int64#5,<tt2d2=int64#7
# asm 2: cmovl <tt2=%r8,<tt2d2=%rax
cmovl %r8,%rax

# qhasm: tt2d3 = tt3 if signed<
# asm 1: cmovl <tt3=int64#10,<tt2d3=int64#8
# asm 2: cmovl <tt3=%r12,<tt2d3=%r10
cmovl %r12,%r10

# qhasm: tt2d4 = tt4 if signed<
# asm 1: cmovl <tt4=int64#11,<tt2d4=int64#9
# asm 2: cmovl <tt4=%r13,<tt2d4=%r11
cmovl %r13,%r11

# qhasm: *(uint64 *)(tp + 80) = tt2d0
# asm 1: movq   <tt2d0=int64#2,80(<tp=int64#15)
# asm 2: movq   <tt2d0=%rsi,80(<tp=%rbp)
movq   %rsi,120(%rbp)

# qhasm: *(uint64 *)(tp + 88) = tt2d1
# asm 1: movq   <tt2d1=int64#6,88(<tp=int64#15)
# asm 2: movq   <tt2d1=%r9,88(<tp=%rbp)
movq   %r9,128(%rbp)

# qhasm: *(uint64 *)(tp + 96) = tt2d2
# asm 1: movq   <tt2d2=int64#7,96(<tp=int64#15)
# asm 2: movq   <tt2d2=%rax,96(<tp=%rbp)
movq   %rax,136(%rbp)

# qhasm: *(uint64 *)(tp + 104) = tt2d3
# asm 1: movq   <tt2d3=int64#8,104(<tp=int64#15)
# asm 2: movq   <tt2d3=%r10,104(<tp=%rbp)
movq   %r10,144(%rbp)

# qhasm: *(uint64 *)(tp + 112) = tt2d4
# asm 1: movq   <tt2d4=int64#9,112(<tp=int64#15)
# asm 2: movq   <tt2d4=%r11,112(<tp=%rbp)
movq   %r11,152(%rbp)

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
