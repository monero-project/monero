#
# Makefile: compilation of pyUnbound and documentation, testing
#
# Copyright (c) 2009, Zdenek Vasicek (vasicek AT fit.vutbr.cz)
#                     Marek Vavrusa  (xvavru00 AT stud.fit.vutbr.cz)
#
# This software is open source.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
#    * Redistributions of source code must retain the above copyright notice,
#      this list of conditions and the following disclaimer.
# 
#    * Redistributions in binary form must reproduce the above copyright notice,
#      this list of conditions and the following disclaimer in the documentation
#      and/or other materials provided with the distribution.
# 
#    * Neither the name of the organization nor the names of its
#      contributors may be used to endorse or promote products derived from this
#      software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

help:
	@echo "Please use \`make <target>' where <target> is one of"
	@echo "  testenv   to make test environment and run bash "
	@echo "            usefull in case you don't want to install unbound but want to test examples"
	@echo "  doc       to make documentation"
	@echo "  clean     clean all"

.PHONY: testenv clean doc swig

#_unbound.so:	../../Makefile
	#$(MAKE) -C ../..

#../../.libs/libunbound.so.0:	../../Makefile
	#$(MAKE) -C ../..

clean:
	rm -rdf examples/unbound
	rm -f _unbound.so libunbound_wrap.o
	$(MAKE) -C ../.. clean

testenv: ../../.libs/libunbound.so.2 ../../.libs/_unbound.so
	rm -rdf examples/unbound
	cd examples && mkdir unbound && ln -s ../../unbound.py unbound/__init__.py && ln -s ../../_unbound.so unbound/_unbound.so && ln -s ../../../../.libs/libunbound.so.2 unbound/libunbound.so.2 && ls -la
	cd examples && if test -f ../../../.libs/_unbound.so; then cp ../../../.libs/_unbound.so . ; fi
	@echo "Run a script by typing ./script_name.py"
	cd examples && LD_LIBRARY_PATH=unbound bash
	rm -rdf examples/unbound examples/_unbound.so

doc: ../../.libs/libunbound.so.0 _unbound.so
	$(MAKE) -C docs html

#for development only
swig: libunbound.i 
	swig -python -o libunbound_wrap.c -I../.. libunbound.i
	gcc -c libunbound_wrap.c -O9 -fPIC -I../.. -I/usr/include/python2.5 -I. -o libunbound_wrap.o
	gcc -shared libunbound_wrap.o -L../../.libs -lunbound -o _unbound.so 

