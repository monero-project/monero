require 'cryptolib.pl';

$key = 'fc7557a2595788aea7205ffd801b8a157dc9c698adb2c598ba543eaa67cb700e';
$pt = '664422cf6f4100dc6b3298e41ca53b173a98918fc9cb50fc2d590b7d1285f4ab';
$m = keccak_256(pack 'H*', 'c8fedd380dbae40ffb52');


$s = '26a9589121e569ee0ac2e8ac7a7ea331d348f9a0fa8d28926d27c7506759e406';
$e = '780be966ad89ba526cc7adf4b771adbdaa0568038e6a30e776839a81e57dee0c';

print "   self SIG -- OK\n" if check_s($m,$pt,sign($m,$key));
print "   test SIG -- OK\n" if check_s($m,$pt,$s,$e);

@aa = r_sign($m,im_gen($pt,$key),$key,1,ec_pack(ec_mul(111,$x0,$y0)),$pt,ec_pack(ec_mul(47,$x0,$y0)));
print "   self RSIG -- OK\n" if r_check_s($m,im_gen($pt,$key),ec_pack(ec_mul(111,$x0,$y0)),$pt,ec_pack(ec_mul(47,$x0,$y0)),@aa);

$k1 = '6a7a81a52ba91b9785b484d761bfb3ad9a473c147e17b7fbbc3992e8c97108d7';
$sk1 = '3ce3eb784016a53fa915053d24f55dc8fbc7af3fabc915701adb67e61a25f50f';
$k2 = '0f3fe9c20b24a11bf4d6d1acd335c6a80543f1f0380590d7323caf1390c78e88';		
$sk2 = '4967a2bfa0c8a0afc0df238d068b6c7182577afd0781c9d3720bb7a6cf71630c';		#main key
$m = keccak_256(pack 'H*', '5020c4d530b6ec6cb4d9');
@sig = ('b7903a4a3aca7253bb98be335014bebb33683aedca0bc46e288e229ecfccbe0e',
		'2c15e4de88ff38d655e2deef0e06a7ca4541a7754c37e7b20875cce791754508',
		'6acae497177b2eeaf658b813eaf50e1e06f3d1107694beff9b520c65ee624f05',
		'026c8d9801f7330aa82426adf5bacf4546d83df0cc12321ede90df8c0d9aa800');

		
print "   test RSIG -- OK" if r_check_s($m,im_gen($k2,$sk2),$k1, $k2, @sig);
