:name:fe:r->X:r->Y:r->Z:r->T:t0:t1:t2:t3:t4:t5:p->X:p->Y:p->Z:p->T:q->YplusX:q->YminusX:q->Z:q->T2d:
fe r:var/r=fe:

enter f:enter/f:>X1=fe#11:>Y1=fe#12:>Z1=fe#13:>T1=fe#14:>YpX2=fe#15:>YmX2=fe#16:>Z2=fe#17:>T2d2=fe#18:
return:nofallthrough:<X3=fe#1:<Y3=fe#2:<Z3=fe#3:<T3=fe#4:leave:

h=f+g:<f=fe:<g=fe:>h=fe:asm/fe_add(>h,<f,<g);:
h=f-g:<f=fe:<g=fe:>h=fe:asm/fe_sub(>h,<f,<g);:
h=f*g:<f=fe:<g=fe:>h=fe:asm/fe_mul(>h,<f,<g);:
h=f^2:<f=fe:>h=fe:asm/fe_sq(>h,<f);:
h=2*g:<g=fe:>h=fe:asm/fe_add(>h,<g,<g);:

:

enter ge_sub

fe X1
fe Y1
fe Z1
fe Z2
fe T1
fe ZZ
fe YpX2
fe YmX2
fe T2d2
fe X3
fe Y3
fe Z3
fe T3
fe YpX1
fe YmX1
fe A
fe B
fe C
fe D

YpX1 = Y1+X1
YmX1 = Y1-X1
A = YpX1*YmX2
B = YmX1*YpX2
C = T2d2*T1
ZZ = Z1*Z2
D = 2*ZZ
X3 = A-B
Y3 = A+B
Z3 = D-C
T3 = D+C

return
