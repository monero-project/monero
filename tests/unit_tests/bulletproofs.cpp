// Copyright (c) 2017-2018, The Monero Project
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

#include "gtest/gtest.h"

#include "string_tools.h"
#include "ringct/rctOps.h"
#include "ringct/rctSigs.h"
#include "ringct/bulletproofs.h"
#include "cryptonote_basic/blobdatatype.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "device/device.hpp"
#include "misc_log_ex.h"

TEST(bulletproofs, valid_zero)
{
  rct::Bulletproof proof = bulletproof_PROVE(0, rct::skGen());
  ASSERT_TRUE(rct::bulletproof_VERIFY(proof));
}

TEST(bulletproofs, valid_max)
{
  rct::Bulletproof proof = bulletproof_PROVE(0xffffffffffffffff, rct::skGen());
  ASSERT_TRUE(rct::bulletproof_VERIFY(proof));
}

TEST(bulletproofs, valid_random)
{
  for (int n = 0; n < 8; ++n)
  {
    rct::Bulletproof proof = bulletproof_PROVE(crypto::rand<uint64_t>(), rct::skGen());
    ASSERT_TRUE(rct::bulletproof_VERIFY(proof));
  }
}

TEST(bulletproofs, valid_multi_random)
{
  for (int n = 0; n < 8; ++n)
  {
    size_t outputs = 2 + n;
    std::vector<uint64_t> amounts;
    rct::keyV gamma;
    for (size_t i = 0; i < outputs; ++i)
    {
      amounts.push_back(crypto::rand<uint64_t>());
      gamma.push_back(rct::skGen());
    }
    rct::Bulletproof proof = bulletproof_PROVE(amounts, gamma);
    ASSERT_TRUE(rct::bulletproof_VERIFY(proof));
  }
}

TEST(bulletproofs, multi_splitting)
{
  rct::ctkeyV sc, pc;
  rct::ctkey sctmp, pctmp;
  std::vector<unsigned int> index;
  std::vector<uint64_t> inamounts, outamounts;

  std::tie(sctmp, pctmp) = rct::ctskpkGen(6000);
  sc.push_back(sctmp);
  pc.push_back(pctmp);
  inamounts.push_back(6000);
  index.push_back(1);

  std::tie(sctmp, pctmp) = rct::ctskpkGen(7000);
  sc.push_back(sctmp);
  pc.push_back(pctmp);
  inamounts.push_back(7000);
  index.push_back(1);

  const int mixin = 3, max_outputs = 16;

  for (int n_outputs = 1; n_outputs <= max_outputs; ++n_outputs)
  {
    std::vector<uint64_t> outamounts;
    rct::keyV amount_keys;
    rct::keyV destinations;
    rct::key Sk, Pk;
    uint64_t available = 6000 + 7000;
    uint64_t amount;
    rct::ctkeyM mixRing(sc.size());

    //add output
    for (size_t i = 0; i < n_outputs; ++i)
    {
      amount = rct::randXmrAmount(available);
      outamounts.push_back(amount);
      amount_keys.push_back(rct::hash_to_scalar(rct::zero()));
      rct::skpkGen(Sk, Pk);
      destinations.push_back(Pk);
      available -= amount;
    }

    for (size_t i = 0; i < sc.size(); ++i)
    {
      for (size_t j = 0; j <= mixin; ++j)
      {
        if (j == 1)
          mixRing[i].push_back(pc[i]);
        else
          mixRing[i].push_back({rct::scalarmultBase(rct::skGen()), rct::scalarmultBase(rct::skGen())});
      }
    }

    rct::ctkeyV outSk;
    rct::rctSig s = rct::genRctSimple(rct::zero(), sc, destinations, inamounts, outamounts, available, mixRing, amount_keys, NULL, NULL, index, outSk, rct::RangeProofPaddedBulletproof, hw::get_device("default"));
    ASSERT_TRUE(rct::verRctSimple(s));
    for (size_t i = 0; i < n_outputs; ++i)
    {
      rct::key mask;
      rct::decodeRctSimple(s, amount_keys[i], i, mask, hw::get_device("default"));
      ASSERT_TRUE(mask == outSk[i].mask);
    }
  }
}

TEST(bulletproofs, valid_aggregated)
{
  static const size_t N_PROOFS = 8;
  std::vector<rct::Bulletproof> proofs(N_PROOFS);
  for (size_t n = 0; n < N_PROOFS; ++n)
  {
    size_t outputs = 2 + n;
    std::vector<uint64_t> amounts;
    rct::keyV gamma;
    for (size_t i = 0; i < outputs; ++i)
    {
      amounts.push_back(crypto::rand<uint64_t>());
      gamma.push_back(rct::skGen());
    }
    proofs[n] = bulletproof_PROVE(amounts, gamma);
  }
  ASSERT_TRUE(rct::bulletproof_VERIFY(proofs));
}


