
# qhasm: int64 rp

# qhasm: int64 pp

# qhasm: input rp

# qhasm: input pp

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

# qhasm: int64 x0

# qhasm: int64 x1

# qhasm: int64 x2

# qhasm: int64 x3

# qhasm: int64 x4

# qhasm: int64 y0

# qhasm: int64 y1

# qhasm: int64 y2

# qhasm: int64 y3

# qhasm: int64 y4

# qhasm: int64 ysubx0

# qhasm: int64 ysubx1

# qhasm: int64 ysubx2

# qhasm: int64 ysubx3

# qhasm: int64 ysubx4

# qhasm: int64 xaddy0

# qhasm: int64 xaddy1

# qhasm: int64 xaddy2

# qhasm: int64 xaddy3

# qhasm: int64 xaddy4

# qhasm: int64 rz0

# qhasm: int64 rz1

# qhasm: int64 rz2

# qhasm: int64 rz3

# qhasm: int64 rz4

# qhasm: int64 t0

# qhasm: int64 t1

# qhasm: int64 t2

# qhasm: int64 t3

# qhasm: int64 t4

# qhasm: int64 t2d0

# qhasm: int64 t2d1

# qhasm: int64 t2d2

# qhasm: int64 t2d3

# qhasm: int64 t2d4

# qhasm: stack64 stackt0

# qhasm: stack64 stackt1

# qhasm: stack64 stackt2

# qhasm: stack64 stackt3

# qhasm: stack64 stackt4

# qhasm: stack64 stackx0

# qhasm: stack64 stackx1

# qhasm: stack64 stackx2

# qhasm: stack64 stackx3

# qhasm: stack64 stackx4

# qhasm: stack64 stacky1

# qhasm: stack64 stacky2

# qhasm: stack64 stacky3

# qhasm: stack64 stacky4

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

# qhasm: enter crypto_sign_ed25519_amd64_51_30k_batch_ge25519_p1p1_to_pniels
.text
.p2align 5
.globl _crypto_sign_ed25519_amd64_51_30k_batch_ge25519_p1p1_to_pniels
.globl crypto_sign_ed25519_amd64_51_30k_batch_ge25519_p1p1_to_pniels
_crypto_sign_ed25519_amd64_51_30k_batch_ge25519_p1p1_to_pniels:
crypto_sign_ed25519_amd64_51_30k_batch_ge25519_p1p1_to_pniels:
mov %rsp,%r11
and $31,%r11
add $128,%r11
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

# qhasm:   mulrax = *(uint64 *)(pp + 24)
# asm 1: movq   24(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   24(<pp=%rsi),>mulrax=%rdx
movq   24(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx319_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx319_stack=stack64#8
# asm 2: movq <mulrax=%rax,>mulx319_stack=56(%rsp)
movq %rax,56(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 136)
# asm 1: mulq  136(<pp=int64#2)
# asm 2: mulq  136(<pp=%rsi)
mulq  136(%rsi)

# qhasm:   x0 = mulrax
# asm 1: mov  <mulrax=int64#7,>x0=int64#4
# asm 2: mov  <mulrax=%rax,>x0=%rcx
mov  %rax,%rcx

# qhasm:   mulr01 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr01=int64#5
# asm 2: mov  <mulrdx=%rdx,>mulr01=%r8
mov  %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 32)
# asm 1: movq   32(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   32(<pp=%rsi),>mulrax=%rdx
movq   32(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx419_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx419_stack=stack64#9
# asm 2: movq <mulrax=%rax,>mulx419_stack=64(%rsp)
movq %rax,64(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 128)
# asm 1: mulq  128(<pp=int64#2)
# asm 2: mulq  128(<pp=%rsi)
mulq  128(%rsi)

# qhasm:   carry? x0 += mulrax
# asm 1: add  <mulrax=int64#7,<x0=int64#4
# asm 2: add  <mulrax=%rax,<x0=%rcx
add  %rax,%rcx

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#5
# asm 2: adc <mulrdx=%rdx,<mulr01=%r8
adc %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 0)
# asm 1: movq   0(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   0(<pp=%rsi),>mulrax=%rax
movq   0(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 120)
# asm 1: mulq  120(<pp=int64#2)
# asm 2: mulq  120(<pp=%rsi)
mulq  120(%rsi)

# qhasm:   carry? x0 += mulrax
# asm 1: add  <mulrax=int64#7,<x0=int64#4
# asm 2: add  <mulrax=%rax,<x0=%rcx
add  %rax,%rcx

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#5
# asm 2: adc <mulrdx=%rdx,<mulr01=%r8
adc %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 0)
# asm 1: movq   0(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   0(<pp=%rsi),>mulrax=%rax
movq   0(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 128)
# asm 1: mulq  128(<pp=int64#2)
# asm 2: mulq  128(<pp=%rsi)
mulq  128(%rsi)

# qhasm:   x1 = mulrax
# asm 1: mov  <mulrax=int64#7,>x1=int64#6
# asm 2: mov  <mulrax=%rax,>x1=%r9
mov  %rax,%r9

# qhasm:   mulr11 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr11=int64#8
# asm 2: mov  <mulrdx=%rdx,>mulr11=%r10
mov  %rdx,%r10

# qhasm:   mulrax = *(uint64 *)(pp + 0)
# asm 1: movq   0(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   0(<pp=%rsi),>mulrax=%rax
movq   0(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 136)
# asm 1: mulq  136(<pp=int64#2)
# asm 2: mulq  136(<pp=%rsi)
mulq  136(%rsi)

# qhasm:   x2 = mulrax
# asm 1: mov  <mulrax=int64#7,>x2=int64#9
# asm 2: mov  <mulrax=%rax,>x2=%r11
mov  %rax,%r11

# qhasm:   mulr21 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr21=int64#10
# asm 2: mov  <mulrdx=%rdx,>mulr21=%r12
mov  %rdx,%r12

# qhasm:   mulrax = *(uint64 *)(pp + 0)
# asm 1: movq   0(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   0(<pp=%rsi),>mulrax=%rax
movq   0(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 144)
# asm 1: mulq  144(<pp=int64#2)
# asm 2: mulq  144(<pp=%rsi)
mulq  144(%rsi)

# qhasm:   x3 = mulrax
# asm 1: mov  <mulrax=int64#7,>x3=int64#11
# asm 2: mov  <mulrax=%rax,>x3=%r13
mov  %rax,%r13

# qhasm:   mulr31 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr31=int64#12
# asm 2: mov  <mulrdx=%rdx,>mulr31=%r14
mov  %rdx,%r14

# qhasm:   mulrax = *(uint64 *)(pp + 0)
# asm 1: movq   0(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   0(<pp=%rsi),>mulrax=%rax
movq   0(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 152)
# asm 1: mulq  152(<pp=int64#2)
# asm 2: mulq  152(<pp=%rsi)
mulq  152(%rsi)

# qhasm:   x4 = mulrax
# asm 1: mov  <mulrax=int64#7,>x4=int64#13
# asm 2: mov  <mulrax=%rax,>x4=%r15
mov  %rax,%r15

# qhasm:   mulr41 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr41=int64#14
# asm 2: mov  <mulrdx=%rdx,>mulr41=%rbx
mov  %rdx,%rbx

# qhasm:   mulrax = *(uint64 *)(pp + 8)
# asm 1: movq   8(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   8(<pp=%rsi),>mulrax=%rax
movq   8(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 120)
# asm 1: mulq  120(<pp=int64#2)
# asm 2: mulq  120(<pp=%rsi)
mulq  120(%rsi)

# qhasm:   carry? x1 += mulrax
# asm 1: add  <mulrax=int64#7,<x1=int64#6
# asm 2: add  <mulrax=%rax,<x1=%r9
add  %rax,%r9

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#8
# asm 2: adc <mulrdx=%rdx,<mulr11=%r10
adc %rdx,%r10

# qhasm:   mulrax = *(uint64 *)(pp + 8)
# asm 1: movq   8(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   8(<pp=%rsi),>mulrax=%rax
movq   8(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 128)
# asm 1: mulq  128(<pp=int64#2)
# asm 2: mulq  128(<pp=%rsi)
mulq  128(%rsi)

# qhasm:   carry? x2 += mulrax
# asm 1: add  <mulrax=int64#7,<x2=int64#9
# asm 2: add  <mulrax=%rax,<x2=%r11
add  %rax,%r11

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#10
# asm 2: adc <mulrdx=%rdx,<mulr21=%r12
adc %rdx,%r12

# qhasm:   mulrax = *(uint64 *)(pp + 8)
# asm 1: movq   8(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   8(<pp=%rsi),>mulrax=%rax
movq   8(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 136)
# asm 1: mulq  136(<pp=int64#2)
# asm 2: mulq  136(<pp=%rsi)
mulq  136(%rsi)

# qhasm:   carry? x3 += mulrax
# asm 1: add  <mulrax=int64#7,<x3=int64#11
# asm 2: add  <mulrax=%rax,<x3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulrax = *(uint64 *)(pp + 8)
# asm 1: movq   8(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   8(<pp=%rsi),>mulrax=%rax
movq   8(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 144)
# asm 1: mulq  144(<pp=int64#2)
# asm 2: mulq  144(<pp=%rsi)
mulq  144(%rsi)

# qhasm:   carry? x4 += mulrax
# asm 1: add  <mulrax=int64#7,<x4=int64#13
# asm 2: add  <mulrax=%rax,<x4=%r15
add  %rax,%r15

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#14
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbx
adc %rdx,%rbx

# qhasm:   mulrax = *(uint64 *)(pp + 8)
# asm 1: movq   8(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   8(<pp=%rsi),>mulrax=%rdx
movq   8(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 152)
# asm 1: mulq  152(<pp=int64#2)
# asm 2: mulq  152(<pp=%rsi)
mulq  152(%rsi)

# qhasm:   carry? x0 += mulrax
# asm 1: add  <mulrax=int64#7,<x0=int64#4
# asm 2: add  <mulrax=%rax,<x0=%rcx
add  %rax,%rcx

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#5
# asm 2: adc <mulrdx=%rdx,<mulr01=%r8
adc %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 16)
# asm 1: movq   16(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   16(<pp=%rsi),>mulrax=%rax
movq   16(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 120)
# asm 1: mulq  120(<pp=int64#2)
# asm 2: mulq  120(<pp=%rsi)
mulq  120(%rsi)

# qhasm:   carry? x2 += mulrax
# asm 1: add  <mulrax=int64#7,<x2=int64#9
# asm 2: add  <mulrax=%rax,<x2=%r11
add  %rax,%r11

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#10
# asm 2: adc <mulrdx=%rdx,<mulr21=%r12
adc %rdx,%r12

# qhasm:   mulrax = *(uint64 *)(pp + 16)
# asm 1: movq   16(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   16(<pp=%rsi),>mulrax=%rax
movq   16(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 128)
# asm 1: mulq  128(<pp=int64#2)
# asm 2: mulq  128(<pp=%rsi)
mulq  128(%rsi)

# qhasm:   carry? x3 += mulrax
# asm 1: add  <mulrax=int64#7,<x3=int64#11
# asm 2: add  <mulrax=%rax,<x3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulrax = *(uint64 *)(pp + 16)
# asm 1: movq   16(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   16(<pp=%rsi),>mulrax=%rax
movq   16(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 136)
# asm 1: mulq  136(<pp=int64#2)
# asm 2: mulq  136(<pp=%rsi)
mulq  136(%rsi)

# qhasm:   carry? x4 += mulrax
# asm 1: add  <mulrax=int64#7,<x4=int64#13
# asm 2: add  <mulrax=%rax,<x4=%r15
add  %rax,%r15

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#14
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbx
adc %rdx,%rbx

# qhasm:   mulrax = *(uint64 *)(pp + 16)
# asm 1: movq   16(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   16(<pp=%rsi),>mulrax=%rdx
movq   16(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 144)
# asm 1: mulq  144(<pp=int64#2)
# asm 2: mulq  144(<pp=%rsi)
mulq  144(%rsi)

# qhasm:   carry? x0 += mulrax
# asm 1: add  <mulrax=int64#7,<x0=int64#4
# asm 2: add  <mulrax=%rax,<x0=%rcx
add  %rax,%rcx

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#5
# asm 2: adc <mulrdx=%rdx,<mulr01=%r8
adc %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 16)
# asm 1: movq   16(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   16(<pp=%rsi),>mulrax=%rdx
movq   16(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 152)
# asm 1: mulq  152(<pp=int64#2)
# asm 2: mulq  152(<pp=%rsi)
mulq  152(%rsi)

# qhasm:   carry? x1 += mulrax
# asm 1: add  <mulrax=int64#7,<x1=int64#6
# asm 2: add  <mulrax=%rax,<x1=%r9
add  %rax,%r9

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#8
# asm 2: adc <mulrdx=%rdx,<mulr11=%r10
adc %rdx,%r10

# qhasm:   mulrax = *(uint64 *)(pp + 24)
# asm 1: movq   24(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   24(<pp=%rsi),>mulrax=%rax
movq   24(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 120)
# asm 1: mulq  120(<pp=int64#2)
# asm 2: mulq  120(<pp=%rsi)
mulq  120(%rsi)

# qhasm:   carry? x3 += mulrax
# asm 1: add  <mulrax=int64#7,<x3=int64#11
# asm 2: add  <mulrax=%rax,<x3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulrax = *(uint64 *)(pp + 24)
# asm 1: movq   24(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   24(<pp=%rsi),>mulrax=%rax
movq   24(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 128)
# asm 1: mulq  128(<pp=int64#2)
# asm 2: mulq  128(<pp=%rsi)
mulq  128(%rsi)

# qhasm:   carry? x4 += mulrax
# asm 1: add  <mulrax=int64#7,<x4=int64#13
# asm 2: add  <mulrax=%rax,<x4=%r15
add  %rax,%r15

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#14
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbx
adc %rdx,%rbx

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <mulx319_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 144)
# asm 1: mulq  144(<pp=int64#2)
# asm 2: mulq  144(<pp=%rsi)
mulq  144(%rsi)

# qhasm:   carry? x1 += mulrax
# asm 1: add  <mulrax=int64#7,<x1=int64#6
# asm 2: add  <mulrax=%rax,<x1=%r9
add  %rax,%r9

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#8
# asm 2: adc <mulrdx=%rdx,<mulr11=%r10
adc %rdx,%r10

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <mulx319_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 152)
# asm 1: mulq  152(<pp=int64#2)
# asm 2: mulq  152(<pp=%rsi)
mulq  152(%rsi)

