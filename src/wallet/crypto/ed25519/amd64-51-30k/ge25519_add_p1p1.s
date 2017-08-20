
# qhasm: int64 rp

# qhasm: int64 pp

# qhasm: int64 qp

# qhasm: input rp

# qhasm: input pp

# qhasm: input qp

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

# qhasm: int64 t10

# qhasm: int64 t11

# qhasm: int64 t12

# qhasm: int64 t13

# qhasm: int64 t14

# qhasm: stack64 t10_stack

# qhasm: stack64 t11_stack

# qhasm: stack64 t12_stack

# qhasm: stack64 t13_stack

# qhasm: stack64 t14_stack

# qhasm: int64 t20

# qhasm: int64 t21

# qhasm: int64 t22

# qhasm: int64 t23

# qhasm: int64 t24

# qhasm: stack64 t20_stack

# qhasm: stack64 t21_stack

# qhasm: stack64 t22_stack

# qhasm: stack64 t23_stack

# qhasm: stack64 t24_stack

# qhasm: int64 rx0

# qhasm: int64 rx1

# qhasm: int64 rx2

# qhasm: int64 rx3

# qhasm: int64 rx4

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

# qhasm: int64 x0

# qhasm: int64 x1

# qhasm: int64 x2

# qhasm: int64 x3

# qhasm: int64 x4

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

# qhasm: enter crypto_sign_ed25519_amd64_51_30k_batch_ge25519_add_p1p1
.text
.p2align 5
.globl _crypto_sign_ed25519_amd64_51_30k_batch_ge25519_add_p1p1
.globl crypto_sign_ed25519_amd64_51_30k_batch_ge25519_add_p1p1
_crypto_sign_ed25519_amd64_51_30k_batch_ge25519_add_p1p1:
crypto_sign_ed25519_amd64_51_30k_batch_ge25519_add_p1p1:
mov %rsp,%r11
and $31,%r11
add $256,%r11
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

# qhasm: qp = qp
# asm 1: mov  <qp=int64#3,>qp=int64#4
# asm 2: mov  <qp=%rdx,>qp=%rcx
mov  %rdx,%rcx

# qhasm: a0 = *(uint64 *)(pp + 40)
# asm 1: movq   40(<pp=int64#2),>a0=int64#3
# asm 2: movq   40(<pp=%rsi),>a0=%rdx
movq   40(%rsi),%rdx

# qhasm: a1 = *(uint64 *)(pp + 48)
# asm 1: movq   48(<pp=int64#2),>a1=int64#5
# asm 2: movq   48(<pp=%rsi),>a1=%r8
movq   48(%rsi),%r8

# qhasm: a2 = *(uint64 *)(pp + 56)
# asm 1: movq   56(<pp=int64#2),>a2=int64#6
# asm 2: movq   56(<pp=%rsi),>a2=%r9
movq   56(%rsi),%r9

# qhasm: a3 = *(uint64 *)(pp + 64)
# asm 1: movq   64(<pp=int64#2),>a3=int64#7
# asm 2: movq   64(<pp=%rsi),>a3=%rax
movq   64(%rsi),%rax

# qhasm: a4 = *(uint64 *)(pp + 72)
# asm 1: movq   72(<pp=int64#2),>a4=int64#8
# asm 2: movq   72(<pp=%rsi),>a4=%r10
movq   72(%rsi),%r10

# qhasm: b0 = a0
# asm 1: mov  <a0=int64#3,>b0=int64#9
# asm 2: mov  <a0=%rdx,>b0=%r11
mov  %rdx,%r11

# qhasm: a0 += *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_2P0
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P0,<a0=int64#3
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P0,<a0=%rdx
add  crypto_sign_ed25519_amd64_51_30k_batch_2P0,%rdx

# qhasm: x0 = *(uint64 *)(pp + 0)
# asm 1: movq   0(<pp=int64#2),>x0=int64#10
# asm 2: movq   0(<pp=%rsi),>x0=%r12
movq   0(%rsi),%r12

# qhasm: b0 += x0
# asm 1: add  <x0=int64#10,<b0=int64#9
# asm 2: add  <x0=%r12,<b0=%r11
add  %r12,%r11

# qhasm: a0 -= x0
# asm 1: sub  <x0=int64#10,<a0=int64#3
# asm 2: sub  <x0=%r12,<a0=%rdx
sub  %r12,%rdx

# qhasm: b1 = a1
# asm 1: mov  <a1=int64#5,>b1=int64#10
# asm 2: mov  <a1=%r8,>b1=%r12
mov  %r8,%r12

# qhasm: a1 += *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<a1=int64#5
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<a1=%r8
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r8

# qhasm: x1 = *(uint64 *)(pp + 8)
# asm 1: movq   8(<pp=int64#2),>x1=int64#11
# asm 2: movq   8(<pp=%rsi),>x1=%r13
movq   8(%rsi),%r13

# qhasm: b1 += x1
# asm 1: add  <x1=int64#11,<b1=int64#10
# asm 2: add  <x1=%r13,<b1=%r12
add  %r13,%r12

# qhasm: a1 -= x1
# asm 1: sub  <x1=int64#11,<a1=int64#5
# asm 2: sub  <x1=%r13,<a1=%r8
sub  %r13,%r8

# qhasm: b2 = a2
# asm 1: mov  <a2=int64#6,>b2=int64#11
# asm 2: mov  <a2=%r9,>b2=%r13
mov  %r9,%r13

# qhasm: a2 += *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<a2=int64#6
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<a2=%r9
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r9

# qhasm: x2 = *(uint64 *)(pp + 16)
# asm 1: movq   16(<pp=int64#2),>x2=int64#12
# asm 2: movq   16(<pp=%rsi),>x2=%r14
movq   16(%rsi),%r14

# qhasm: b2 += x2
# asm 1: add  <x2=int64#12,<b2=int64#11
# asm 2: add  <x2=%r14,<b2=%r13
add  %r14,%r13

# qhasm: a2 -= x2
# asm 1: sub  <x2=int64#12,<a2=int64#6
# asm 2: sub  <x2=%r14,<a2=%r9
sub  %r14,%r9

# qhasm: b3 = a3
# asm 1: mov  <a3=int64#7,>b3=int64#12
# asm 2: mov  <a3=%rax,>b3=%r14
mov  %rax,%r14

# qhasm: a3 += *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<a3=int64#7
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<a3=%rax
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%rax

# qhasm: x3 = *(uint64 *)(pp + 24)
# asm 1: movq   24(<pp=int64#2),>x3=int64#13
# asm 2: movq   24(<pp=%rsi),>x3=%r15
movq   24(%rsi),%r15

# qhasm: b3 += x3
# asm 1: add  <x3=int64#13,<b3=int64#12
# asm 2: add  <x3=%r15,<b3=%r14
add  %r15,%r14

# qhasm: a3 -= x3
# asm 1: sub  <x3=int64#13,<a3=int64#7
# asm 2: sub  <x3=%r15,<a3=%rax
sub  %r15,%rax

# qhasm: b4 = a4
# asm 1: mov  <a4=int64#8,>b4=int64#13
# asm 2: mov  <a4=%r10,>b4=%r15
mov  %r10,%r15

# qhasm: a4 += *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<a4=int64#8
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<a4=%r10
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r10

# qhasm: x4 = *(uint64 *)(pp + 32)
# asm 1: movq   32(<pp=int64#2),>x4=int64#14
# asm 2: movq   32(<pp=%rsi),>x4=%rbx
movq   32(%rsi),%rbx

# qhasm: b4 += x4
# asm 1: add  <x4=int64#14,<b4=int64#13
# asm 2: add  <x4=%rbx,<b4=%r15
add  %rbx,%r15

# qhasm: a4 -= x4
# asm 1: sub  <x4=int64#14,<a4=int64#8
# asm 2: sub  <x4=%rbx,<a4=%r10
sub  %rbx,%r10

# qhasm: a0_stack = a0
# asm 1: movq <a0=int64#3,>a0_stack=stack64#8
# asm 2: movq <a0=%rdx,>a0_stack=56(%rsp)
movq %rdx,56(%rsp)

# qhasm: a1_stack = a1
# asm 1: movq <a1=int64#5,>a1_stack=stack64#9
# asm 2: movq <a1=%r8,>a1_stack=64(%rsp)
movq %r8,64(%rsp)

# qhasm: a2_stack = a2
# asm 1: movq <a2=int64#6,>a2_stack=stack64#10
# asm 2: movq <a2=%r9,>a2_stack=72(%rsp)
movq %r9,72(%rsp)

# qhasm: a3_stack = a3
# asm 1: movq <a3=int64#7,>a3_stack=stack64#11
# asm 2: movq <a3=%rax,>a3_stack=80(%rsp)
movq %rax,80(%rsp)

# qhasm: a4_stack = a4
# asm 1: movq <a4=int64#8,>a4_stack=stack64#12
# asm 2: movq <a4=%r10,>a4_stack=88(%rsp)
movq %r10,88(%rsp)

# qhasm: b0_stack = b0
# asm 1: movq <b0=int64#9,>b0_stack=stack64#13
# asm 2: movq <b0=%r11,>b0_stack=96(%rsp)
movq %r11,96(%rsp)

# qhasm: b1_stack = b1
# asm 1: movq <b1=int64#10,>b1_stack=stack64#14
# asm 2: movq <b1=%r12,>b1_stack=104(%rsp)
movq %r12,104(%rsp)

# qhasm: b2_stack = b2
# asm 1: movq <b2=int64#11,>b2_stack=stack64#15
# asm 2: movq <b2=%r13,>b2_stack=112(%rsp)
movq %r13,112(%rsp)

# qhasm: b3_stack = b3
# asm 1: movq <b3=int64#12,>b3_stack=stack64#16
# asm 2: movq <b3=%r14,>b3_stack=120(%rsp)
movq %r14,120(%rsp)

# qhasm: b4_stack = b4
# asm 1: movq <b4=int64#13,>b4_stack=stack64#17
# asm 2: movq <b4=%r15,>b4_stack=128(%rsp)
movq %r15,128(%rsp)

# qhasm: t10 = *(uint64 *)(qp + 40)
# asm 1: movq   40(<qp=int64#4),>t10=int64#3
# asm 2: movq   40(<qp=%rcx),>t10=%rdx
movq   40(%rcx),%rdx

# qhasm: t11 = *(uint64 *)(qp + 48)
# asm 1: movq   48(<qp=int64#4),>t11=int64#5
# asm 2: movq   48(<qp=%rcx),>t11=%r8
movq   48(%rcx),%r8

# qhasm: t12 = *(uint64 *)(qp + 56)
# asm 1: movq   56(<qp=int64#4),>t12=int64#6
# asm 2: movq   56(<qp=%rcx),>t12=%r9
movq   56(%rcx),%r9

# qhasm: t13 = *(uint64 *)(qp + 64)
# asm 1: movq   64(<qp=int64#4),>t13=int64#7
# asm 2: movq   64(<qp=%rcx),>t13=%rax
movq   64(%rcx),%rax

# qhasm: t14 = *(uint64 *)(qp + 72)
# asm 1: movq   72(<qp=int64#4),>t14=int64#8
# asm 2: movq   72(<qp=%rcx),>t14=%r10
movq   72(%rcx),%r10

# qhasm: t20 = t10
# asm 1: mov  <t10=int64#3,>t20=int64#9
# asm 2: mov  <t10=%rdx,>t20=%r11
mov  %rdx,%r11

# qhasm: t10 += *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_2P0
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P0,<t10=int64#3
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P0,<t10=%rdx
add  crypto_sign_ed25519_amd64_51_30k_batch_2P0,%rdx

# qhasm: rx0 = *(uint64 *) (qp + 0)
# asm 1: movq   0(<qp=int64#4),>rx0=int64#10
# asm 2: movq   0(<qp=%rcx),>rx0=%r12
movq   0(%rcx),%r12

# qhasm: t20 += rx0
# asm 1: add  <rx0=int64#10,<t20=int64#9
# asm 2: add  <rx0=%r12,<t20=%r11
add  %r12,%r11

# qhasm: t10 -= rx0
# asm 1: sub  <rx0=int64#10,<t10=int64#3
# asm 2: sub  <rx0=%r12,<t10=%rdx
sub  %r12,%rdx

# qhasm: t21 = t11
# asm 1: mov  <t11=int64#5,>t21=int64#10
# asm 2: mov  <t11=%r8,>t21=%r12
mov  %r8,%r12

# qhasm: t11 += *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<t11=int64#5
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<t11=%r8
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r8

# qhasm: rx1 = *(uint64 *) (qp + 8)
# asm 1: movq   8(<qp=int64#4),>rx1=int64#11
# asm 2: movq   8(<qp=%rcx),>rx1=%r13
movq   8(%rcx),%r13

# qhasm: t21 += rx1
# asm 1: add  <rx1=int64#11,<t21=int64#10
# asm 2: add  <rx1=%r13,<t21=%r12
add  %r13,%r12

# qhasm: t11 -= rx1
# asm 1: sub  <rx1=int64#11,<t11=int64#5
# asm 2: sub  <rx1=%r13,<t11=%r8
sub  %r13,%r8

# qhasm: t22 = t12
# asm 1: mov  <t12=int64#6,>t22=int64#11
# asm 2: mov  <t12=%r9,>t22=%r13
mov  %r9,%r13

# qhasm: t12 += *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<t12=int64#6
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<t12=%r9
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r9

# qhasm: rx2 = *(uint64 *) (qp + 16)
# asm 1: movq   16(<qp=int64#4),>rx2=int64#12
# asm 2: movq   16(<qp=%rcx),>rx2=%r14
movq   16(%rcx),%r14

# qhasm: t22 += rx2
# asm 1: add  <rx2=int64#12,<t22=int64#11
# asm 2: add  <rx2=%r14,<t22=%r13
add  %r14,%r13

# qhasm: t12 -= rx2
# asm 1: sub  <rx2=int64#12,<t12=int64#6
# asm 2: sub  <rx2=%r14,<t12=%r9
sub  %r14,%r9

# qhasm: t23 = t13
# asm 1: mov  <t13=int64#7,>t23=int64#12
# asm 2: mov  <t13=%rax,>t23=%r14
mov  %rax,%r14

# qhasm: t13 += *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<t13=int64#7
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<t13=%rax
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%rax

# qhasm: rx3 = *(uint64 *) (qp + 24)
# asm 1: movq   24(<qp=int64#4),>rx3=int64#13
# asm 2: movq   24(<qp=%rcx),>rx3=%r15
movq   24(%rcx),%r15

# qhasm: t23 += rx3
# asm 1: add  <rx3=int64#13,<t23=int64#12
# asm 2: add  <rx3=%r15,<t23=%r14
add  %r15,%r14

# qhasm: t13 -= rx3
# asm 1: sub  <rx3=int64#13,<t13=int64#7
# asm 2: sub  <rx3=%r15,<t13=%rax
sub  %r15,%rax

# qhasm: t24 = t14
# asm 1: mov  <t14=int64#8,>t24=int64#13
# asm 2: mov  <t14=%r10,>t24=%r15
mov  %r10,%r15

# qhasm: t14 += *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<t14=int64#8
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<t14=%r10
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r10

# qhasm: rx4 = *(uint64 *) (qp + 32)
# asm 1: movq   32(<qp=int64#4),>rx4=int64#14
# asm 2: movq   32(<qp=%rcx),>rx4=%rbx
movq   32(%rcx),%rbx

# qhasm: t24 += rx4
# asm 1: add  <rx4=int64#14,<t24=int64#13
# asm 2: add  <rx4=%rbx,<t24=%r15
add  %rbx,%r15

# qhasm: t14 -= rx4
# asm 1: sub  <rx4=int64#14,<t14=int64#8
# asm 2: sub  <rx4=%rbx,<t14=%r10
sub  %rbx,%r10

# qhasm: t10_stack = t10
# asm 1: movq <t10=int64#3,>t10_stack=stack64#18
# asm 2: movq <t10=%rdx,>t10_stack=136(%rsp)
movq %rdx,136(%rsp)