TEST(bulletproofs, invalid_8)
{
  rct::key invalid_amount = rct::zero();
  invalid_amount[8] = 1;
  rct::Bulletproof proof = bulletproof_PROVE(invalid_amount, rct::skGen());
  ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
}

TEST(bulletproofs, invalid_31)
{
  rct::key invalid_amount = rct::zero();
  invalid_amount[31] = 1;
  rct::Bulletproof proof = bulletproof_PROVE(invalid_amount, rct::skGen());
  ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
}

TEST(bulletproofs, invalid_gamma_0)
{
  rct::key invalid_amount = rct::zero();
  invalid_amount[8] = 1;
  rct::key gamma = rct::zero();
  rct::Bulletproof proof = bulletproof_PROVE(invalid_amount, gamma);
  ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
}

static const char * const torsion_elements[] =
{
  "c7176a703d4dd84fba3c0b760d10670f2a2053fa2c39ccc64ec7fd7792ac03fa",
  "0000000000000000000000000000000000000000000000000000000000000000",
  "26e8958fc2b227b045c3f489f2ef98f0d5dfac05d3c63339b13802886d53fc85",
  "ecffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7f",
  "26e8958fc2b227b045c3f489f2ef98f0d5dfac05d3c63339b13802886d53fc05",
  "0000000000000000000000000000000000000000000000000000000000000080",
  "c7176a703d4dd84fba3c0b760d10670f2a2053fa2c39ccc64ec7fd7792ac037a",
};

TEST(bulletproofs, invalid_torsion)
{
  rct::Bulletproof proof = bulletproof_PROVE(7329838943733, rct::skGen());
  ASSERT_TRUE(rct::bulletproof_VERIFY(proof));
  for (const auto &xs: torsion_elements)
  {
    rct::key x;
    ASSERT_TRUE(epee::string_tools::hex_to_pod(xs, x));
    ASSERT_FALSE(rct::isInMainSubgroup(x));
    for (auto &k: proof.V)
    {
      const rct::key org_k = k;
      rct::addKeys(k, org_k, x);
      ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
      k = org_k;
    }
    for (auto &k: proof.L)
    {
      const rct::key org_k = k;
      rct::addKeys(k, org_k, x);
      ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
      k = org_k;
    }
    for (auto &k: proof.R)
    {
      const rct::key org_k = k;
      rct::addKeys(k, org_k, x);
      ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
      k = org_k;
    }
    const rct::key org_A = proof.A;
    rct::addKeys(proof.A, org_A, x);
    ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
    proof.A = org_A;
    const rct::key org_S = proof.S;
    rct::addKeys(proof.S, org_S, x);
    ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
    proof.S = org_S;
    const rct::key org_T1 = proof.T1;
    rct::addKeys(proof.T1, org_T1, x);
    ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
    proof.T1 = org_T1;
    const rct::key org_T2 = proof.T2;
    rct::addKeys(proof.T2, org_T2, x);
    ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
    proof.T2 = org_T2;
  }
}