# qhasm:   carry? x2 += mulrax
# asm 1: add  <mulrax=int64#7,<x2=int64#9
# asm 2: add  <mulrax=%rax,<x2=%r11
add  %rax,%r11

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#10
# asm 2: adc <mulrdx=%rdx,<mulr21=%r12
adc %rdx,%r12

# qhasm:   mulrax = *(uint64 *)(pp + 32)
# asm 1: movq   32(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   32(<pp=%rsi),>mulrax=%rax
movq   32(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 120)
# asm 1: mulq  120(<pp=int64#2)
# asm 2: mulq  120(<pp=%rsi)
mulq  120(%rsi)

# qhasm:   carry? x4 += mulrax
# asm 1: add  <mulrax=int64#7,<x4=int64#13
# asm 2: add  <mulrax=%rax,<x4=%r15
add  %rax,%r15

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#14
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbx
adc %rdx,%rbx

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <mulx419_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 136)
# asm 1: mulq  136(<pp=int64#2)
# asm 2: mulq  136(<pp=%rsi)
mulq  136(%rsi)

# qhasm:   carry? x1 += mulrax
# asm 1: add  <mulrax=int64#7,<x1=int64#6
# asm 2: add  <mulrax=%rax,<x1=%r9
add  %rax,%r9

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#8
# asm 2: adc <mulrdx=%rdx,<mulr11=%r10
adc %rdx,%r10

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <mulx419_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 144)
# asm 1: mulq  144(<pp=int64#2)
# asm 2: mulq  144(<pp=%rsi)
mulq  144(%rsi)

# qhasm:   carry? x2 += mulrax
# asm 1: add  <mulrax=int64#7,<x2=int64#9
# asm 2: add  <mulrax=%rax,<x2=%r11
add  %rax,%r11

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#10
# asm 2: adc <mulrdx=%rdx,<mulr21=%r12
adc %rdx,%r12

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <mulx419_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 152)
# asm 1: mulq  152(<pp=int64#2)
# asm 2: mulq  152(<pp=%rsi)
mulq  152(%rsi)

# qhasm:   carry? x3 += mulrax
# asm 1: add  <mulrax=int64#7,<x3=int64#11
# asm 2: add  <mulrax=%rax,<x3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=int64#3
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=%rdx
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,%rdx

# qhasm:   mulr01 = (mulr01.x0) << 13
# asm 1: shld $13,<x0=int64#4,<mulr01=int64#5
# asm 2: shld $13,<x0=%rcx,<mulr01=%r8
shld $13,%rcx,%r8

# qhasm:   x0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<x0=int64#4
# asm 2: and  <mulredmask=%rdx,<x0=%rcx
and  %rdx,%rcx

# qhasm:   mulr11 = (mulr11.x1) << 13
# asm 1: shld $13,<x1=int64#6,<mulr11=int64#8
# asm 2: shld $13,<x1=%r9,<mulr11=%r10
shld $13,%r9,%r10

# qhasm:   x1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<x1=int64#6
# asm 2: and  <mulredmask=%rdx,<x1=%r9
and  %rdx,%r9

# qhasm:   x1 += mulr01
# asm 1: add  <mulr01=int64#5,<x1=int64#6
# asm 2: add  <mulr01=%r8,<x1=%r9
add  %r8,%r9

# qhasm:   mulr21 = (mulr21.x2) << 13
# asm 1: shld $13,<x2=int64#9,<mulr21=int64#10
# asm 2: shld $13,<x2=%r11,<mulr21=%r12
shld $13,%r11,%r12

# qhasm:   x2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<x2=int64#9
# asm 2: and  <mulredmask=%rdx,<x2=%r11
and  %rdx,%r11

# qhasm:   x2 += mulr11
# asm 1: add  <mulr11=int64#8,<x2=int64#9
# asm 2: add  <mulr11=%r10,<x2=%r11
add  %r10,%r11

# qhasm:   mulr31 = (mulr31.x3) << 13
# asm 1: shld $13,<x3=int64#11,<mulr31=int64#12
# asm 2: shld $13,<x3=%r13,<mulr31=%r14
shld $13,%r13,%r14

# qhasm:   x3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<x3=int64#11
# asm 2: and  <mulredmask=%rdx,<x3=%r13
and  %rdx,%r13

# qhasm:   x3 += mulr21
# asm 1: add  <mulr21=int64#10,<x3=int64#11
# asm 2: add  <mulr21=%r12,<x3=%r13
add  %r12,%r13

# qhasm:   mulr41 = (mulr41.x4) << 13
# asm 1: shld $13,<x4=int64#13,<mulr41=int64#14
# asm 2: shld $13,<x4=%r15,<mulr41=%rbx
shld $13,%r15,%rbx

# qhasm:   x4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<x4=int64#13
# asm 2: and  <mulredmask=%rdx,<x4=%r15
and  %rdx,%r15

# qhasm:   x4 += mulr31
# asm 1: add  <mulr31=int64#12,<x4=int64#13
# asm 2: add  <mulr31=%r14,<x4=%r15
add  %r14,%r15

# qhasm:   mulr41 = mulr41 * 19
# asm 1: imulq  $19,<mulr41=int64#14,>mulr41=int64#5
# asm 2: imulq  $19,<mulr41=%rbx,>mulr41=%r8
imulq  $19,%rbx,%r8

# qhasm:   x0 += mulr41
# asm 1: add  <mulr41=int64#5,<x0=int64#4
# asm 2: add  <mulr41=%r8,<x0=%rcx
add  %r8,%rcx

# qhasm:   mult = x0
# asm 1: mov  <x0=int64#4,>mult=int64#5
# asm 2: mov  <x0=%rcx,>mult=%r8
mov  %rcx,%r8

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   mult += x1
# asm 1: add  <x1=int64#6,<mult=int64#5
# asm 2: add  <x1=%r9,<mult=%r8
add  %r9,%r8

# qhasm:   x1 = mult
# asm 1: mov  <mult=int64#5,>x1=int64#6
# asm 2: mov  <mult=%r8,>x1=%r9
mov  %r8,%r9

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   x0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<x0=int64#4
# asm 2: and  <mulredmask=%rdx,<x0=%rcx
and  %rdx,%rcx

# qhasm:   mult += x2
# asm 1: add  <x2=int64#9,<mult=int64#5
# asm 2: add  <x2=%r11,<mult=%r8
add  %r11,%r8

# qhasm:   x2 = mult
# asm 1: mov  <mult=int64#5,>x2=int64#7
# asm 2: mov  <mult=%r8,>x2=%rax
mov  %r8,%rax

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   x1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<x1=int64#6
# asm 2: and  <mulredmask=%rdx,<x1=%r9
and  %rdx,%r9

# qhasm:   mult += x3
# asm 1: add  <x3=int64#11,<mult=int64#5
# asm 2: add  <x3=%r13,<mult=%r8
add  %r13,%r8

# qhasm:   x3 = mult
# asm 1: mov  <mult=int64#5,>x3=int64#8
# asm 2: mov  <mult=%r8,>x3=%r10
mov  %r8,%r10

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   x2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<x2=int64#7
# asm 2: and  <mulredmask=%rdx,<x2=%rax
and  %rdx,%rax

# qhasm:   mult += x4
# asm 1: add  <x4=int64#13,<mult=int64#5
# asm 2: add  <x4=%r15,<mult=%r8
add  %r15,%r8

# qhasm:   x4 = mult
# asm 1: mov  <mult=int64#5,>x4=int64#9
# asm 2: mov  <mult=%r8,>x4=%r11
mov  %r8,%r11

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   x3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<x3=int64#8
# asm 2: and  <mulredmask=%rdx,<x3=%r10
and  %rdx,%r10

# qhasm:   mult *= 19
# asm 1: imulq  $19,<mult=int64#5,>mult=int64#5
# asm 2: imulq  $19,<mult=%r8,>mult=%r8
imulq  $19,%r8,%r8

# qhasm:   x0 += mult
# asm 1: add  <mult=int64#5,<x0=int64#4
# asm 2: add  <mult=%r8,<x0=%rcx
add  %r8,%rcx

# qhasm:   x4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<x4=int64#9
# asm 2: and  <mulredmask=%rdx,<x4=%r11
and  %rdx,%r11

# qhasm: stackx0 = x0
# asm 1: movq <x0=int64#4,>stackx0=stack64#8
# asm 2: movq <x0=%rcx,>stackx0=56(%rsp)
movq %rcx,56(%rsp)

# qhasm: stackx1 = x1
# asm 1: movq <x1=int64#6,>stackx1=stack64#9
# asm 2: movq <x1=%r9,>stackx1=64(%rsp)
movq %r9,64(%rsp)

# qhasm: stackx2 = x2
# asm 1: movq <x2=int64#7,>stackx2=stack64#10
# asm 2: movq <x2=%rax,>stackx2=72(%rsp)
movq %rax,72(%rsp)

# qhasm: stackx3 = x3
# asm 1: movq <x3=int64#8,>stackx3=stack64#11
# asm 2: movq <x3=%r10,>stackx3=80(%rsp)
movq %r10,80(%rsp)

# qhasm: stackx4 = x4
# asm 1: movq <x4=int64#9,>stackx4=stack64#12
# asm 2: movq <x4=%r11,>stackx4=88(%rsp)
movq %r11,88(%rsp)

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

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 56)
# asm 1: mulq  56(<pp=int64#2)
# asm 2: mulq  56(<pp=%rsi)
mulq  56(%rsi)

# qhasm:   y0 = mulrax
# asm 1: mov  <mulrax=int64#7,>y0=int64#4
# asm 2: mov  <mulrax=%rax,>y0=%rcx
mov  %rax,%rcx

# qhasm:   mulr01 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr01=int64#5
# asm 2: mov  <mulrdx=%rdx,>mulr01=%r8
mov  %rdx,%r8

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

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 48)
# asm 1: mulq  48(<pp=int64#2)
# asm 2: mulq  48(<pp=%rsi)
mulq  48(%rsi)

# qhasm:   carry? y0 += mulrax
# asm 1: add  <mulrax=int64#7,<y0=int64#4
# asm 2: add  <mulrax=%rax,<y0=%rcx
add  %rax,%rcx

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#5
# asm 2: adc <mulrdx=%rdx,<mulr01=%r8
adc %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 80)
# asm 1: movq   80(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   80(<pp=%rsi),>mulrax=%rax
movq   80(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 40)
# asm 1: mulq  40(<pp=int64#2)
# asm 2: mulq  40(<pp=%rsi)
mulq  40(%rsi)

# qhasm:   carry? y0 += mulrax
# asm 1: add  <mulrax=int64#7,<y0=int64#4
# asm 2: add  <mulrax=%rax,<y0=%rcx
add  %rax,%rcx

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#5
# asm 2: adc <mulrdx=%rdx,<mulr01=%r8
adc %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 80)
# asm 1: movq   80(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   80(<pp=%rsi),>mulrax=%rax
movq   80(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 48)
# asm 1: mulq  48(<pp=int64#2)
# asm 2: mulq  48(<pp=%rsi)
mulq  48(%rsi)

# qhasm:   y1 = mulrax
# asm 1: mov  <mulrax=int64#7,>y1=int64#6
# asm 2: mov  <mulrax=%rax,>y1=%r9
mov  %rax,%r9

# qhasm:   mulr11 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr11=int64#8
# asm 2: mov  <mulrdx=%rdx,>mulr11=%r10
mov  %rdx,%r10

# qhasm:   mulrax = *(uint64 *)(pp + 80)
# asm 1: movq   80(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   80(<pp=%rsi),>mulrax=%rax
movq   80(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 56)
# asm 1: mulq  56(<pp=int64#2)
# asm 2: mulq  56(<pp=%rsi)
mulq  56(%rsi)

# qhasm:   y2 = mulrax
# asm 1: mov  <mulrax=int64#7,>y2=int64#9
# asm 2: mov  <mulrax=%rax,>y2=%r11
mov  %rax,%r11

# qhasm:   mulr21 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr21=int64#10
# asm 2: mov  <mulrdx=%rdx,>mulr21=%r12
mov  %rdx,%r12

# qhasm:   mulrax = *(uint64 *)(pp + 80)
# asm 1: movq   80(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   80(<pp=%rsi),>mulrax=%rax
movq   80(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 64)
# asm 1: mulq  64(<pp=int64#2)
# asm 2: mulq  64(<pp=%rsi)
mulq  64(%rsi)

# qhasm:   y3 = mulrax
# asm 1: mov  <mulrax=int64#7,>y3=int64#11
# asm 2: mov  <mulrax=%rax,>y3=%r13
mov  %rax,%r13

# qhasm:   mulr31 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr31=int64#12
# asm 2: mov  <mulrdx=%rdx,>mulr31=%r14
mov  %rdx,%r14

# qhasm:   mulrax = *(uint64 *)(pp + 80)
# asm 1: movq   80(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   80(<pp=%rsi),>mulrax=%rax
movq   80(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 72)
# asm 1: mulq  72(<pp=int64#2)
# asm 2: mulq  72(<pp=%rsi)
mulq  72(%rsi)

# qhasm:   y4 = mulrax
# asm 1: mov  <mulrax=int64#7,>y4=int64#13
# asm 2: mov  <mulrax=%rax,>y4=%r15
mov  %rax,%r15

# qhasm:   mulr41 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr41=int64#14
# asm 2: mov  <mulrdx=%rdx,>mulr41=%rbx
mov  %rdx,%rbx

# qhasm:   mulrax = *(uint64 *)(pp + 88)
# asm 1: movq   88(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   88(<pp=%rsi),>mulrax=%rax
movq   88(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 40)
# asm 1: mulq  40(<pp=int64#2)
# asm 2: mulq  40(<pp=%rsi)
mulq  40(%rsi)

# qhasm:   carry? y1 += mulrax
# asm 1: add  <mulrax=int64#7,<y1=int64#6
# asm 2: add  <mulrax=%rax,<y1=%r9
add  %rax,%r9

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#8
# asm 2: adc <mulrdx=%rdx,<mulr11=%r10
adc %rdx,%r10

# qhasm:   mulrax = *(uint64 *)(pp + 88)
# asm 1: movq   88(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   88(<pp=%rsi),>mulrax=%rax
movq   88(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 48)
# asm 1: mulq  48(<pp=int64#2)
# asm 2: mulq  48(<pp=%rsi)
mulq  48(%rsi)

