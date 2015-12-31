#assumes you have gnu sed, osx sed might need slight syntax changeo
#c.f. http://unix.stackexchange.com/questions/112023/how-can-i-replace-a-string-in-a-files

#written by shen-noether monero research labs

import os #for copying and sed etc.
import glob #for copy files
import textwrap #for comments etc

print("make sure you have cat and grep installed")
print("also assumes gnu sed syntax, c.f. :http://unix.stackexchange.com/questions/112023/how-can-i-replace-a-string-in-a-files")
print("I believe osx may have slightly different version of sed")
print("maybe someone smart can replace the sed with perl..")

a = ""

license = textwrap.dedent("""\
    // Copyright (c) 2014-2016, The Monero Project
    // 
    // All rights reserved.
    // 
    // Redistribution and use in source and binary forms, with or without modification, are
    // permitted provided that the following conditions are met:
    // 
    // 1. Redistributions of source code must retain the above copyright notice, this list of
    //    conditions and the following disclaimer.
    // 
    // 2. Redistributions in binary form must reproduce the above copyright notice, this list
    //    of conditions and the following disclaimer in the documentation and/or other
    //    materials provided with the distribution.
    // 
    // 3. Neither the name of the copyright holder nor the names of its contributors may be
    //    used to endorse or promote products derived from this software without specific
    //    prior written permission.
    // 
    // THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
    // EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
    // MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
    // THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    // SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    // PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    // INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
    // STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
    // THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    // 
    // Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers
    """)

crypto_ops_includes = textwrap.dedent("""\
    #include <assert.h>
    #include <stdint.h>

    #include "warnings.h"
    #include "crypto-ops.h"

    DISABLE_VS_WARNINGS(4146 4244)
    """)

predeclarations = textwrap.dedent("""\
    /* Predeclarations */

    static void fe_mul(fe, const fe, const fe);
    static void fe_sq(fe, const fe);
    static void fe_tobytes(unsigned char *, const fe);
    static int fe_isnonzero(const fe); 
    static void ge_madd(ge_p1p1 *, const ge_p3 *, const ge_precomp *);
    static void ge_msub(ge_p1p1 *, const ge_p3 *, const ge_precomp *);
    static void ge_p2_0(ge_p2 *);
    static void ge_p3_dbl(ge_p1p1 *, const ge_p3 *);
    static void ge_sub(ge_p1p1 *, const ge_p3 *, const ge_cached *);
    static void fe_divpowm1(fe, const fe, const fe);
    """)



fe_comments = textwrap.dedent("""\
    /*
    fe means field element.
    Here the field is \Z/(2^255-19).
    An element t, entries t[0]...t[9], represents the integer
    t[0]+2^26 t[1]+2^51 t[2]+2^77 t[3]+2^102 t[4]+...+2^230 t[9].
    Bounds on each t[i] vary depending on context.
    */
    """)

sc_comments = textwrap.dedent("""\
    /*
     *
     * sc code
     *
     *
    The set of scalars is \Z/l
    where l = 2^252 + 27742317777372353535851937790883648493.

    This is the order of the curve ed25519. 
    The point is that if a is a scalar and P is a point,
    and b is congruent to a mod l, then aP = bP.
    Thus, reducing mod l can possibly give you a smaller scalar,
    so your elliptic curve operations take less time
    */
    """)

ge_comments = textwrap.dedent("""\
    /*
     *
     * ge code
     *
     *
    ge means group element.

    Here the group is the set of pairs (x,y) of field elements (see fe.h)
    satisfying -x^2 + y^2 = 1 + d x^2y^2
    where d = -121665/121666.

    Representations:
      ge_p2 (projective): (X:Y:Z) satisfying x=X/Z, y=Y/Z
      ge_p3 (extended): (X:Y:Z:T) satisfying x=X/Z, y=Y/Z, XY=ZT
      ge_p1p1 (completed): ((X:Z),(Y:T)) satisfying x=X/Z, y=Y/T
      ge_precomp (Duif): (y+x,y-x,2dxy)
    */
    """)

xmr_comments = textwrap.dedent("""\
    /*
     *
     * xmr specific code
     *
     *
    This code is from the original CryptoNote.
    Some additional functions were needed to compute ring signatures
    which are not needed for signing. 
    Note that sc_sub and sc_mulsub have been given their own file
    since these have been rewritten 

    */
    """)




def qhasmToC(fi, header, out):
    #replaces mentiones of "header" in "fi" with output in "out"
    #also removes qhasm comments
    out1 = out+".tmp"
    rem_qhasm = " |grep -v 'qhasm' |grep -v ' asm'"
    com = "sed -e '/#include \""+header+"\"/ {' -e 'r "+header+"' -e 'd' -e '}' "+fi+rem_qhasm+" > "+out1
    com2 = "awk 'NF' "+out1+" > "+out
    print(com)
    os.system(com)
    print(com2)
    os.system(com2)
    os.remove(out1) #temporary

while (a != "m") and (a != "m") and (a != "c"):
    a = raw_input("Make / Clean/ Quit    m / c / q?")