TEST(bulletproof, weight_equal)
{
  static const char *tx_hex = "02000102000b849b08f2b70b9891019707a8081bc7040d9f0b55d3019669afc83528a6e18454cf13ca392a581098c067df30e66dee8aaddf14c61a8f020002775faa070d3b3ab1d9de66deb402f635aca2580191bce277c26fef7c00cb3f3500025c9c10a978bfe085d42a7b73980f53eab4cbfde73d8023e21978ec8a467375e22101a340cd8bc95636a0ba6ffe5ebfda5eb637d44ad73c32150a469008cb870d22aa03d0cca632f376c5417327569d497d42f09386c5dd4b5efecd9dd20719861ef5aed810e70d824e8e77189c35e6d79993eeeea77b219106df29dd9e77370e7f2fb5ead175064ba8a59397a3ce6804bde23b4d90039c5ad4d1282bc23f791221bc185d70b30d84dda556348a3b9af09513946a03c190b9c53fbeb970a286b1ff8d462630ef0a2737ff40f238461e8ed3eedb8f2a01492abcb96e116ae9d51c4b35e9ba2f3bbe78228618f17a5708c0e30a47b7ed15d4a20ded508f9daddd92e07c6e74167cdf0100000099c4e562de6abd309b4cc26ab41aac39eb0eb252468f79bc5369eae8ba7f94ef2d795fb6b61a0e69e6a95dd3e257615188e80bc1c90c5f571028bb9d2b99c13d41a1e1a770e592ae7a9cda9014f6d4f3233d30f062b774a7241b6e0bb0b83b4a3e36200234a288fcf65cf8a35dfd7710dc5ece5d7abb5ec58451f1cbd41513b1bb6190c609c25e2a2b94eadfe22e8a9eb28ea3d16fa49cb1eb4d7f5c3706b50e7ae60cedf6af2c3e8dc8f96113c029749ae2b266090cc2e6650cf0a869f6c20b0792987702834ff278516dccbd3cff94a6ff36361178a302b37a62c9134b50739228430306ff2bc6a6d282d4cfa9bf6b92486f0e0dd594f2334296e248514c28436b3e86f9d527a8b1ed9f6ed09fa48514364df41d50cb3d376b71b3585cad9de30c465302ae91818ce42eb77e26a31242b4f1255f455df49409197a6d0e468f2c2d781684bb697a785ac77d41950901e9b67a2a4d6a3ec05fffec9e3a0313c972120ac3f5e01f1bc595438d7e07ff6de4ede96915a8696bcbaf449fae978565eceaebe2c3bd2f8315c535ff25fa8924fc2d49e0cb7ecc1c3fd72ce821513fa113078fda233e1588022c6267ba2f78a8a4f9ac8c7ea2dc4dca464902f46fb92702db8d26afa628f2aa182c2b34768a2b0581e7196ce041e73924af51d713db75093bf292e4263be8fc08a0b2f531e1a10ce79b95ab1fab726478cea8e79e0313ffc895069938ecf7ed14a037577f4f461ae6cde9bae6ade8a1d9e46040321b250d7ff9f3612b278757717596040dc58e7f68687b72c1ba71f36daeeb7ebdcbfd77d3518dff7d0fee252887ee38db33dffd714924d5823c539288d581eba17053beb273a13ca6f43132da705308bdc53c80c45e347bffb5c1fae7907369598660ce2c70d34083fec197b914c3b77f50e57ec54d89d0031df92a1241d40f9ea3ed14008ecc339323118ad22adca5c56687f854bc5fd47a3223016eee46e7d94b31a101df22d87b1404bbceaaaab2a8bde72aa318d3364e8926119d792cad21e51faf0cbd5ea0bbe939c5bcfbaa489dfda38aa124f3fc007b9e58f55ad8acd25d17a40bd4c1c17e03610fecb789702b0b8a4aa3a79028a7292212c550dec72f2c356f02bc0f2a0513ae07892143b8aa5ab30e9f6d71eeb3df2ea64a839b5b857000db043bf506a26953a909116b10cdce03a27d549db2f51f9a341c721bb0e442b5d0034038fbb0cd2ef27fb48f5acbd6b4104af18a98a1692d10d59884fcd2eb4641000ac32df57b5dcf387c4c097e5e7e702b2f07cdb18a69d5c69a5f7e135a9f8e020670758a1e4d955878de2f93181adfddd8cff4d20365c4663e870ff09d6b15065bbd81555d6aeb92e07ebbeae426cd0ab982a03ffeec31627ae140cd1e78f60ab6a55811d9d4051d50050c9e920e0b11c526530e613e0d3f925271f90ef0990e3df2c46170153e553a0035c0e8e87d957f40f072fd6b1ff30ee7aca3af88c40f1c255b3546dba9d23f352c729a0466729918336560df233843734e7dad57960f8d5592a299f6b762efdbd37aa0ff5310c940d03622023146a042079c8097fe01606594ab3578d0c0a90f8088d5c93504896ed80e809d22bf9483bf62398feb06099904cc23480b27709845ef1e26059d4730aeb5c2bb34c2ff34bff3c1a1c10a5898584fac078225bd435541fd2f4244e14118c8a08af7a3027d41b7af62420d12ba05466f905fe49882db44994180a1a549acfec42549254feda65aa6ee0c0e35e5a7525ae373ea0053fd536d4b6605ee833a0fa85e863807c30f02b46fde0305864da7d10f60b44ec1c2944a45de27912a39cebdc0ae18034397e4f5cfaf0ebe9ea5b225e80075f1bf6ac2211b7512870cc556e685a2464bf91100b36e5d0ea64af85d92d2aa1c2625e5bcbe93352a92dec8d735e54a2e6dfba6a91cc7c40e5c883d932769ce2d57b21ba898a2437ae6a39cfda1f3adefab0241548ad88104cbf113df4d1a243a5ae639b75169ae60b2c0dd1091a994e2a4d6d3536e3f4405a723c50ba4e9f822a2de189fd8158b0aa94c4b6255e5d4b504f789e4036d4206e8afd25693198f7bb3b04c23a6dc83f09260ae7c83726d4d524e7f9f851c39f5";
  cryptonote::blobdata bd;
  ASSERT_TRUE(epee::string_tools::parse_hexstr_to_binbuff(std::string(tx_hex), bd));
  cryptonote::transaction tx;
  crypto::hash tx_hash, tx_prefix_hash;
  ASSERT_TRUE(parse_and_validate_tx_from_blob(bd, tx, tx_hash, tx_prefix_hash));
  ASSERT_TRUE(tx.version == 2);
  ASSERT_TRUE(rct::is_rct_bulletproof(tx.rct_signatures.type));
  const uint64_t tx_size = bd.size();
  const uint64_t tx_weight = cryptonote::get_transaction_weight(tx);
  ASSERT_TRUE(tx_weight == tx_size); // it has two outputs, <= 2 makes weight == size
}

