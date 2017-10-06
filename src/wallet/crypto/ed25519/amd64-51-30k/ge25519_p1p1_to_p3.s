
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

# qhasm: enter crypto_sign_ed25519_amd64_51_30k_batch_ge25519_p1p1_to_p3
.text
.p2align 5
.globl _crypto_sign_ed25519_amd64_51_30k_batch_ge25519_p1p1_to_p3
.globl crypto_sign_ed25519_amd64_51_30k_batch_ge25519_p1p1_to_p3
_crypto_sign_ed25519_amd64_51_30k_batch_ge25519_p1p1_to_p3:
crypto_sign_ed25519_amd64_51_30k_batch_ge25519_p1p1_to_p3:
mov %rsp,%r11
and $31,%r11
add $96,%r11
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

# qhasm:   rx0 = mulrax
# asm 1: mov  <mulrax=int64#7,>rx0=int64#4
# asm 2: mov  <mulrax=%rax,>rx0=%rcx
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

# qhasm:   carry? rx0 += mulrax
# asm 1: add  <mulrax=int64#7,<rx0=int64#4
# asm 2: add  <mulrax=%rax,<rx0=%rcx
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

# qhasm:   carry? rx0 += mulrax
# asm 1: add  <mulrax=int64#7,<rx0=int64#4
# asm 2: add  <mulrax=%rax,<rx0=%rcx
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

# qhasm:   rx1 = mulrax
# asm 1: mov  <mulrax=int64#7,>rx1=int64#6
# asm 2: mov  <mulrax=%rax,>rx1=%r9
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

# qhasm:   rx2 = mulrax
# asm 1: mov  <mulrax=int64#7,>rx2=int64#9
# asm 2: mov  <mulrax=%rax,>rx2=%r11
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

# qhasm:   rx3 = mulrax
# asm 1: mov  <mulrax=int64#7,>rx3=int64#11
# asm 2: mov  <mulrax=%rax,>rx3=%r13
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

# qhasm:   rx4 = mulrax
# asm 1: mov  <mulrax=int64#7,>rx4=int64#13
# asm 2: mov  <mulrax=%rax,>rx4=%r15
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

# qhasm:   carry? rx1 += mulrax
# asm 1: add  <mulrax=int64#7,<rx1=int64#6
# asm 2: add  <mulrax=%rax,<rx1=%r9
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

# qhasm:   carry? rx2 += mulrax
# asm 1: add  <mulrax=int64#7,<rx2=int64#9
# asm 2: add  <mulrax=%rax,<rx2=%r11
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

# qhasm:   carry? rx3 += mulrax
# asm 1: add  <mulrax=int64#7,<rx3=int64#11
# asm 2: add  <mulrax=%rax,<rx3=%r13
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

# qhasm:   carry? rx4 += mulrax
# asm 1: add  <mulrax=int64#7,<rx4=int64#13
# asm 2: add  <mulrax=%rax,<rx4=%r15
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

# qhasm:   carry? rx0 += mulrax
# asm 1: add  <mulrax=int64#7,<rx0=int64#4
# asm 2: add  <mulrax=%rax,<rx0=%rcx
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

# qhasm:   carry? rx2 += mulrax
# asm 1: add  <mulrax=int64#7,<rx2=int64#9
# asm 2: add  <mulrax=%rax,<rx2=%r11
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

# qhasm:   carry? rx3 += mulrax
# asm 1: add  <mulrax=int64#7,<rx3=int64#11
# asm 2: add  <mulrax=%rax,<rx3=%r13
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

# qhasm:   carry? rx4 += mulrax
# asm 1: add  <mulrax=int64#7,<rx4=int64#13
# asm 2: add  <mulrax=%rax,<rx4=%r15
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

# qhasm:   carry? rx0 += mulrax
# asm 1: add  <mulrax=int64#7,<rx0=int64#4
# asm 2: add  <mulrax=%rax,<rx0=%rcx
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

# qhasm:   carry? rx1 += mulrax
# asm 1: add  <mulrax=int64#7,<rx1=int64#6
# asm 2: add  <mulrax=%rax,<rx1=%r9
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

# qhasm:   carry? rx3 += mulrax
# asm 1: add  <mulrax=int64#7,<rx3=int64#11
# asm 2: add  <mulrax=%rax,<rx3=%r13
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

# qhasm:   carry? rx4 += mulrax
# asm 1: add  <mulrax=int64#7,<rx4=int64#13
# asm 2: add  <mulrax=%rax,<rx4=%r15
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

# qhasm:   carry? rx1 += mulrax
# asm 1: add  <mulrax=int64#7,<rx1=int64#6
# asm 2: add  <mulrax=%rax,<rx1=%r9
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

# qhasm:   carry? rx2 += mulrax
# asm 1: add  <mulrax=int64#7,<rx2=int64#9
# asm 2: add  <mulrax=%rax,<rx2=%r11
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

# qhasm:   carry? rx4 += mulrax
# asm 1: add  <mulrax=int64#7,<rx4=int64#13
# asm 2: add  <mulrax=%rax,<rx4=%r15
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

