# Copyright (c) 2014-2022, The Monero Project
# 
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without modification, are
# permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice, this list of
#    conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright notice, this list
#    of conditions and the following disclaimer in the documentation and/or other
#    materials provided with the distribution.
# 
# 3. Neither the name of the copyright holder nor the names of its contributors may be
#    used to endorse or promote products derived from this software without specific
#    prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
# THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

use Math::BigInt only => 'GMP';
use Digest::Keccak qw(keccak_256);

my $p = Math::BigInt->new(2)->bpow(255)->bsub(19);		#F_p
my $l = Math::BigInt->new(2)->bpow(252)->badd('27742317777372353535851937790883648493');
#my $d = Math::BigInt->new(486662); 					#motgomery: y^2 = x^3 + 486662x^2 + x
my $d = Math::BigInt->new(-121665)->bmul(minv(121666))->bmod($p); 	#twisted edwards: -x^2 +y^2 = 1 + d*x^2*y^2
my $x0 = Math::BigInt->new('15112221349535400772501151409588531511454012693041857206046113283949847762202');
my $y0 = Math::BigInt->new('46316835694926478169428394003475163141307993866256225615783033603165251855960');		#y0 = 4/5
my $m = Math::BigInt->new('7237005577332262213973186563042994240829374041602535252466099000494570602493'); 		#p = 8m+5
my $ps = $p->copy()->bdec->bdiv(4);
my $pl = $p->copy()->bdec->bdiv(2);
my $ii = Math::BigInt->new(2)->bmodpow($ps,$p);		#sqrt(-1)

sub ec_rec {
	my $y = Math::BigInt->new($_[0]);
	my $xx = $y->copy()->bpow(2)->bdec()->bmul(minv($y->copy()->bpow(2)->bmul($d)->binc))->bmod($p);
	return 0 if !($xx->copy()->bmodpow($pl,$p)->binc->bmod($p));
	my $p2 = $p->copy()->badd(3)->bdiv(8);
	my $x = $xx->copy()->bmodpow($p2, $p);
	if ($x->copy()->bpow(2)->bsub($xx)->bmod($p)) {$x->bmul($ii)->bmod($p)}
	if ($x->is_odd) {$x = $p->copy()->bsub($x)};
	return $x;
	} 

sub h2i {
	return Math::BigInt->new('0x'.(unpack 'H*', (reverse pack 'H*', shift)));;
	}

sub i2h {
	my $t = substr(Math::BigInt->new(shift)->as_hex(),2,64);
	if (length($t)%2 == 1) {$t = '0'.$t}
	return unpack 'H*', (reverse pack 'H*', $t);
	}
	
	
sub random {
	return keccak_256(rand(2**20));
	#return keccak_256(3); #I swear that's random!
	}

sub ec_pack {
	my $x = Math::BigInt->new($_[0]);
	my $y = Math::BigInt->new($_[1]);
	my $or = Math::BigInt->new(2)->bpow(255);
	$y |= $or if ($x->is_odd());
	return unpack 'H*', (reverse pack 'H*', substr($y->as_hex(),2,64));
	}
	
sub ec_unpack {
	my $y = Math::BigInt->new(h2i(shift));
	my $b = $y >> 255;
	my $and = Math::BigInt->new(2)->bpow(255)->bdec();
	$y &= $and;
	my $x = ec_rec($y);
	return (0,0) if $x==0;
	($b==0) || ($x = $p->copy()->bsub($x));
	return ($x,$y);
	}


sub minv {
	my $x = Math::BigInt->new(shift);
	$x->bmodpow($p-2,$p);
	return $x;
	}
	
	