TEST(bulletproof, weight_more)
{
  static const char *tx_hex = "02000102000be98714944aeb01c006c80cbd0aaa04e5023e9003fa089669afc83528a6e18454cf13ca392a581098c067df30e66dee8aaddf14c61a8f040002377a0483ad63d58e7667a8325349c89e41e9ad5dce5aef30204fd4a6dc8eb7a100022a518afc3d690a992150646c559a24add698c98e87e732244cf2855cb7d0cff10002485a8de9d099c96fce8f26ad320cd627a6bb188f719c380517861c031aa65011000218ebf7b40a5ba25fb98ad7c6543239c2a3343b9e7cfd5140280e587c8f930f0d2101b330267408724dcf7fc2902e7d74a7962995e7905cc5d043fa1c8c6379e0eccd03e089bb498d0fffca61afd0d61f4875e8b32fa63f8729c654bba5f167199b7b518433680242640504c7568d273b2a74fe2d204dbb97eea4724ee5a2cf19eb3349ec3a2602a244de2a33c62bbfaa0b4cb85bf36863f765b237138929e43462e4bf19684d0924fd73c30bee474f0e927e8eb84dd6cc987acaf41e19e2f1e07381bd95e0f504964c8d10793972e88d64683a4a3960a9645735a76cc62d99e7a87b62c3cb590ca9dc27f91e9103c1b55ece5d5a932a04c99bf019463455b5d78397bd2295be075af5ae9bf0e43e724e11f83f336ca3c1bd9601c7fd6642795e8618b5c5b9d0045a6766daf2118f994b418504c6939b94c72e875423989ea7069d73e8d02f7b0bd9c1c7eb2289eaeda5fabd8142ee0ebafcbe101c58e99034d0c9ca34a703180e1dd7000e9f11cb4bcbe11f0c0041a0cc30f5b8b3bd7f2ace0266dd0282aea17f088c88e98a22f764e32507d1c900ef50b1157b49dbfda2fd9a2ea3be5182fb10fa590b464907049d88ff9c33fbe6d8b05898abf196dd097e1009d9bd1a1997830100000006ccdcc8aa53e183578656540fa393e6d9f96c07497b9dd009b48e8f990a75be18f5f37a47ff07f3c4d6427626afc5d24897ad31a98d01cc44476fba41f6bc3dc3de91d2655130090393e3ebcb7f436470edefff5aa11fa1016fecccf1824a5cdd66ea51dcd8f0193a6e507309d6a13680605febb971c3df4cceac5078be996c0d686c72627696e6961447145143e23cae2c97686524c0587b6cce7b05851b087d658a795bd22de18c0f68e824e1673c47f4b7f4ba7d4bedd95f46ffc22d9409087a58a80088679d3775e46f75dc6dc48f485a2c35a8dd60e7dabe29f656cbaad8d25a5e01d165fa9df29acd6e2471c4880d3129fd110066788de9c979f03d9c2e46ab80bf0dbd24ac6aaba2db0d723e1ba3a002efd2aebf245a2fd53767fa490244396284826b64a4a3f069f21047486a27c5ebdf802c23d8d276b9c83fa2e329aefb953ad34f382975204706c14249a496791cd3d20f4bd98d8f6325f5e7d7aa2d1f8b23361434f74584136cf1ead94365a6ce134159141fa4c68660a99ad90caa8c711abe5411dffa7132f8ce71dba63619e1410380c56766c79ff8e433eb806f49bca6f1bcb0c66dfd61c3133e7c095a11abd068b6a5774a02a5825cbd82a408d5580ee4dbe9a4ba07282f5a764279ad27f7ac27e7cccb3c76b5dd64be7bbdf3ecc4abcc29bc561e81cbc502ed3a4ce277b567a6eb09dfd8454c4d4e8c038b9dd6042a0515b0d1dfbb45585e79ca5705a22fcb3c67bc0261cc0cec6998354448e83fa7ff8706178e14a482e73719df33c9d753757131f3560391be2dd6c40391e3e7882ea07bb23c4d2d157349965082e1447e94849fde224452f6c98efa44f6438b731859fac8f49761e4447e8d34275e7dd9ae01d8550dcc75284715e026d25e9c444265fb4fee3f783ff2a2a5c414714a57525738884bbdbd8d997dbbcafcafd8e283b524bef0ded141160f47ce352b2104257b312d12594de45a0241d9753ec19e2f8603b5fc8682d72bf1de51d7f4caa7026989a7e46b9dd41075ad480df9de6a952e8562e548e3576e9c9230cb2cd0ee7f955e2d29240d7fa55b8e0b0b6c92823d9636592c460af670bb0b8714ca626497c68403793fe8495a7542c60587d117e3adb3644e62053817fa600910e2dfad97b2a7492ac6fa13c0a9a03e0ce12c3d12a09a4e22b9d0d74b9d431d53252fcbd06cb119d128646042eef81002fc6e9ff5006e06247f40f0391ad095c0d50a78863c975edbf0498e58ba7e6e0505d5eaa4d8ba2aa3f40e728f1a6aef6b2f9f5705cc3d2591bb5b878c258b4107857dc6ee75d591ea7af7b16196cd6c979a0bf819db39658491889c83a41c2c0035116fcac23ac45144731592ea0e3a11c335a278b2a6798d46828c8590b92701468e9d9c3560ed58c3ab861995aca439ee49ecf4a5b0ad160a12bd23a437f90c383095e85a95bb441bcb8dc1752e805e85d1ee0f6967ce95dcd888efa7ac440813c78b11d56d2a1b870c59d36430b10cd28bc693c4f64e769acfbcff27d8e904e0ca7e73aab5439de571b66f91fe9c05264e070aa223b68e5de763d838985e0ec0e8ac0dd0b1f6eb1e145c4473f9edda5732b1f9d3627423b5c60e055377bd044ff30017d25b3d26b5590b53d8aeaf10ce73d86fe4c40fa14e6f710f72c7da0600e7fc495a75a875d1aeee246b70cf24a8a85fba2d31f96faa42ece112b9030987ce0e735ab51eb4222a48bc51ab69d644bda77fb2aa0cd3a0477a2a2d92510103dbd58ee1c28eb20cfb31f5268f4a70a431ff4aedbfdfc59ea6709283a51902202effba960da6170b1a25c26a52890da54757c93156d250540590266eed8c00647270cb302cff7cbe4a8ed27da21dbaa303d1aea0eb152e1f6fd24dbdfaea0c5b0f5d6acb6724cf711ec3194a94f52f8cce13e1e3d1d7758d3d7e3cd37fd1011265199eb4126975687ce958dd1a75b6a71cf397fb618003e85af842dc3ff50a134411bbe18a1dea4beeb1e8d1ca5ac67f7f6ce2bbdeb2efcf6dcfdef64b360d4fb1849947800a3595e0a8029b631a06508b5d9f4f6e6a1be110524e5584f209b9db1651ddc8571102a58e7823dcf026f89d59ca213b6c6e32088d6c4967b20b28ffe86aed6c11d6aa0072691ff133d7bbc6d013629faebadc087c0f4f84d106677013893be4ca55018fbafcc2cee8be4ad0bcf1ad8762ec0c285e8c414bb204";
  cryptonote::blobdata bd;
  ASSERT_TRUE(epee::string_tools::parse_hexstr_to_binbuff(std::string(tx_hex), bd));
  cryptonote::transaction tx;
  crypto::hash tx_hash, tx_prefix_hash;
  ASSERT_TRUE(parse_and_validate_tx_from_blob(bd, tx, tx_hash, tx_prefix_hash));
  ASSERT_TRUE(tx.version == 2);
  ASSERT_TRUE(rct::is_rct_bulletproof(tx.rct_signatures.type));
  const uint64_t tx_size = bd.size();
  const uint64_t tx_weight = cryptonote::get_transaction_weight(tx);
  ASSERT_TRUE(tx_weight > tx_size); // it has four outputs, > 2 makes weight > size
}