# qhasm:   carry? y2 += mulrax
# asm 1: add  <mulrax=int64#7,<y2=int64#9
# asm 2: add  <mulrax=%rax,<y2=%r11
add  %rax,%r11

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#10
# asm 2: adc <mulrdx=%rdx,<mulr21=%r12
adc %rdx,%r12

# qhasm:   mulrax = *(uint64 *)(pp + 88)
# asm 1: movq   88(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   88(<pp=%rsi),>mulrax=%rax
movq   88(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 56)
# asm 1: mulq  56(<pp=int64#2)
# asm 2: mulq  56(<pp=%rsi)
mulq  56(%rsi)

# qhasm:   carry? y3 += mulrax
# asm 1: add  <mulrax=int64#7,<y3=int64#11
# asm 2: add  <mulrax=%rax,<y3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulrax = *(uint64 *)(pp + 88)
# asm 1: movq   88(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   88(<pp=%rsi),>mulrax=%rax
movq   88(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 64)
# asm 1: mulq  64(<pp=int64#2)
# asm 2: mulq  64(<pp=%rsi)
mulq  64(%rsi)

# qhasm:   carry? y4 += mulrax
# asm 1: add  <mulrax=int64#7,<y4=int64#13
# asm 2: add  <mulrax=%rax,<y4=%r15
add  %rax,%r15

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#14
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbx
adc %rdx,%rbx

# qhasm:   mulrax = *(uint64 *)(pp + 88)
# asm 1: movq   88(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   88(<pp=%rsi),>mulrax=%rdx
movq   88(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 72)
# asm 1: mulq  72(<pp=int64#2)
# asm 2: mulq  72(<pp=%rsi)
mulq  72(%rsi)

# qhasm:   carry? y0 += mulrax
# asm 1: add  <mulrax=int64#7,<y0=int64#4
# asm 2: add  <mulrax=%rax,<y0=%rcx
add  %rax,%rcx

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#5
# asm 2: adc <mulrdx=%rdx,<mulr01=%r8
adc %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 96)
# asm 1: movq   96(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   96(<pp=%rsi),>mulrax=%rax
movq   96(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 40)
# asm 1: mulq  40(<pp=int64#2)
# asm 2: mulq  40(<pp=%rsi)
mulq  40(%rsi)

# qhasm:   carry? y2 += mulrax
# asm 1: add  <mulrax=int64#7,<y2=int64#9
# asm 2: add  <mulrax=%rax,<y2=%r11
add  %rax,%r11

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#10
# asm 2: adc <mulrdx=%rdx,<mulr21=%r12
adc %rdx,%r12

# qhasm:   mulrax = *(uint64 *)(pp + 96)
# asm 1: movq   96(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   96(<pp=%rsi),>mulrax=%rax
movq   96(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 48)
# asm 1: mulq  48(<pp=int64#2)
# asm 2: mulq  48(<pp=%rsi)
mulq  48(%rsi)

# qhasm:   carry? y3 += mulrax
# asm 1: add  <mulrax=int64#7,<y3=int64#11
# asm 2: add  <mulrax=%rax,<y3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulrax = *(uint64 *)(pp + 96)
# asm 1: movq   96(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   96(<pp=%rsi),>mulrax=%rax
movq   96(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 56)
# asm 1: mulq  56(<pp=int64#2)
# asm 2: mulq  56(<pp=%rsi)
mulq  56(%rsi)

# qhasm:   carry? y4 += mulrax
# asm 1: add  <mulrax=int64#7,<y4=int64#13
# asm 2: add  <mulrax=%rax,<y4=%r15
add  %rax,%r15

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#14
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbx
adc %rdx,%rbx

# qhasm:   mulrax = *(uint64 *)(pp + 96)
# asm 1: movq   96(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   96(<pp=%rsi),>mulrax=%rdx
movq   96(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 64)
# asm 1: mulq  64(<pp=int64#2)
# asm 2: mulq  64(<pp=%rsi)
mulq  64(%rsi)

# qhasm:   carry? y0 += mulrax
# asm 1: add  <mulrax=int64#7,<y0=int64#4
# asm 2: add  <mulrax=%rax,<y0=%rcx
add  %rax,%rcx

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#5
# asm 2: adc <mulrdx=%rdx,<mulr01=%r8
adc %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 96)
# asm 1: movq   96(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   96(<pp=%rsi),>mulrax=%rdx
movq   96(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 72)
# asm 1: mulq  72(<pp=int64#2)
# asm 2: mulq  72(<pp=%rsi)
mulq  72(%rsi)

# qhasm:   carry? y1 += mulrax
# asm 1: add  <mulrax=int64#7,<y1=int64#6
# asm 2: add  <mulrax=%rax,<y1=%r9
add  %rax,%r9

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#8
# asm 2: adc <mulrdx=%rdx,<mulr11=%r10
adc %rdx,%r10

# qhasm:   mulrax = *(uint64 *)(pp + 104)
# asm 1: movq   104(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   104(<pp=%rsi),>mulrax=%rax
movq   104(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 40)
# asm 1: mulq  40(<pp=int64#2)
# asm 2: mulq  40(<pp=%rsi)
mulq  40(%rsi)

# qhasm:   carry? y3 += mulrax
# asm 1: add  <mulrax=int64#7,<y3=int64#11
# asm 2: add  <mulrax=%rax,<y3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulrax = *(uint64 *)(pp + 104)
# asm 1: movq   104(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   104(<pp=%rsi),>mulrax=%rax
movq   104(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 48)
# asm 1: mulq  48(<pp=int64#2)
# asm 2: mulq  48(<pp=%rsi)
mulq  48(%rsi)

# qhasm:   carry? y4 += mulrax
# asm 1: add  <mulrax=int64#7,<y4=int64#13
# asm 2: add  <mulrax=%rax,<y4=%r15
add  %rax,%r15

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#14
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbx
adc %rdx,%rbx

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#13,>mulrax=int64#7
# asm 2: movq <mulx319_stack=96(%rsp),>mulrax=%rax
movq 96(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 64)
# asm 1: mulq  64(<pp=int64#2)
# asm 2: mulq  64(<pp=%rsi)
mulq  64(%rsi)

# qhasm:   carry? y1 += mulrax
# asm 1: add  <mulrax=int64#7,<y1=int64#6
# asm 2: add  <mulrax=%rax,<y1=%r9
add  %rax,%r9

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#8
# asm 2: adc <mulrdx=%rdx,<mulr11=%r10
adc %rdx,%r10

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#13,>mulrax=int64#7
# asm 2: movq <mulx319_stack=96(%rsp),>mulrax=%rax
movq 96(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 72)
# asm 1: mulq  72(<pp=int64#2)
# asm 2: mulq  72(<pp=%rsi)
mulq  72(%rsi)

# qhasm:   carry? y2 += mulrax
# asm 1: add  <mulrax=int64#7,<y2=int64#9
# asm 2: add  <mulrax=%rax,<y2=%r11
add  %rax,%r11

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#10
# asm 2: adc <mulrdx=%rdx,<mulr21=%r12
adc %rdx,%r12

# qhasm:   mulrax = *(uint64 *)(pp + 112)
# asm 1: movq   112(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   112(<pp=%rsi),>mulrax=%rax
movq   112(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 40)
# asm 1: mulq  40(<pp=int64#2)
# asm 2: mulq  40(<pp=%rsi)
mulq  40(%rsi)

# qhasm:   carry? y4 += mulrax
# asm 1: add  <mulrax=int64#7,<y4=int64#13
# asm 2: add  <mulrax=%rax,<y4=%r15
add  %rax,%r15

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#14
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbx
adc %rdx,%rbx

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#14,>mulrax=int64#7
# asm 2: movq <mulx419_stack=104(%rsp),>mulrax=%rax
movq 104(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 56)
# asm 1: mulq  56(<pp=int64#2)
# asm 2: mulq  56(<pp=%rsi)
mulq  56(%rsi)

# qhasm:   carry? y1 += mulrax
# asm 1: add  <mulrax=int64#7,<y1=int64#6
# asm 2: add  <mulrax=%rax,<y1=%r9
add  %rax,%r9

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#8
# asm 2: adc <mulrdx=%rdx,<mulr11=%r10
adc %rdx,%r10

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#14,>mulrax=int64#7
# asm 2: movq <mulx419_stack=104(%rsp),>mulrax=%rax
movq 104(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 64)
# asm 1: mulq  64(<pp=int64#2)
# asm 2: mulq  64(<pp=%rsi)
mulq  64(%rsi)

# qhasm:   carry? y2 += mulrax
# asm 1: add  <mulrax=int64#7,<y2=int64#9
# asm 2: add  <mulrax=%rax,<y2=%r11
add  %rax,%r11

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#10
# asm 2: adc <mulrdx=%rdx,<mulr21=%r12
adc %rdx,%r12

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#14,>mulrax=int64#7
# asm 2: movq <mulx419_stack=104(%rsp),>mulrax=%rax
movq 104(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 72)
# asm 1: mulq  72(<pp=int64#2)
# asm 2: mulq  72(<pp=%rsi)
mulq  72(%rsi)

# qhasm:   carry? y3 += mulrax
# asm 1: add  <mulrax=int64#7,<y3=int64#11
# asm 2: add  <mulrax=%rax,<y3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=int64#3
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=%rdx
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,%rdx

# qhasm:   mulr01 = (mulr01.y0) << 13
# asm 1: shld $13,<y0=int64#4,<mulr01=int64#5
# asm 2: shld $13,<y0=%rcx,<mulr01=%r8
shld $13,%rcx,%r8

# qhasm:   y0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<y0=int64#4
# asm 2: and  <mulredmask=%rdx,<y0=%rcx
and  %rdx,%rcx

# qhasm:   mulr11 = (mulr11.y1) << 13
# asm 1: shld $13,<y1=int64#6,<mulr11=int64#8
# asm 2: shld $13,<y1=%r9,<mulr11=%r10
shld $13,%r9,%r10

# qhasm:   y1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<y1=int64#6
# asm 2: and  <mulredmask=%rdx,<y1=%r9
and  %rdx,%r9

# qhasm:   y1 += mulr01
# asm 1: add  <mulr01=int64#5,<y1=int64#6
# asm 2: add  <mulr01=%r8,<y1=%r9
add  %r8,%r9

# qhasm:   mulr21 = (mulr21.y2) << 13
# asm 1: shld $13,<y2=int64#9,<mulr21=int64#10
# asm 2: shld $13,<y2=%r11,<mulr21=%r12
shld $13,%r11,%r12

# qhasm:   y2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<y2=int64#9
# asm 2: and  <mulredmask=%rdx,<y2=%r11
and  %rdx,%r11

# qhasm:   y2 += mulr11
# asm 1: add  <mulr11=int64#8,<y2=int64#9
# asm 2: add  <mulr11=%r10,<y2=%r11
add  %r10,%r11

# qhasm:   mulr31 = (mulr31.y3) << 13
# asm 1: shld $13,<y3=int64#11,<mulr31=int64#12
# asm 2: shld $13,<y3=%r13,<mulr31=%r14
shld $13,%r13,%r14

# qhasm:   y3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<y3=int64#11
# asm 2: and  <mulredmask=%rdx,<y3=%r13
and  %rdx,%r13

# qhasm:   y3 += mulr21
# asm 1: add  <mulr21=int64#10,<y3=int64#11
# asm 2: add  <mulr21=%r12,<y3=%r13
add  %r12,%r13

# qhasm:   mulr41 = (mulr41.y4) << 13
# asm 1: shld $13,<y4=int64#13,<mulr41=int64#14
# asm 2: shld $13,<y4=%r15,<mulr41=%rbx
shld $13,%r15,%rbx

# qhasm:   y4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<y4=int64#13
# asm 2: and  <mulredmask=%rdx,<y4=%r15
and  %rdx,%r15

# qhasm:   y4 += mulr31
# asm 1: add  <mulr31=int64#12,<y4=int64#13
# asm 2: add  <mulr31=%r14,<y4=%r15
add  %r14,%r15

# qhasm:   mulr41 = mulr41 * 19
# asm 1: imulq  $19,<mulr41=int64#14,>mulr41=int64#5
# asm 2: imulq  $19,<mulr41=%rbx,>mulr41=%r8
imulq  $19,%rbx,%r8

# qhasm:   y0 += mulr41
# asm 1: add  <mulr41=int64#5,<y0=int64#4
# asm 2: add  <mulr41=%r8,<y0=%rcx
add  %r8,%rcx

# qhasm:   mult = y0
# asm 1: mov  <y0=int64#4,>mult=int64#5
# asm 2: mov  <y0=%rcx,>mult=%r8
mov  %rcx,%r8

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   mult += y1
# asm 1: add  <y1=int64#6,<mult=int64#5
# asm 2: add  <y1=%r9,<mult=%r8
add  %r9,%r8

# qhasm:   y1 = mult
# asm 1: mov  <mult=int64#5,>y1=int64#6
# asm 2: mov  <mult=%r8,>y1=%r9
mov  %r8,%r9

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   y0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<y0=int64#4
# asm 2: and  <mulredmask=%rdx,<y0=%rcx
and  %rdx,%rcx

# qhasm:   mult += y2
# asm 1: add  <y2=int64#9,<mult=int64#5
# asm 2: add  <y2=%r11,<mult=%r8
add  %r11,%r8

# qhasm:   y2 = mult
# asm 1: mov  <mult=int64#5,>y2=int64#7
# asm 2: mov  <mult=%r8,>y2=%rax
mov  %r8,%rax

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   y1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<y1=int64#6
# asm 2: and  <mulredmask=%rdx,<y1=%r9
and  %rdx,%r9

# qhasm:   mult += y3
# asm 1: add  <y3=int64#11,<mult=int64#5
# asm 2: add  <y3=%r13,<mult=%r8
add  %r13,%r8

# qhasm:   y3 = mult
# asm 1: mov  <mult=int64#5,>y3=int64#8
# asm 2: mov  <mult=%r8,>y3=%r10
mov  %r8,%r10

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   y2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<y2=int64#7
# asm 2: and  <mulredmask=%rdx,<y2=%rax
and  %rdx,%rax

# qhasm:   mult += y4
# asm 1: add  <y4=int64#13,<mult=int64#5
# asm 2: add  <y4=%r15,<mult=%r8
add  %r15,%r8

