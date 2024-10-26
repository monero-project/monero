:name:fe:r->X:r->Y:r->Z:r->T:t0:t1:t2:t3:t4:t5:p->X:p->Y:p->Z:
fe r:var/r=fe:

enter f:enter/f:>X1=fe#11:>Y1=fe#12:>Z1=fe#13:
return:nofallthrough:<X3=fe#1:<Y3=fe#2:<Z3=fe#3:<T3=fe#4:leave:

h=f+g:<f=fe:<g=fe:>h=fe:asm/fe_add(>h,<f,<g);:
h=f-g:<f=fe:<g=fe:>h=fe:asm/fe_sub(>h,<f,<g);:
h=f*g:<f=fe:<g=fe:>h=fe:asm/fe_mul(>h,<f,<g);:
h=f^2:<f=fe:>h=fe:asm/fe_sq(>h,<f);:
h=2*f^2:<f=fe:>h=fe:asm/fe_sq2(>h,<f);:
h=2*g:<g=fe:>h=fe:asm/fe_add(>h,<g,<g);:

:

enter ge_p2_dbl

fe X1
fe Y1
fe Z1
fe A
fe AA
fe XX
fe YY
fe B
fe X3
fe Y3
fe Z3
fe T3

XX=X1^2
YY=Y1^2
B=2*Z1^2
A=X1+Y1
AA=A^2
Y3=YY+XX
Z3=YY-XX
X3=AA-Y3
T3=B-Z3

return