# qhasm:   carry? rx1 += mulrax
# asm 1: add  <mulrax=int64#7,<rx1=int64#6
# asm 2: add  <mulrax=%rax,<rx1=%r9
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

# qhasm:   carry? rx2 += mulrax
# asm 1: add  <mulrax=int64#7,<rx2=int64#9
# asm 2: add  <mulrax=%rax,<rx2=%r11
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

# qhasm:   carry? rx3 += mulrax
# asm 1: add  <mulrax=int64#7,<rx3=int64#11
# asm 2: add  <mulrax=%rax,<rx3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=int64#3
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=%rdx
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51(%rip),%rdx

# qhasm:   mulr01 = (mulr01.rx0) << 13
# asm 1: shld $13,<rx0=int64#4,<mulr01=int64#5
# asm 2: shld $13,<rx0=%rcx,<mulr01=%r8
shld $13,%rcx,%r8

# qhasm:   rx0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx0=int64#4
# asm 2: and  <mulredmask=%rdx,<rx0=%rcx
and  %rdx,%rcx

# qhasm:   mulr11 = (mulr11.rx1) << 13
# asm 1: shld $13,<rx1=int64#6,<mulr11=int64#8
# asm 2: shld $13,<rx1=%r9,<mulr11=%r10
shld $13,%r9,%r10

# qhasm:   rx1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx1=int64#6
# asm 2: and  <mulredmask=%rdx,<rx1=%r9
and  %rdx,%r9

# qhasm:   rx1 += mulr01
# asm 1: add  <mulr01=int64#5,<rx1=int64#6
# asm 2: add  <mulr01=%r8,<rx1=%r9
add  %r8,%r9

# qhasm:   mulr21 = (mulr21.rx2) << 13
# asm 1: shld $13,<rx2=int64#9,<mulr21=int64#10
# asm 2: shld $13,<rx2=%r11,<mulr21=%r12
shld $13,%r11,%r12

# qhasm:   rx2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx2=int64#9
# asm 2: and  <mulredmask=%rdx,<rx2=%r11
and  %rdx,%r11

# qhasm:   rx2 += mulr11
# asm 1: add  <mulr11=int64#8,<rx2=int64#9
# asm 2: add  <mulr11=%r10,<rx2=%r11
add  %r10,%r11

# qhasm:   mulr31 = (mulr31.rx3) << 13
# asm 1: shld $13,<rx3=int64#11,<mulr31=int64#12
# asm 2: shld $13,<rx3=%r13,<mulr31=%r14
shld $13,%r13,%r14

# qhasm:   rx3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx3=int64#11
# asm 2: and  <mulredmask=%rdx,<rx3=%r13
and  %rdx,%r13

# qhasm:   rx3 += mulr21
# asm 1: add  <mulr21=int64#10,<rx3=int64#11
# asm 2: add  <mulr21=%r12,<rx3=%r13
add  %r12,%r13

# qhasm:   mulr41 = (mulr41.rx4) << 13
# asm 1: shld $13,<rx4=int64#13,<mulr41=int64#14
# asm 2: shld $13,<rx4=%r15,<mulr41=%rbx
shld $13,%r15,%rbx

# qhasm:   rx4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx4=int64#13
# asm 2: and  <mulredmask=%rdx,<rx4=%r15
and  %rdx,%r15

# qhasm:   rx4 += mulr31
# asm 1: add  <mulr31=int64#12,<rx4=int64#13
# asm 2: add  <mulr31=%r14,<rx4=%r15
add  %r14,%r15

# qhasm:   mulr41 = mulr41 * 19
# asm 1: imulq  $19,<mulr41=int64#14,>mulr41=int64#5
# asm 2: imulq  $19,<mulr41=%rbx,>mulr41=%r8
imulq  $19,%rbx,%r8

# qhasm:   rx0 += mulr41
# asm 1: add  <mulr41=int64#5,<rx0=int64#4
# asm 2: add  <mulr41=%r8,<rx0=%rcx
add  %r8,%rcx

# qhasm:   mult = rx0
# asm 1: mov  <rx0=int64#4,>mult=int64#5
# asm 2: mov  <rx0=%rcx,>mult=%r8
mov  %rcx,%r8

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   mult += rx1
# asm 1: add  <rx1=int64#6,<mult=int64#5
# asm 2: add  <rx1=%r9,<mult=%r8
add  %r9,%r8

# qhasm:   rx1 = mult
# asm 1: mov  <mult=int64#5,>rx1=int64#6
# asm 2: mov  <mult=%r8,>rx1=%r9
mov  %r8,%r9

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   rx0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx0=int64#4
# asm 2: and  <mulredmask=%rdx,<rx0=%rcx
and  %rdx,%rcx

# qhasm:   mult += rx2
# asm 1: add  <rx2=int64#9,<mult=int64#5
# asm 2: add  <rx2=%r11,<mult=%r8
add  %r11,%r8

# qhasm:   rx2 = mult
# asm 1: mov  <mult=int64#5,>rx2=int64#7
# asm 2: mov  <mult=%r8,>rx2=%rax
mov  %r8,%rax

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   rx1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx1=int64#6
# asm 2: and  <mulredmask=%rdx,<rx1=%r9
and  %rdx,%r9