# qhasm:   y4 = mult
# asm 1: mov  <mult=int64#5,>y4=int64#9
# asm 2: mov  <mult=%r8,>y4=%r11
mov  %r8,%r11

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   y3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<y3=int64#8
# asm 2: and  <mulredmask=%rdx,<y3=%r10
and  %rdx,%r10

# qhasm:   mult *= 19
# asm 1: imulq  $19,<mult=int64#5,>mult=int64#5
# asm 2: imulq  $19,<mult=%r8,>mult=%r8
imulq  $19,%r8,%r8

# qhasm:   y0 += mult
# asm 1: add  <mult=int64#5,<y0=int64#4
# asm 2: add  <mult=%r8,<y0=%rcx
add  %r8,%rcx

# qhasm:   y4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<y4=int64#9
# asm 2: and  <mulredmask=%rdx,<y4=%r11
and  %rdx,%r11

# qhasm: ysubx0 = y0
# asm 1: mov  <y0=int64#4,>ysubx0=int64#3
# asm 2: mov  <y0=%rcx,>ysubx0=%rdx
mov  %rcx,%rdx

# qhasm: ysubx1 = y1
# asm 1: mov  <y1=int64#6,>ysubx1=int64#5
# asm 2: mov  <y1=%r9,>ysubx1=%r8
mov  %r9,%r8

# qhasm: ysubx2 = y2
# asm 1: mov  <y2=int64#7,>ysubx2=int64#10
# asm 2: mov  <y2=%rax,>ysubx2=%r12
mov  %rax,%r12

# qhasm: ysubx3 = y3
# asm 1: mov  <y3=int64#8,>ysubx3=int64#11
# asm 2: mov  <y3=%r10,>ysubx3=%r13
mov  %r10,%r13

# qhasm: ysubx4 = y4
# asm 1: mov  <y4=int64#9,>ysubx4=int64#12
# asm 2: mov  <y4=%r11,>ysubx4=%r14
mov  %r11,%r14

# qhasm: ysubx0 += *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_2P0
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P0,<ysubx0=int64#3
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P0,<ysubx0=%rdx
add  crypto_sign_ed25519_amd64_51_30k_batch_2P0,%rdx

# qhasm: ysubx1 += *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<ysubx1=int64#5
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<ysubx1=%r8
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r8

# qhasm: ysubx2 += *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<ysubx2=int64#10
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<ysubx2=%r12
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r12

# qhasm: ysubx3 += *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<ysubx3=int64#11
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<ysubx3=%r13
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r13

# qhasm: ysubx4 += *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_2P1234
# asm 1: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<ysubx4=int64#12
# asm 2: add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,<ysubx4=%r14
add  crypto_sign_ed25519_amd64_51_30k_batch_2P1234,%r14

# qhasm: x0 = stackx0
# asm 1: movq <stackx0=stack64#8,>x0=int64#13
# asm 2: movq <stackx0=56(%rsp),>x0=%r15
movq 56(%rsp),%r15

# qhasm: ysubx0 -= x0
# asm 1: sub  <x0=int64#13,<ysubx0=int64#3
# asm 2: sub  <x0=%r15,<ysubx0=%rdx
sub  %r15,%rdx

# qhasm: y0 += x0
# asm 1: add  <x0=int64#13,<y0=int64#4
# asm 2: add  <x0=%r15,<y0=%rcx
add  %r15,%rcx

# qhasm: x1 = stackx1
# asm 1: movq <stackx1=stack64#9,>x1=int64#13
# asm 2: movq <stackx1=64(%rsp),>x1=%r15
movq 64(%rsp),%r15

# qhasm: ysubx1 -= x1
# asm 1: sub  <x1=int64#13,<ysubx1=int64#5
# asm 2: sub  <x1=%r15,<ysubx1=%r8
sub  %r15,%r8

# qhasm: y1 += x1
# asm 1: add  <x1=int64#13,<y1=int64#6
# asm 2: add  <x1=%r15,<y1=%r9
add  %r15,%r9

# qhasm: x2 = stackx2
# asm 1: movq <stackx2=stack64#10,>x2=int64#13
# asm 2: movq <stackx2=72(%rsp),>x2=%r15
movq 72(%rsp),%r15

# qhasm: ysubx2 -= x2
# asm 1: sub  <x2=int64#13,<ysubx2=int64#10
# asm 2: sub  <x2=%r15,<ysubx2=%r12
sub  %r15,%r12

# qhasm: y2 += x2
# asm 1: add  <x2=int64#13,<y2=int64#7
# asm 2: add  <x2=%r15,<y2=%rax
add  %r15,%rax

# qhasm: x3 = stackx3
# asm 1: movq <stackx3=stack64#11,>x3=int64#13
# asm 2: movq <stackx3=80(%rsp),>x3=%r15
movq 80(%rsp),%r15

# qhasm: ysubx3 -= x3
# asm 1: sub  <x3=int64#13,<ysubx3=int64#11
# asm 2: sub  <x3=%r15,<ysubx3=%r13
sub  %r15,%r13

# qhasm: y3 += x3
# asm 1: add  <x3=int64#13,<y3=int64#8
# asm 2: add  <x3=%r15,<y3=%r10
add  %r15,%r10

# qhasm: x4 = stackx4
# asm 1: movq <stackx4=stack64#12,>x4=int64#13
# asm 2: movq <stackx4=88(%rsp),>x4=%r15
movq 88(%rsp),%r15

# qhasm: ysubx4 -= x4
# asm 1: sub  <x4=int64#13,<ysubx4=int64#12
# asm 2: sub  <x4=%r15,<ysubx4=%r14
sub  %r15,%r14

# qhasm: y4 += x4
# asm 1: add  <x4=int64#13,<y4=int64#9
# asm 2: add  <x4=%r15,<y4=%r11
add  %r15,%r11

# qhasm: *(uint64 *)(rp + 0) = ysubx0
# asm 1: movq   <ysubx0=int64#3,0(<rp=int64#1)
# asm 2: movq   <ysubx0=%rdx,0(<rp=%rdi)
movq   %rdx,0(%rdi)

# qhasm: *(uint64 *)(rp + 8) = ysubx1
# asm 1: movq   <ysubx1=int64#5,8(<rp=int64#1)
# asm 2: movq   <ysubx1=%r8,8(<rp=%rdi)
movq   %r8,8(%rdi)

# qhasm: *(uint64 *)(rp + 16) = ysubx2
# asm 1: movq   <ysubx2=int64#10,16(<rp=int64#1)
# asm 2: movq   <ysubx2=%r12,16(<rp=%rdi)
movq   %r12,16(%rdi)

# qhasm: *(uint64 *)(rp + 24) = ysubx3
# asm 1: movq   <ysubx3=int64#11,24(<rp=int64#1)
# asm 2: movq   <ysubx3=%r13,24(<rp=%rdi)
movq   %r13,24(%rdi)

# qhasm: *(uint64 *)(rp + 32) = ysubx4
# asm 1: movq   <ysubx4=int64#12,32(<rp=int64#1)
# asm 2: movq   <ysubx4=%r14,32(<rp=%rdi)
movq   %r14,32(%rdi)

# qhasm: *(uint64 *)(rp + 40) = y0
# asm 1: movq   <y0=int64#4,40(<rp=int64#1)
# asm 2: movq   <y0=%rcx,40(<rp=%rdi)
movq   %rcx,40(%rdi)

# qhasm: *(uint64 *)(rp + 48) = y1
# asm 1: movq   <y1=int64#6,48(<rp=int64#1)
# asm 2: movq   <y1=%r9,48(<rp=%rdi)
movq   %r9,48(%rdi)

# qhasm: *(uint64 *)(rp + 56) = y2
# asm 1: movq   <y2=int64#7,56(<rp=int64#1)
# asm 2: movq   <y2=%rax,56(<rp=%rdi)
movq   %rax,56(%rdi)

# qhasm: *(uint64 *)(rp + 64) = y3
# asm 1: movq   <y3=int64#8,64(<rp=int64#1)
# asm 2: movq   <y3=%r10,64(<rp=%rdi)
movq   %r10,64(%rdi)

# qhasm: *(uint64 *)(rp + 72) = y4
# asm 1: movq   <y4=int64#9,72(<rp=int64#1)
# asm 2: movq   <y4=%r11,72(<rp=%rdi)
movq   %r11,72(%rdi)

# qhasm:   mulrax = *(uint64 *)(pp + 64)
# asm 1: movq   64(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   64(<pp=%rsi),>mulrax=%rdx
movq   64(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx319_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx319_stack=stack64#8
# asm 2: movq <mulrax=%rax,>mulx319_stack=56(%rsp)
movq %rax,56(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 136)
# asm 1: mulq  136(<pp=int64#2)
# asm 2: mulq  136(<pp=%rsi)
mulq  136(%rsi)

# qhasm:   rz0 = mulrax
# asm 1: mov  <mulrax=int64#7,>rz0=int64#4
# asm 2: mov  <mulrax=%rax,>rz0=%rcx
mov  %rax,%rcx

# qhasm:   mulr01 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr01=int64#5
# asm 2: mov  <mulrdx=%rdx,>mulr01=%r8
mov  %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 72)
# asm 1: movq   72(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   72(<pp=%rsi),>mulrax=%rdx
movq   72(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx419_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx419_stack=stack64#9
# asm 2: movq <mulrax=%rax,>mulx419_stack=64(%rsp)
movq %rax,64(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 128)
# asm 1: mulq  128(<pp=int64#2)
# asm 2: mulq  128(<pp=%rsi)
mulq  128(%rsi)

# qhasm:   carry? rz0 += mulrax
# asm 1: add  <mulrax=int64#7,<rz0=int64#4
# asm 2: add  <mulrax=%rax,<rz0=%rcx
add  %rax,%rcx

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#5
# asm 2: adc <mulrdx=%rdx,<mulr01=%r8
adc %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 40)
# asm 1: movq   40(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   40(<pp=%rsi),>mulrax=%rax
movq   40(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 120)
# asm 1: mulq  120(<pp=int64#2)
# asm 2: mulq  120(<pp=%rsi)
mulq  120(%rsi)

# qhasm:   carry? rz0 += mulrax
# asm 1: add  <mulrax=int64#7,<rz0=int64#4
# asm 2: add  <mulrax=%rax,<rz0=%rcx
add  %rax,%rcx

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#5
# asm 2: adc <mulrdx=%rdx,<mulr01=%r8
adc %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 40)
# asm 1: movq   40(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   40(<pp=%rsi),>mulrax=%rax
movq   40(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 128)
# asm 1: mulq  128(<pp=int64#2)
# asm 2: mulq  128(<pp=%rsi)
mulq  128(%rsi)

# qhasm:   rz1 = mulrax
# asm 1: mov  <mulrax=int64#7,>rz1=int64#6
# asm 2: mov  <mulrax=%rax,>rz1=%r9
mov  %rax,%r9

# qhasm:   mulr11 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr11=int64#8
# asm 2: mov  <mulrdx=%rdx,>mulr11=%r10
mov  %rdx,%r10

# qhasm:   mulrax = *(uint64 *)(pp + 40)
# asm 1: movq   40(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   40(<pp=%rsi),>mulrax=%rax
movq   40(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 136)
# asm 1: mulq  136(<pp=int64#2)
# asm 2: mulq  136(<pp=%rsi)
mulq  136(%rsi)

# qhasm:   rz2 = mulrax
# asm 1: mov  <mulrax=int64#7,>rz2=int64#9
# asm 2: mov  <mulrax=%rax,>rz2=%r11
mov  %rax,%r11

# qhasm:   mulr21 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr21=int64#10
# asm 2: mov  <mulrdx=%rdx,>mulr21=%r12
mov  %rdx,%r12

# qhasm:   mulrax = *(uint64 *)(pp + 40)
# asm 1: movq   40(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   40(<pp=%rsi),>mulrax=%rax
movq   40(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 144)
# asm 1: mulq  144(<pp=int64#2)
# asm 2: mulq  144(<pp=%rsi)
mulq  144(%rsi)

# qhasm:   rz3 = mulrax
# asm 1: mov  <mulrax=int64#7,>rz3=int64#11
# asm 2: mov  <mulrax=%rax,>rz3=%r13
mov  %rax,%r13

# qhasm:   mulr31 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr31=int64#12
# asm 2: mov  <mulrdx=%rdx,>mulr31=%r14
mov  %rdx,%r14

# qhasm:   mulrax = *(uint64 *)(pp + 40)
# asm 1: movq   40(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   40(<pp=%rsi),>mulrax=%rax
movq   40(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 152)
# asm 1: mulq  152(<pp=int64#2)
# asm 2: mulq  152(<pp=%rsi)
mulq  152(%rsi)

# qhasm:   rz4 = mulrax
# asm 1: mov  <mulrax=int64#7,>rz4=int64#13
# asm 2: mov  <mulrax=%rax,>rz4=%r15
mov  %rax,%r15

# qhasm:   mulr41 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr41=int64#14
# asm 2: mov  <mulrdx=%rdx,>mulr41=%rbx
mov  %rdx,%rbx

# qhasm:   mulrax = *(uint64 *)(pp + 48)
# asm 1: movq   48(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   48(<pp=%rsi),>mulrax=%rax
movq   48(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 120)
# asm 1: mulq  120(<pp=int64#2)
# asm 2: mulq  120(<pp=%rsi)
mulq  120(%rsi)

# qhasm:   carry? rz1 += mulrax
# asm 1: add  <mulrax=int64#7,<rz1=int64#6
# asm 2: add  <mulrax=%rax,<rz1=%r9
add  %rax,%r9

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#8
# asm 2: adc <mulrdx=%rdx,<mulr11=%r10
adc %rdx,%r10

# qhasm:   mulrax = *(uint64 *)(pp + 48)
# asm 1: movq   48(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   48(<pp=%rsi),>mulrax=%rax
movq   48(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 128)
# asm 1: mulq  128(<pp=int64#2)
# asm 2: mulq  128(<pp=%rsi)
mulq  128(%rsi)

# qhasm:   carry? rz2 += mulrax
# asm 1: add  <mulrax=int64#7,<rz2=int64#9
# asm 2: add  <mulrax=%rax,<rz2=%r11
add  %rax,%r11

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#10
# asm 2: adc <mulrdx=%rdx,<mulr21=%r12
adc %rdx,%r12

# qhasm:   mulrax = *(uint64 *)(pp + 48)
# asm 1: movq   48(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   48(<pp=%rsi),>mulrax=%rax
movq   48(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 136)
# asm 1: mulq  136(<pp=int64#2)
# asm 2: mulq  136(<pp=%rsi)
mulq  136(%rsi)