sub ec_doub {
	my $x = Math::BigInt->new($_[0]);
	my $y = Math::BigInt->new($_[1]);
	
	#$t = $x->copy()->bpow(2)->bmul(3)->badd($x->copy()->bmul($d)->bmul(2))->binc()->bmul(minv($y->copy()->bmul(2)));			#montgomery
	#$x2 = $t->copy()->bpow(2)->bsub($d)->bsub($x)->bsub($x)->bmod($p);												#montgomery
	#$y2 = $x->copy()->bmul(2)->badd($x)->badd($d)->bmul($t)->bsub($t->copy()->bpow(3))->bsub($y)->bmod($p);		#montgomery
	$t = $x->copy()->bmul($x)->bmul($y)->bmul($y)->bmul($d)->bmod($p);
	$x3 = $x->copy()->bmul($y)->bmul(2)->bmul(minv($t+1))->bmod($p);
	$y3 = $y->copy()->bpow(2)->badd($x->copy()->bpow(2))->bmul(minv(1-$t))->bmod($p);
	return ($x3,$y3);
	}
sub ec_add {
	my $x1 = Math::BigInt->new($_[0]);
	my $y1 = Math::BigInt->new($_[1]);
	my $x2 = Math::BigInt->new($_[2]);
	my $y2 = Math::BigInt->new($_[3]);	
	
	#$t = $y2->copy()->bsub($y1)->bmul(minv($x2->copy()->bsub($x1)));
	#$x3 = $t->copy()->bpow(2)->bsub($d)->bsub($x1)->bsub($x2)->bmod($p);
	#$y3 = $x1->copy()->bmul(2)->badd($x2)->badd($d)->bmul($t)->bsub($t->copy()->bpow(3))->bsub($y1)->bmod($p);
	$t = $x1->copy->bmul($x2)->bmul($y1)->bmul($y2)->bmul($d)->bmod($p);
	$x3 = $x1->copy()->bmul($y2)->badd($y1->copy()->bmul($x2))->bmul(minv($t+1))->bmod($p); 
	$y3 = $y1->copy()->bmul($y2)->badd($x1->copy()->bmul($x2))->bmul(minv(1-$t))->bmod($p);
	
	
	return ($x3,$y3);
	}
	
sub ec_mul {
	my $n = Math::BigInt->new($_[0]);
	my $x = Math::BigInt->new($_[1]);
	my $y = Math::BigInt->new($_[2]);

	if ($n->is_one()) {
		return ($x,$y);
		last;
		}
	elsif ($n->is_even()) {	
		$n->bdiv(2);
		return ec_mul($n,&ec_doub($x,$y));
		}
	else {
		$n->bdec()->bdiv(2);
		return ec_add($x,$y,ec_mul($n,&ec_doub($x,$y))); 	
		}
	}
	
sub pkeygen {
	my $key = Math::BigInt->new(h2i(shift))->bmod($l);
	return ec_pack(ec_mul($key,$x0,$y0));
	}

sub ec_hash {
	my $h = pack 'H*', shift;
	my $h = Math::BigInt->new('0x'.(unpack 'H*', reverse keccak_256($h)));
	my ($x,$y) = (0,0);
	while ($x == 0) {
		($x,$y) = ec_unpack(i2h($h));
		$h->binc();
		}
	return ec_mul(8,$x,$y);
	}
	
sub im_gen {
	my ($x,$y) = ec_hash(shift);
	my $k = Math::BigInt->new(h2i(shift))->bmod($l);
	return ec_pack(ec_mul($k,$x,$y));
	}
	
	
sub sign {
	my ($m,$sec_key) = @_;
	my $sec_key = Math::BigInt->new(h2i($sec_key));
	my ($x,$y) = ec_mul($sec_key,$x0,$y0);
	my $k = Math::BigInt->new('0x'.(unpack 'H*', random()))->bmod($l);
	#my $k = Math::BigInt->new('5267557024171956683337957876581522196748200715787296882078421399301151717969');
	my $e = unpack 'H*', keccak_256($m.(pack 'H*', ec_pack(ec_mul($k,$x0,$y0))));
	my $s = i2h(Math::BigInt->new(h2i($e))->bmul($sec_key)->bneg()->badd($k)->bmod($l));
	$e = i2h(Math::BigInt->new(h2i($e))->bmod($l));
	return ($s,$e);	
	}
	