# qhasm:   mult += rx3
# asm 1: add  <rx3=int64#11,<mult=int64#5
# asm 2: add  <rx3=%r13,<mult=%r8
add  %r13,%r8

# qhasm:   rx3 = mult
# asm 1: mov  <mult=int64#5,>rx3=int64#8
# asm 2: mov  <mult=%r8,>rx3=%r10
mov  %r8,%r10

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   rx2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx2=int64#7
# asm 2: and  <mulredmask=%rdx,<rx2=%rax
and  %rdx,%rax

# qhasm:   mult += rx4
# asm 1: add  <rx4=int64#13,<mult=int64#5
# asm 2: add  <rx4=%r15,<mult=%r8
add  %r15,%r8

# qhasm:   rx4 = mult
# asm 1: mov  <mult=int64#5,>rx4=int64#9
# asm 2: mov  <mult=%r8,>rx4=%r11
mov  %r8,%r11

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   rx3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx3=int64#8
# asm 2: and  <mulredmask=%rdx,<rx3=%r10
and  %rdx,%r10

# qhasm:   mult *= 19
# asm 1: imulq  $19,<mult=int64#5,>mult=int64#5
# asm 2: imulq  $19,<mult=%r8,>mult=%r8
imulq  $19,%r8,%r8

# qhasm:   rx0 += mult
# asm 1: add  <mult=int64#5,<rx0=int64#4
# asm 2: add  <mult=%r8,<rx0=%rcx
add  %r8,%rcx

# qhasm:   rx4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<rx4=int64#9
# asm 2: and  <mulredmask=%rdx,<rx4=%r11
and  %rdx,%r11

# qhasm: *(uint64 *)(rp + 0) = rx0
# asm 1: movq   <rx0=int64#4,0(<rp=int64#1)
# asm 2: movq   <rx0=%rcx,0(<rp=%rdi)
movq   %rcx,0(%rdi)

# qhasm: *(uint64 *)(rp + 8) = rx1
# asm 1: movq   <rx1=int64#6,8(<rp=int64#1)
# asm 2: movq   <rx1=%r9,8(<rp=%rdi)
movq   %r9,8(%rdi)

# qhasm: *(uint64 *)(rp + 16) = rx2
# asm 1: movq   <rx2=int64#7,16(<rp=int64#1)
# asm 2: movq   <rx2=%rax,16(<rp=%rdi)
movq   %rax,16(%rdi)

# qhasm: *(uint64 *)(rp + 24) = rx3
# asm 1: movq   <rx3=int64#8,24(<rp=int64#1)
# asm 2: movq   <rx3=%r10,24(<rp=%rdi)
movq   %r10,24(%rdi)

# qhasm: *(uint64 *)(rp + 32) = rx4
# asm 1: movq   <rx4=int64#9,32(<rp=int64#1)
# asm 2: movq   <rx4=%r11,32(<rp=%rdi)
movq   %r11,32(%rdi)

# qhasm:   mulrax = *(uint64 *)(pp + 104)
# asm 1: movq   104(<pp=int64#2),>mulrax=int64#3
# asm 2: movq   104(<pp=%rsi),>mulrax=%rdx
movq   104(%rsi),%rdx

# qhasm:   mulrax *= 19
# asm 1: imulq  $19,<mulrax=int64#3,>mulrax=int64#7
# asm 2: imulq  $19,<mulrax=%rdx,>mulrax=%rax
imulq  $19,%rdx,%rax

# qhasm:   mulx319_stack = mulrax
# asm 1: movq <mulrax=int64#7,>mulx319_stack=stack64#8
# asm 2: movq <mulrax=%rax,>mulx319_stack=56(%rsp)
movq %rax,56(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 56)
# asm 1: mulq  56(<pp=int64#2)
# asm 2: mulq  56(<pp=%rsi)
mulq  56(%rsi)

# qhasm:   ry0 = mulrax
# asm 1: mov  <mulrax=int64#7,>ry0=int64#4
# asm 2: mov  <mulrax=%rax,>ry0=%rcx
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
# asm 1: movq <mulrax=int64#7,>mulx419_stack=stack64#9
# asm 2: movq <mulrax=%rax,>mulx419_stack=64(%rsp)
movq %rax,64(%rsp)

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 48)
# asm 1: mulq  48(<pp=int64#2)
# asm 2: mulq  48(<pp=%rsi)
mulq  48(%rsi)

# qhasm:   carry? ry0 += mulrax
# asm 1: add  <mulrax=int64#7,<ry0=int64#4
# asm 2: add  <mulrax=%rax,<ry0=%rcx
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

# qhasm:   carry? ry0 += mulrax
# asm 1: add  <mulrax=int64#7,<ry0=int64#4
# asm 2: add  <mulrax=%rax,<ry0=%rcx
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

# qhasm:   ry1 = mulrax
# asm 1: mov  <mulrax=int64#7,>ry1=int64#6
# asm 2: mov  <mulrax=%rax,>ry1=%r9
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

# qhasm:   ry2 = mulrax
# asm 1: mov  <mulrax=int64#7,>ry2=int64#9
# asm 2: mov  <mulrax=%rax,>ry2=%r11
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