# qhasm:   carry? rz3 += mulrax
# asm 1: add  <mulrax=int64#7,<rz3=int64#11
# asm 2: add  <mulrax=%rax,<rz3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulrax = *(uint64 *)(pp + 48)
# asm 1: movq   48(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   48(<pp=%rsi),>mulrax=%rax
movq   48(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 144)
# asm 1: mulq  144(<pp=int64#2)
# asm 2: mulq  144(<pp=%rsi)
mulq  144(%rsi)

# qhasm:   carry? rz4 += mulrax
# asm 1: add  <mulrax=int64#7,<rz4=int64#13
# asm 2: add  <mulrax=%rax,<rz4=%r15
add  %rax,%r15

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#14
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbx
adc %rdx,%rbx

# qhasm:   mulrax = *(uint64 *)(pp + 48)
# asm 1: movq   48(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   48(<pp=%rsi),>mulrax=%rdx
movq   48(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 152)
# asm 1: mulq  152(<pp=int64#2)
# asm 2: mulq  152(<pp=%rsi)
mulq  152(%rsi)

# qhasm:   carry? rz0 += mulrax
# asm 1: add  <mulrax=int64#7,<rz0=int64#4
# asm 2: add  <mulrax=%rax,<rz0=%rcx
add  %rax,%rcx

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#5
# asm 2: adc <mulrdx=%rdx,<mulr01=%r8
adc %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 56)
# asm 1: movq   56(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   56(<pp=%rsi),>mulrax=%rax
movq   56(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 120)
# asm 1: mulq  120(<pp=int64#2)
# asm 2: mulq  120(<pp=%rsi)
mulq  120(%rsi)

# qhasm:   carry? rz2 += mulrax
# asm 1: add  <mulrax=int64#7,<rz2=int64#9
# asm 2: add  <mulrax=%rax,<rz2=%r11
add  %rax,%r11

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#10
# asm 2: adc <mulrdx=%rdx,<mulr21=%r12
adc %rdx,%r12

# qhasm:   mulrax = *(uint64 *)(pp + 56)
# asm 1: movq   56(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   56(<pp=%rsi),>mulrax=%rax
movq   56(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 128)
# asm 1: mulq  128(<pp=int64#2)
# asm 2: mulq  128(<pp=%rsi)
mulq  128(%rsi)

# qhasm:   carry? rz3 += mulrax
# asm 1: add  <mulrax=int64#7,<rz3=int64#11
# asm 2: add  <mulrax=%rax,<rz3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulrax = *(uint64 *)(pp + 56)
# asm 1: movq   56(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   56(<pp=%rsi),>mulrax=%rax
movq   56(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 136)
# asm 1: mulq  136(<pp=int64#2)
# asm 2: mulq  136(<pp=%rsi)
mulq  136(%rsi)

# qhasm:   carry? rz4 += mulrax
# asm 1: add  <mulrax=int64#7,<rz4=int64#13
# asm 2: add  <mulrax=%rax,<rz4=%r15
add  %rax,%r15

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#14
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbx
adc %rdx,%rbx

# qhasm:   mulrax = *(uint64 *)(pp + 56)
# asm 1: movq   56(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   56(<pp=%rsi),>mulrax=%rdx
movq   56(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 144)
# asm 1: mulq  144(<pp=int64#2)
# asm 2: mulq  144(<pp=%rsi)
mulq  144(%rsi)

# qhasm:   carry? rz0 += mulrax
# asm 1: add  <mulrax=int64#7,<rz0=int64#4
# asm 2: add  <mulrax=%rax,<rz0=%rcx
add  %rax,%rcx

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#5
# asm 2: adc <mulrdx=%rdx,<mulr01=%r8
adc %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 56)
# asm 1: movq   56(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   56(<pp=%rsi),>mulrax=%rdx
movq   56(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 152)
# asm 1: mulq  152(<pp=int64#2)
# asm 2: mulq  152(<pp=%rsi)
mulq  152(%rsi)

# qhasm:   carry? rz1 += mulrax
# asm 1: add  <mulrax=int64#7,<rz1=int64#6
# asm 2: add  <mulrax=%rax,<rz1=%r9
add  %rax,%r9

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#8
# asm 2: adc <mulrdx=%rdx,<mulr11=%r10
adc %rdx,%r10

# qhasm:   mulrax = *(uint64 *)(pp + 64)
# asm 1: movq   64(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   64(<pp=%rsi),>mulrax=%rax
movq   64(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 120)
# asm 1: mulq  120(<pp=int64#2)
# asm 2: mulq  120(<pp=%rsi)
mulq  120(%rsi)

# qhasm:   carry? rz3 += mulrax
# asm 1: add  <mulrax=int64#7,<rz3=int64#11
# asm 2: add  <mulrax=%rax,<rz3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulrax = *(uint64 *)(pp + 64)
# asm 1: movq   64(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   64(<pp=%rsi),>mulrax=%rax
movq   64(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 128)
# asm 1: mulq  128(<pp=int64#2)
# asm 2: mulq  128(<pp=%rsi)
mulq  128(%rsi)

# qhasm:   carry? rz4 += mulrax
# asm 1: add  <mulrax=int64#7,<rz4=int64#13
# asm 2: add  <mulrax=%rax,<rz4=%r15
add  %rax,%r15

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#14
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbx
adc %rdx,%rbx

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <mulx319_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 144)
# asm 1: mulq  144(<pp=int64#2)
# asm 2: mulq  144(<pp=%rsi)
mulq  144(%rsi)

# qhasm:   carry? rz1 += mulrax
# asm 1: add  <mulrax=int64#7,<rz1=int64#6
# asm 2: add  <mulrax=%rax,<rz1=%r9
add  %rax,%r9

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#8
# asm 2: adc <mulrdx=%rdx,<mulr11=%r10
adc %rdx,%r10

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <mulx319_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 152)
# asm 1: mulq  152(<pp=int64#2)
# asm 2: mulq  152(<pp=%rsi)
mulq  152(%rsi)

# qhasm:   carry? rz2 += mulrax
# asm 1: add  <mulrax=int64#7,<rz2=int64#9
# asm 2: add  <mulrax=%rax,<rz2=%r11
add  %rax,%r11

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#10
# asm 2: adc <mulrdx=%rdx,<mulr21=%r12
adc %rdx,%r12

# qhasm:   mulrax = *(uint64 *)(pp + 72)
# asm 1: movq   72(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   72(<pp=%rsi),>mulrax=%rax
movq   72(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 120)
# asm 1: mulq  120(<pp=int64#2)
# asm 2: mulq  120(<pp=%rsi)
mulq  120(%rsi)

# qhasm:   carry? rz4 += mulrax
# asm 1: add  <mulrax=int64#7,<rz4=int64#13
# asm 2: add  <mulrax=%rax,<rz4=%r15
add  %rax,%r15

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#14
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbx
adc %rdx,%rbx

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <mulx419_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 136)
# asm 1: mulq  136(<pp=int64#2)
# asm 2: mulq  136(<pp=%rsi)
mulq  136(%rsi)

# qhasm:   carry? rz1 += mulrax
# asm 1: add  <mulrax=int64#7,<rz1=int64#6
# asm 2: add  <mulrax=%rax,<rz1=%r9
add  %rax,%r9

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#8
# asm 2: adc <mulrdx=%rdx,<mulr11=%r10
adc %rdx,%r10

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <mulx419_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 144)
# asm 1: mulq  144(<pp=int64#2)
# asm 2: mulq  144(<pp=%rsi)
mulq  144(%rsi)

# qhasm:   carry? rz2 += mulrax
# asm 1: add  <mulrax=int64#7,<rz2=int64#9
# asm 2: add  <mulrax=%rax,<rz2=%r11
add  %rax,%r11

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#10
# asm 2: adc <mulrdx=%rdx,<mulr21=%r12
adc %rdx,%r12

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <mulx419_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 152)
# asm 1: mulq  152(<pp=int64#2)
# asm 2: mulq  152(<pp=%rsi)
mulq  152(%rsi)

# qhasm:   carry? rz3 += mulrax
# asm 1: add  <mulrax=int64#7,<rz3=int64#11
# asm 2: add  <mulrax=%rax,<rz3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=int64#3
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=%rdx
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,%rdx

# qhasm:   mulr01 = (mulr01.rz0) << 13
# asm 1: shld $13,<rz0=int64#4,<mulr01=int64#5
# asm 2: shld $13,<rz0=%rcx,<mulr01=%r8
shld $13,%rcx,%r8

# qhasm:   rz0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rz0=int64#4
# asm 2: and  <mulredmask=%rdx,<rz0=%rcx
and  %rdx,%rcx

# qhasm:   mulr11 = (mulr11.rz1) << 13
# asm 1: shld $13,<rz1=int64#6,<mulr11=int64#8
# asm 2: shld $13,<rz1=%r9,<mulr11=%r10
shld $13,%r9,%r10

# qhasm:   rz1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rz1=int64#6
# asm 2: and  <mulredmask=%rdx,<rz1=%r9
and  %rdx,%r9

# qhasm:   rz1 += mulr01
# asm 1: add  <mulr01=int64#5,<rz1=int64#6
# asm 2: add  <mulr01=%r8,<rz1=%r9
add  %r8,%r9

# qhasm:   mulr21 = (mulr21.rz2) << 13
# asm 1: shld $13,<rz2=int64#9,<mulr21=int64#10
# asm 2: shld $13,<rz2=%r11,<mulr21=%r12
shld $13,%r11,%r12

# qhasm:   rz2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rz2=int64#9
# asm 2: and  <mulredmask=%rdx,<rz2=%r11
and  %rdx,%r11

# qhasm:   rz2 += mulr11
# asm 1: add  <mulr11=int64#8,<rz2=int64#9
# asm 2: add  <mulr11=%r10,<rz2=%r11
add  %r10,%r11

# qhasm:   mulr31 = (mulr31.rz3) << 13
# asm 1: shld $13,<rz3=int64#11,<mulr31=int64#12
# asm 2: shld $13,<rz3=%r13,<mulr31=%r14
shld $13,%r13,%r14

# qhasm:   rz3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rz3=int64#11
# asm 2: and  <mulredmask=%rdx,<rz3=%r13
and  %rdx,%r13

# qhasm:   rz3 += mulr21
# asm 1: add  <mulr21=int64#10,<rz3=int64#11
# asm 2: add  <mulr21=%r12,<rz3=%r13
add  %r12,%r13

# qhasm:   mulr41 = (mulr41.rz4) << 13
# asm 1: shld $13,<rz4=int64#13,<mulr41=int64#14
# asm 2: shld $13,<rz4=%r15,<mulr41=%rbx
shld $13,%r15,%rbx

# qhasm:   rz4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rz4=int64#13
# asm 2: and  <mulredmask=%rdx,<rz4=%r15
and  %rdx,%r15

# qhasm:   rz4 += mulr31
# asm 1: add  <mulr31=int64#12,<rz4=int64#13
# asm 2: add  <mulr31=%r14,<rz4=%r15
add  %r14,%r15

# qhasm:   mulr41 = mulr41 * 19
# asm 1: imulq  $19,<mulr41=int64#14,>mulr41=int64#5
# asm 2: imulq  $19,<mulr41=%rbx,>mulr41=%r8
imulq  $19,%rbx,%r8

# qhasm:   rz0 += mulr41
# asm 1: add  <mulr41=int64#5,<rz0=int64#4
# asm 2: add  <mulr41=%r8,<rz0=%rcx
add  %r8,%rcx

# qhasm:   mult = rz0
# asm 1: mov  <rz0=int64#4,>mult=int64#5
# asm 2: mov  <rz0=%rcx,>mult=%r8
mov  %rcx,%r8

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   mult += rz1
# asm 1: add  <rz1=int64#6,<mult=int64#5
# asm 2: add  <rz1=%r9,<mult=%r8
add  %r9,%r8

# qhasm:   rz1 = mult
# asm 1: mov  <mult=int64#5,>rz1=int64#6
# asm 2: mov  <mult=%r8,>rz1=%r9
mov  %r8,%r9

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   rz0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rz0=int64#4
# asm 2: and  <mulredmask=%rdx,<rz0=%rcx
and  %rdx,%rcx

# qhasm:   mult += rz2
# asm 1: add  <rz2=int64#9,<mult=int64#5
# asm 2: add  <rz2=%r11,<mult=%r8
add  %r11,%r8

# qhasm:   rz2 = mult
# asm 1: mov  <mult=int64#5,>rz2=int64#7
# asm 2: mov  <mult=%r8,>rz2=%rax
mov  %r8,%rax

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   rz1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rz1=int64#6
# asm 2: and  <mulredmask=%rdx,<rz1=%r9
and  %rdx,%r9

# qhasm:   mult += rz3
# asm 1: add  <rz3=int64#11,<mult=int64#5
# asm 2: add  <rz3=%r13,<mult=%r8
add  %r13,%r8

# qhasm:   rz3 = mult
# asm 1: mov  <mult=int64#5,>rz3=int64#8
# asm 2: mov  <mult=%r8,>rz3=%r10
mov  %r8,%r10

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   rz2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rz2=int64#7
# asm 2: and  <mulredmask=%rdx,<rz2=%rax
and  %rdx,%rax

# qhasm:   mult += rz4
# asm 1: add  <rz4=int64#13,<mult=int64#5
# asm 2: add  <rz4=%r15,<mult=%r8
add  %r15,%r8

# qhasm:   rz4 = mult
# asm 1: mov  <mult=int64#5,>rz4=int64#9
# asm 2: mov  <mult=%r8,>rz4=%r11
mov  %r8,%r11

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   rz3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rz3=int64#8
# asm 2: and  <mulredmask=%rdx,<rz3=%r10
and  %rdx,%r10

# qhasm:   mult *= 19
# asm 1: imulq  $19,<mult=int64#5,>mult=int64#5
# asm 2: imulq  $19,<mult=%r8,>mult=%r8
imulq  $19,%r8,%r8

# qhasm:   rz0 += mult
# asm 1: add  <mult=int64#5,<rz0=int64#4
# asm 2: add  <mult=%r8,<rz0=%rcx
add  %r8,%rcx

# qhasm:   rz4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rz4=int64#9
# asm 2: and  <mulredmask=%rdx,<rz4=%r11
and  %rdx,%r11

# qhasm: *(uint64 *)(rp + 80) = rz0
# asm 1: movq   <rz0=int64#4,80(<rp=int64#1)
# asm 2: movq   <rz0=%rcx,80(<rp=%rdi)
movq   %rcx,80(%rdi)

