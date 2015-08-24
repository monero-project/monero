all: d.h d2.h sqrtm1.h base.h base2.h \
ge_add.h ge_sub.h \
ge_madd.h ge_msub.h \
ge_p2_dbl.h \
pow225521.h pow22523.h

d.h: d.py
	python d.py > d.h

d2.h: d2.py
	python d2.py > d2.h

sqrtm1.h: sqrtm1.py
	python sqrtm1.py > sqrtm1.h

base.h: base.py
	python base.py > base.h

base2.h: base2.py
	python base2.py > base2.h

ge_add.h: ge_add.q q2h.sh
	./q2h.sh < ge_add.q > ge_add.h

ge_sub.h: ge_sub.q q2h.sh
	./q2h.sh < ge_sub.q > ge_sub.h

ge_madd.h: ge_madd.q q2h.sh
	./q2h.sh < ge_madd.q > ge_madd.h

ge_msub.h: ge_msub.q q2h.sh
	./q2h.sh < ge_msub.q > ge_msub.h

ge_p2_dbl.h: ge_p2_dbl.q q2h.sh
	./q2h.sh < ge_p2_dbl.q > ge_p2_dbl.h

pow22523.h: pow22523.q q2h.sh
	./q2h.sh < pow22523.q > pow22523.h

pow225521.h: pow225521.q q2h.sh
	./q2h.sh < pow225521.q > pow225521.h