# qhasm:   ry3 = mulrax
# asm 1: mov  <mulrax=int64#7,>ry3=int64#11
# asm 2: mov  <mulrax=%rax,>ry3=%r13
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

# qhasm:   ry4 = mulrax
# asm 1: mov  <mulrax=int64#7,>ry4=int64#13
# asm 2: mov  <mulrax=%rax,>ry4=%r15
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

# qhasm:   carry? ry1 += mulrax
# asm 1: add  <mulrax=int64#7,<ry1=int64#6
# asm 2: add  <mulrax=%rax,<ry1=%r9
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

# qhasm:   carry? ry2 += mulrax
# asm 1: add  <mulrax=int64#7,<ry2=int64#9
# asm 2: add  <mulrax=%rax,<ry2=%r11
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

# qhasm:   carry? ry3 += mulrax
# asm 1: add  <mulrax=int64#7,<ry3=int64#11
# asm 2: add  <mulrax=%rax,<ry3=%r13
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

# qhasm:   carry? ry4 += mulrax
# asm 1: add  <mulrax=int64#7,<ry4=int64#13
# asm 2: add  <mulrax=%rax,<ry4=%r15
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

# qhasm:   carry? ry0 += mulrax
# asm 1: add  <mulrax=int64#7,<ry0=int64#4
# asm 2: add  <mulrax=%rax,<ry0=%rcx
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

# qhasm:   carry? ry2 += mulrax
# asm 1: add  <mulrax=int64#7,<ry2=int64#9
# asm 2: add  <mulrax=%rax,<ry2=%r11
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

# qhasm:   carry? ry3 += mulrax
# asm 1: add  <mulrax=int64#7,<ry3=int64#11
# asm 2: add  <mulrax=%rax,<ry3=%r13
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

# qhasm:   carry? ry4 += mulrax
# asm 1: add  <mulrax=int64#7,<ry4=int64#13
# asm 2: add  <mulrax=%rax,<ry4=%r15
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

# qhasm:   carry? ry0 += mulrax
# asm 1: add  <mulrax=int64#7,<ry0=int64#4
# asm 2: add  <mulrax=%rax,<ry0=%rcx
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

# qhasm:   carry? ry1 += mulrax
# asm 1: add  <mulrax=int64#7,<ry1=int64#6
# asm 2: add  <mulrax=%rax,<ry1=%r9
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

# qhasm:   carry? ry3 += mulrax
# asm 1: add  <mulrax=int64#7,<ry3=int64#11
# asm 2: add  <mulrax=%rax,<ry3=%r13
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

# qhasm:   carry? ry4 += mulrax
# asm 1: add  <mulrax=int64#7,<ry4=int64#13
# asm 2: add  <mulrax=%rax,<ry4=%r15
add  %rax,%r15

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#14
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbx
adc %rdx,%rbx

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <mulx319_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 64)
# asm 1: mulq  64(<pp=int64#2)
# asm 2: mulq  64(<pp=%rsi)
mulq  64(%rsi)

# qhasm:   carry? ry1 += mulrax
# asm 1: add  <mulrax=int64#7,<ry1=int64#6
# asm 2: add  <mulrax=%rax,<ry1=%r9
add  %rax,%r9

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#8
# asm 2: adc <mulrdx=%rdx,<mulr11=%r10
adc %rdx,%r10

# qhasm:   mulrax = mulx319_stack
# asm 1: movq <mulx319_stack=stack64#8,>mulrax=int64#7
# asm 2: movq <mulx319_stack=56(%rsp),>mulrax=%rax
movq 56(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 72)
# asm 1: mulq  72(<pp=int64#2)
# asm 2: mulq  72(<pp=%rsi)
mulq  72(%rsi)

# qhasm:   carry? ry2 += mulrax
# asm 1: add  <mulrax=int64#7,<ry2=int64#9
# asm 2: add  <mulrax=%rax,<ry2=%r11
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

# qhasm:   carry? ry4 += mulrax
# asm 1: add  <mulrax=int64#7,<ry4=int64#13
# asm 2: add  <mulrax=%rax,<ry4=%r15
add  %rax,%r15

# qhasm:   mulr41 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr41=int64#14
# asm 2: adc <mulrdx=%rdx,<mulr41=%rbx
adc %rdx,%rbx

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <mulx419_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 56)
# asm 1: mulq  56(<pp=int64#2)
# asm 2: mulq  56(<pp=%rsi)
mulq  56(%rsi)

# qhasm:   carry? ry1 += mulrax
# asm 1: add  <mulrax=int64#7,<ry1=int64#6
# asm 2: add  <mulrax=%rax,<ry1=%r9
add  %rax,%r9

# qhasm:   mulr11 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr11=int64#8
# asm 2: adc <mulrdx=%rdx,<mulr11=%r10
adc %rdx,%r10

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <mulx419_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 64)
# asm 1: mulq  64(<pp=int64#2)
# asm 2: mulq  64(<pp=%rsi)
mulq  64(%rsi)