# qhasm: *(uint64 *)(rp + 88) = rz1
# asm 1: movq   <rz1=int64#6,88(<rp=int64#1)
# asm 2: movq   <rz1=%r9,88(<rp=%rdi)
movq   %r9,88(%rdi)

# qhasm: *(uint64 *)(rp + 96) = rz2
# asm 1: movq   <rz2=int64#7,96(<rp=int64#1)
# asm 2: movq   <rz2=%rax,96(<rp=%rdi)
movq   %rax,96(%rdi)

# qhasm: *(uint64 *)(rp + 104) = rz3
# asm 1: movq   <rz3=int64#8,104(<rp=int64#1)
# asm 2: movq   <rz3=%r10,104(<rp=%rdi)
movq   %r10,104(%rdi)

# qhasm: *(uint64 *)(rp + 112) = rz4
# asm 1: movq   <rz4=int64#9,112(<rp=int64#1)
# asm 2: movq   <rz4=%r11,112(<rp=%rdi)
movq   %r11,112(%rdi)

# qhasm:   mulrax = *(uint64 *)(pp + 24)
# asm 1: movq   24(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   24(<pp=%rsi),>mulrax=%rdx
movq   24(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx319_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx319_stack=stack64#8
# asm 2: movq <mulrax=%rax,>mulx319_stack=56(%rsp)
movq %rax,56(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 96)
# asm 1: mulq  96(<pp=int64#2)
# asm 2: mulq  96(<pp=%rsi)
mulq  96(%rsi)

# qhasm:   t0 = mulrax
# asm 1: mov  <mulrax=int64#7,>t0=int64#4
# asm 2: mov  <mulrax=%rax,>t0=%rcx
mov  %rax,%rcx

# qhasm:   mulr01 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr01=int64#5
# asm 2: mov  <mulrdx=%rdx,>mulr01=%r8
mov  %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 32)
# asm 1: movq   32(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   32(<pp=%rsi),>mulrax=%rdx
movq   32(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx419_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx419_stack=stack64#9
# asm 2: movq <mulrax=%rax,>mulx419_stack=64(%rsp)
movq %rax,64(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 88)
# asm 1: mulq  88(<pp=int64#2)
# asm 2: mulq  88(<pp=%rsi)
mulq  88(%rsi)

# qhasm:   carry? t0 += mulrax
# asm 1: add  <mulrax=int64#7,<t0=int64#4
# asm 2: add  <mulrax=%rax,<t0=%rcx
add  %rax,%rcx

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#5
# asm 2: adc <mulrdx=%rdx,<mulr01=%r8
adc %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 0)
# asm 1: movq   0(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   0(<pp=%rsi),>mulrax=%rax
movq   0(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 80)
# asm 1: mulq  80(<pp=int64#2)
# asm 2: mulq  80(<pp=%rsi)
mulq  80(%rsi)

# qhasm:   carry? t0 += mulrax
# asm 1: add  <mulrax=int64#7,<t0=int64#4
# asm 2: add  <mulrax=%rax,<t0=%rcx
add  %rax,%rcx

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#5
# asm 2: adc <mulrdx=%rdx,<mulr01=%r8
adc %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 0)
# asm 1: movq   0(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   0(<pp=%rsi),>mulrax=%rax
movq   0(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 88)
# asm 1: mulq  88(<pp=int64#2)
# asm 2: mulq  88(<pp=%rsi)
mulq  88(%rsi)

# qhasm:   t1 = mulrax
# asm 1: mov  <mulrax=int64#7,>t1=int64#6
# asm 2: mov  <mulrax=%rax,>t1=%r9
mov  %rax,%r9

# qhasm:   mulr11 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr11=int64#8
# asm 2: mov  <mulrdx=%rdx,>mulr11=%r10
mov  %rdx,%r10

# qhasm:   mulrax = *(uint64 *)(pp + 0)
# asm 1: movq   0(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   0(<pp=%rsi),>mulrax=%rax
movq   0(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 96)
# asm 1: mulq  96(<pp=int64#2)
# asm 2: mulq  96(<pp=%rsi)
mulq  96(%rsi)

# qhasm:   t2 = mulrax
# asm 1: mov  <mulrax=int64#7,>t2=int64#9
# asm 2: mov  <mulrax=%rax,>t2=%r11
mov  %rax,%r11

# qhasm:   mulr21 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr21=int64#10
# asm 2: mov  <mulrdx=%rdx,>mulr21=%r12
mov  %rdx,%r12

# qhasm:   mulrax = *(uint64 *)(pp + 0)
# asm 1: movq   0(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   0(<pp=%rsi),>mulrax=%rax
movq   0(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 104)
# asm 1: mulq  104(<pp=int64#2)
# asm 2: mulq  104(<pp=%rsi)
mulq  104(%rsi)

# qhasm:   t3 = mulrax
# asm 1: mov  <mulrax=int64#7,>t3=int64#11
# asm 2: mov  <mulrax=%rax,>t3=%r13
mov  %rax,%r13

# qhasm:   mulr31 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr31=int64#12
# asm 2: mov  <mulrdx=%rdx,>mulr31=%r14
mov  %rdx,%r14

# qhasm:   mulrax = *(uint64 *)(pp + 0)
# asm 1: movq   0(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   0(<pp=%rsi),>mulrax=%rax
movq   0(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 112)
# asm 1: mulq  112(<pp=int64#2)
# asm 2: mulq  112(<pp=%rsi)
mulq  112(%rsi)

# qhasm:   t4 = mulrax
# asm 1: mov  <mulrax=int64#7,>t4=int64#13
# asm 2: mov  <mulrax=%rax,>t4=%r15
mov  %rax,%r15

# qhasm:   mulr41 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr41=int64#14
# asm 2: mov  <mulrdx=%rdx,>mulr41=%rbx
mov  %rdx,%rbx

# qhasm:   mulrax = *(uint64 *)(pp + 8)
# asm 1: movq   8(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   8(<pp=%rsi),>mulrax=%rax
movq   8(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 80)
# asm 1: mulq  80(<pp=int64#2)
# asm 2: mulq  80(<pp=%rsi)
mulq  80(%rsi)

# qhasm:   carry? t1 += mulrax
# asm 1: add  <mulrax=int64#7,<t1=int64#6
# asm 2: add  <mulrax=%rax,<t1=%r9
add  %rax,%r9

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#8
# asm 2: adc <mulrdx=%rdx,<mulr11=%r10
adc %rdx,%r10

# qhasm:   mulrax = *(uint64 *)(pp + 8)
# asm 1: movq   8(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   8(<pp=%rsi),>mulrax=%rax
movq   8(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 88)
# asm 1: mulq  88(<pp=int64#2)
# asm 2: mulq  88(<pp=%rsi)
mulq  88(%rsi)

# qhasm:   carry? t2 += mulrax
# asm 1: add  <mulrax=int64#7,<t2=int64#9
# asm 2: add  <mulrax=%rax,<t2=%r11
add  %rax,%r11

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#10
# asm 2: adc <mulrdx=%rdx,<mulr21=%r12
adc %rdx,%r12

# qhasm:   mulrax = *(uint64 *)(pp + 8)
# asm 1: movq   8(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   8(<pp=%rsi),>mulrax=%rax
movq   8(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 96)
# asm 1: mulq  96(<pp=int64#2)
# asm 2: mulq  96(<pp=%rsi)
mulq  96(%rsi)

# qhasm:   carry? t3 += mulrax
# asm 1: add  <mulrax=int64#7,<t3=int64#11
# asm 2: add  <mulrax=%rax,<t3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulrax = *(uint64 *)(pp + 8)
# asm 1: movq   8(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   8(<pp=%rsi),>mulrax=%rax
movq   8(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 104)
# asm 1: mulq  104(<pp=int64#2)
# asm 2: mulq  104(<pp=%rsi)
mulq  104(%rsi)

# qhasm:   carry? t4 += mulrax
# asm 1: add  <mulrax=int64#7,<t4=int64#13
# asm 2: add  <mulrax=%rax,<t4=%r15
add  %rax,%r15

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#14
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbx
adc %rdx,%rbx

# qhasm:   mulrax = *(uint64 *)(pp + 8)
# asm 1: movq   8(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   8(<pp=%rsi),>mulrax=%rdx
movq   8(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 112)
# asm 1: mulq  112(<pp=int64#2)
# asm 2: mulq  112(<pp=%rsi)
mulq  112(%rsi)

# qhasm:   carry? t0 += mulrax
# asm 1: add  <mulrax=int64#7,<t0=int64#4
# asm 2: add  <mulrax=%rax,<t0=%rcx
add  %rax,%rcx

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#5
# asm 2: adc <mulrdx=%rdx,<mulr01=%r8
adc %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 16)
# asm 1: movq   16(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   16(<pp=%rsi),>mulrax=%rax
movq   16(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 80)
# asm 1: mulq  80(<pp=int64#2)
# asm 2: mulq  80(<pp=%rsi)
mulq  80(%rsi)

# qhasm:   carry? t2 += mulrax
# asm 1: add  <mulrax=int64#7,<t2=int64#9
# asm 2: add  <mulrax=%rax,<t2=%r11
add  %rax,%r11

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#10
# asm 2: adc <mulrdx=%rdx,<mulr21=%r12
adc %rdx,%r12

# qhasm:   mulrax = *(uint64 *)(pp + 16)
# asm 1: movq   16(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   16(<pp=%rsi),>mulrax=%rax
movq   16(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 88)
# asm 1: mulq  88(<pp=int64#2)
# asm 2: mulq  88(<pp=%rsi)
mulq  88(%rsi)

# qhasm:   carry? t3 += mulrax
# asm 1: add  <mulrax=int64#7,<t3=int64#11
# asm 2: add  <mulrax=%rax,<t3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulrax = *(uint64 *)(pp + 16)
# asm 1: movq   16(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   16(<pp=%rsi),>mulrax=%rax
movq   16(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 96)
# asm 1: mulq  96(<pp=int64#2)
# asm 2: mulq  96(<pp=%rsi)
mulq  96(%rsi)

# qhasm:   carry? t4 += mulrax
# asm 1: add  <mulrax=int64#7,<t4=int64#13
# asm 2: add  <mulrax=%rax,<t4=%r15
add  %rax,%r15

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#14
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbx
adc %rdx,%rbx

# qhasm:   mulrax = *(uint64 *)(pp + 16)
# asm 1: movq   16(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   16(<pp=%rsi),>mulrax=%rdx
movq   16(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 104)
# asm 1: mulq  104(<pp=int64#2)
# asm 2: mulq  104(<pp=%rsi)
mulq  104(%rsi)

# qhasm:   carry? t0 += mulrax
# asm 1: add  <mulrax=int64#7,<t0=int64#4
# asm 2: add  <mulrax=%rax,<t0=%rcx
add  %rax,%rcx

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#5
# asm 2: adc <mulrdx=%rdx,<mulr01=%r8
adc %rdx,%r8

# qhasm:   mulrax = *(uint64 *)(pp + 16)
# asm 1: movq   16(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   16(<pp=%rsi),>mulrax=%rdx
movq   16(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 112)
# asm 1: mulq  112(<pp=int64#2)
# asm 2: mulq  112(<pp=%rsi)
mulq  112(%rsi)

# qhasm:   carry? t1 += mulrax
# asm 1: add  <mulrax=int64#7,<t1=int64#6
# asm 2: add  <mulrax=%rax,<t1=%r9
add  %rax,%r9

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#8
# asm 2: adc <mulrdx=%rdx,<mulr11=%r10
adc %rdx,%r10

# qhasm:   mulrax = *(uint64 *)(pp + 24)
# asm 1: movq   24(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   24(<pp=%rsi),>mulrax=%rax
movq   24(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 80)
# asm 1: mulq  80(<pp=int64#2)
# asm 2: mulq  80(<pp=%rsi)
mulq  80(%rsi)

# qhasm:   carry? t3 += mulrax
# asm 1: add  <mulrax=int64#7,<t3=int64#11
# asm 2: add  <mulrax=%rax,<t3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulrax = *(uint64 *)(pp + 24)
# asm 1: movq   24(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   24(<pp=%rsi),>mulrax=%rax
movq   24(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 88)
# asm 1: mulq  88(<pp=int64#2)
# asm 2: mulq  88(<pp=%rsi)
mulq  88(%rsi)

# qhasm:   carry? t4 += mulrax
# asm 1: add  <mulrax=int64#7,<t4=int64#13
# asm 2: add  <mulrax=%rax,<t4=%r15
add  %rax,%r15

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#14
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbx
adc %rdx,%rbx

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <mulx319_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 104)
# asm 1: mulq  104(<pp=int64#2)
# asm 2: mulq  104(<pp=%rsi)
mulq  104(%rsi)

# qhasm:   carry? t1 += mulrax
# asm 1: add  <mulrax=int64#7,<t1=int64#6
# asm 2: add  <mulrax=%rax,<t1=%r9
add  %rax,%r9

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#8
# asm 2: adc <mulrdx=%rdx,<mulr11=%r10
adc %rdx,%r10

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <mulx319_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 112)
# asm 1: mulq  112(<pp=int64#2)
# asm 2: mulq  112(<pp=%rsi)
mulq  112(%rsi)

# qhasm:   carry? t2 += mulrax
# asm 1: add  <mulrax=int64#7,<t2=int64#9
# asm 2: add  <mulrax=%rax,<t2=%r11
add  %rax,%r11

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#10
# asm 2: adc <mulrdx=%rdx,<mulr21=%r12
adc %rdx,%r12

# qhasm:   mulrax = *(uint64 *)(pp + 32)
# asm 1: movq   32(<pp=int64#2),>mulrax=int64#7
# asm 2: movq   32(<pp=%rsi),>mulrax=%rax
movq   32(%rsi),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 80)
# asm 1: mulq  80(<pp=int64#2)
# asm 2: mulq  80(<pp=%rsi)
mulq  80(%rsi)

# qhasm:   carry? t4 += mulrax
# asm 1: add  <mulrax=int64#7,<t4=int64#13
# asm 2: add  <mulrax=%rax,<t4=%r15
add  %rax,%r15

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#14
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbx
adc %rdx,%rbx

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <mulx419_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 96)
# asm 1: mulq  96(<pp=int64#2)
# asm 2: mulq  96(<pp=%rsi)
mulq  96(%rsi)