# qhasm: t11_stack = t11
# asm 1: movq <t11=int64#5,>t11_stack=stack64#19
# asm 2: movq <t11=%r8,>t11_stack=144(%rsp)
movq %r8,144(%rsp)

# qhasm: t12_stack = t12
# asm 1: movq <t12=int64#6,>t12_stack=stack64#20
# asm 2: movq <t12=%r9,>t12_stack=152(%rsp)
movq %r9,152(%rsp)

# qhasm: t13_stack = t13
# asm 1: movq <t13=int64#7,>t13_stack=stack64#21
# asm 2: movq <t13=%rax,>t13_stack=160(%rsp)
movq %rax,160(%rsp)

# qhasm: t14_stack = t14
# asm 1: movq <t14=int64#8,>t14_stack=stack64#22
# asm 2: movq <t14=%r10,>t14_stack=168(%rsp)
movq %r10,168(%rsp)

# qhasm: t20_stack = t20
# asm 1: movq <t20=int64#9,>t20_stack=stack64#23
# asm 2: movq <t20=%r11,>t20_stack=176(%rsp)
movq %r11,176(%rsp)

# qhasm: t21_stack = t21
# asm 1: movq <t21=int64#10,>t21_stack=stack64#24
# asm 2: movq <t21=%r12,>t21_stack=184(%rsp)
movq %r12,184(%rsp)

# qhasm: t22_stack = t22
# asm 1: movq <t22=int64#11,>t22_stack=stack64#25
# asm 2: movq <t22=%r13,>t22_stack=192(%rsp)
movq %r13,192(%rsp)

# qhasm: t23_stack = t23
# asm 1: movq <t23=int64#12,>t23_stack=stack64#26
# asm 2: movq <t23=%r14,>t23_stack=200(%rsp)
movq %r14,200(%rsp)

# qhasm: t24_stack = t24
# asm 1: movq <t24=int64#13,>t24_stack=stack64#27
# asm 2: movq <t24=%r15,>t24_stack=208(%rsp)
movq %r15,208(%rsp)