# qhasm:   carry? ry2 += mulrax
# asm 1: add  <mulrax=int64#7,<ry2=int64#9
# asm 2: add  <mulrax=%rax,<ry2=%r11
add  %rax,%r11

# qhasm:   mulr21 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr21=int64#10
# asm 2: adc <mulrdx=%rdx,<mulr21=%r12
adc %rdx,%r12

# qhasm:   mulrax = mulx419_stack
# asm 1: movq <mulx419_stack=stack64#9,>mulrax=int64#7
# asm 2: movq <mulx419_stack=64(%rsp),>mulrax=%rax
movq 64(%rsp),%rax

# qhasm:   (uint128) mulrdx mulrax = mulrax * *(uint64 *)(pp + 72)
# asm 1: mulq  72(<pp=int64#2)
# asm 2: mulq  72(<pp=%rsi)
mulq  72(%rsi)

# qhasm:   carry? ry3 += mulrax
# asm 1: add  <mulrax=int64#7,<ry3=int64#11
# asm 2: add  <mulrax=%rax,<ry3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=int64#3
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=%rdx
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51(%rip),%rdx

# qhasm:   mulr01 = (mulr01.ry0) << 13
# asm 1: shld $13,<ry0=int64#4,<mulr01=int64#5
# asm 2: shld $13,<ry0=%rcx,<mulr01=%r8
shld $13,%rcx,%r8

# qhasm:   ry0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<ry0=int64#4
# asm 2: and  <mulredmask=%rdx,<ry0=%rcx
and  %rdx,%rcx

# qhasm:   mulr11 = (mulr11.ry1) << 13
# asm 1: shld $13,<ry1=int64#6,<mulr11=int64#8
# asm 2: shld $13,<ry1=%r9,<mulr11=%r10
shld $13,%r9,%r10

# qhasm:   ry1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<ry1=int64#6
# asm 2: and  <mulredmask=%rdx,<ry1=%r9
and  %rdx,%r9

# qhasm:   ry1 += mulr01
# asm 1: add  <mulr01=int64#5,<ry1=int64#6
# asm 2: add  <mulr01=%r8,<ry1=%r9
add  %r8,%r9

# qhasm:   mulr21 = (mulr21.ry2) << 13
# asm 1: shld $13,<ry2=int64#9,<mulr21=int64#10
# asm 2: shld $13,<ry2=%r11,<mulr21=%r12
shld $13,%r11,%r12

# qhasm:   ry2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<ry2=int64#9
# asm 2: and  <mulredmask=%rdx,<ry2=%r11
and  %rdx,%r11

# qhasm:   ry2 += mulr11
# asm 1: add  <mulr11=int64#8,<ry2=int64#9
# asm 2: add  <mulr11=%r10,<ry2=%r11
add  %r10,%r11

# qhasm:   mulr31 = (mulr31.ry3) << 13
# asm 1: shld $13,<ry3=int64#11,<mulr31=int64#12
# asm 2: shld $13,<ry3=%r13,<mulr31=%r14
shld $13,%r13,%r14

# qhasm:   ry3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<ry3=int64#11
# asm 2: and  <mulredmask=%rdx,<ry3=%r13
and  %rdx,%r13

# qhasm:   ry3 += mulr21
# asm 1: add  <mulr21=int64#10,<ry3=int64#11
# asm 2: add  <mulr21=%r12,<ry3=%r13
add  %r12,%r13

# qhasm:   mulr41 = (mulr41.ry4) << 13
# asm 1: shld $13,<ry4=int64#13,<mulr41=int64#14
# asm 2: shld $13,<ry4=%r15,<mulr41=%rbx
shld $13,%r15,%rbx

# qhasm:   ry4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<ry4=int64#13
# asm 2: and  <mulredmask=%rdx,<ry4=%r15
and  %rdx,%r15

# qhasm:   ry4 += mulr31
# asm 1: add  <mulr31=int64#12,<ry4=int64#13
# asm 2: add  <mulr31=%r14,<ry4=%r15
add  %r14,%r15

# qhasm:   mulr41 = mulr41 * 19
# asm 1: imulq  $19,<mulr41=int64#14,>mulr41=int64#5
# asm 2: imulq  $19,<mulr41=%rbx,>mulr41=%r8
imulq  $19,%rbx,%r8

# qhasm:   ry0 += mulr41
# asm 1: add  <mulr41=int64#5,<ry0=int64#4
# asm 2: add  <mulr41=%r8,<ry0=%rcx
add  %r8,%rcx

# qhasm:   mult = ry0
# asm 1: mov  <ry0=int64#4,>mult=int64#5
# asm 2: mov  <ry0=%rcx,>mult=%r8
mov  %rcx,%r8

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   mult += ry1
# asm 1: add  <ry1=int64#6,<mult=int64#5
# asm 2: add  <ry1=%r9,<mult=%r8
add  %r9,%r8

# qhasm:   ry1 = mult
# asm 1: mov  <mult=int64#5,>ry1=int64#6
# asm 2: mov  <mult=%r8,>ry1=%r9
mov  %r8,%r9

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   ry0 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<ry0=int64#4
# asm 2: and  <mulredmask=%rdx,<ry0=%rcx
and  %rdx,%rcx