# qhasm:   carry? t1 += mulrax
# asm 1: add  <mulrax=int64#7,<t1=int64#6
# asm 2: add  <mulrax=%rax,<t1=%r9
add  %rax,%r9

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#8
# asm 2: adc <mulrdx=%rdx,<mulr11=%r10
adc %rdx,%r10

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <mulx419_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 104)
# asm 1: mulq  104(<pp=int64#2)
# asm 2: mulq  104(<pp=%rsi)
mulq  104(%rsi)

# qhasm:   carry? t2 += mulrax
# asm 1: add  <mulrax=int64#7,<t2=int64#9
# asm 2: add  <mulrax=%rax,<t2=%r11
add  %rax,%r11

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#10
# asm 2: adc <mulrdx=%rdx,<mulr21=%r12
adc %rdx,%r12

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <mulx419_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 112)
# asm 1: mulq  112(<pp=int64#2)
# asm 2: mulq  112(<pp=%rsi)
mulq  112(%rsi)

# qhasm:   carry? t3 += mulrax
# asm 1: add  <mulrax=int64#7,<t3=int64#11
# asm 2: add  <mulrax=%rax,<t3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=int64#2
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=%rsi
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,%rsi

# qhasm:   mulr01 = (mulr01.t0) << 13
# asm 1: shld $13,<t0=int64#4,<mulr01=int64#5
# asm 2: shld $13,<t0=%rcx,<mulr01=%r8
shld $13,%rcx,%r8

# qhasm:   t0 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<t0=int64#4
# asm 2: and  <mulredmask=%rsi,<t0=%rcx
and  %rsi,%rcx

# qhasm:   mulr11 = (mulr11.t1) << 13
# asm 1: shld $13,<t1=int64#6,<mulr11=int64#8
# asm 2: shld $13,<t1=%r9,<mulr11=%r10
shld $13,%r9,%r10

# qhasm:   t1 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<t1=int64#6
# asm 2: and  <mulredmask=%rsi,<t1=%r9
and  %rsi,%r9

# qhasm:   t1 += mulr01
# asm 1: add  <mulr01=int64#5,<t1=int64#6
# asm 2: add  <mulr01=%r8,<t1=%r9
add  %r8,%r9

# qhasm:   mulr21 = (mulr21.t2) << 13
# asm 1: shld $13,<t2=int64#9,<mulr21=int64#10
# asm 2: shld $13,<t2=%r11,<mulr21=%r12
shld $13,%r11,%r12

# qhasm:   t2 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<t2=int64#9
# asm 2: and  <mulredmask=%rsi,<t2=%r11
and  %rsi,%r11

# qhasm:   t2 += mulr11
# asm 1: add  <mulr11=int64#8,<t2=int64#9
# asm 2: add  <mulr11=%r10,<t2=%r11
add  %r10,%r11

# qhasm:   mulr31 = (mulr31.t3) << 13
# asm 1: shld $13,<t3=int64#11,<mulr31=int64#12
# asm 2: shld $13,<t3=%r13,<mulr31=%r14
shld $13,%r13,%r14

# qhasm:   t3 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<t3=int64#11
# asm 2: and  <mulredmask=%rsi,<t3=%r13
and  %rsi,%r13

# qhasm:   t3 += mulr21
# asm 1: add  <mulr21=int64#10,<t3=int64#11
# asm 2: add  <mulr21=%r12,<t3=%r13
add  %r12,%r13

# qhasm:   mulr41 = (mulr41.t4) << 13
# asm 1: shld $13,<t4=int64#13,<mulr41=int64#14
# asm 2: shld $13,<t4=%r15,<mulr41=%rbx
shld $13,%r15,%rbx

# qhasm:   t4 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<t4=int64#13
# asm 2: and  <mulredmask=%rsi,<t4=%r15
and  %rsi,%r15

# qhasm:   t4 += mulr31
# asm 1: add  <mulr31=int64#12,<t4=int64#13
# asm 2: add  <mulr31=%r14,<t4=%r15
add  %r14,%r15

# qhasm:   mulr41 = mulr41 * 19
# asm 1: imulq  $19,<mulr41=int64#14,>mulr41=int64#3
# asm 2: imulq  $19,<mulr41=%rbx,>mulr41=%rdx
imulq  $19,%rbx,%rdx

# qhasm:   t0 += mulr41
# asm 1: add  <mulr41=int64#3,<t0=int64#4
# asm 2: add  <mulr41=%rdx,<t0=%rcx
add  %rdx,%rcx

# qhasm:   mult = t0
# asm 1: mov  <t0=int64#4,>mult=int64#3
# asm 2: mov  <t0=%rcx,>mult=%rdx
mov  %rcx,%rdx

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#3
# asm 2: shr  $51,<mult=%rdx
shr  $51,%rdx

# qhasm:   mult += t1
# asm 1: add  <t1=int64#6,<mult=int64#3
# asm 2: add  <t1=%r9,<mult=%rdx
add  %r9,%rdx

# qhasm:   t1 = mult
# asm 1: mov  <mult=int64#3,>t1=int64#5
# asm 2: mov  <mult=%rdx,>t1=%r8
mov  %rdx,%r8

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#3
# asm 2: shr  $51,<mult=%rdx
shr  $51,%rdx

# qhasm:   t0 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<t0=int64#4
# asm 2: and  <mulredmask=%rsi,<t0=%rcx
and  %rsi,%rcx

# qhasm:   mult += t2
# asm 1: add  <t2=int64#9,<mult=int64#3
# asm 2: add  <t2=%r11,<mult=%rdx
add  %r11,%rdx

# qhasm:   t2 = mult
# asm 1: mov  <mult=int64#3,>t2=int64#6
# asm 2: mov  <mult=%rdx,>t2=%r9
mov  %rdx,%r9

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#3
# asm 2: shr  $51,<mult=%rdx
shr  $51,%rdx

# qhasm:   t1 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<t1=int64#5
# asm 2: and  <mulredmask=%rsi,<t1=%r8
and  %rsi,%r8

# qhasm:   mult += t3
# asm 1: add  <t3=int64#11,<mult=int64#3
# asm 2: add  <t3=%r13,<mult=%rdx
add  %r13,%rdx

# qhasm:   t3 = mult
# asm 1: mov  <mult=int64#3,>t3=int64#7
# asm 2: mov  <mult=%rdx,>t3=%rax
mov  %rdx,%rax

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#3
# asm 2: shr  $51,<mult=%rdx
shr  $51,%rdx

# qhasm:   t2 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<t2=int64#6
# asm 2: and  <mulredmask=%rsi,<t2=%r9
and  %rsi,%r9

# qhasm:   mult += t4
# asm 1: add  <t4=int64#13,<mult=int64#3
# asm 2: add  <t4=%r15,<mult=%rdx
add  %r15,%rdx

# qhasm:   t4 = mult
# asm 1: mov  <mult=int64#3,>t4=int64#8
# asm 2: mov  <mult=%rdx,>t4=%r10
mov  %rdx,%r10

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#3
# asm 2: shr  $51,<mult=%rdx
shr  $51,%rdx

# qhasm:   t3 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<t3=int64#7
# asm 2: and  <mulredmask=%rsi,<t3=%rax
and  %rsi,%rax

# qhasm:   mult *= 19
# asm 1: imulq  $19,<mult=int64#3,>mult=int64#3
# asm 2: imulq  $19,<mult=%rdx,>mult=%rdx
imulq  $19,%rdx,%rdx

# qhasm:   t0 += mult
# asm 1: add  <mult=int64#3,<t0=int64#4
# asm 2: add  <mult=%rdx,<t0=%rcx
add  %rdx,%rcx

# qhasm:   t4 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<t4=int64#8
# asm 2: and  <mulredmask=%rsi,<t4=%r10
and  %rsi,%r10

# qhasm: stackt0 = t0
# asm 1: movq <t0=int64#4,>stackt0=stack64#8
# asm 2: movq <t0=%rcx,>stackt0=56(%rsp)
movq %rcx,56(%rsp)

# qhasm: stackt1 = t1
# asm 1: movq <t1=int64#5,>stackt1=stack64#9
# asm 2: movq <t1=%r8,>stackt1=64(%rsp)
movq %r8,64(%rsp)

# qhasm: stackt2 = t2
# asm 1: movq <t2=int64#6,>stackt2=stack64#10
# asm 2: movq <t2=%r9,>stackt2=72(%rsp)
movq %r9,72(%rsp)

# qhasm: stackt3 = t3
# asm 1: movq <t3=int64#7,>stackt3=stack64#11
# asm 2: movq <t3=%rax,>stackt3=80(%rsp)
movq %rax,80(%rsp)

# qhasm: stackt4 = t4
# asm 1: movq <t4=int64#8,>stackt4=stack64#12
# asm 2: movq <t4=%r10,>stackt4=88(%rsp)
movq %r10,88(%rsp)

# qhasm:   mulrax = stackt3
# asm 1: movq <stackt3=stack64#11,>mulrax=int64#2
# asm 2: movq <stackt3=80(%rsp),>mulrax=%rsi
movq 80(%rsp),%rsi

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#2,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rsi,>mulrax=%rax
imulq  $19,%rsi,%rax