sub check_s {
	my ($m,$pt,$s1,$e1) = @_;
	my ($x,$y) = ec_unpack($pt);
	my $s = Math::BigInt->new(h2i($s1))->bmod($l);
	my $e = Math::BigInt->new(h2i($e1))->bmod($l);
	my ($x1,$y1) = ec_add(ec_mul($s,$x0,$y0),ec_mul($e,$x,$y));
	$m = $m.(pack 'H*', ec_pack($x1,$y1));
	my $ev = Math::BigInt->new(h2i(unpack 'H*', keccak_256($m)))->bmod($l);
	
	return !$ev->bcmp($e); 	
	}

sub r_sign {
	my ($m,$image,$sec_key,$index,@pkeys) = @_;
	my ($ix,$iy) = ec_unpack($image);
	my $n = @pkeys;		
	my $data = $m;
	my $w = $a = $b = $hx = $hy = $px = $py = 0;
	my @zc = ();
	my $sum = Math::BigInt->new();
	#print "begin signing ($n keys)\n";
	for $i (0..$n-1) {
		($hx, $hy) = ec_hash(@pkeys[$i]);
		($px,$py) = ec_unpack(@pkeys[$i]);
		if ($i == $index) {
			$w = Math::BigInt->new('0x'.(unpack 'H*', random()))->bmod($l);
			$a = pack 'H*', ec_pack(ec_mul($w,$x0,$y0));
			$b = pack 'H*', ec_pack(ec_mul($w,$hx,$hy));
			push @zc,0,0;
			} 
		else {
			$z = Math::BigInt->new('0x'.(unpack 'H*', random()))->bmod($l);
			$c = Math::BigInt->new('0x'.(unpack 'H*', random()))->bmod($l);
			$sum->badd($c);
			$a = pack 'H*', ec_pack(ec_add(ec_mul($z,$x0,$y0),ec_mul($c,$px,$py)));
			$b = pack 'H*', ec_pack(ec_add(ec_mul($z,$hx,$hy),ec_mul($c,$ix,$iy)));
			push @zc,i2h($z),i2h($c);
			}
		$data = $data.$a.$b;
		#print "key number $i done\n";
		}
	#print "generating ringsig..\n";
	my $h = unpack 'H*', keccak_256($data);
	my $cy = Math::BigInt->new(h2i($h))->bsub($sum)->bmod($l);
	my $zy = $cy->copy()->bmul(h2i($sec_key))->bneg()->badd($w)->bmod($l);
	@zc[2*$index] = i2h($zy);
	@zc[2*$index+1] = i2h($cy);
	return @zc;	
	}

sub r_check_s {
	my ($m,$image,@zc) = @_;
	my $n = @zc/3;
	for $j (0..$n-1) {
		@pkeys[$j] = shift @zc;
		}
	my $data = $m;
	my ($ix,$iy) = ec_unpack($image);	
	my $a = $b = $hx = $hy = $px = $py = $z = $c = 0;
	my $sum = Math::BigInt->new();
	#print "\nBegin checking ($n keys)\n";
	for $i (0..$n-1) {
		$z = Math::BigInt->new(h2i(shift @zc))->bmod($l);
		$c = Math::BigInt->new(h2i(shift @zc))->bmod($l);
		$sum->badd($c)->bmod($l);
		($px,$py) = ec_unpack(@pkeys[$i]);
		$a = pack 'H*', ec_pack(ec_add(ec_mul($z,$x0,$y0),ec_mul($c,$px,$py)));
		($hx, $hy) = ec_hash(@pkeys[$i]);
		$b = pack 'H*', ec_pack(ec_add(ec_mul($z,$hx,$hy),ec_mul($c,$ix,$iy)));
		$data = $data.$a.$b;
		#print "key number $i done\n";
		}
	my $h = Math::BigInt->new(h2i(unpack 'H*',  keccak_256($data)))->bmod($l);
	
	return !$h->bcmp($sum);
	}

	
	
	