# qhasm:   mult += ry2
# asm 1: add  <ry2=int64#9,<mult=int64#5
# asm 2: add  <ry2=%r11,<mult=%r8
add  %r11,%r8

# qhasm:   ry2 = mult
# asm 1: mov  <mult=int64#5,>ry2=int64#7
# asm 2: mov  <mult=%r8,>ry2=%rax
mov  %r8,%rax

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   ry1 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<ry1=int64#6
# asm 2: and  <mulredmask=%rdx,<ry1=%r9
and  %rdx,%r9

# qhasm:   mult += ry3
# asm 1: add  <ry3=int64#11,<mult=int64#5
# asm 2: add  <ry3=%r13,<mult=%r8
add  %r13,%r8

# qhasm:   ry3 = mult
# asm 1: mov  <mult=int64#5,>ry3=int64#8
# asm 2: mov  <mult=%r8,>ry3=%r10
mov  %r8,%r10

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   ry2 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<ry2=int64#7
# asm 2: and  <mulredmask=%rdx,<ry2=%rax
and  %rdx,%rax

# qhasm:   mult += ry4
# asm 1: add  <ry4=int64#13,<mult=int64#5
# asm 2: add  <ry4=%r15,<mult=%r8
add  %r15,%r8

# qhasm:   ry4 = mult
# asm 1: mov  <mult=int64#5,>ry4=int64#9
# asm 2: mov  <mult=%r8,>ry4=%r11
mov  %r8,%r11

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#5
# asm 2: shr  $51,<mult=%r8
shr  $51,%r8

# qhasm:   ry3 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<ry3=int64#8
# asm 2: and  <mulredmask=%rdx,<ry3=%r10
and  %rdx,%r10

# qhasm:   mult *= 19
# asm 1: imulq  $19,<mult=int64#5,>mult=int64#5
# asm 2: imulq  $19,<mult=%r8,>mult=%r8
imulq  $19,%r8,%r8

# qhasm:   ry0 += mult
# asm 1: add  <mult=int64#5,<ry0=int64#4
# asm 2: add  <mult=%r8,<ry0=%rcx
add  %r8,%rcx

# qhasm:   ry4 &= mulredmask
# asm 1: and  <mulredmask=int64#3,<ry4=int64#9
# asm 2: and  <mulredmask=%rdx,<ry4=%r11
and  %rdx,%r11

# qhasm: *(uint64 *)(rp + 40) = ry0
# asm 1: movq   <ry0=int64#4,40(<rp=int64#1)
# asm 2: movq   <ry0=%rcx,40(<rp=%rdi)
movq   %rcx,40(%rdi)

# qhasm: *(uint64 *)(rp + 48) = ry1
# asm 1: movq   <ry1=int64#6,48(<rp=int64#1)
# asm 2: movq   <ry1=%r9,48(<rp=%rdi)
movq   %r9,48(%rdi)

# qhasm: *(uint64 *)(rp + 56) = ry2
# asm 1: movq   <ry2=int64#7,56(<rp=int64#1)
# asm 2: movq   <ry2=%rax,56(<rp=%rdi)
movq   %rax,56(%rdi)

# qhasm: *(uint64 *)(rp + 64) = ry3
# asm 1: movq   <ry3=int64#8,64(<rp=int64#1)
# asm 2: movq   <ry3=%r10,64(<rp=%rdi)
movq   %r10,64(%rdi)

# qhasm: *(uint64 *)(rp + 72) = ry4
# asm 1: movq   <ry4=int64#9,72(<rp=int64#1)
# asm 2: movq   <ry4=%r11,72(<rp=%rdi)
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
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51(%rip),%rdx

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

# qhasm:   rt0 = mulrax
# asm 1: mov  <mulrax=int64#7,>rt0=int64#4
# asm 2: mov  <mulrax=%rax,>rt0=%rcx
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

# qhasm:   carry? rt0 += mulrax
# asm 1: add  <mulrax=int64#7,<rt0=int64#4
# asm 2: add  <mulrax=%rax,<rt0=%rcx
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

# qhasm:   carry? rt0 += mulrax
# asm 1: add  <mulrax=int64#7,<rt0=int64#4
# asm 2: add  <mulrax=%rax,<rt0=%rcx
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

# qhasm:   rt1 = mulrax
# asm 1: mov  <mulrax=int64#7,>rt1=int64#6
# asm 2: mov  <mulrax=%rax,>rt1=%r9
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

# qhasm:   rt2 = mulrax
# asm 1: mov  <mulrax=int64#7,>rt2=int64#9
# asm 2: mov  <mulrax=%rax,>rt2=%r11
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

# qhasm:   rt3 = mulrax
# asm 1: mov  <mulrax=int64#7,>rt3=int64#11
# asm 2: mov  <mulrax=%rax,>rt3=%r13
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

# qhasm:   rt4 = mulrax
# asm 1: mov  <mulrax=int64#7,>rt4=int64#13
# asm 2: mov  <mulrax=%rax,>rt4=%r15
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

# qhasm:   carry? rt1 += mulrax
# asm 1: add  <mulrax=int64#7,<rt1=int64#6
# asm 2: add  <mulrax=%rax,<rt1=%r9
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