# qhasm:   mulx319_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx319_stack=stack64#13
# asm 2: movq <mulrax=%rax,>mulx319_stack=96(%rsp)
movq %rax,96(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D2
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D2

# qhasm:   t2d0 = mulrax
# asm 1: mov  <mulrax=int64#7,>t2d0=int64#2
# asm 2: mov  <mulrax=%rax,>t2d0=%rsi
mov  %rax,%rsi

# qhasm:   mulr01 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr01=int64#4
# asm 2: mov  <mulrdx=%rdx,>mulr01=%rcx
mov  %rdx,%rcx

# qhasm:   mulrax = stackt4
# asm 1: movq <stackt4=stack64#12,>mulrax=int64#3
# asm 2: movq <stackt4=88(%rsp),>mulrax=%rdx
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

# qhasm:   carry? t2d0 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d0=int64#2
# asm 2: add  <mulrax=%rax,<t2d0=%rsi
add  %rax,%rsi

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#4
# asm 2: adc <mulrdx=%rdx,<mulr01=%rcx
adc %rdx,%rcx

# qhasm:   mulrax = stackt0
# asm 1: movq <stackt0=stack64#8,>mulrax=int64#7
# asm 2: movq <stackt0=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D0
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D0

# qhasm:   carry? t2d0 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d0=int64#2
# asm 2: add  <mulrax=%rax,<t2d0=%rsi
add  %rax,%rsi

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#4
# asm 2: adc <mulrdx=%rdx,<mulr01=%rcx
adc %rdx,%rcx

# qhasm:   mulrax = stackt0
# asm 1: movq <stackt0=stack64#8,>mulrax=int64#7
# asm 2: movq <stackt0=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D1
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D1

# qhasm:   t2d1 = mulrax
# asm 1: mov  <mulrax=int64#7,>t2d1=int64#5
# asm 2: mov  <mulrax=%rax,>t2d1=%r8
mov  %rax,%r8

# qhasm:   mulr11 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr11=int64#6
# asm 2: mov  <mulrdx=%rdx,>mulr11=%r9
mov  %rdx,%r9

# qhasm:   mulrax = stackt0
# asm 1: movq <stackt0=stack64#8,>mulrax=int64#7
# asm 2: movq <stackt0=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D2
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D2

# qhasm:   t2d2 = mulrax
# asm 1: mov  <mulrax=int64#7,>t2d2=int64#8
# asm 2: mov  <mulrax=%rax,>t2d2=%r10
mov  %rax,%r10

# qhasm:   mulr21 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr21=int64#9
# asm 2: mov  <mulrdx=%rdx,>mulr21=%r11
mov  %rdx,%r11

# qhasm:   mulrax = stackt0
# asm 1: movq <stackt0=stack64#8,>mulrax=int64#7
# asm 2: movq <stackt0=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D3
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D3

# qhasm:   t2d3 = mulrax
# asm 1: mov  <mulrax=int64#7,>t2d3=int64#10
# asm 2: mov  <mulrax=%rax,>t2d3=%r12
mov  %rax,%r12

# qhasm:   mulr31 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr31=int64#11
# asm 2: mov  <mulrdx=%rdx,>mulr31=%r13
mov  %rdx,%r13

# qhasm:   mulrax = stackt0
# asm 1: movq <stackt0=stack64#8,>mulrax=int64#7
# asm 2: movq <stackt0=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D4
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D4

# qhasm:   t2d4 = mulrax
# asm 1: mov  <mulrax=int64#7,>t2d4=int64#12
# asm 2: mov  <mulrax=%rax,>t2d4=%r14
mov  %rax,%r14

# qhasm:   mulr41 = mulrdx
# asm 1: mov  <mulrdx=int64#3,>mulr41=int64#13
# asm 2: mov  <mulrdx=%rdx,>mulr41=%r15
mov  %rdx,%r15

# qhasm:   mulrax = stackt1
# asm 1: movq <stackt1=stack64#9,>mulrax=int64#7
# asm 2: movq <stackt1=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D0
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D0

# qhasm:   carry? t2d1 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d1=int64#5
# asm 2: add  <mulrax=%rax,<t2d1=%r8
add  %rax,%r8

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr11=%r9
adc %rdx,%r9

# qhasm:   mulrax = stackt1
# asm 1: movq <stackt1=stack64#9,>mulrax=int64#7
# asm 2: movq <stackt1=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D1
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D1

# qhasm:   carry? t2d2 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d2=int64#8
# asm 2: add  <mulrax=%rax,<t2d2=%r10
add  %rax,%r10

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr21=%r11
adc %rdx,%r11

# qhasm:   mulrax = stackt1
# asm 1: movq <stackt1=stack64#9,>mulrax=int64#7
# asm 2: movq <stackt1=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D2
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D2

# qhasm:   carry? t2d3 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d3=int64#10
# asm 2: add  <mulrax=%rax,<t2d3=%r12
add  %rax,%r12

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr31=%r13
adc %rdx,%r13

# qhasm:   mulrax = stackt1
# asm 1: movq <stackt1=stack64#9,>mulrax=int64#7
# asm 2: movq <stackt1=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D3
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D3

# qhasm:   carry? t2d4 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d4=int64#12
# asm 2: add  <mulrax=%rax,<t2d4=%r14
add  %rax,%r14

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr41=%r15
adc %rdx,%r15

# qhasm:   mulrax = stackt1
# asm 1: movq <stackt1=stack64#9,>mulrax=int64#3
# asm 2: movq <stackt1=64(%rsp),>mulrax=%rdx
movq 64(%rsp),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D4
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D4

# qhasm:   carry? t2d0 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d0=int64#2
# asm 2: add  <mulrax=%rax,<t2d0=%rsi
add  %rax,%rsi

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#4
# asm 2: adc <mulrdx=%rdx,<mulr01=%rcx
adc %rdx,%rcx

# qhasm:   mulrax = stackt2
# asm 1: movq <stackt2=stack64#10,>mulrax=int64#7
# asm 2: movq <stackt2=72(%rsp),>mulrax=%rax
movq 72(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D0
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D0

# qhasm:   carry? t2d2 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d2=int64#8
# asm 2: add  <mulrax=%rax,<t2d2=%r10
add  %rax,%r10

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr21=%r11
adc %rdx,%r11

# qhasm:   mulrax = stackt2
# asm 1: movq <stackt2=stack64#10,>mulrax=int64#7
# asm 2: movq <stackt2=72(%rsp),>mulrax=%rax
movq 72(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D1
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D1

# qhasm:   carry? t2d3 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d3=int64#10
# asm 2: add  <mulrax=%rax,<t2d3=%r12
add  %rax,%r12

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr31=%r13
adc %rdx,%r13

# qhasm:   mulrax = stackt2
# asm 1: movq <stackt2=stack64#10,>mulrax=int64#7
# asm 2: movq <stackt2=72(%rsp),>mulrax=%rax
movq 72(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D2
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D2

# qhasm:   carry? t2d4 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d4=int64#12
# asm 2: add  <mulrax=%rax,<t2d4=%r14
add  %rax,%r14

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr41=%r15
adc %rdx,%r15

# qhasm:   mulrax = stackt2
# asm 1: movq <stackt2=stack64#10,>mulrax=int64#3
# asm 2: movq <stackt2=72(%rsp),>mulrax=%rdx
movq 72(%rsp),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D3
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D3

# qhasm:   carry? t2d0 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d0=int64#2
# asm 2: add  <mulrax=%rax,<t2d0=%rsi
add  %rax,%rsi

# qhasm:   mulr01 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr01=int64#4
# asm 2: adc <mulrdx=%rdx,<mulr01=%rcx
adc %rdx,%rcx

# qhasm:   mulrax = stackt2
# asm 1: movq <stackt2=stack64#10,>mulrax=int64#3
# asm 2: movq <stackt2=72(%rsp),>mulrax=%rdx
movq 72(%rsp),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D4
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D4

# qhasm:   carry? t2d1 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d1=int64#5
# asm 2: add  <mulrax=%rax,<t2d1=%r8
add  %rax,%r8

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr11=%r9
adc %rdx,%r9

# qhasm:   mulrax = stackt3
# asm 1: movq <stackt3=stack64#11,>mulrax=int64#7
# asm 2: movq <stackt3=80(%rsp),>mulrax=%rax
movq 80(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D0
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D0

# qhasm:   carry? t2d3 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d3=int64#10
# asm 2: add  <mulrax=%rax,<t2d3=%r12
add  %rax,%r12

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr31=%r13
adc %rdx,%r13

# qhasm:   mulrax = stackt3
# asm 1: movq <stackt3=stack64#11,>mulrax=int64#7
# asm 2: movq <stackt3=80(%rsp),>mulrax=%rax
movq 80(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D1
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D1

# qhasm:   carry? t2d4 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d4=int64#12
# asm 2: add  <mulrax=%rax,<t2d4=%r14
add  %rax,%r14

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr41=%r15
adc %rdx,%r15

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#13,>mulrax=int64#7
# asm 2: movq <mulx319_stack=96(%rsp),>mulrax=%rax
movq 96(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D3
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D3

# qhasm:   carry? t2d1 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d1=int64#5
# asm 2: add  <mulrax=%rax,<t2d1=%r8
add  %rax,%r8

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr11=%r9
adc %rdx,%r9

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#13,>mulrax=int64#7
# asm 2: movq <mulx319_stack=96(%rsp),>mulrax=%rax
movq 96(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D4
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D4

# qhasm:   carry? t2d2 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d2=int64#8
# asm 2: add  <mulrax=%rax,<t2d2=%r10
add  %rax,%r10

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr21=%r11
adc %rdx,%r11

# qhasm:   mulrax = stackt4
# asm 1: movq <stackt4=stack64#12,>mulrax=int64#7
# asm 2: movq <stackt4=88(%rsp),>mulrax=%rax
movq 88(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D0
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D0

# qhasm:   carry? t2d4 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d4=int64#12
# asm 2: add  <mulrax=%rax,<t2d4=%r14
add  %rax,%r14

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#13
# asm 2: adc <mulrdx=%rdx,<mulr41=%r15
adc %rdx,%r15

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#14,>mulrax=int64#7
# asm 2: movq <mulx419_stack=104(%rsp),>mulrax=%rax
movq 104(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D2
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D2

# qhasm:   carry? t2d1 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d1=int64#5
# asm 2: add  <mulrax=%rax,<t2d1=%r8
add  %rax,%r8

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#6
# asm 2: adc <mulrdx=%rdx,<mulr11=%r9
adc %rdx,%r9

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#14,>mulrax=int64#7
# asm 2: movq <mulx419_stack=104(%rsp),>mulrax=%rax
movq 104(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D3
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D3

# qhasm:   carry? t2d2 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d2=int64#8
# asm 2: add  <mulrax=%rax,<t2d2=%r10
add  %rax,%r10

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#9
# asm 2: adc <mulrdx=%rdx,<mulr21=%r11
adc %rdx,%r11

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#14,>mulrax=int64#7
# asm 2: movq <mulx419_stack=104(%rsp),>mulrax=%rax
movq 104(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)&crypto_sign_ed25519_amd64_51_30k_batch_EC2D4
mulq  crypto_sign_ed25519_amd64_51_30k_batch_EC2D4

# qhasm:   carry? t2d3 += mulrax
# asm 1: add  <mulrax=int64#7,<t2d3=int64#10
# asm 2: add  <mulrax=%rax,<t2d3=%r12
add  %rax,%r12

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#11
# asm 2: adc <mulrdx=%rdx,<mulr31=%r13
adc %rdx,%r13

# qhasm:   mulredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=int64#3
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=%rdx
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,%rdx

# qhasm:   mulr01 = (mulr01.t2d0) << 13
# asm 1: shld $13,<t2d0=int64#2,<mulr01=int64#4
# asm 2: shld $13,<t2d0=%rsi,<mulr01=%rcx
shld $13,%rsi,%rcx

# qhasm:   t2d0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<t2d0=int64#2
# asm 2: and  <mulredmask=%rdx,<t2d0=%rsi
and  %rdx,%rsi

# qhasm:   mulr11 = (mulr11.t2d1) << 13
# asm 1: shld $13,<t2d1=int64#5,<mulr11=int64#6
# asm 2: shld $13,<t2d1=%r8,<mulr11=%r9
shld $13,%r8,%r9

# qhasm:   t2d1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<t2d1=int64#5
# asm 2: and  <mulredmask=%rdx,<t2d1=%r8
and  %rdx,%r8

# qhasm:   t2d1 += mulr01
# asm 1: add  <mulr01=int64#4,<t2d1=int64#5
# asm 2: add  <mulr01=%rcx,<t2d1=%r8
add  %rcx,%r8

# qhasm:   mulr21 = (mulr21.t2d2) << 13
# asm 1: shld $13,<t2d2=int64#8,<mulr21=int64#9
# asm 2: shld $13,<t2d2=%r10,<mulr21=%r11
shld $13,%r10,%r11

# qhasm:   t2d2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<t2d2=int64#8
# asm 2: and  <mulredmask=%rdx,<t2d2=%r10
and  %rdx,%r10

# qhasm:   t2d2 += mulr11
# asm 1: add  <mulr11=int64#6,<t2d2=int64#8
# asm 2: add  <mulr11=%r9,<t2d2=%r10
add  %r9,%r10

# qhasm:   mulr31 = (mulr31.t2d3) << 13
# asm 1: shld $13,<t2d3=int64#10,<mulr31=int64#11
# asm 2: shld $13,<t2d3=%r12,<mulr31=%r13
shld $13,%r12,%r13

# qhasm:   t2d3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<t2d3=int64#10
# asm 2: and  <mulredmask=%rdx,<t2d3=%r12
and  %rdx,%r12

# qhasm:   t2d3 += mulr21
# asm 1: add  <mulr21=int64#9,<t2d3=int64#10
# asm 2: add  <mulr21=%r11,<t2d3=%r12
add  %r11,%r12

# qhasm:   mulr41 = (mulr41.t2d4) << 13
# asm 1: shld $13,<t2d4=int64#12,<mulr41=int64#13
# asm 2: shld $13,<t2d4=%r14,<mulr41=%r15
shld $13,%r14,%r15

# qhasm:   t2d4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<t2d4=int64#12
# asm 2: and  <mulredmask=%rdx,<t2d4=%r14
and  %rdx,%r14

# qhasm:   t2d4 += mulr31
# asm 1: add  <mulr31=int64#11,<t2d4=int64#12
# asm 2: add  <mulr31=%r13,<t2d4=%r14
add  %r13,%r14

# qhasm:   mulr41 = mulr41 * 19
# asm 1: imulq  $19,<mulr41=int64#13,>mulr41=int64#4
# asm 2: imulq  $19,<mulr41=%r15,>mulr41=%rcx
imulq  $19,%r15,%rcx

# qhasm:   t2d0 += mulr41
# asm 1: add  <mulr41=int64#4,<t2d0=int64#2
# asm 2: add  <mulr41=%rcx,<t2d0=%rsi
add  %rcx,%rsi

# qhasm:   mult = t2d0
# asm 1: mov  <t2d0=int64#2,>mult=int64#4
# asm 2: mov  <t2d0=%rsi,>mult=%rcx
mov  %rsi,%rcx

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#4
# asm 2: shr  $51,<mult=%rcx
shr  $51,%rcx

# qhasm:   mult += t2d1
# asm 1: add  <t2d1=int64#5,<mult=int64#4
# asm 2: add  <t2d1=%r8,<mult=%rcx
add  %r8,%rcx

# qhasm:   t2d1 = mult
# asm 1: mov  <mult=int64#4,>t2d1=int64#5
# asm 2: mov  <mult=%rcx,>t2d1=%r8
mov  %rcx,%r8

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#4
# asm 2: shr  $51,<mult=%rcx
shr  $51,%rcx

# qhasm:   t2d0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<t2d0=int64#2
# asm 2: and  <mulredmask=%rdx,<t2d0=%rsi
and  %rdx,%rsi

# qhasm:   mult += t2d2
# asm 1: add  <t2d2=int64#8,<mult=int64#4
# asm 2: add  <t2d2=%r10,<mult=%rcx
add  %r10,%rcx

# qhasm:   t2d2 = mult
# asm 1: mov  <mult=int64#4,>t2d2=int64#6
# asm 2: mov  <mult=%rcx,>t2d2=%r9
mov  %rcx,%r9

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#4
# asm 2: shr  $51,<mult=%rcx
shr  $51,%rcx

# qhasm:   t2d1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<t2d1=int64#5
# asm 2: and  <mulredmask=%rdx,<t2d1=%r8
and  %rdx,%r8

# qhasm:   mult += t2d3
# asm 1: add  <t2d3=int64#10,<mult=int64#4
# asm 2: add  <t2d3=%r12,<mult=%rcx
add  %r12,%rcx

# qhasm:   t2d3 = mult
# asm 1: mov  <mult=int64#4,>t2d3=int64#7
# asm 2: mov  <mult=%rcx,>t2d3=%rax
mov  %rcx,%rax

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#4
# asm 2: shr  $51,<mult=%rcx
shr  $51,%rcx

# qhasm:   t2d2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<t2d2=int64#6
# asm 2: and  <mulredmask=%rdx,<t2d2=%r9
and  %rdx,%r9

# qhasm:   mult += t2d4
# asm 1: add  <t2d4=int64#12,<mult=int64#4
# asm 2: add  <t2d4=%r14,<mult=%rcx
add  %r14,%rcx

# qhasm:   t2d4 = mult
# asm 1: mov  <mult=int64#4,>t2d4=int64#8
# asm 2: mov  <mult=%rcx,>t2d4=%r10
mov  %rcx,%r10

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#4
# asm 2: shr  $51,<mult=%rcx
shr  $51,%rcx

# qhasm:   t2d3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<t2d3=int64#7
# asm 2: and  <mulredmask=%rdx,<t2d3=%rax
and  %rdx,%rax

# qhasm:   mult *= 19
# asm 1: imulq  $19,<mult=int64#4,>mult=int64#4
# asm 2: imulq  $19,<mult=%rcx,>mult=%rcx
imulq  $19,%rcx,%rcx

# qhasm:   t2d0 += mult
# asm 1: add  <mult=int64#4,<t2d0=int64#2
# asm 2: add  <mult=%rcx,<t2d0=%rsi
add  %rcx,%rsi

# qhasm:   t2d4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<t2d4=int64#8
# asm 2: and  <mulredmask=%rdx,<t2d4=%r10
and  %rdx,%r10

# qhasm: *(uint64 *)(rp + 120) = t2d0
# asm 1: movq   <t2d0=int64#2,120(<rp=int64#1)
# asm 2: movq   <t2d0=%rsi,120(<rp=%rdi)
movq   %rsi,120(%rdi)

# qhasm: *(uint64 *)(rp + 128) = t2d1
# asm 1: movq   <t2d1=int64#5,128(<rp=int64#1)
# asm 2: movq   <t2d1=%r8,128(<rp=%rdi)
movq   %r8,128(%rdi)

# qhasm: *(uint64 *)(rp + 136) = t2d2
# asm 1: movq   <t2d2=int64#6,136(<rp=int64#1)
# asm 2: movq   <t2d2=%r9,136(<rp=%rdi)
movq   %r9,136(%rdi)

# qhasm: *(uint64 *)(rp + 144) = t2d3
# asm 1: movq   <t2d3=int64#7,144(<rp=int64#1)
# asm 2: movq   <t2d3=%rax,144(<rp=%rdi)
movq   %rax,144(%rdi)

# qhasm: *(uint64 *)(rp + 152) = t2d4
# asm 1: movq   <t2d4=int64#8,152(<rp=int64#1)
# asm 2: movq   <t2d4=%r10,152(<rp=%rdi)
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