if a == "m":
    print("making crypto-ops.c and crypto-ops.h")


    #ref10 header files

    #ref10 c files

    #fe things
    #still need to do d2, d, sqrtm1
    print("making fe.c")
    print(fe_comments)
    fe = glob.glob("fe*.c")
    for g in fe:
        os.system("cp "+g+" "+g.replace("fe", "fe.monero."))
    qhasmToC("fe_pow22523.c", "pow22523.h", "fe.monero._pow22523.c")
    qhasmToC("fe_invert.c", "pow225521.h", "fe.monero._invert.c")
    os.system("rm fe.monero._isnonzero.c") #since it's modified, it's in xmrSpecificOld
    os.system("cat fe.monero.*.c | grep -v '^#include' > fe.monero.c")

    #sc things
    print("\nmaking sc.c")
    print(sc_comments)
    #so you don't get multiple "loads"
    os.system("tail -n +24 sc_reduce.c > sc.monero._reduce.c") #also good on linux
    os.system("tail -n +24 sc_muladd.c > sc.monero._muladd.c")
    os.system("tail -n +31 sc_sub.xmr.c > sc.monero._sub.xmr.c") #careful with the tails if you change these files!
    os.system("cat sc.monero.*.c | grep -v '^#include' > sc.monero.c")

    #ge stuff
    print("making ge.c")
    ge = glob.glob("ge*.c")
    for g in ge:
        os.system("cp "+g+" "+g.replace("ge", "ge.monero."))
    print(ge_comments)
    #need to substitute the below lines for their .h files in the appropriate places
    qhasmToC("ge_add.c", "ge_add.h", "ge.monero._add.c")
    qhasmToC("ge_madd.c", "ge_madd.h", "ge.monero._madd.c")
    qhasmToC("ge_sub.c", "ge_sub.h", "ge.monero._sub.c")
    qhasmToC("ge_msub.c", "ge_msub.h", "ge.monero._msub.c")
    qhasmToC("ge_p2_dbl.c", "ge_p2_dbl.h", "ge.monero._p2_dbl.c")
    qhasmToC("ge_frombytes.c", "d.h", "ge.monero._frombytes.c")
    qhasmToC("ge.monero._frombytes.c", "sqrtm1.h", "ge.monero._frombytes.c")
    qhasmToC("ge_p3_to_cached.c", "d2.h", "ge.monero._p3_to_cached.c")



    #also ge_double_scalarmult needs base2.h for ge_precomp Bi
    #note, base2.h is a large file!
    #also in ge_scalarmult_base ge_precomp base needs base.h included

    qhasmToC("ge_double_scalarmult.c", "base2.h", "ge.monero._double_scalarmult.c")
    qhasmToC("ge_scalarmult_base.c", "base.h", "ge.monero._scalarmult_base.c")
    #qhasmToC("ge.monero._scalarmult_base.c", "base.h", "ge.monero._scalarmult_base.c")
    os.system("sed -i 's/ cmov/ ge_precomp_cmov/g' ge.monero._scalarmult_base.c")
    os.system("cat ge.monero.*.c | grep -v '^#include' > ge.monero.c")


    print("making crypto-ops.c")

    #sqrtm1 things

    #comments
    with open("fe.monero.comments", "w") as text_file:
            text_file.write(fe_comments)
    with open("ge.monero.comments", "w") as text_file:
            text_file.write(ge_comments)
    with open("sc.monero.comments", "w") as text_file:
            text_file.write(sc_comments)
    with open("xmr.monero.comments", "w") as text_file:
            text_file.write(xmr_comments)
    with open("xmr.monero.predeclarations", "w") as text_file:
            text_file.write(predeclarations)


    #license
    with open("monero.license", "w") as text_file:
            text_file.write(license)

    #crypto-ops.c includes
    with open("crypto-ops.monero.includes", "w") as text_file:
        text_file.write(crypto_ops_includes)

    #note you may have duplicates of load_3, load_4 and possibly some other functions ... 
    os.system("cat monero.license crypto-ops.monero.includes xmr.monero.predeclarations fe.monero.comments fe.monero.c sc.monero.comments sc.monero.c ge.monero.comments ge.monero.c xmr.monero.comments xmrSpecificOld.c > crypto-ops.c")

    #monero specific header files
    #print("making crypto-ops-tmp.h")
    #os.system("cat fe.h ge.h sc.h |grep -v crypto_sign_ed25519 |grep -v fe.h > crypto-ops-tmp.h")
    #we'll just use the old header crypto-ops.h

    #replace crypto_ints
    os.system("sed -i 's/crypto_int32/int32_t/g' crypto-ops.c")
    os.system("sed -i 's/crypto_int64/int64_t/g' crypto-ops.c")
    os.system("sed -i 's/crypto_uint32/uint32_t/g' crypto-ops.c")
    os.system("sed -i 's/crypto_uint64/uint64_t/g' crypto-ops.c")

    #cleaning up 
    os.system("rm *monero*")

    #monero specific c files
if a == "c":
    #turn the directory back into ref10
    os.system("rm *monero*")
    os.system("rm crypto-ops.c")