# qhasm:   carry? rt2 += mulrax
# asm 1: add  <mulrax=int64#7,<rt2=int64#9
# asm 2: add  <mulrax=%rax,<rt2=%r11
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

# qhasm:   carry? rt3 += mulrax
# asm 1: add  <mulrax=int64#7,<rt3=int64#11
# asm 2: add  <mulrax=%rax,<rt3=%r13
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

# qhasm:   carry? rt4 += mulrax
# asm 1: add  <mulrax=int64#7,<rt4=int64#13
# asm 2: add  <mulrax=%rax,<rt4=%r15
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

# qhasm:   carry? rt0 += mulrax
# asm 1: add  <mulrax=int64#7,<rt0=int64#4
# asm 2: add  <mulrax=%rax,<rt0=%rcx
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

# qhasm:   carry? rt2 += mulrax
# asm 1: add  <mulrax=int64#7,<rt2=int64#9
# asm 2: add  <mulrax=%rax,<rt2=%r11
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

# qhasm:   carry? rt3 += mulrax
# asm 1: add  <mulrax=int64#7,<rt3=int64#11
# asm 2: add  <mulrax=%rax,<rt3=%r13
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

# qhasm:   carry? rt4 += mulrax
# asm 1: add  <mulrax=int64#7,<rt4=int64#13
# asm 2: add  <mulrax=%rax,<rt4=%r15
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

# qhasm:   carry? rt0 += mulrax
# asm 1: add  <mulrax=int64#7,<rt0=int64#4
# asm 2: add  <mulrax=%rax,<rt0=%rcx
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

# qhasm:   carry? rt1 += mulrax
# asm 1: add  <mulrax=int64#7,<rt1=int64#6
# asm 2: add  <mulrax=%rax,<rt1=%r9
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

# qhasm:   carry? rt3 += mulrax
# asm 1: add  <mulrax=int64#7,<rt3=int64#11
# asm 2: add  <mulrax=%rax,<rt3=%r13
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

# qhasm:   carry? rt4 += mulrax
# asm 1: add  <mulrax=int64#7,<rt4=int64#13
# asm 2: add  <mulrax=%rax,<rt4=%r15
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

# qhasm:   carry? rt1 += mulrax
# asm 1: add  <mulrax=int64#7,<rt1=int64#6
# asm 2: add  <mulrax=%rax,<rt1=%r9
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

# qhasm:   carry? rt2 += mulrax
# asm 1: add  <mulrax=int64#7,<rt2=int64#9
# asm 2: add  <mulrax=%rax,<rt2=%r11
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

# qhasm:   carry? rt4 += mulrax
# asm 1: add  <mulrax=int64#7,<rt4=int64#13
# asm 2: add  <mulrax=%rax,<rt4=%r15
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

# qhasm:   carry? rt1 += mulrax
# asm 1: add  <mulrax=int64#7,<rt1=int64#6
# asm 2: add  <mulrax=%rax,<rt1=%r9
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

# qhasm:   carry? rt2 += mulrax
# asm 1: add  <mulrax=int64#7,<rt2=int64#9
# asm 2: add  <mulrax=%rax,<rt2=%r11
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

# qhasm:   carry? rt3 += mulrax
# asm 1: add  <mulrax=int64#7,<rt3=int64#11
# asm 2: add  <mulrax=%rax,<rt3=%r13
add  %rax,%r13

# qhasm:   mulr31 += mulrdx + carry
# asm 1: adc <mulrdx=int64#3,<mulr31=int64#12
# asm 2: adc <mulrdx=%rdx,<mulr31=%r14
adc %rdx,%r14

# qhasm:   mulredmask = *(uint64 *) &crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51
# asm 1: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=int64#2
# asm 2: movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51,>mulredmask=%rsi
movq crypto_sign_ed25519_amd64_51_30k_batch_REDMASK51(%rip),%rsi

# qhasm:   mulr01 = (mulr01.rt0) << 13
# asm 1: shld $13,<rt0=int64#4,<mulr01=int64#5
# asm 2: shld $13,<rt0=%rcx,<mulr01=%r8
shld $13,%rcx,%r8

# qhasm:   rt0 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<rt0=int64#4
# asm 2: and  <mulredmask=%rsi,<rt0=%rcx
and  %rsi,%rcx

# qhasm:   mulr11 = (mulr11.rt1) << 13
# asm 1: shld $13,<rt1=int64#6,<mulr11=int64#8
# asm 2: shld $13,<rt1=%r9,<mulr11=%r10
shld $13,%r9,%r10

# qhasm:   rt1 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<rt1=int64#6
# asm 2: and  <mulredmask=%rsi,<rt1=%r9
and  %rsi,%r9

# qhasm:   rt1 += mulr01
# asm 1: add  <mulr01=int64#5,<rt1=int64#6
# asm 2: add  <mulr01=%r8,<rt1=%r9
add  %r8,%r9

# qhasm:   mulr21 = (mulr21.rt2) << 13
# asm 1: shld $13,<rt2=int64#9,<mulr21=int64#10
# asm 2: shld $13,<rt2=%r11,<mulr21=%r12
shld $13,%r11,%r12