# qhasm:   mulrax = a3_stack
# asm 1: movq <a3_stack=stack64#11,>mulrax=int64#3
# asm 2: movq <a3_stack=80(%rsp),>mulrax=%rdx
movq 80(%rsp),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx319_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx319_stack=stack64#28
# asm 2: movq <mulrax=%rax,>mulx319_stack=216(%rsp)
movq %rax,216(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * t12_stack
# asm 1: mulq  <t12_stack=stack64#20
# asm 2: mulq  <t12_stack=152(%rsp)
mulq  152(%rsp)

# qhasm:   a0 = mulrax
# asm 1: mov  <mulrax=int64#7,>a0=int64#5
# asm 2: mov  <mulrax=%rax,>a0=%r8
mov  %rax,%r8

# qhasm:   mulr01 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr01=int64#6
# asm 2: mov  <mulrdx=%rdx,>mulr01=%r9
mov  %rdx,%r9

# qhasm:   mulrax = a4_stack
# asm 1: movq <a4_stack=stack64#12,>mulrax=int64#3
# asm 2: movq <a4_stack=88(%rsp),>mulrax=%rdx
movq 88(%rsp),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx419_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx419_stack=stack64#29
# asm 2: movq <mulrax=%rax,>mulx419_stack=224(%rsp)
movq %rax,224(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * t11_stack
# asm 1: mulq  <t11_stack=stack64#19
# asm 2: mulq  <t11_stack=144(%rsp)
mulq  144(%rsp)

# qhasm:   carry? a0 += mulrax
# asm 1: add  <mulrax=int64#7,<a0=int64#5
# asm 2: add  <mulrax=%rax,<a0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = a0_stack
# asm 1: movq <a0_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <a0_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t10_stack
# asm 1: mulq  <t10_stack=stack64#18
# asm 2: mulq  <t10_stack=136(%rsp)
mulq  136(%rsp)

# qhasm:   carry? a0 += mulrax
# asm 1: add  <mulrax=int64#7,<a0=int64#5
# asm 2: add  <mulrax=%rax,<a0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = a0_stack
# asm 1: movq <a0_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <a0_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t11_stack
# asm 1: mulq  <t11_stack=stack64#19
# asm 2: mulq  <t11_stack=144(%rsp)
mulq  144(%rsp)

# qhasm:   a1 = mulrax
# asm 1: mov  <mulrax=int64#7,>a1=int64#8
# asm 2: mov  <mulrax=%rax,>a1=%r10
mov  %rax,%r10

# qhasm:   mulr11 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr11=int64#9
# asm 2: mov  <mulrdx=%rdx,>mulr11=%r11
mov  %rdx,%r11

# qhasm:   mulrax = a0_stack
# asm 1: movq <a0_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <a0_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t12_stack
# asm 1: mulq  <t12_stack=stack64#20
# asm 2: mulq  <t12_stack=152(%rsp)
mulq  152(%rsp)

# qhasm:   a2 = mulrax
# asm 1: mov  <mulrax=int64#7,>a2=int64#10
# asm 2: mov  <mulrax=%rax,>a2=%r12
mov  %rax,%r12

# qhasm:   mulr21 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr21=int64#11
# asm 2: mov  <mulrdx=%rdx,>mulr21=%r13
mov  %rdx,%r13

# qhasm:   mulrax = a0_stack
# asm 1: movq <a0_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <a0_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t13_stack
# asm 1: mulq  <t13_stack=stack64#21
# asm 2: mulq  <t13_stack=160(%rsp)
mulq  160(%rsp)

# qhasm:   a3 = mulrax
# asm 1: mov  <mulrax=int64#7,>a3=int64#12
# asm 2: mov  <mulrax=%rax,>a3=%r14
mov  %rax,%r14

# qhasm:   mulr31 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr31=int64#13
# asm 2: mov  <mulrdx=%rdx,>mulr31=%r15
mov  %rdx,%r15

# qhasm:   mulrax = a0_stack
# asm 1: movq <a0_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <a0_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t14_stack
# asm 1: mulq  <t14_stack=stack64#22
# asm 2: mulq  <t14_stack=168(%rsp)
mulq  168(%rsp)

# qhasm:   a4 = mulrax
# asm 1: mov  <mulrax=int64#7,>a4=int64#14
# asm 2: mov  <mulrax=%rax,>a4=%rbx
mov  %rax,%rbx

# qhasm:   mulr41 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr41=int64#15
# asm 2: mov  <mulrdx=%rdx,>mulr41=%rbp
mov  %rdx,%rbp

# qhasm:   mulrax = a1_stack
# asm 1: movq <a1_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <a1_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t10_stack
# asm 1: mulq  <t10_stack=stack64#18
# asm 2: mulq  <t10_stack=136(%rsp)
mulq  136(%rsp)

# qhasm:   carry? a1 += mulrax
# asm 1: add  <mulrax=int64#7,<a1=int64#8
# asm 2: add  <mulrax=%rax,<a1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = a1_stack
# asm 1: movq <a1_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <a1_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t11_stack
# asm 1: mulq  <t11_stack=stack64#19
# asm 2: mulq  <t11_stack=144(%rsp)
mulq  144(%rsp)

# qhasm:   carry? a2 += mulrax
# asm 1: add  <mulrax=int64#7,<a2=int64#10
# asm 2: add  <mulrax=%rax,<a2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = a1_stack
# asm 1: movq <a1_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <a1_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t12_stack
# asm 1: mulq  <t12_stack=stack64#20
# asm 2: mulq  <t12_stack=152(%rsp)
mulq  152(%rsp)

# qhasm:   carry? a3 += mulrax
# asm 1: add  <mulrax=int64#7,<a3=int64#12
# asm 2: add  <mulrax=%rax,<a3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulrax = a1_stack
# asm 1: movq <a1_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <a1_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t13_stack
# asm 1: mulq  <t13_stack=stack64#21
# asm 2: mulq  <t13_stack=160(%rsp)
mulq  160(%rsp)

# qhasm:   carry? a4 += mulrax
# asm 1: add  <mulrax=int64#7,<a4=int64#14
# asm 2: add  <mulrax=%rax,<a4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = a1_stack
# asm 1: movq <a1_stack=stack64#9,>mulrax=int64#3
# asm 2: movq <a1_stack=64(%rsp),>mulrax=%rdx
movq 64(%rsp),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t14_stack
# asm 1: mulq  <t14_stack=stack64#22
# asm 2: mulq  <t14_stack=168(%rsp)
mulq  168(%rsp)

# qhasm:   carry? a0 += mulrax
# asm 1: add  <mulrax=int64#7,<a0=int64#5
# asm 2: add  <mulrax=%rax,<a0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = a2_stack
# asm 1: movq <a2_stack=stack64#10,>mulrax=int64#7
# asm 2: movq <a2_stack=72(%rsp),>mulrax=%rax
movq 72(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t10_stack
# asm 1: mulq  <t10_stack=stack64#18
# asm 2: mulq  <t10_stack=136(%rsp)
mulq  136(%rsp)

# qhasm:   carry? a2 += mulrax
# asm 1: add  <mulrax=int64#7,<a2=int64#10
# asm 2: add  <mulrax=%rax,<a2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = a2_stack
# asm 1: movq <a2_stack=stack64#10,>mulrax=int64#7
# asm 2: movq <a2_stack=72(%rsp),>mulrax=%rax
movq 72(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t11_stack
# asm 1: mulq  <t11_stack=stack64#19
# asm 2: mulq  <t11_stack=144(%rsp)
mulq  144(%rsp)

# qhasm:   carry? a3 += mulrax
# asm 1: add  <mulrax=int64#7,<a3=int64#12
# asm 2: add  <mulrax=%rax,<a3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulrax = a2_stack
# asm 1: movq <a2_stack=stack64#10,>mulrax=int64#7
# asm 2: movq <a2_stack=72(%rsp),>mulrax=%rax
movq 72(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t12_stack
# asm 1: mulq  <t12_stack=stack64#20
# asm 2: mulq  <t12_stack=152(%rsp)
mulq  152(%rsp)

# qhasm:   carry? a4 += mulrax
# asm 1: add  <mulrax=int64#7,<a4=int64#14
# asm 2: add  <mulrax=%rax,<a4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = a2_stack
# asm 1: movq <a2_stack=stack64#10,>mulrax=int64#3
# asm 2: movq <a2_stack=72(%rsp),>mulrax=%rdx
movq 72(%rsp),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t13_stack
# asm 1: mulq  <t13_stack=stack64#21
# asm 2: mulq  <t13_stack=160(%rsp)
mulq  160(%rsp)

# qhasm:   carry? a0 += mulrax
# asm 1: add  <mulrax=int64#7,<a0=int64#5
# asm 2: add  <mulrax=%rax,<a0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = a2_stack
# asm 1: movq <a2_stack=stack64#10,>mulrax=int64#3
# asm 2: movq <a2_stack=72(%rsp),>mulrax=%rdx
movq 72(%rsp),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t14_stack
# asm 1: mulq  <t14_stack=stack64#22
# asm 2: mulq  <t14_stack=168(%rsp)
mulq  168(%rsp)

# qhasm:   carry? a1 += mulrax
# asm 1: add  <mulrax=int64#7,<a1=int64#8
# asm 2: add  <mulrax=%rax,<a1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = a3_stack
# asm 1: movq <a3_stack=stack64#11,>mulrax=int64#7
# asm 2: movq <a3_stack=80(%rsp),>mulrax=%rax
movq 80(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t10_stack
# asm 1: mulq  <t10_stack=stack64#18
# asm 2: mulq  <t10_stack=136(%rsp)
mulq  136(%rsp)

# qhasm:   carry? a3 += mulrax
# asm 1: add  <mulrax=int64#7,<a3=int64#12
# asm 2: add  <mulrax=%rax,<a3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulrax = a3_stack
# asm 1: movq <a3_stack=stack64#11,>mulrax=int64#7
# asm 2: movq <a3_stack=80(%rsp),>mulrax=%rax
movq 80(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t11_stack
# asm 1: mulq  <t11_stack=stack64#19
# asm 2: mulq  <t11_stack=144(%rsp)
mulq  144(%rsp)

# qhasm:   carry? a4 += mulrax
# asm 1: add  <mulrax=int64#7,<a4=int64#14
# asm 2: add  <mulrax=%rax,<a4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#28,>mulrax=int64#7
# asm 2: movq <mulx319_stack=216(%rsp),>mulrax=%rax
movq 216(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t13_stack
# asm 1: mulq  <t13_stack=stack64#21
# asm 2: mulq  <t13_stack=160(%rsp)
mulq  160(%rsp)

# qhasm:   carry? a1 += mulrax
# asm 1: add  <mulrax=int64#7,<a1=int64#8
# asm 2: add  <mulrax=%rax,<a1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#28,>mulrax=int64#7
# asm 2: movq <mulx319_stack=216(%rsp),>mulrax=%rax
movq 216(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t14_stack
# asm 1: mulq  <t14_stack=stack64#22
# asm 2: mulq  <t14_stack=168(%rsp)
mulq  168(%rsp)

# qhasm:   carry? a2 += mulrax
# asm 1: add  <mulrax=int64#7,<a2=int64#10
# asm 2: add  <mulrax=%rax,<a2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = a4_stack
# asm 1: movq <a4_stack=stack64#12,>mulrax=int64#7
# asm 2: movq <a4_stack=88(%rsp),>mulrax=%rax
movq 88(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t10_stack
# asm 1: mulq  <t10_stack=stack64#18
# asm 2: mulq  <t10_stack=136(%rsp)
mulq  136(%rsp)

# qhasm:   carry? a4 += mulrax
# asm 1: add  <mulrax=int64#7,<a4=int64#14
# asm 2: add  <mulrax=%rax,<a4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#29,>mulrax=int64#7
# asm 2: movq <mulx419_stack=224(%rsp),>mulrax=%rax
movq 224(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t12_stack
# asm 1: mulq  <t12_stack=stack64#20
# asm 2: mulq  <t12_stack=152(%rsp)
mulq  152(%rsp)

# qhasm:   carry? a1 += mulrax
# asm 1: add  <mulrax=int64#7,<a1=int64#8
# asm 2: add  <mulrax=%rax,<a1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#29,>mulrax=int64#7
# asm 2: movq <mulx419_stack=224(%rsp),>mulrax=%rax
movq 224(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t13_stack
# asm 1: mulq  <t13_stack=stack64#21
# asm 2: mulq  <t13_stack=160(%rsp)
mulq  160(%rsp)

# qhasm:   carry? a2 += mulrax
# asm 1: add  <mulrax=int64#7,<a2=int64#10
# asm 2: add  <mulrax=%rax,<a2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#29,>mulrax=int64#7
# asm 2: movq <mulx419_stack=224(%rsp),>mulrax=%rax
movq 224(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t14_stack
# asm 1: mulq  <t14_stack=stack64#22
# asm 2: mulq  <t14_stack=168(%rsp)
mulq  168(%rsp)

# qhasm:   carry? a3 += mulrax
# asm 1: add  <mulrax=int64#7,<a3=int64#12
# asm 2: add  <mulrax=%rax,<a3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=int64#3
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=%rdx
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,%rdx

# qhasm:   mulr01 = (mulr01.a0) << 13
# asm 1: shld $13,<a0=int64#5,<mulr01=int64#6
# asm 2: shld $13,<a0=%r8,<mulr01=%r9
shld $13,%r8,%r9

# qhasm:   a0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<a0=int64#5
# asm 2: and  <mulredmask=%rdx,<a0=%r8
and  %rdx,%r8

# qhasm:   mulr11 = (mulr11.a1) << 13
# asm 1: shld $13,<a1=int64#8,<mulr11=int64#9
# asm 2: shld $13,<a1=%r10,<mulr11=%r11
shld $13,%r10,%r11

# qhasm:   a1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<a1=int64#8
# asm 2: and  <mulredmask=%rdx,<a1=%r10
and  %rdx,%r10

# qhasm:   a1 += mulr01
# asm 1: add  <mulr01=int64#6,<a1=int64#8
# asm 2: add  <mulr01=%r9,<a1=%r10
add  %r9,%r10

# qhasm:   mulr21 = (mulr21.a2) << 13
# asm 1: shld $13,<a2=int64#10,<mulr21=int64#11
# asm 2: shld $13,<a2=%r12,<mulr21=%r13
shld $13,%r12,%r13

# qhasm:   a2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<a2=int64#10
# asm 2: and  <mulredmask=%rdx,<a2=%r12
and  %rdx,%r12

# qhasm:   a2 += mulr11
# asm 1: add  <mulr11=int64#9,<a2=int64#10
# asm 2: add  <mulr11=%r11,<a2=%r12
add  %r11,%r12

# qhasm:   mulr31 = (mulr31.a3) << 13
# asm 1: shld $13,<a3=int64#12,<mulr31=int64#13
# asm 2: shld $13,<a3=%r14,<mulr31=%r15
shld $13,%r14,%r15

# qhasm:   a3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<a3=int64#12
# asm 2: and  <mulredmask=%rdx,<a3=%r14
and  %rdx,%r14

# qhasm:   a3 += mulr21
# asm 1: add  <mulr21=int64#11,<a3=int64#12
# asm 2: add  <mulr21=%r13,<a3=%r14
add  %r13,%r14

# qhasm:   mulr41 = (mulr41.a4) << 13
# asm 1: shld $13,<a4=int64#14,<mulr41=int64#15
# asm 2: shld $13,<a4=%rbx,<mulr41=%rbp
shld $13,%rbx,%rbp

# qhasm:   a4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<a4=int64#14
# asm 2: and  <mulredmask=%rdx,<a4=%rbx
and  %rdx,%rbx

# qhasm:   a4 += mulr31
# asm 1: add  <mulr31=int64#13,<a4=int64#14
# asm 2: add  <mulr31=%r15,<a4=%rbx
add  %r15,%rbx

# qhasm:   mulr41 = mulr41 * 19
# asm 1: imulq  $19,<mulr41=int64#15,>mulr41=int64#6
# asm 2: imulq  $19,<mulr41=%rbp,>mulr41=%r9
imulq  $19,%rbp,%r9

# qhasm:   a0 += mulr41
# asm 1: add  <mulr41=int64#6,<a0=int64#5
# asm 2: add  <mulr41=%r9,<a0=%r8
add  %r9,%r8

# qhasm:   mult = a0
# asm 1: mov  <a0=int64#5,>mult=int64#6
# asm 2: mov  <a0=%r8,>mult=%r9
mov  %r8,%r9

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   mult += a1
# asm 1: add  <a1=int64#8,<mult=int64#6
# asm 2: add  <a1=%r10,<mult=%r9
add  %r10,%r9

# qhasm:   a1 = mult
# asm 1: mov  <mult=int64#6,>a1=int64#7
# asm 2: mov  <mult=%r9,>a1=%rax
mov  %r9,%rax

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   a0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<a0=int64#5
# asm 2: and  <mulredmask=%rdx,<a0=%r8
and  %rdx,%r8

# qhasm:   mult += a2
# asm 1: add  <a2=int64#10,<mult=int64#6
# asm 2: add  <a2=%r12,<mult=%r9
add  %r12,%r9

# qhasm:   a2 = mult
# asm 1: mov  <mult=int64#6,>a2=int64#8
# asm 2: mov  <mult=%r9,>a2=%r10
mov  %r9,%r10

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   a1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<a1=int64#7
# asm 2: and  <mulredmask=%rdx,<a1=%rax
and  %rdx,%rax

# qhasm:   mult += a3
# asm 1: add  <a3=int64#12,<mult=int64#6
# asm 2: add  <a3=%r14,<mult=%r9
add  %r14,%r9

# qhasm:   a3 = mult
# asm 1: mov  <mult=int64#6,>a3=int64#9
# asm 2: mov  <mult=%r9,>a3=%r11
mov  %r9,%r11

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   a2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<a2=int64#8
# asm 2: and  <mulredmask=%rdx,<a2=%r10
and  %rdx,%r10

# qhasm:   mult += a4
# asm 1: add  <a4=int64#14,<mult=int64#6
# asm 2: add  <a4=%rbx,<mult=%r9
add  %rbx,%r9

# qhasm:   a4 = mult
# asm 1: mov  <mult=int64#6,>a4=int64#10
# asm 2: mov  <mult=%r9,>a4=%r12
mov  %r9,%r12

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   a3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<a3=int64#9
# asm 2: and  <mulredmask=%rdx,<a3=%r11
and  %rdx,%r11

# qhasm:   mult *= 19
# asm 1: imulq  $19,<mult=int64#6,>mult=int64#6
# asm 2: imulq  $19,<mult=%r9,>mult=%r9
imulq  $19,%r9,%r9

# qhasm:   a0 += mult
# asm 1: add  <mult=int64#6,<a0=int64#5
# asm 2: add  <mult=%r9,<a0=%r8
add  %r9,%r8

# qhasm:   a4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<a4=int64#10
# asm 2: and  <mulredmask=%rdx,<a4=%r12
and  %rdx,%r12

# qhasm: a0_stack = a0
# asm 1: movq <a0=int64#5,>a0_stack=stack64#8
# asm 2: movq <a0=%r8,>a0_stack=56(%rsp)
movq %r8,56(%rsp)

# qhasm: a1_stack = a1
# asm 1: movq <a1=int64#7,>a1_stack=stack64#9
# asm 2: movq <a1=%rax,>a1_stack=64(%rsp)
movq %rax,64(%rsp)

# qhasm: a2_stack = a2
# asm 1: movq <a2=int64#8,>a2_stack=stack64#10
# asm 2: movq <a2=%r10,>a2_stack=72(%rsp)
movq %r10,72(%rsp)

# qhasm: a3_stack = a3
# asm 1: movq <a3=int64#9,>a3_stack=stack64#11
# asm 2: movq <a3=%r11,>a3_stack=80(%rsp)
movq %r11,80(%rsp)

# qhasm: a4_stack = a4
# asm 1: movq <a4=int64#10,>a4_stack=stack64#12
# asm 2: movq <a4=%r12,>a4_stack=88(%rsp)
movq %r12,88(%rsp)

# qhasm:   mulrax = b3_stack
# asm 1: movq <b3_stack=stack64#16,>mulrax=int64#3
# asm 2: movq <b3_stack=120(%rsp),>mulrax=%rdx
movq 120(%rsp),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx319_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx319_stack=stack64#18
# asm 2: movq <mulrax=%rax,>mulx319_stack=136(%rsp)
movq %rax,136(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * t22_stack
# asm 1: mulq  <t22_stack=stack64#25
# asm 2: mulq  <t22_stack=192(%rsp)
mulq  192(%rsp)

# qhasm:   rx0 = mulrax
# asm 1: mov  <mulrax=int64#7,>rx0=int64#5
# asm 2: mov  <mulrax=%rax,>rx0=%r8
mov  %rax,%r8

# qhasm:   mulr01 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr01=int64#6
# asm 2: mov  <mulrdx=%rdx,>mulr01=%r9
mov  %rdx,%r9

# qhasm:   mulrax = b4_stack
# asm 1: movq <b4_stack=stack64#17,>mulrax=int64#3
# asm 2: movq <b4_stack=128(%rsp),>mulrax=%rdx
movq 128(%rsp),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx419_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx419_stack=stack64#19
# asm 2: movq <mulrax=%rax,>mulx419_stack=144(%rsp)
movq %rax,144(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * t21_stack
# asm 1: mulq  <t21_stack=stack64#24
# asm 2: mulq  <t21_stack=184(%rsp)
mulq  184(%rsp)

# qhasm:   carry? rx0 += mulrax
# asm 1: add  <mulrax=int64#7,<rx0=int64#5
# asm 2: add  <mulrax=%rax,<rx0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = b0_stack
# asm 1: movq <b0_stack=stack64#13,>mulrax=int64#7
# asm 2: movq <b0_stack=96(%rsp),>mulrax=%rax
movq 96(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t20_stack
# asm 1: mulq  <t20_stack=stack64#23
# asm 2: mulq  <t20_stack=176(%rsp)
mulq  176(%rsp)

# qhasm:   carry? rx0 += mulrax
# asm 1: add  <mulrax=int64#7,<rx0=int64#5
# asm 2: add  <mulrax=%rax,<rx0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = b0_stack
# asm 1: movq <b0_stack=stack64#13,>mulrax=int64#7
# asm 2: movq <b0_stack=96(%rsp),>mulrax=%rax
movq 96(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t21_stack
# asm 1: mulq  <t21_stack=stack64#24
# asm 2: mulq  <t21_stack=184(%rsp)
mulq  184(%rsp)

# qhasm:   rx1 = mulrax
# asm 1: mov  <mulrax=int64#7,>rx1=int64#8
# asm 2: mov  <mulrax=%rax,>rx1=%r10
mov  %rax,%r10

# qhasm:   mulr11 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr11=int64#9
# asm 2: mov  <mulrdx=%rdx,>mulr11=%r11
mov  %rdx,%r11

# qhasm:   mulrax = b0_stack
# asm 1: movq <b0_stack=stack64#13,>mulrax=int64#7
# asm 2: movq <b0_stack=96(%rsp),>mulrax=%rax
movq 96(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t22_stack
# asm 1: mulq  <t22_stack=stack64#25
# asm 2: mulq  <t22_stack=192(%rsp)
mulq  192(%rsp)

# qhasm:   rx2 = mulrax
# asm 1: mov  <mulrax=int64#7,>rx2=int64#10
# asm 2: mov  <mulrax=%rax,>rx2=%r12
mov  %rax,%r12

# qhasm:   mulr21 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr21=int64#11
# asm 2: mov  <mulrdx=%rdx,>mulr21=%r13
mov  %rdx,%r13

# qhasm:   mulrax = b0_stack
# asm 1: movq <b0_stack=stack64#13,>mulrax=int64#7
# asm 2: movq <b0_stack=96(%rsp),>mulrax=%rax
movq 96(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t23_stack
# asm 1: mulq  <t23_stack=stack64#26
# asm 2: mulq  <t23_stack=200(%rsp)
mulq  200(%rsp)

# qhasm:   rx3 = mulrax
# asm 1: mov  <mulrax=int64#7,>rx3=int64#12
# asm 2: mov  <mulrax=%rax,>rx3=%r14
mov  %rax,%r14

# qhasm:   mulr31 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr31=int64#13
# asm 2: mov  <mulrdx=%rdx,>mulr31=%r15
mov  %rdx,%r15

# qhasm:   mulrax = b0_stack
# asm 1: movq <b0_stack=stack64#13,>mulrax=int64#7
# asm 2: movq <b0_stack=96(%rsp),>mulrax=%rax
movq 96(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t24_stack
# asm 1: mulq  <t24_stack=stack64#27
# asm 2: mulq  <t24_stack=208(%rsp)
mulq  208(%rsp)

# qhasm:   rx4 = mulrax
# asm 1: mov  <mulrax=int64#7,>rx4=int64#14
# asm 2: mov  <mulrax=%rax,>rx4=%rbx
mov  %rax,%rbx

# qhasm:   mulr41 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr41=int64#15
# asm 2: mov  <mulrdx=%rdx,>mulr41=%rbp
mov  %rdx,%rbp

# qhasm:   mulrax = b1_stack
# asm 1: movq <b1_stack=stack64#14,>mulrax=int64#7
# asm 2: movq <b1_stack=104(%rsp),>mulrax=%rax
movq 104(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t20_stack
# asm 1: mulq  <t20_stack=stack64#23
# asm 2: mulq  <t20_stack=176(%rsp)
mulq  176(%rsp)

# qhasm:   carry? rx1 += mulrax
# asm 1: add  <mulrax=int64#7,<rx1=int64#8
# asm 2: add  <mulrax=%rax,<rx1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = b1_stack
# asm 1: movq <b1_stack=stack64#14,>mulrax=int64#7
# asm 2: movq <b1_stack=104(%rsp),>mulrax=%rax
movq 104(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t21_stack
# asm 1: mulq  <t21_stack=stack64#24
# asm 2: mulq  <t21_stack=184(%rsp)
mulq  184(%rsp)

# qhasm:   carry? rx2 += mulrax
# asm 1: add  <mulrax=int64#7,<rx2=int64#10
# asm 2: add  <mulrax=%rax,<rx2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = b1_stack
# asm 1: movq <b1_stack=stack64#14,>mulrax=int64#7
# asm 2: movq <b1_stack=104(%rsp),>mulrax=%rax
movq 104(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t22_stack
# asm 1: mulq  <t22_stack=stack64#25
# asm 2: mulq  <t22_stack=192(%rsp)
mulq  192(%rsp)

# qhasm:   carry? rx3 += mulrax
# asm 1: add  <mulrax=int64#7,<rx3=int64#12
# asm 2: add  <mulrax=%rax,<rx3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulrax = b1_stack
# asm 1: movq <b1_stack=stack64#14,>mulrax=int64#7
# asm 2: movq <b1_stack=104(%rsp),>mulrax=%rax
movq 104(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t23_stack
# asm 1: mulq  <t23_stack=stack64#26
# asm 2: mulq  <t23_stack=200(%rsp)
mulq  200(%rsp)

# qhasm:   carry? rx4 += mulrax
# asm 1: add  <mulrax=int64#7,<rx4=int64#14
# asm 2: add  <mulrax=%rax,<rx4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = b1_stack
# asm 1: movq <b1_stack=stack64#14,>mulrax=int64#3
# asm 2: movq <b1_stack=104(%rsp),>mulrax=%rdx
movq 104(%rsp),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t24_stack
# asm 1: mulq  <t24_stack=stack64#27
# asm 2: mulq  <t24_stack=208(%rsp)
mulq  208(%rsp)

# qhasm:   carry? rx0 += mulrax
# asm 1: add  <mulrax=int64#7,<rx0=int64#5
# asm 2: add  <mulrax=%rax,<rx0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = b2_stack
# asm 1: movq <b2_stack=stack64#15,>mulrax=int64#7
# asm 2: movq <b2_stack=112(%rsp),>mulrax=%rax
movq 112(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t20_stack
# asm 1: mulq  <t20_stack=stack64#23
# asm 2: mulq  <t20_stack=176(%rsp)
mulq  176(%rsp)

# qhasm:   carry? rx2 += mulrax
# asm 1: add  <mulrax=int64#7,<rx2=int64#10
# asm 2: add  <mulrax=%rax,<rx2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = b2_stack
# asm 1: movq <b2_stack=stack64#15,>mulrax=int64#7
# asm 2: movq <b2_stack=112(%rsp),>mulrax=%rax
movq 112(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t21_stack
# asm 1: mulq  <t21_stack=stack64#24
# asm 2: mulq  <t21_stack=184(%rsp)
mulq  184(%rsp)

# qhasm:   carry? rx3 += mulrax
# asm 1: add  <mulrax=int64#7,<rx3=int64#12
# asm 2: add  <mulrax=%rax,<rx3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulrax = b2_stack
# asm 1: movq <b2_stack=stack64#15,>mulrax=int64#7
# asm 2: movq <b2_stack=112(%rsp),>mulrax=%rax
movq 112(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t22_stack
# asm 1: mulq  <t22_stack=stack64#25
# asm 2: mulq  <t22_stack=192(%rsp)
mulq  192(%rsp)

# qhasm:   carry? rx4 += mulrax
# asm 1: add  <mulrax=int64#7,<rx4=int64#14
# asm 2: add  <mulrax=%rax,<rx4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = b2_stack
# asm 1: movq <b2_stack=stack64#15,>mulrax=int64#3
# asm 2: movq <b2_stack=112(%rsp),>mulrax=%rdx
movq 112(%rsp),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t23_stack
# asm 1: mulq  <t23_stack=stack64#26
# asm 2: mulq  <t23_stack=200(%rsp)
mulq  200(%rsp)

# qhasm:   carry? rx0 += mulrax
# asm 1: add  <mulrax=int64#7,<rx0=int64#5
# asm 2: add  <mulrax=%rax,<rx0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = b2_stack
# asm 1: movq <b2_stack=stack64#15,>mulrax=int64#3
# asm 2: movq <b2_stack=112(%rsp),>mulrax=%rdx
movq 112(%rsp),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t24_stack
# asm 1: mulq  <t24_stack=stack64#27
# asm 2: mulq  <t24_stack=208(%rsp)
mulq  208(%rsp)

# qhasm:   carry? rx1 += mulrax
# asm 1: add  <mulrax=int64#7,<rx1=int64#8
# asm 2: add  <mulrax=%rax,<rx1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = b3_stack
# asm 1: movq <b3_stack=stack64#16,>mulrax=int64#7
# asm 2: movq <b3_stack=120(%rsp),>mulrax=%rax
movq 120(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t20_stack
# asm 1: mulq  <t20_stack=stack64#23
# asm 2: mulq  <t20_stack=176(%rsp)
mulq  176(%rsp)

# qhasm:   carry? rx3 += mulrax
# asm 1: add  <mulrax=int64#7,<rx3=int64#12
# asm 2: add  <mulrax=%rax,<rx3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulrax = b3_stack
# asm 1: movq <b3_stack=stack64#16,>mulrax=int64#7
# asm 2: movq <b3_stack=120(%rsp),>mulrax=%rax
movq 120(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t21_stack
# asm 1: mulq  <t21_stack=stack64#24
# asm 2: mulq  <t21_stack=184(%rsp)
mulq  184(%rsp)

# qhasm:   carry? rx4 += mulrax
# asm 1: add  <mulrax=int64#7,<rx4=int64#14
# asm 2: add  <mulrax=%rax,<rx4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#18,>mulrax=int64#7
# asm 2: movq <mulx319_stack=136(%rsp),>mulrax=%rax
movq 136(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t23_stack
# asm 1: mulq  <t23_stack=stack64#26
# asm 2: mulq  <t23_stack=200(%rsp)
mulq  200(%rsp)

# qhasm:   carry? rx1 += mulrax
# asm 1: add  <mulrax=int64#7,<rx1=int64#8
# asm 2: add  <mulrax=%rax,<rx1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#18,>mulrax=int64#7
# asm 2: movq <mulx319_stack=136(%rsp),>mulrax=%rax
movq 136(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t24_stack
# asm 1: mulq  <t24_stack=stack64#27
# asm 2: mulq  <t24_stack=208(%rsp)
mulq  208(%rsp)

# qhasm:   carry? rx2 += mulrax
# asm 1: add  <mulrax=int64#7,<rx2=int64#10
# asm 2: add  <mulrax=%rax,<rx2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = b4_stack
# asm 1: movq <b4_stack=stack64#17,>mulrax=int64#7
# asm 2: movq <b4_stack=128(%rsp),>mulrax=%rax
movq 128(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t20_stack
# asm 1: mulq  <t20_stack=stack64#23
# asm 2: mulq  <t20_stack=176(%rsp)
mulq  176(%rsp)

# qhasm:   carry? rx4 += mulrax
# asm 1: add  <mulrax=int64#7,<rx4=int64#14
# asm 2: add  <mulrax=%rax,<rx4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#19,>mulrax=int64#7
# asm 2: movq <mulx419_stack=144(%rsp),>mulrax=%rax
movq 144(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t22_stack
# asm 1: mulq  <t22_stack=stack64#25
# asm 2: mulq  <t22_stack=192(%rsp)
mulq  192(%rsp)

# qhasm:   carry? rx1 += mulrax
# asm 1: add  <mulrax=int64#7,<rx1=int64#8
# asm 2: add  <mulrax=%rax,<rx1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#19,>mulrax=int64#7
# asm 2: movq <mulx419_stack=144(%rsp),>mulrax=%rax
movq 144(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t23_stack
# asm 1: mulq  <t23_stack=stack64#26
# asm 2: mulq  <t23_stack=200(%rsp)
mulq  200(%rsp)

# qhasm:   carry? rx2 += mulrax
# asm 1: add  <mulrax=int64#7,<rx2=int64#10
# asm 2: add  <mulrax=%rax,<rx2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#19,>mulrax=int64#7
# asm 2: movq <mulx419_stack=144(%rsp),>mulrax=%rax
movq 144(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * t24_stack
# asm 1: mulq  <t24_stack=stack64#27
# asm 2: mulq  <t24_stack=208(%rsp)
mulq  208(%rsp)

# qhasm:   carry? rx3 += mulrax
# asm 1: add  <mulrax=int64#7,<rx3=int64#12
# asm 2: add  <mulrax=%rax,<rx3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=int64#3
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=%rdx
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,%rdx

# qhasm:   mulr01 = (mulr01.rx0) << 13
# asm 1: shld $13,<rx0=int64#5,<mulr01=int64#6
# asm 2: shld $13,<rx0=%r8,<mulr01=%r9
shld $13,%r8,%r9

# qhasm:   rx0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx0=int64#5
# asm 2: and  <mulredmask=%rdx,<rx0=%r8
and  %rdx,%r8

# qhasm:   mulr11 = (mulr11.rx1) << 13
# asm 1: shld $13,<rx1=int64#8,<mulr11=int64#9
# asm 2: shld $13,<rx1=%r10,<mulr11=%r11
shld $13,%r10,%r11

# qhasm:   rx1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx1=int64#8
# asm 2: and  <mulredmask=%rdx,<rx1=%r10
and  %rdx,%r10

# qhasm:   rx1 += mulr01
# asm 1: add  <mulr01=int64#6,<rx1=int64#8
# asm 2: add  <mulr01=%r9,<rx1=%r10
add  %r9,%r10

# qhasm:   mulr21 = (mulr21.rx2) << 13
# asm 1: shld $13,<rx2=int64#10,<mulr21=int64#11
# asm 2: shld $13,<rx2=%r12,<mulr21=%r13
shld $13,%r12,%r13

# qhasm:   rx2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx2=int64#10
# asm 2: and  <mulredmask=%rdx,<rx2=%r12
and  %rdx,%r12

# qhasm:   rx2 += mulr11
# asm 1: add  <mulr11=int64#9,<rx2=int64#10
# asm 2: add  <mulr11=%r11,<rx2=%r12
add  %r11,%r12

# qhasm:   mulr31 = (mulr31.rx3) << 13
# asm 1: shld $13,<rx3=int64#12,<mulr31=int64#13
# asm 2: shld $13,<rx3=%r14,<mulr31=%r15
shld $13,%r14,%r15

# qhasm:   rx3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx3=int64#12
# asm 2: and  <mulredmask=%rdx,<rx3=%r14
and  %rdx,%r14

# qhasm:   rx3 += mulr21
# asm 1: add  <mulr21=int64#11,<rx3=int64#12
# asm 2: add  <mulr21=%r13,<rx3=%r14
add  %r13,%r14

# qhasm:   mulr41 = (mulr41.rx4) << 13
# asm 1: shld $13,<rx4=int64#14,<mulr41=int64#15
# asm 2: shld $13,<rx4=%rbx,<mulr41=%rbp
shld $13,%rbx,%rbp

# qhasm:   rx4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx4=int64#14
# asm 2: and  <mulredmask=%rdx,<rx4=%rbx
and  %rdx,%rbx

# qhasm:   rx4 += mulr31
# asm 1: add  <mulr31=int64#13,<rx4=int64#14
# asm 2: add  <mulr31=%r15,<rx4=%rbx
add  %r15,%rbx

# qhasm:   mulr41 = mulr41 * 19
# asm 1: imulq  $19,<mulr41=int64#15,>mulr41=int64#6
# asm 2: imulq  $19,<mulr41=%rbp,>mulr41=%r9
imulq  $19,%rbp,%r9

# qhasm:   rx0 += mulr41
# asm 1: add  <mulr41=int64#6,<rx0=int64#5
# asm 2: add  <mulr41=%r9,<rx0=%r8
add  %r9,%r8

# qhasm:   mult = rx0
# asm 1: mov  <rx0=int64#5,>mult=int64#6
# asm 2: mov  <rx0=%r8,>mult=%r9
mov  %r8,%r9

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   mult += rx1
# asm 1: add  <rx1=int64#8,<mult=int64#6
# asm 2: add  <rx1=%r10,<mult=%r9
add  %r10,%r9

# qhasm:   rx1 = mult
# asm 1: mov  <mult=int64#6,>rx1=int64#7
# asm 2: mov  <mult=%r9,>rx1=%rax
mov  %r9,%rax

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   rx0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx0=int64#5
# asm 2: and  <mulredmask=%rdx,<rx0=%r8
and  %rdx,%r8

# qhasm:   mult += rx2
# asm 1: add  <rx2=int64#10,<mult=int64#6
# asm 2: add  <rx2=%r12,<mult=%r9
add  %r12,%r9

# qhasm:   rx2 = mult
# asm 1: mov  <mult=int64#6,>rx2=int64#8
# asm 2: mov  <mult=%r9,>rx2=%r10
mov  %r9,%r10

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   rx1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx1=int64#7
# asm 2: and  <mulredmask=%rdx,<rx1=%rax
and  %rdx,%rax

# qhasm:   mult += rx3
# asm 1: add  <rx3=int64#12,<mult=int64#6
# asm 2: add  <rx3=%r14,<mult=%r9
add  %r14,%r9

# qhasm:   rx3 = mult
# asm 1: mov  <mult=int64#6,>rx3=int64#9
# asm 2: mov  <mult=%r9,>rx3=%r11
mov  %r9,%r11

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   rx2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx2=int64#8
# asm 2: and  <mulredmask=%rdx,<rx2=%r10
and  %rdx,%r10

# qhasm:   mult += rx4
# asm 1: add  <rx4=int64#14,<mult=int64#6
# asm 2: add  <rx4=%rbx,<mult=%r9
add  %rbx,%r9

# qhasm:   rx4 = mult
# asm 1: mov  <mult=int64#6,>rx4=int64#10
# asm 2: mov  <mult=%r9,>rx4=%r12
mov  %r9,%r12

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   rx3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx3=int64#9
# asm 2: and  <mulredmask=%rdx,<rx3=%r11
and  %rdx,%r11

# qhasm:   mult *= 19
# asm 1: imulq  $19,<mult=int64#6,>mult=int64#6
# asm 2: imulq  $19,<mult=%r9,>mult=%r9
imulq  $19,%r9,%r9

# qhasm:   rx0 += mult
# asm 1: add  <mult=int64#6,<rx0=int64#5
# asm 2: add  <mult=%r9,<rx0=%r8
add  %r9,%r8

# qhasm:   rx4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx4=int64#10
# asm 2: and  <mulredmask=%rdx,<rx4=%r12
and  %rdx,%r12

# qhasm: ry0 = rx0
# asm 1: mov  <rx0=int64#5,>ry0=int64#3
# asm 2: mov  <rx0=%r8,>ry0=%rdx
mov  %r8,%rdx

# qhasm: ry1 = rx1
# asm 1: mov  <rx1=int64#7,>ry1=int64#6
# asm 2: mov  <rx1=%rax,>ry1=%r9
mov  %rax,%r9

# qhasm: ry2 = rx2
# asm 1: mov  <rx2=int64#8,>ry2=int64#11
# asm 2: mov  <rx2=%r10,>ry2=%r13
mov  %r10,%r13

# qhasm: ry3 = rx3
# asm 1: mov  <rx3=int64#9,>ry3=int64#12
# asm 2: mov  <rx3=%r11,>ry3=%r14
mov  %r11,%r14

# qhasm: ry4 = rx4
# asm 1: mov  <rx4=int64#10,>ry4=int64#13
# asm 2: mov  <rx4=%r12,>ry4=%r15
mov  %r12,%r15

# qhasm: rx0 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P0
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P0,<rx0=int64#5
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P0,<rx0=%r8
add  crypto_sign_ed25519_amd64_51_30k_batch_2P0,%r8

# qhasm: rx1 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<rx1=int64#7
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<rx1=%rax
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%rax

# qhasm: rx2 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<rx2=int64#8
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<rx2=%r10
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r10

# qhasm: rx3 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<rx3=int64#9
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<rx3=%r11
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r11

# qhasm: rx4 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<rx4=int64#10
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<rx4=%r12
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r12

# qhasm: ry0 += a0_stack
# asm 1: addq <a0_stack=stack64#8,<ry0=int64#3
# asm 2: addq <a0_stack=56(%rsp),<ry0=%rdx
addq 56(%rsp),%rdx

# qhasm: ry1 += a1_stack
# asm 1: addq <a1_stack=stack64#9,<ry1=int64#6
# asm 2: addq <a1_stack=64(%rsp),<ry1=%r9
addq 64(%rsp),%r9

# qhasm: ry2 += a2_stack
# asm 1: addq <a2_stack=stack64#10,<ry2=int64#11
# asm 2: addq <a2_stack=72(%rsp),<ry2=%r13
addq 72(%rsp),%r13

# qhasm: ry3 += a3_stack
# asm 1: addq <a3_stack=stack64#11,<ry3=int64#12
# asm 2: addq <a3_stack=80(%rsp),<ry3=%r14
addq 80(%rsp),%r14

# qhasm: ry4 += a4_stack
# asm 1: addq <a4_stack=stack64#12,<ry4=int64#13
# asm 2: addq <a4_stack=88(%rsp),<ry4=%r15
addq 88(%rsp),%r15

# qhasm: rx0 -= a0_stack
# asm 1: subq <a0_stack=stack64#8,<rx0=int64#5
# asm 2: subq <a0_stack=56(%rsp),<rx0=%r8
subq 56(%rsp),%r8

# qhasm: rx1 -= a1_stack
# asm 1: subq <a1_stack=stack64#9,<rx1=int64#7
# asm 2: subq <a1_stack=64(%rsp),<rx1=%rax
subq 64(%rsp),%rax

# qhasm: rx2 -= a2_stack
# asm 1: subq <a2_stack=stack64#10,<rx2=int64#8
# asm 2: subq <a2_stack=72(%rsp),<rx2=%r10
subq 72(%rsp),%r10

# qhasm: rx3 -= a3_stack
# asm 1: subq <a3_stack=stack64#11,<rx3=int64#9
# asm 2: subq <a3_stack=80(%rsp),<rx3=%r11
subq 80(%rsp),%r11

# qhasm: rx4 -= a4_stack
# asm 1: subq <a4_stack=stack64#12,<rx4=int64#10
# asm 2: subq <a4_stack=88(%rsp),<rx4=%r12
subq 88(%rsp),%r12

# qhasm: *(uint64 *) (rp + 0) = rx0
# asm 1: movq   <rx0=int64#5,0(<rp=int64#1)
# asm 2: movq   <rx0=%r8,0(<rp=%rdi)
movq   %r8,0(%rdi)

# qhasm: *(uint64 *) (rp + 8) = rx1
# asm 1: movq   <rx1=int64#7,8(<rp=int64#1)
# asm 2: movq   <rx1=%rax,8(<rp=%rdi)
movq   %rax,8(%rdi)

# qhasm: *(uint64 *) (rp + 16) = rx2
# asm 1: movq   <rx2=int64#8,16(<rp=int64#1)
# asm 2: movq   <rx2=%r10,16(<rp=%rdi)
movq   %r10,16(%rdi)

# qhasm: *(uint64 *) (rp + 24) = rx3
# asm 1: movq   <rx3=int64#9,24(<rp=int64#1)
# asm 2: movq   <rx3=%r11,24(<rp=%rdi)
movq   %r11,24(%rdi)

# qhasm: *(uint64 *) (rp + 32) = rx4
# asm 1: movq   <rx4=int64#10,32(<rp=int64#1)
# asm 2: movq   <rx4=%r12,32(<rp=%rdi)
movq   %r12,32(%rdi)

# qhasm: *(uint64 *) (rp + 80) = ry0
# asm 1: movq   <ry0=int64#3,80(<rp=int64#1)
# asm 2: movq   <ry0=%rdx,80(<rp=%rdi)
movq   %rdx,80(%rdi)

# qhasm: *(uint64 *) (rp + 88) = ry1
# asm 1: movq   <ry1=int64#6,88(<rp=int64#1)
# asm 2: movq   <ry1=%r9,88(<rp=%rdi)
movq   %r9,88(%rdi)

# qhasm: *(uint64 *) (rp + 96) = ry2
# asm 1: movq   <ry2=int64#11,96(<rp=int64#1)
# asm 2: movq   <ry2=%r13,96(<rp=%rdi)
movq   %r13,96(%rdi)

# qhasm: *(uint64 *) (rp + 104) = ry3
# asm 1: movq   <ry3=int64#12,104(<rp=int64#1)
# asm 2: movq   <ry3=%r14,104(<rp=%rdi)
movq   %r14,104(%rdi)

# qhasm: *(uint64 *) (rp + 112) = ry4
# asm 1: movq   <ry4=int64#13,112(<rp=int64#1)
# asm 2: movq   <ry4=%r15,112(<rp=%rdi)
movq   %r15,112(%rdi)

# qhasm:   mulrax = *(uint64 *)(pp + 144)
# asm 1: movq   144(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   144(<pp=%rsi),>mulrax=%rdx
movq   144(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx319_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx319_stack=stack64#8
# asm 2: movq <mulrax=%rax,>mulx319_stack=56(%rsp)
movq %rax,56(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 136)
# asm 1: mulq  136(<qp=int64#4)
# asm 2: mulq  136(<qp=%rcx)
mulq  136(%rcx)

# qhasm:   c0 = mulrax
# asm 1: mov  <mulrax=int64#7,>c0=int64#5
# asm 2: mov  <mulrax=%rax,>c0=%r8
mov  %rax,%r8

# qhasm:   mulr01 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr01=int64#6
# asm 2: mov  <mulrdx=%rdx,>mulr01=%r9
mov  %rdx,%r9

# qhasm:   mulrax = *(uint64 *)(pp + 152)
# asm 1: movq   152(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   152(<pp=%rsi),>mulrax=%rdx
movq   152(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx419_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx419_stack=stack64#9
# asm 2: movq <mulrax=%rax,>mulx419_stack=64(%rsp)
movq %rax,64(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 128)
# asm 1: mulq  128(<qp=int64#4)
# asm 2: mulq  128(<qp=%rcx)
mulq  128(%rcx)

# qhasm:   carry? c0 += mulrax
# asm 1: add  <mulrax=int64#7,<c0=int64#5
# asm 2: add  <mulrax=%rax,<c0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = *(uint64 *)(pp + 120)
# asm 1: movq   120(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   120(<pp=%rsi),>mulrax=%rax
movq   120(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 120)
# asm 1: mulq  120(<qp=int64#4)
# asm 2: mulq  120(<qp=%rcx)
mulq  120(%rcx)

# qhasm:   carry? c0 += mulrax
# asm 1: add  <mulrax=int64#7,<c0=int64#5
# asm 2: add  <mulrax=%rax,<c0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = *(uint64 *)(pp + 120)
# asm 1: movq   120(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   120(<pp=%rsi),>mulrax=%rax
movq   120(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 128)
# asm 1: mulq  128(<qp=int64#4)
# asm 2: mulq  128(<qp=%rcx)
mulq  128(%rcx)

# qhasm:   c1 = mulrax
# asm 1: mov  <mulrax=int64#7,>c1=int64#8
# asm 2: mov  <mulrax=%rax,>c1=%r10
mov  %rax,%r10

# qhasm:   mulr11 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr11=int64#9
# asm 2: mov  <mulrdx=%rdx,>mulr11=%r11
mov  %rdx,%r11

# qhasm:   mulrax = *(uint64 *)(pp + 120)
# asm 1: movq   120(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   120(<pp=%rsi),>mulrax=%rax
movq   120(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 136)
# asm 1: mulq  136(<qp=int64#4)
# asm 2: mulq  136(<qp=%rcx)
mulq  136(%rcx)

# qhasm:   c2 = mulrax
# asm 1: mov  <mulrax=int64#7,>c2=int64#10
# asm 2: mov  <mulrax=%rax,>c2=%r12
mov  %rax,%r12

# qhasm:   mulr21 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr21=int64#11
# asm 2: mov  <mulrdx=%rdx,>mulr21=%r13
mov  %rdx,%r13

# qhasm:   mulrax = *(uint64 *)(pp + 120)
# asm 1: movq   120(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   120(<pp=%rsi),>mulrax=%rax
movq   120(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 144)
# asm 1: mulq  144(<qp=int64#4)
# asm 2: mulq  144(<qp=%rcx)
mulq  144(%rcx)

# qhasm:   c3 = mulrax
# asm 1: mov  <mulrax=int64#7,>c3=int64#12
# asm 2: mov  <mulrax=%rax,>c3=%r14
mov  %rax,%r14

# qhasm:   mulr31 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr31=int64#13
# asm 2: mov  <mulrdx=%rdx,>mulr31=%r15
mov  %rdx,%r15

# qhasm:   mulrax = *(uint64 *)(pp + 120)
# asm 1: movq   120(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   120(<pp=%rsi),>mulrax=%rax
movq   120(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 152)
# asm 1: mulq  152(<qp=int64#4)
# asm 2: mulq  152(<qp=%rcx)
mulq  152(%rcx)

# qhasm:   c4 = mulrax
# asm 1: mov  <mulrax=int64#7,>c4=int64#14
# asm 2: mov  <mulrax=%rax,>c4=%rbx
mov  %rax,%rbx

# qhasm:   mulr41 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr41=int64#15
# asm 2: mov  <mulrdx=%rdx,>mulr41=%rbp
mov  %rdx,%rbp

# qhasm:   mulrax = *(uint64 *)(pp + 128)
# asm 1: movq   128(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   128(<pp=%rsi),>mulrax=%rax
movq   128(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 120)
# asm 1: mulq  120(<qp=int64#4)
# asm 2: mulq  120(<qp=%rcx)
mulq  120(%rcx)

# qhasm:   carry? c1 += mulrax
# asm 1: add  <mulrax=int64#7,<c1=int64#8
# asm 2: add  <mulrax=%rax,<c1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = *(uint64 *)(pp + 128)
# asm 1: movq   128(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   128(<pp=%rsi),>mulrax=%rax
movq   128(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 128)
# asm 1: mulq  128(<qp=int64#4)
# asm 2: mulq  128(<qp=%rcx)
mulq  128(%rcx)

# qhasm:   carry? c2 += mulrax
# asm 1: add  <mulrax=int64#7,<c2=int64#10
# asm 2: add  <mulrax=%rax,<c2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = *(uint64 *)(pp + 128)
# asm 1: movq   128(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   128(<pp=%rsi),>mulrax=%rax
movq   128(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 136)
# asm 1: mulq  136(<qp=int64#4)
# asm 2: mulq  136(<qp=%rcx)
mulq  136(%rcx)

# qhasm:   carry? c3 += mulrax
# asm 1: add  <mulrax=int64#7,<c3=int64#12
# asm 2: add  <mulrax=%rax,<c3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulrax = *(uint64 *)(pp + 128)
# asm 1: movq   128(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   128(<pp=%rsi),>mulrax=%rax
movq   128(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 144)
# asm 1: mulq  144(<qp=int64#4)
# asm 2: mulq  144(<qp=%rcx)
mulq  144(%rcx)

# qhasm:   carry? c4 += mulrax
# asm 1: add  <mulrax=int64#7,<c4=int64#14
# asm 2: add  <mulrax=%rax,<c4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = *(uint64 *)(pp + 128)
# asm 1: movq   128(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   128(<pp=%rsi),>mulrax=%rdx
movq   128(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 152)
# asm 1: mulq  152(<qp=int64#4)
# asm 2: mulq  152(<qp=%rcx)
mulq  152(%rcx)

# qhasm:   carry? c0 += mulrax
# asm 1: add  <mulrax=int64#7,<c0=int64#5
# asm 2: add  <mulrax=%rax,<c0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = *(uint64 *)(pp + 136)
# asm 1: movq   136(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   136(<pp=%rsi),>mulrax=%rax
movq   136(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 120)
# asm 1: mulq  120(<qp=int64#4)
# asm 2: mulq  120(<qp=%rcx)
mulq  120(%rcx)

# qhasm:   carry? c2 += mulrax
# asm 1: add  <mulrax=int64#7,<c2=int64#10
# asm 2: add  <mulrax=%rax,<c2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = *(uint64 *)(pp + 136)
# asm 1: movq   136(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   136(<pp=%rsi),>mulrax=%rax
movq   136(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 128)
# asm 1: mulq  128(<qp=int64#4)
# asm 2: mulq  128(<qp=%rcx)
mulq  128(%rcx)

# qhasm:   carry? c3 += mulrax
# asm 1: add  <mulrax=int64#7,<c3=int64#12
# asm 2: add  <mulrax=%rax,<c3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulrax = *(uint64 *)(pp + 136)
# asm 1: movq   136(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   136(<pp=%rsi),>mulrax=%rax
movq   136(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 136)
# asm 1: mulq  136(<qp=int64#4)
# asm 2: mulq  136(<qp=%rcx)
mulq  136(%rcx)

# qhasm:   carry? c4 += mulrax
# asm 1: add  <mulrax=int64#7,<c4=int64#14
# asm 2: add  <mulrax=%rax,<c4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = *(uint64 *)(pp + 136)
# asm 1: movq   136(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   136(<pp=%rsi),>mulrax=%rdx
movq   136(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 144)
# asm 1: mulq  144(<qp=int64#4)
# asm 2: mulq  144(<qp=%rcx)
mulq  144(%rcx)

# qhasm:   carry? c0 += mulrax
# asm 1: add  <mulrax=int64#7,<c0=int64#5
# asm 2: add  <mulrax=%rax,<c0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = *(uint64 *)(pp + 136)
# asm 1: movq   136(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   136(<pp=%rsi),>mulrax=%rdx
movq   136(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 152)
# asm 1: mulq  152(<qp=int64#4)
# asm 2: mulq  152(<qp=%rcx)
mulq  152(%rcx)

# qhasm:   carry? c1 += mulrax
# asm 1: add  <mulrax=int64#7,<c1=int64#8
# asm 2: add  <mulrax=%rax,<c1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = *(uint64 *)(pp + 144)
# asm 1: movq   144(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   144(<pp=%rsi),>mulrax=%rax
movq   144(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 120)
# asm 1: mulq  120(<qp=int64#4)
# asm 2: mulq  120(<qp=%rcx)
mulq  120(%rcx)

# qhasm:   carry? c3 += mulrax
# asm 1: add  <mulrax=int64#7,<c3=int64#12
# asm 2: add  <mulrax=%rax,<c3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulrax = *(uint64 *)(pp + 144)
# asm 1: movq   144(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   144(<pp=%rsi),>mulrax=%rax
movq   144(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 128)
# asm 1: mulq  128(<qp=int64#4)
# asm 2: mulq  128(<qp=%rcx)
mulq  128(%rcx)

# qhasm:   carry? c4 += mulrax
# asm 1: add  <mulrax=int64#7,<c4=int64#14
# asm 2: add  <mulrax=%rax,<c4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <mulx319_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 144)
# asm 1: mulq  144(<qp=int64#4)
# asm 2: mulq  144(<qp=%rcx)
mulq  144(%rcx)

# qhasm:   carry? c1 += mulrax
# asm 1: add  <mulrax=int64#7,<c1=int64#8
# asm 2: add  <mulrax=%rax,<c1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <mulx319_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 152)
# asm 1: mulq  152(<qp=int64#4)
# asm 2: mulq  152(<qp=%rcx)
mulq  152(%rcx)

# qhasm:   carry? c2 += mulrax
# asm 1: add  <mulrax=int64#7,<c2=int64#10
# asm 2: add  <mulrax=%rax,<c2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = *(uint64 *)(pp + 152)
# asm 1: movq   152(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   152(<pp=%rsi),>mulrax=%rax
movq   152(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 120)
# asm 1: mulq  120(<qp=int64#4)
# asm 2: mulq  120(<qp=%rcx)
mulq  120(%rcx)

# qhasm:   carry? c4 += mulrax
# asm 1: add  <mulrax=int64#7,<c4=int64#14
# asm 2: add  <mulrax=%rax,<c4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <mulx419_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 136)
# asm 1: mulq  136(<qp=int64#4)
# asm 2: mulq  136(<qp=%rcx)
mulq  136(%rcx)

# qhasm:   carry? c1 += mulrax
# asm 1: add  <mulrax=int64#7,<c1=int64#8
# asm 2: add  <mulrax=%rax,<c1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <mulx419_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 144)
# asm 1: mulq  144(<qp=int64#4)
# asm 2: mulq  144(<qp=%rcx)
mulq  144(%rcx)

# qhasm:   carry? c2 += mulrax
# asm 1: add  <mulrax=int64#7,<c2=int64#10
# asm 2: add  <mulrax=%rax,<c2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <mulx419_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 152)
# asm 1: mulq  152(<qp=int64#4)
# asm 2: mulq  152(<qp=%rcx)
mulq  152(%rcx)

# qhasm:   carry? c3 += mulrax
# asm 1: add  <mulrax=int64#7,<c3=int64#12
# asm 2: add  <mulrax=%rax,<c3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=int64#3
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=%rdx
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,%rdx

# qhasm:   mulr01 = (mulr01.c0) << 13
# asm 1: shld $13,<c0=int64#5,<mulr01=int64#6
# asm 2: shld $13,<c0=%r8,<mulr01=%r9
shld $13,%r8,%r9

# qhasm:   c0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c0=int64#5
# asm 2: and  <mulredmask=%rdx,<c0=%r8
and  %rdx,%r8

# qhasm:   mulr11 = (mulr11.c1) << 13
# asm 1: shld $13,<c1=int64#8,<mulr11=int64#9
# asm 2: shld $13,<c1=%r10,<mulr11=%r11
shld $13,%r10,%r11

# qhasm:   c1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c1=int64#8
# asm 2: and  <mulredmask=%rdx,<c1=%r10
and  %rdx,%r10

# qhasm:   c1 += mulr01
# asm 1: add  <mulr01=int64#6,<c1=int64#8
# asm 2: add  <mulr01=%r9,<c1=%r10
add  %r9,%r10

# qhasm:   mulr21 = (mulr21.c2) << 13
# asm 1: shld $13,<c2=int64#10,<mulr21=int64#11
# asm 2: shld $13,<c2=%r12,<mulr21=%r13
shld $13,%r12,%r13

# qhasm:   c2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c2=int64#10
# asm 2: and  <mulredmask=%rdx,<c2=%r12
and  %rdx,%r12

# qhasm:   c2 += mulr11
# asm 1: add  <mulr11=int64#9,<c2=int64#10
# asm 2: add  <mulr11=%r11,<c2=%r12
add  %r11,%r12

# qhasm:   mulr31 = (mulr31.c3) << 13
# asm 1: shld $13,<c3=int64#12,<mulr31=int64#13
# asm 2: shld $13,<c3=%r14,<mulr31=%r15
shld $13,%r14,%r15

# qhasm:   c3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c3=int64#12
# asm 2: and  <mulredmask=%rdx,<c3=%r14
and  %rdx,%r14

# qhasm:   c3 += mulr21
# asm 1: add  <mulr21=int64#11,<c3=int64#12
# asm 2: add  <mulr21=%r13,<c3=%r14
add  %r13,%r14

# qhasm:   mulr41 = (mulr41.c4) << 13
# asm 1: shld $13,<c4=int64#14,<mulr41=int64#15
# asm 2: shld $13,<c4=%rbx,<mulr41=%rbp
shld $13,%rbx,%rbp

# qhasm:   c4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c4=int64#14
# asm 2: and  <mulredmask=%rdx,<c4=%rbx
and  %rdx,%rbx

# qhasm:   c4 += mulr31
# asm 1: add  <mulr31=int64#13,<c4=int64#14
# asm 2: add  <mulr31=%r15,<c4=%rbx
add  %r15,%rbx

# qhasm:   mulr41 = mulr41 * 19
# asm 1: imulq  $19,<mulr41=int64#15,>mulr41=int64#6
# asm 2: imulq  $19,<mulr41=%rbp,>mulr41=%r9
imulq  $19,%rbp,%r9

# qhasm:   c0 += mulr41
# asm 1: add  <mulr41=int64#6,<c0=int64#5
# asm 2: add  <mulr41=%r9,<c0=%r8
add  %r9,%r8

# qhasm:   mult = c0
# asm 1: mov  <c0=int64#5,>mult=int64#6
# asm 2: mov  <c0=%r8,>mult=%r9
mov  %r8,%r9

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   mult += c1
# asm 1: add  <c1=int64#8,<mult=int64#6
# asm 2: add  <c1=%r10,<mult=%r9
add  %r10,%r9

# qhasm:   c1 = mult
# asm 1: mov  <mult=int64#6,>c1=int64#7
# asm 2: mov  <mult=%r9,>c1=%rax
mov  %r9,%rax

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   c0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c0=int64#5
# asm 2: and  <mulredmask=%rdx,<c0=%r8
and  %rdx,%r8

# qhasm:   mult += c2
# asm 1: add  <c2=int64#10,<mult=int64#6
# asm 2: add  <c2=%r12,<mult=%r9
add  %r12,%r9

# qhasm:   c2 = mult
# asm 1: mov  <mult=int64#6,>c2=int64#8
# asm 2: mov  <mult=%r9,>c2=%r10
mov  %r9,%r10

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   c1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c1=int64#7
# asm 2: and  <mulredmask=%rdx,<c1=%rax
and  %rdx,%rax

# qhasm:   mult += c3
# asm 1: add  <c3=int64#12,<mult=int64#6
# asm 2: add  <c3=%r14,<mult=%r9
add  %r14,%r9

# qhasm:   c3 = mult
# asm 1: mov  <mult=int64#6,>c3=int64#9
# asm 2: mov  <mult=%r9,>c3=%r11
mov  %r9,%r11

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   c2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c2=int64#8
# asm 2: and  <mulredmask=%rdx,<c2=%r10
and  %rdx,%r10

# qhasm:   mult += c4
# asm 1: add  <c4=int64#14,<mult=int64#6
# asm 2: add  <c4=%rbx,<mult=%r9
add  %rbx,%r9

# qhasm:   c4 = mult
# asm 1: mov  <mult=int64#6,>c4=int64#10
# asm 2: mov  <mult=%r9,>c4=%r12
mov  %r9,%r12

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   c3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c3=int64#9
# asm 2: and  <mulredmask=%rdx,<c3=%r11
and  %rdx,%r11

# qhasm:   mult *= 19
# asm 1: imulq  $19,<mult=int64#6,>mult=int64#6
# asm 2: imulq  $19,<mult=%r9,>mult=%r9
imulq  $19,%r9,%r9

# qhasm:   c0 += mult
# asm 1: add  <mult=int64#6,<c0=int64#5
# asm 2: add  <mult=%r9,<c0=%r8
add  %r9,%r8

# qhasm:   c4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c4=int64#10
# asm 2: and  <mulredmask=%rdx,<c4=%r12
and  %rdx,%r12

# qhasm: c0_stack = c0
# asm 1: movq <c0=int64#5,>c0_stack=stack64#8
# asm 2: movq <c0=%r8,>c0_stack=56(%rsp)
movq %r8,56(%rsp)

# qhasm: c1_stack = c1
# asm 1: movq <c1=int64#7,>c1_stack=stack64#9
# asm 2: movq <c1=%rax,>c1_stack=64(%rsp)
movq %rax,64(%rsp)

# qhasm: c2_stack = c2
# asm 1: movq <c2=int64#8,>c2_stack=stack64#10
# asm 2: movq <c2=%r10,>c2_stack=72(%rsp)
movq %r10,72(%rsp)

# qhasm: c3_stack = c3
# asm 1: movq <c3=int64#9,>c3_stack=stack64#11
# asm 2: movq <c3=%r11,>c3_stack=80(%rsp)
movq %r11,80(%rsp)

# qhasm: c4_stack = c4
# asm 1: movq <c4=int64#10,>c4_stack=stack64#12
# asm 2: movq <c4=%r12,>c4_stack=88(%rsp)
movq %r12,88(%rsp)

# qhasm:   mulrax = c3_stack
# asm 1: movq <c3_stack=stack64#11,>mulrax=int64#3
# asm 2: movq <c3_stack=80(%rsp),>mulrax=%rdx
movq 80(%rsp),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx319_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx319_stack=stack64#13
# asm 2: movq <mulrax=%rax,>mulx319_stack=96(%rsp)
movq %rax,96(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D2
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D2

# qhasm:   c0 = mulrax
# asm 1: mov  <mulrax=int64#7,>c0=int64#5
# asm 2: mov  <mulrax=%rax,>c0=%r8
mov  %rax,%r8

# qhasm:   mulr01 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr01=int64#6
# asm 2: mov  <mulrdx=%rdx,>mulr01=%r9
mov  %rdx,%r9

# qhasm:   mulrax = c4_stack
# asm 1: movq <c4_stack=stack64#12,>mulrax=int64#3
# asm 2: movq <c4_stack=88(%rsp),>mulrax=%rdx
movq 88(%rsp),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx419_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx419_stack=stack64#14
# asm 2: movq <mulrax=%rax,>mulx419_stack=104(%rsp)
movq %rax,104(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D1
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D1

# qhasm:   carry? c0 += mulrax
# asm 1: add  <mulrax=int64#7,<c0=int64#5
# asm 2: add  <mulrax=%rax,<c0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = c0_stack
# asm 1: movq <c0_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <c0_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D0
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D0

# qhasm:   carry? c0 += mulrax
# asm 1: add  <mulrax=int64#7,<c0=int64#5
# asm 2: add  <mulrax=%rax,<c0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = c0_stack
# asm 1: movq <c0_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <c0_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D1
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D1

# qhasm:   c1 = mulrax
# asm 1: mov  <mulrax=int64#7,>c1=int64#8
# asm 2: mov  <mulrax=%rax,>c1=%r10
mov  %rax,%r10

# qhasm:   mulr11 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr11=int64#9
# asm 2: mov  <mulrdx=%rdx,>mulr11=%r11
mov  %rdx,%r11

# qhasm:   mulrax = c0_stack
# asm 1: movq <c0_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <c0_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D2
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D2

# qhasm:   c2 = mulrax
# asm 1: mov  <mulrax=int64#7,>c2=int64#10
# asm 2: mov  <mulrax=%rax,>c2=%r12
mov  %rax,%r12

# qhasm:   mulr21 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr21=int64#11
# asm 2: mov  <mulrdx=%rdx,>mulr21=%r13
mov  %rdx,%r13

# qhasm:   mulrax = c0_stack
# asm 1: movq <c0_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <c0_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D3
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D3

# qhasm:   c3 = mulrax
# asm 1: mov  <mulrax=int64#7,>c3=int64#12
# asm 2: mov  <mulrax=%rax,>c3=%r14
mov  %rax,%r14

# qhasm:   mulr31 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr31=int64#13
# asm 2: mov  <mulrdx=%rdx,>mulr31=%r15
mov  %rdx,%r15

# qhasm:   mulrax = c0_stack
# asm 1: movq <c0_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <c0_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D4
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D4

# qhasm:   c4 = mulrax
# asm 1: mov  <mulrax=int64#7,>c4=int64#14
# asm 2: mov  <mulrax=%rax,>c4=%rbx
mov  %rax,%rbx

# qhasm:   mulr41 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr41=int64#15
# asm 2: mov  <mulrdx=%rdx,>mulr41=%rbp
mov  %rdx,%rbp

# qhasm:   mulrax = c1_stack
# asm 1: movq <c1_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <c1_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D0
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D0

# qhasm:   carry? c1 += mulrax
# asm 1: add  <mulrax=int64#7,<c1=int64#8
# asm 2: add  <mulrax=%rax,<c1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = c1_stack
# asm 1: movq <c1_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <c1_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D1
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D1

# qhasm:   carry? c2 += mulrax
# asm 1: add  <mulrax=int64#7,<c2=int64#10
# asm 2: add  <mulrax=%rax,<c2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = c1_stack
# asm 1: movq <c1_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <c1_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D2
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D2

# qhasm:   carry? c3 += mulrax
# asm 1: add  <mulrax=int64#7,<c3=int64#12
# asm 2: add  <mulrax=%rax,<c3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulrax = c1_stack
# asm 1: movq <c1_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <c1_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D3
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D3

# qhasm:   carry? c4 += mulrax
# asm 1: add  <mulrax=int64#7,<c4=int64#14
# asm 2: add  <mulrax=%rax,<c4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = c1_stack
# asm 1: movq <c1_stack=stack64#9,>mulrax=int64#3
# asm 2: movq <c1_stack=64(%rsp),>mulrax=%rdx
movq 64(%rsp),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D4
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D4

# qhasm:   carry? c0 += mulrax
# asm 1: add  <mulrax=int64#7,<c0=int64#5
# asm 2: add  <mulrax=%rax,<c0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = c2_stack
# asm 1: movq <c2_stack=stack64#10,>mulrax=int64#7
# asm 2: movq <c2_stack=72(%rsp),>mulrax=%rax
movq 72(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D0
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D0

# qhasm:   carry? c2 += mulrax
# asm 1: add  <mulrax=int64#7,<c2=int64#10
# asm 2: add  <mulrax=%rax,<c2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = c2_stack
# asm 1: movq <c2_stack=stack64#10,>mulrax=int64#7
# asm 2: movq <c2_stack=72(%rsp),>mulrax=%rax
movq 72(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D1
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D1

# qhasm:   carry? c3 += mulrax
# asm 1: add  <mulrax=int64#7,<c3=int64#12
# asm 2: add  <mulrax=%rax,<c3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulrax = c2_stack
# asm 1: movq <c2_stack=stack64#10,>mulrax=int64#7
# asm 2: movq <c2_stack=72(%rsp),>mulrax=%rax
movq 72(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D2
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D2

# qhasm:   carry? c4 += mulrax
# asm 1: add  <mulrax=int64#7,<c4=int64#14
# asm 2: add  <mulrax=%rax,<c4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = c2_stack
# asm 1: movq <c2_stack=stack64#10,>mulrax=int64#3
# asm 2: movq <c2_stack=72(%rsp),>mulrax=%rdx
movq 72(%rsp),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D3
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D3

# qhasm:   carry? c0 += mulrax
# asm 1: add  <mulrax=int64#7,<c0=int64#5
# asm 2: add  <mulrax=%rax,<c0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = c2_stack
# asm 1: movq <c2_stack=stack64#10,>mulrax=int64#3
# asm 2: movq <c2_stack=72(%rsp),>mulrax=%rdx
movq 72(%rsp),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D4
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D4

# qhasm:   carry? c1 += mulrax
# asm 1: add  <mulrax=int64#7,<c1=int64#8
# asm 2: add  <mulrax=%rax,<c1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = c3_stack
# asm 1: movq <c3_stack=stack64#11,>mulrax=int64#7
# asm 2: movq <c3_stack=80(%rsp),>mulrax=%rax
movq 80(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D0
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D0

# qhasm:   carry? c3 += mulrax
# asm 1: add  <mulrax=int64#7,<c3=int64#12
# asm 2: add  <mulrax=%rax,<c3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulrax = c3_stack
# asm 1: movq <c3_stack=stack64#11,>mulrax=int64#7
# asm 2: movq <c3_stack=80(%rsp),>mulrax=%rax
movq 80(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D1
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D1

# qhasm:   carry? c4 += mulrax
# asm 1: add  <mulrax=int64#7,<c4=int64#14
# asm 2: add  <mulrax=%rax,<c4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#13,>mulrax=int64#7
# asm 2: movq <mulx319_stack=96(%rsp),>mulrax=%rax
movq 96(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D3
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D3

# qhasm:   carry? c1 += mulrax
# asm 1: add  <mulrax=int64#7,<c1=int64#8
# asm 2: add  <mulrax=%rax,<c1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#13,>mulrax=int64#7
# asm 2: movq <mulx319_stack=96(%rsp),>mulrax=%rax
movq 96(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D4
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D4

# qhasm:   carry? c2 += mulrax
# asm 1: add  <mulrax=int64#7,<c2=int64#10
# asm 2: add  <mulrax=%rax,<c2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = c4_stack
# asm 1: movq <c4_stack=stack64#12,>mulrax=int64#7
# asm 2: movq <c4_stack=88(%rsp),>mulrax=%rax
movq 88(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D0
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D0

# qhasm:   carry? c4 += mulrax
# asm 1: add  <mulrax=int64#7,<c4=int64#14
# asm 2: add  <mulrax=%rax,<c4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#14,>mulrax=int64#7
# asm 2: movq <mulx419_stack=104(%rsp),>mulrax=%rax
movq 104(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D2
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D2

# qhasm:   carry? c1 += mulrax
# asm 1: add  <mulrax=int64#7,<c1=int64#8
# asm 2: add  <mulrax=%rax,<c1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#14,>mulrax=int64#7
# asm 2: movq <mulx419_stack=104(%rsp),>mulrax=%rax
movq 104(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D3
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D3

# qhasm:   carry? c2 += mulrax
# asm 1: add  <mulrax=int64#7,<c2=int64#10
# asm 2: add  <mulrax=%rax,<c2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#14,>mulrax=int64#7
# asm 2: movq <mulx419_stack=104(%rsp),>mulrax=%rax
movq 104(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D4
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D4

# qhasm:   carry? c3 += mulrax
# asm 1: add  <mulrax=int64#7,<c3=int64#12
# asm 2: add  <mulrax=%rax,<c3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=int64#3
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=%rdx
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,%rdx

# qhasm:   mulr01 = (mulr01.c0) << 13
# asm 1: shld $13,<c0=int64#5,<mulr01=int64#6
# asm 2: shld $13,<c0=%r8,<mulr01=%r9
shld $13,%r8,%r9

# qhasm:   c0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c0=int64#5
# asm 2: and  <mulredmask=%rdx,<c0=%r8
and  %rdx,%r8

# qhasm:   mulr11 = (mulr11.c1) << 13
# asm 1: shld $13,<c1=int64#8,<mulr11=int64#9
# asm 2: shld $13,<c1=%r10,<mulr11=%r11
shld $13,%r10,%r11

# qhasm:   c1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c1=int64#8
# asm 2: and  <mulredmask=%rdx,<c1=%r10
and  %rdx,%r10

# qhasm:   c1 += mulr01
# asm 1: add  <mulr01=int64#6,<c1=int64#8
# asm 2: add  <mulr01=%r9,<c1=%r10
add  %r9,%r10

# qhasm:   mulr21 = (mulr21.c2) << 13
# asm 1: shld $13,<c2=int64#10,<mulr21=int64#11
# asm 2: shld $13,<c2=%r12,<mulr21=%r13
shld $13,%r12,%r13

# qhasm:   c2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c2=int64#10
# asm 2: and  <mulredmask=%rdx,<c2=%r12
and  %rdx,%r12

# qhasm:   c2 += mulr11
# asm 1: add  <mulr11=int64#9,<c2=int64#10
# asm 2: add  <mulr11=%r11,<c2=%r12
add  %r11,%r12

# qhasm:   mulr31 = (mulr31.c3) << 13
# asm 1: shld $13,<c3=int64#12,<mulr31=int64#13
# asm 2: shld $13,<c3=%r14,<mulr31=%r15
shld $13,%r14,%r15

# qhasm:   c3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c3=int64#12
# asm 2: and  <mulredmask=%rdx,<c3=%r14
and  %rdx,%r14

# qhasm:   c3 += mulr21
# asm 1: add  <mulr21=int64#11,<c3=int64#12
# asm 2: add  <mulr21=%r13,<c3=%r14
add  %r13,%r14

# qhasm:   mulr41 = (mulr41.c4) << 13
# asm 1: shld $13,<c4=int64#14,<mulr41=int64#15
# asm 2: shld $13,<c4=%rbx,<mulr41=%rbp
shld $13,%rbx,%rbp

# qhasm:   c4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c4=int64#14
# asm 2: and  <mulredmask=%rdx,<c4=%rbx
and  %rdx,%rbx

# qhasm:   c4 += mulr31
# asm 1: add  <mulr31=int64#13,<c4=int64#14
# asm 2: add  <mulr31=%r15,<c4=%rbx
add  %r15,%rbx

# qhasm:   mulr41 = mulr41 * 19
# asm 1: imulq  $19,<mulr41=int64#15,>mulr41=int64#6
# asm 2: imulq  $19,<mulr41=%rbp,>mulr41=%r9
imulq  $19,%rbp,%r9

# qhasm:   c0 += mulr41
# asm 1: add  <mulr41=int64#6,<c0=int64#5
# asm 2: add  <mulr41=%r9,<c0=%r8
add  %r9,%r8

# qhasm:   mult = c0
# asm 1: mov  <c0=int64#5,>mult=int64#6
# asm 2: mov  <c0=%r8,>mult=%r9
mov  %r8,%r9

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   mult += c1
# asm 1: add  <c1=int64#8,<mult=int64#6
# asm 2: add  <c1=%r10,<mult=%r9
add  %r10,%r9

# qhasm:   c1 = mult
# asm 1: mov  <mult=int64#6,>c1=int64#7
# asm 2: mov  <mult=%r9,>c1=%rax
mov  %r9,%rax

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   c0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c0=int64#5
# asm 2: and  <mulredmask=%rdx,<c0=%r8
and  %rdx,%r8

# qhasm:   mult += c2
# asm 1: add  <c2=int64#10,<mult=int64#6
# asm 2: add  <c2=%r12,<mult=%r9
add  %r12,%r9

# qhasm:   c2 = mult
# asm 1: mov  <mult=int64#6,>c2=int64#8
# asm 2: mov  <mult=%r9,>c2=%r10
mov  %r9,%r10

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   c1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c1=int64#7
# asm 2: and  <mulredmask=%rdx,<c1=%rax
and  %rdx,%rax

# qhasm:   mult += c3
# asm 1: add  <c3=int64#12,<mult=int64#6
# asm 2: add  <c3=%r14,<mult=%r9
add  %r14,%r9

# qhasm:   c3 = mult
# asm 1: mov  <mult=int64#6,>c3=int64#9
# asm 2: mov  <mult=%r9,>c3=%r11
mov  %r9,%r11

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   c2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c2=int64#8
# asm 2: and  <mulredmask=%rdx,<c2=%r10
and  %rdx,%r10

# qhasm:   mult += c4
# asm 1: add  <c4=int64#14,<mult=int64#6
# asm 2: add  <c4=%rbx,<mult=%r9
add  %rbx,%r9

# qhasm:   c4 = mult
# asm 1: mov  <mult=int64#6,>c4=int64#10
# asm 2: mov  <mult=%r9,>c4=%r12
mov  %r9,%r12

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#6
# asm 2: shr  $51,<mult=%r9
shr  $51,%r9

# qhasm:   c3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c3=int64#9
# asm 2: and  <mulredmask=%rdx,<c3=%r11
and  %rdx,%r11

# qhasm:   mult *= 19
# asm 1: imulq  $19,<mult=int64#6,>mult=int64#6
# asm 2: imulq  $19,<mult=%r9,>mult=%r9
imulq  $19,%r9,%r9

# qhasm:   c0 += mult
# asm 1: add  <mult=int64#6,<c0=int64#5
# asm 2: add  <mult=%r9,<c0=%r8
add  %r9,%r8

# qhasm:   c4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<c4=int64#10
# asm 2: and  <mulredmask=%rdx,<c4=%r12
and  %rdx,%r12

# qhasm: c0_stack = c0
# asm 1: movq <c0=int64#5,>c0_stack=stack64#8
# asm 2: movq <c0=%r8,>c0_stack=56(%rsp)
movq %r8,56(%rsp)

# qhasm: c1_stack = c1
# asm 1: movq <c1=int64#7,>c1_stack=stack64#9
# asm 2: movq <c1=%rax,>c1_stack=64(%rsp)
movq %rax,64(%rsp)

# qhasm: c2_stack = c2
# asm 1: movq <c2=int64#8,>c2_stack=stack64#10
# asm 2: movq <c2=%r10,>c2_stack=72(%rsp)
movq %r10,72(%rsp)

# qhasm: c3_stack = c3
# asm 1: movq <c3=int64#9,>c3_stack=stack64#11
# asm 2: movq <c3=%r11,>c3_stack=80(%rsp)
movq %r11,80(%rsp)

# qhasm: c4_stack = c4
# asm 1: movq <c4=int64#10,>c4_stack=stack64#12
# asm 2: movq <c4=%r12,>c4_stack=88(%rsp)
movq %r12,88(%rsp)

# qhasm:   mulrax = *(uint64 *)(pp + 104)
# asm 1: movq   104(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   104(<pp=%rsi),>mulrax=%rdx
movq   104(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx319_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx319_stack=stack64#13
# asm 2: movq <mulrax=%rax,>mulx319_stack=96(%rsp)
movq %rax,96(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 96)
# asm 1: mulq  96(<qp=int64#4)
# asm 2: mulq  96(<qp=%rcx)
mulq  96(%rcx)

# qhasm:   rt0 = mulrax
# asm 1: mov  <mulrax=int64#7,>rt0=int64#5
# asm 2: mov  <mulrax=%rax,>rt0=%r8
mov  %rax,%r8

# qhasm:   mulr01 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr01=int64#6
# asm 2: mov  <mulrdx=%rdx,>mulr01=%r9
mov  %rdx,%r9

# qhasm:   mulrax = *(uint64 *)(pp + 112)
# asm 1: movq   112(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   112(<pp=%rsi),>mulrax=%rdx
movq   112(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx419_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx419_stack=stack64#14
# asm 2: movq <mulrax=%rax,>mulx419_stack=104(%rsp)
movq %rax,104(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 88)
# asm 1: mulq  88(<qp=int64#4)
# asm 2: mulq  88(<qp=%rcx)
mulq  88(%rcx)

# qhasm:   carry? rt0 += mulrax
# asm 1: add  <mulrax=int64#7,<rt0=int64#5
# asm 2: add  <mulrax=%rax,<rt0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = *(uint64 *)(pp + 80)
# asm 1: movq   80(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   80(<pp=%rsi),>mulrax=%rax
movq   80(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 80)
# asm 1: mulq  80(<qp=int64#4)
# asm 2: mulq  80(<qp=%rcx)
mulq  80(%rcx)

# qhasm:   carry? rt0 += mulrax
# asm 1: add  <mulrax=int64#7,<rt0=int64#5
# asm 2: add  <mulrax=%rax,<rt0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = *(uint64 *)(pp + 80)
# asm 1: movq   80(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   80(<pp=%rsi),>mulrax=%rax
movq   80(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 88)
# asm 1: mulq  88(<qp=int64#4)
# asm 2: mulq  88(<qp=%rcx)
mulq  88(%rcx)

# qhasm:   rt1 = mulrax
# asm 1: mov  <mulrax=int64#7,>rt1=int64#8
# asm 2: mov  <mulrax=%rax,>rt1=%r10
mov  %rax,%r10

# qhasm:   mulr11 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr11=int64#9
# asm 2: mov  <mulrdx=%rdx,>mulr11=%r11
mov  %rdx,%r11

# qhasm:   mulrax = *(uint64 *)(pp + 80)
# asm 1: movq   80(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   80(<pp=%rsi),>mulrax=%rax
movq   80(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 96)
# asm 1: mulq  96(<qp=int64#4)
# asm 2: mulq  96(<qp=%rcx)
mulq  96(%rcx)

# qhasm:   rt2 = mulrax
# asm 1: mov  <mulrax=int64#7,>rt2=int64#10
# asm 2: mov  <mulrax=%rax,>rt2=%r12
mov  %rax,%r12

# qhasm:   mulr21 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr21=int64#11
# asm 2: mov  <mulrdx=%rdx,>mulr21=%r13
mov  %rdx,%r13

# qhasm:   mulrax = *(uint64 *)(pp + 80)
# asm 1: movq   80(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   80(<pp=%rsi),>mulrax=%rax
movq   80(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 104)
# asm 1: mulq  104(<qp=int64#4)
# asm 2: mulq  104(<qp=%rcx)
mulq  104(%rcx)

# qhasm:   rt3 = mulrax
# asm 1: mov  <mulrax=int64#7,>rt3=int64#12
# asm 2: mov  <mulrax=%rax,>rt3=%r14
mov  %rax,%r14

# qhasm:   mulr31 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr31=int64#13
# asm 2: mov  <mulrdx=%rdx,>mulr31=%r15
mov  %rdx,%r15

# qhasm:   mulrax = *(uint64 *)(pp + 80)
# asm 1: movq   80(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   80(<pp=%rsi),>mulrax=%rax
movq   80(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 112)
# asm 1: mulq  112(<qp=int64#4)
# asm 2: mulq  112(<qp=%rcx)
mulq  112(%rcx)

# qhasm:   rt4 = mulrax
# asm 1: mov  <mulrax=int64#7,>rt4=int64#14
# asm 2: mov  <mulrax=%rax,>rt4=%rbx
mov  %rax,%rbx

# qhasm:   mulr41 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr41=int64#15
# asm 2: mov  <mulrdx=%rdx,>mulr41=%rbp
mov  %rdx,%rbp

# qhasm:   mulrax = *(uint64 *)(pp + 88)
# asm 1: movq   88(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   88(<pp=%rsi),>mulrax=%rax
movq   88(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 80)
# asm 1: mulq  80(<qp=int64#4)
# asm 2: mulq  80(<qp=%rcx)
mulq  80(%rcx)

# qhasm:   carry? rt1 += mulrax
# asm 1: add  <mulrax=int64#7,<rt1=int64#8
# asm 2: add  <mulrax=%rax,<rt1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = *(uint64 *)(pp + 88)
# asm 1: movq   88(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   88(<pp=%rsi),>mulrax=%rax
movq   88(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 88)
# asm 1: mulq  88(<qp=int64#4)
# asm 2: mulq  88(<qp=%rcx)
mulq  88(%rcx)

# qhasm:   carry? rt2 += mulrax
# asm 1: add  <mulrax=int64#7,<rt2=int64#10
# asm 2: add  <mulrax=%rax,<rt2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = *(uint64 *)(pp + 88)
# asm 1: movq   88(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   88(<pp=%rsi),>mulrax=%rax
movq   88(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 96)
# asm 1: mulq  96(<qp=int64#4)
# asm 2: mulq  96(<qp=%rcx)
mulq  96(%rcx)

# qhasm:   carry? rt3 += mulrax
# asm 1: add  <mulrax=int64#7,<rt3=int64#12
# asm 2: add  <mulrax=%rax,<rt3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulrax = *(uint64 *)(pp + 88)
# asm 1: movq   88(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   88(<pp=%rsi),>mulrax=%rax
movq   88(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 104)
# asm 1: mulq  104(<qp=int64#4)
# asm 2: mulq  104(<qp=%rcx)
mulq  104(%rcx)

# qhasm:   carry? rt4 += mulrax
# asm 1: add  <mulrax=int64#7,<rt4=int64#14
# asm 2: add  <mulrax=%rax,<rt4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = *(uint64 *)(pp + 88)
# asm 1: movq   88(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   88(<pp=%rsi),>mulrax=%rdx
movq   88(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 112)
# asm 1: mulq  112(<qp=int64#4)
# asm 2: mulq  112(<qp=%rcx)
mulq  112(%rcx)

# qhasm:   carry? rt0 += mulrax
# asm 1: add  <mulrax=int64#7,<rt0=int64#5
# asm 2: add  <mulrax=%rax,<rt0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = *(uint64 *)(pp + 96)
# asm 1: movq   96(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   96(<pp=%rsi),>mulrax=%rax
movq   96(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 80)
# asm 1: mulq  80(<qp=int64#4)
# asm 2: mulq  80(<qp=%rcx)
mulq  80(%rcx)

# qhasm:   carry? rt2 += mulrax
# asm 1: add  <mulrax=int64#7,<rt2=int64#10
# asm 2: add  <mulrax=%rax,<rt2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = *(uint64 *)(pp + 96)
# asm 1: movq   96(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   96(<pp=%rsi),>mulrax=%rax
movq   96(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 88)
# asm 1: mulq  88(<qp=int64#4)
# asm 2: mulq  88(<qp=%rcx)
mulq  88(%rcx)

# qhasm:   carry? rt3 += mulrax
# asm 1: add  <mulrax=int64#7,<rt3=int64#12
# asm 2: add  <mulrax=%rax,<rt3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulrax = *(uint64 *)(pp + 96)
# asm 1: movq   96(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   96(<pp=%rsi),>mulrax=%rax
movq   96(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 96)
# asm 1: mulq  96(<qp=int64#4)
# asm 2: mulq  96(<qp=%rcx)
mulq  96(%rcx)

# qhasm:   carry? rt4 += mulrax
# asm 1: add  <mulrax=int64#7,<rt4=int64#14
# asm 2: add  <mulrax=%rax,<rt4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = *(uint64 *)(pp + 96)
# asm 1: movq   96(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   96(<pp=%rsi),>mulrax=%rdx
movq   96(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 104)
# asm 1: mulq  104(<qp=int64#4)
# asm 2: mulq  104(<qp=%rcx)
mulq  104(%rcx)

# qhasm:   carry? rt0 += mulrax
# asm 1: add  <mulrax=int64#7,<rt0=int64#5
# asm 2: add  <mulrax=%rax,<rt0=%r8
add  %rax,%r8

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr01=%r9
adc %rdx,%r9

# qhasm:   mulrax = *(uint64 *)(pp + 96)
# asm 1: movq   96(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   96(<pp=%rsi),>mulrax=%rdx
movq   96(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 112)
# asm 1: mulq  112(<qp=int64#4)
# asm 2: mulq  112(<qp=%rcx)
mulq  112(%rcx)

# qhasm:   carry? rt1 += mulrax
# asm 1: add  <mulrax=int64#7,<rt1=int64#8
# asm 2: add  <mulrax=%rax,<rt1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = *(uint64 *)(pp + 104)
# asm 1: movq   104(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   104(<pp=%rsi),>mulrax=%rax
movq   104(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 80)
# asm 1: mulq  80(<qp=int64#4)
# asm 2: mulq  80(<qp=%rcx)
mulq  80(%rcx)

# qhasm:   carry? rt3 += mulrax
# asm 1: add  <mulrax=int64#7,<rt3=int64#12
# asm 2: add  <mulrax=%rax,<rt3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulrax = *(uint64 *)(pp + 104)
# asm 1: movq   104(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   104(<pp=%rsi),>mulrax=%rax
movq   104(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 88)
# asm 1: mulq  88(<qp=int64#4)
# asm 2: mulq  88(<qp=%rcx)
mulq  88(%rcx)

# qhasm:   carry? rt4 += mulrax
# asm 1: add  <mulrax=int64#7,<rt4=int64#14
# asm 2: add  <mulrax=%rax,<rt4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#13,>mulrax=int64#7
# asm 2: movq <mulx319_stack=96(%rsp),>mulrax=%rax
movq 96(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 104)
# asm 1: mulq  104(<qp=int64#4)
# asm 2: mulq  104(<qp=%rcx)
mulq  104(%rcx)

# qhasm:   carry? rt1 += mulrax
# asm 1: add  <mulrax=int64#7,<rt1=int64#8
# asm 2: add  <mulrax=%rax,<rt1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#13,>mulrax=int64#7
# asm 2: movq <mulx319_stack=96(%rsp),>mulrax=%rax
movq 96(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 112)
# asm 1: mulq  112(<qp=int64#4)
# asm 2: mulq  112(<qp=%rcx)
mulq  112(%rcx)

# qhasm:   carry? rt2 += mulrax
# asm 1: add  <mulrax=int64#7,<rt2=int64#10
# asm 2: add  <mulrax=%rax,<rt2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = *(uint64 *)(pp + 112)
# asm 1: movq   112(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   112(<pp=%rsi),>mulrax=%rax
movq   112(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 80)
# asm 1: mulq  80(<qp=int64#4)
# asm 2: mulq  80(<qp=%rcx)
mulq  80(%rcx)

# qhasm:   carry? rt4 += mulrax
# asm 1: add  <mulrax=int64#7,<rt4=int64#14
# asm 2: add  <mulrax=%rax,<rt4=%rbx
add  %rax,%rbx

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#15
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbp
adc %rdx,%rbp

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#14,>mulrax=int64#7
# asm 2: movq <mulx419_stack=104(%rsp),>mulrax=%rax
movq 104(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 96)
# asm 1: mulq  96(<qp=int64#4)
# asm 2: mulq  96(<qp=%rcx)
mulq  96(%rcx)

# qhasm:   carry? rt1 += mulrax
# asm 1: add  <mulrax=int64#7,<rt1=int64#8
# asm 2: add  <mulrax=%rax,<rt1=%r10
add  %rax,%r10

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr11=%r11
adc %rdx,%r11

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#14,>mulrax=int64#7
# asm 2: movq <mulx419_stack=104(%rsp),>mulrax=%rax
movq 104(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 104)
# asm 1: mulq  104(<qp=int64#4)
# asm 2: mulq  104(<qp=%rcx)
mulq  104(%rcx)

# qhasm:   carry? rt2 += mulrax
# asm 1: add  <mulrax=int64#7,<rt2=int64#10
# asm 2: add  <mulrax=%rax,<rt2=%r12
add  %rax,%r12

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr21=%r13
adc %rdx,%r13

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#14,>mulrax=int64#7
# asm 2: movq <mulx419_stack=104(%rsp),>mulrax=%rax
movq 104(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(qp + 112)
# asm 1: mulq  112(<qp=int64#4)
# asm 2: mulq  112(<qp=%rcx)
mulq  112(%rcx)

# qhasm:   carry? rt3 += mulrax
# asm 1: add  <mulrax=int64#7,<rt3=int64#12
# asm 2: add  <mulrax=%rax,<rt3=%r14
add  %rax,%r14

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr31=%r15
adc %rdx,%r15

# qhasm:   mulredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=int64#2
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=%rsi
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,%rsi

# qhasm:   mulr01 = (mulr01.rt0) << 13
# asm 1: shld $13,<rt0=int64#5,<mulr01=int64#6
# asm 2: shld $13,<rt0=%r8,<mulr01=%r9
shld $13,%r8,%r9

# qhasm:   rt0 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<rt0=int64#5
# asm 2: and  <mulredmask=%rsi,<rt0=%r8
and  %rsi,%r8

# qhasm:   mulr11 = (mulr11.rt1) << 13
# asm 1: shld $13,<rt1=int64#8,<mulr11=int64#9
# asm 2: shld $13,<rt1=%r10,<mulr11=%r11
shld $13,%r10,%r11

# qhasm:   rt1 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<rt1=int64#8
# asm 2: and  <mulredmask=%rsi,<rt1=%r10
and  %rsi,%r10

# qhasm:   rt1 += mulr01
# asm 1: add  <mulr01=int64#6,<rt1=int64#8
# asm 2: add  <mulr01=%r9,<rt1=%r10
add  %r9,%r10

# qhasm:   mulr21 = (mulr21.rt2) << 13
# asm 1: shld $13,<rt2=int64#10,<mulr21=int64#11
# asm 2: shld $13,<rt2=%r12,<mulr21=%r13
shld $13,%r12,%r13

# qhasm:   rt2 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<rt2=int64#10
# asm 2: and  <mulredmask=%rsi,<rt2=%r12
and  %rsi,%r12

# qhasm:   rt2 += mulr11
# asm 1: add  <mulr11=int64#9,<rt2=int64#10
# asm 2: add  <mulr11=%r11,<rt2=%r12
add  %r11,%r12

# qhasm:   mulr31 = (mulr31.rt3) << 13
# asm 1: shld $13,<rt3=int64#12,<mulr31=int64#13
# asm 2: shld $13,<rt3=%r14,<mulr31=%r15
shld $13,%r14,%r15

# qhasm:   rt3 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<rt3=int64#12
# asm 2: and  <mulredmask=%rsi,<rt3=%r14
and  %rsi,%r14

# qhasm:   rt3 += mulr21
# asm 1: add  <mulr21=int64#11,<rt3=int64#12
# asm 2: add  <mulr21=%r13,<rt3=%r14
add  %r13,%r14

# qhasm:   mulr41 = (mulr41.rt4) << 13
# asm 1: shld $13,<rt4=int64#14,<mulr41=int64#15
# asm 2: shld $13,<rt4=%rbx,<mulr41=%rbp
shld $13,%rbx,%rbp

# qhasm:   rt4 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<rt4=int64#14
# asm 2: and  <mulredmask=%rsi,<rt4=%rbx
and  %rsi,%rbx

# qhasm:   rt4 += mulr31
# asm 1: add  <mulr31=int64#13,<rt4=int64#14
# asm 2: add  <mulr31=%r15,<rt4=%rbx
add  %r15,%rbx

# qhasm:   mulr41 = mulr41 * 19
# asm 1: imulq  $19,<mulr41=int64#15,>mulr41=int64#3
# asm 2: imulq  $19,<mulr41=%rbp,>mulr41=%rdx
imulq  $19,%rbp,%rdx

# qhasm:   rt0 += mulr41
# asm 1: add  <mulr41=int64#3,<rt0=int64#5
# asm 2: add  <mulr41=%rdx,<rt0=%r8
add  %rdx,%r8

# qhasm:   mult = rt0
# asm 1: mov  <rt0=int64#5,>mult=int64#3
# asm 2: mov  <rt0=%r8,>mult=%rdx
mov  %r8,%rdx

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#3
# asm 2: shr  $51,<mult=%rdx
shr  $51,%rdx

# qhasm:   mult += rt1
# asm 1: add  <rt1=int64#8,<mult=int64#3
# asm 2: add  <rt1=%r10,<mult=%rdx
add  %r10,%rdx

# qhasm:   rt1 = mult
# asm 1: mov  <mult=int64#3,>rt1=int64#4
# asm 2: mov  <mult=%rdx,>rt1=%rcx
mov  %rdx,%rcx

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#3
# asm 2: shr  $51,<mult=%rdx
shr  $51,%rdx

# qhasm:   rt0 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<rt0=int64#5
# asm 2: and  <mulredmask=%rsi,<rt0=%r8
and  %rsi,%r8

# qhasm:   mult += rt2
# asm 1: add  <rt2=int64#10,<mult=int64#3
# asm 2: add  <rt2=%r12,<mult=%rdx
add  %r12,%rdx

# qhasm:   rt2 = mult
# asm 1: mov  <mult=int64#3,>rt2=int64#6
# asm 2: mov  <mult=%rdx,>rt2=%r9
mov  %rdx,%r9

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#3
# asm 2: shr  $51,<mult=%rdx
shr  $51,%rdx

# qhasm:   rt1 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<rt1=int64#4
# asm 2: and  <mulredmask=%rsi,<rt1=%rcx
and  %rsi,%rcx

# qhasm:   mult += rt3
# asm 1: add  <rt3=int64#12,<mult=int64#3
# asm 2: add  <rt3=%r14,<mult=%rdx
add  %r14,%rdx

# qhasm:   rt3 = mult
# asm 1: mov  <mult=int64#3,>rt3=int64#7
# asm 2: mov  <mult=%rdx,>rt3=%rax
mov  %rdx,%rax

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#3
# asm 2: shr  $51,<mult=%rdx
shr  $51,%rdx

# qhasm:   rt2 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<rt2=int64#6
# asm 2: and  <mulredmask=%rsi,<rt2=%r9
and  %rsi,%r9

# qhasm:   mult += rt4
# asm 1: add  <rt4=int64#14,<mult=int64#3
# asm 2: add  <rt4=%rbx,<mult=%rdx
add  %rbx,%rdx

# qhasm:   rt4 = mult
# asm 1: mov  <mult=int64#3,>rt4=int64#8
# asm 2: mov  <mult=%rdx,>rt4=%r10
mov  %rdx,%r10

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#3
# asm 2: shr  $51,<mult=%rdx
shr  $51,%rdx

# qhasm:   rt3 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<rt3=int64#7
# asm 2: and  <mulredmask=%rsi,<rt3=%rax
and  %rsi,%rax

# qhasm:   mult *= 19
# asm 1: imulq  $19,<mult=int64#3,>mult=int64#3
# asm 2: imulq  $19,<mult=%rdx,>mult=%rdx
imulq  $19,%rdx,%rdx

# qhasm:   rt0 += mult
# asm 1: add  <mult=int64#3,<rt0=int64#5
# asm 2: add  <mult=%rdx,<rt0=%r8
add  %rdx,%r8

# qhasm:   rt4 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<rt4=int64#8
# asm 2: and  <mulredmask=%rsi,<rt4=%r10
and  %rsi,%r10

# qhasm: rt0 += rt0
# asm 1: add  <rt0=int64#5,<rt0=int64#5
# asm 2: add  <rt0=%r8,<rt0=%r8
add  %r8,%r8

# qhasm: rt1 += rt1
# asm 1: add  <rt1=int64#4,<rt1=int64#4
# asm 2: add  <rt1=%rcx,<rt1=%rcx
add  %rcx,%rcx

# qhasm: rt2 += rt2
# asm 1: add  <rt2=int64#6,<rt2=int64#6
# asm 2: add  <rt2=%r9,<rt2=%r9
add  %r9,%r9

# qhasm: rt3 += rt3
# asm 1: add  <rt3=int64#7,<rt3=int64#7
# asm 2: add  <rt3=%rax,<rt3=%rax
add  %rax,%rax

# qhasm: rt4 += rt4
# asm 1: add  <rt4=int64#8,<rt4=int64#8
# asm 2: add  <rt4=%r10,<rt4=%r10
add  %r10,%r10

# qhasm: rz0 = rt0
# asm 1: mov  <rt0=int64#5,>rz0=int64#2
# asm 2: mov  <rt0=%r8,>rz0=%rsi
mov  %r8,%rsi

# qhasm: rz1 = rt1
# asm 1: mov  <rt1=int64#4,>rz1=int64#3
# asm 2: mov  <rt1=%rcx,>rz1=%rdx
mov  %rcx,%rdx

# qhasm: rz2 = rt2
# asm 1: mov  <rt2=int64#6,>rz2=int64#9
# asm 2: mov  <rt2=%r9,>rz2=%r11
mov  %r9,%r11

# qhasm: rz3 = rt3
# asm 1: mov  <rt3=int64#7,>rz3=int64#10
# asm 2: mov  <rt3=%rax,>rz3=%r12
mov  %rax,%r12

# qhasm: rz4 = rt4
# asm 1: mov  <rt4=int64#8,>rz4=int64#11
# asm 2: mov  <rt4=%r10,>rz4=%r13
mov  %r10,%r13

# qhasm: rt0 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P0
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P0,<rt0=int64#5
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P0,<rt0=%r8
add  crypto_sign_ed25519_amd64_51_30k_batch_2P0,%r8

# qhasm: rt1 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<rt1=int64#4
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<rt1=%rcx
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%rcx

# qhasm: rt2 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<rt2=int64#6
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<rt2=%r9
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r9

# qhasm: rt3 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<rt3=int64#7
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<rt3=%rax
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%rax

# qhasm: rt4 += *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<rt4=int64#8
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<rt4=%r10
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r10

# qhasm: rz0 += c0_stack
# asm 1: addq <c0_stack=stack64#8,<rz0=int64#2
# asm 2: addq <c0_stack=56(%rsp),<rz0=%rsi
addq 56(%rsp),%rsi

# qhasm: rz1 += c1_stack
# asm 1: addq <c1_stack=stack64#9,<rz1=int64#3
# asm 2: addq <c1_stack=64(%rsp),<rz1=%rdx
addq 64(%rsp),%rdx

# qhasm: rz2 += c2_stack
# asm 1: addq <c2_stack=stack64#10,<rz2=int64#9
# asm 2: addq <c2_stack=72(%rsp),<rz2=%r11
addq 72(%rsp),%r11

# qhasm: rz3 += c3_stack
# asm 1: addq <c3_stack=stack64#11,<rz3=int64#10
# asm 2: addq <c3_stack=80(%rsp),<rz3=%r12
addq 80(%rsp),%r12

# qhasm: rz4 += c4_stack
# asm 1: addq <c4_stack=stack64#12,<rz4=int64#11
# asm 2: addq <c4_stack=88(%rsp),<rz4=%r13
addq 88(%rsp),%r13

# qhasm: rt0 -= c0_stack
# asm 1: subq <c0_stack=stack64#8,<rt0=int64#5
# asm 2: subq <c0_stack=56(%rsp),<rt0=%r8
subq 56(%rsp),%r8

# qhasm: rt1 -= c1_stack
# asm 1: subq <c1_stack=stack64#9,<rt1=int64#4
# asm 2: subq <c1_stack=64(%rsp),<rt1=%rcx
subq 64(%rsp),%rcx

# qhasm: rt2 -= c2_stack
# asm 1: subq <c2_stack=stack64#10,<rt2=int64#6
# asm 2: subq <c2_stack=72(%rsp),<rt2=%r9
subq 72(%rsp),%r9

# qhasm: rt3 -= c3_stack
# asm 1: subq <c3_stack=stack64#11,<rt3=int64#7
# asm 2: subq <c3_stack=80(%rsp),<rt3=%rax
subq 80(%rsp),%rax

# qhasm: rt4 -= c4_stack
# asm 1: subq <c4_stack=stack64#12,<rt4=int64#8
# asm 2: subq <c4_stack=88(%rsp),<rt4=%r10
subq 88(%rsp),%r10

# qhasm: *(uint64 *)(rp + 40) = rz0
# asm 1: movq   <rz0=int64#2,40(<rp=int64#1)
# asm 2: movq   <rz0=%rsi,40(<rp=%rdi)
movq   %rsi,40(%rdi)

# qhasm: *(uint64 *)(rp + 48) = rz1
# asm 1: movq   <rz1=int64#3,48(<rp=int64#1)
# asm 2: movq   <rz1=%rdx,48(<rp=%rdi)
movq   %rdx,48(%rdi)

# qhasm: *(uint64 *)(rp + 56) = rz2
# asm 1: movq   <rz2=int64#9,56(<rp=int64#1)
# asm 2: movq   <rz2=%r11,56(<rp=%rdi)
movq   %r11,56(%rdi)

# qhasm: *(uint64 *)(rp + 64) = rz3
# asm 1: movq   <rz3=int64#10,64(<rp=int64#1)
# asm 2: movq   <rz3=%r12,64(<rp=%rdi)
movq   %r12,64(%rdi)

# qhasm: *(uint64 *)(rp + 72) = rz4
# asm 1: movq   <rz4=int64#11,72(<rp=int64#1)
# asm 2: movq   <rz4=%r13,72(<rp=%rdi)
movq   %r13,72(%rdi)

# qhasm: *(uint64 *)(rp + 120) = rt0
# asm 1: movq   <rt0=int64#5,120(<rp=int64#1)
# asm 2: movq   <rt0=%r8,120(<rp=%rdi)
movq   %r8,120(%rdi)

# qhasm: *(uint64 *)(rp + 128) = rt1
# asm 1: movq   <rt1=int64#4,128(<rp=int64#1)
# asm 2: movq   <rt1=%rcx,128(<rp=%rdi)
movq   %rcx,128(%rdi)

# qhasm: *(uint64 *)(rp + 136) = rt2
# asm 1: movq   <rt2=int64#6,136(<rp=int64#1)
# asm 2: movq   <rt2=%r9,136(<rp=%rdi)
movq   %r9,136(%rdi)

# qhasm: *(uint64 *)(rp + 144) = rt3
# asm 1: movq   <rt3=int64#7,144(<rp=int64#1)
# asm 2: movq   <rt3=%rax,144(<rp=%rdi)
movq   %rax,144(%rdi)

# qhasm: *(uint64 *)(rp + 152) = rt4
# asm 1: movq   <rt4=int64#8,152(<rp=int64#1)
# asm 2: movq   <rt4=%r10,152(<rp=%rdi)
movq   %r10,152(%rdi)

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