# qhasm:   rt2 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<rt2=int64#9
# asm 2: and  <mulredmask=%rsi,<rt2=%r11
and  %rsi,%r11

# qhasm:   rt2 += mulr11
# asm 1: add  <mulr11=int64#8,<rt2=int64#9
# asm 2: add  <mulr11=%r10,<rt2=%r11
add  %r10,%r11

# qhasm:   mulr31 = (mulr31.rt3) << 13
# asm 1: shld $13,<rt3=int64#11,<mulr31=int64#12
# asm 2: shld $13,<rt3=%r13,<mulr31=%r14
shld $13,%r13,%r14

# qhasm:   rt3 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<rt3=int64#11
# asm 2: and  <mulredmask=%rsi,<rt3=%r13
and  %rsi,%r13

# qhasm:   rt3 += mulr21
# asm 1: add  <mulr21=int64#10,<rt3=int64#11
# asm 2: add  <mulr21=%r12,<rt3=%r13
add  %r12,%r13

# qhasm:   mulr41 = (mulr41.rt4) << 13
# asm 1: shld $13,<rt4=int64#13,<mulr41=int64#14
# asm 2: shld $13,<rt4=%r15,<mulr41=%rbx
shld $13,%r15,%rbx

# qhasm:   rt4 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<rt4=int64#13
# asm 2: and  <mulredmask=%rsi,<rt4=%r15
and  %rsi,%r15

# qhasm:   rt4 += mulr31
# asm 1: add  <mulr31=int64#12,<rt4=int64#13
# asm 2: add  <mulr31=%r14,<rt4=%r15
add  %r14,%r15

# qhasm:   mulr41 = mulr41 * 19
# asm 1: imulq  $19,<mulr41=int64#14,>mulr41=int64#3
# asm 2: imulq  $19,<mulr41=%rbx,>mulr41=%rdx
imulq  $19,%rbx,%rdx

# qhasm:   rt0 += mulr41
# asm 1: add  <mulr41=int64#3,<rt0=int64#4
# asm 2: add  <mulr41=%rdx,<rt0=%rcx
add  %rdx,%rcx

# qhasm:   mult = rt0
# asm 1: mov  <rt0=int64#4,>mult=int64#3
# asm 2: mov  <rt0=%rcx,>mult=%rdx
mov  %rcx,%rdx

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#3
# asm 2: shr  $51,<mult=%rdx
shr  $51,%rdx

# qhasm:   mult += rt1
# asm 1: add  <rt1=int64#6,<mult=int64#3
# asm 2: add  <rt1=%r9,<mult=%rdx
add  %r9,%rdx

# qhasm:   rt1 = mult
# asm 1: mov  <mult=int64#3,>rt1=int64#5
# asm 2: mov  <mult=%rdx,>rt1=%r8
mov  %rdx,%r8

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#3
# asm 2: shr  $51,<mult=%rdx
shr  $51,%rdx

# qhasm:   rt0 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<rt0=int64#4
# asm 2: and  <mulredmask=%rsi,<rt0=%rcx
and  %rsi,%rcx

# qhasm:   mult += rt2
# asm 1: add  <rt2=int64#9,<mult=int64#3
# asm 2: add  <rt2=%r11,<mult=%rdx
add  %r11,%rdx

# qhasm:   rt2 = mult
# asm 1: mov  <mult=int64#3,>rt2=int64#6
# asm 2: mov  <mult=%rdx,>rt2=%r9
mov  %rdx,%r9

# qhasm:   (uint64) mult >>= 51
# asm 1: shr  $51,<mult=int64#3
# asm 2: shr  $51,<mult=%rdx
shr  $51,%rdx

# qhasm:   rt1 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<rt1=int64#5
# asm 2: and  <mulredmask=%rsi,<rt1=%r8
and  %rsi,%r8

# qhasm:   mult += rt3
# asm 1: add  <rt3=int64#11,<mult=int64#3
# asm 2: add  <rt3=%r13,<mult=%rdx
add  %r13,%rdx

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
# asm 1: add  <rt4=int64#13,<mult=int64#3
# asm 2: add  <rt4=%r15,<mult=%rdx
add  %r15,%rdx

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
# asm 1: add  <mult=int64#3,<rt0=int64#4
# asm 2: add  <mult=%rdx,<rt0=%rcx
add  %rdx,%rcx

# qhasm:   rt4 &= mulredmask
# asm 1: and  <mulredmask=int64#2,<rt4=int64#8
# asm 2: and  <mulredmask=%rsi,<rt4=%r10
and  %rsi,%r10

# qhasm: *(uint64 *)(rp + 120) = rt0
# asm 1: movq   <rt0=int64#4,120(<rp=int64#1)
# asm 2: movq   <rt0=%rcx,120(<rp=%rdi)
movq   %rcx,120(%rdi)

# qhasm: *(uint64 *)(rp + 128) = rt1
# asm 1: movq   <rt1=int64#5,128(<rp=int64#1)
# asm 2: movq   <rt1=%r8,128(<rp=%rdi)
movq   %r8,128(%rdi)

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
