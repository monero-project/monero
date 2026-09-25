// Copyright (c) 2024, The Monero Project
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

#include "gtest/gtest.h"

#include "common/container_helpers.h"
#include "common/threadpool.h"
#include "crypto/crypto.h"
#include "crypto/generators.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/verification_context.h"
#include "cryptonote_core/tx_verification_utils.h"
#include "fcmp_pp/fcmp_pp_serialization.h"
#include "fcmp_pp/fcmp_pp_types.h"
#include "fcmp_pp/verify.h"
#include "misc_log_ex.h"
#include "profile_tools.h"
#include "ringct/rctOps.h"
#include "ringct/rctSigs.h"
#include "unit_tests_utils.h"
#include "serialization/binary_utils.h"
#include "string_tools.h"

#include <utility>
#include <vector>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "unit_tests.fcmp_pp"

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
TEST(fcmp_pp, verify_from_file)
{
    const std::size_t n_inputs = 128;

    // We want a proof per thread, or 4 proofs if < 4 threads
    tools::threadpool& tpool = tools::threadpool::getInstanceForCompute();
    const std::size_t n_proofs = std::max<std::size_t>(4, tpool.get_max_concurrency());

    // We repeat verify attempts in hopes of using all malloc arenas (glibc specific)
    const std::size_t repeat_n_times = 10;

    // Read from file
    crypto::hash signable_tx_hash;
    std::vector<uint8_t> fcmp_pp_proof;
    uint8_t n_layers;
    fcmp_pp::TreeRootShared tree_root;
    std::vector<crypto::ec_point> pseudo_outs;
    std::vector<crypto::key_image> key_images;

    bool r = unit_test::read_fcmp_pp_verify_input_from_file(
        n_inputs,
        signable_tx_hash,
        fcmp_pp_proof,
        n_layers,
        tree_root,
        pseudo_outs,
        key_images);
    ASSERT_TRUE(r);

    // Test single verification
    LOG_PRINT_L1("Verifying (n_inputs=" << n_inputs << ")");
    bool verify = fcmp_pp::verify(
            signable_tx_hash,
            fcmp_pp_proof,
            n_layers,
            tree_root,
            pseudo_outs,
            key_images
        );
    ASSERT_TRUE(verify);
    LOG_PRINT_L1("Successfully verified (n_inputs=" << n_inputs << ")");

    // Repeat batch verify attempts and observe memory usage
    for (std::size_t i = 0; i < repeat_n_times; ++i)
    {
        // Collect the FCMP++ verify inputs
        std::vector<fcmp_pp::FcmpPpVerifyInput> fcmp_pp_verify_inputs;
        fcmp_pp_verify_inputs.reserve(n_proofs);
        for (std::size_t i = 0; i < n_proofs; ++i)
        {
            fcmp_pp_verify_inputs.emplace_back(fcmp_pp::fcmp_pp_verify_input_new(
                    signable_tx_hash,
                    fcmp_pp_proof,
                    n_layers,
                    tree_root,
                    pseudo_outs,
                    key_images
                ));
        }

        // Verify FCMP++ 128-in proofs in parallel using the batch verifier
        LOG_PRINT_L1("Batch verifying " << n_proofs << " FCMP++ txs, attempt " << i+1);
        ASSERT_TRUE(rct::batchVerifyFcmpPpProofs(std::move(fcmp_pp_verify_inputs)));
        LOG_PRINT_L1("Successfully batch verified " << n_proofs << " FCMP++ txs, attempt " << i+1);
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(fcmp_pp, force_init_gen_u_v)
{
#ifdef NDEBUG
    GTEST_SKIP() << "Generator reproduction assert statements don't trigger on Release builds";
#endif

    // these will cause assertion failures in debug mode if the seeds are wrong
    const ge_p3 U_p3 = crypto::get_U_p3();
    const ge_p3 V_p3 = crypto::get_V_p3();
    const ge_cached U_cached = crypto::get_U_cached();
    const ge_cached V_cached = crypto::get_V_cached();

    (void) U_p3, (void) V_p3, (void) U_cached, (void) V_cached;
}
//----------------------------------------------------------------------------------------------------------------------
TEST(fcmp_pp, proof_size_table)
{
    static const uint32_t PROOF_LEN_TABLE[FCMP_PLUS_PLUS_MAX_INPUTS][FCMP_PLUS_PLUS_MAX_LAYERS]
    {
        {3136, 3776, 4160, 4800, 5184, 5824, 6208, 6848, 6272, 5696, 5952, 6336, },
        {4288, 5568, 4992, 6272, 6784, 6592, 7104, 7872, 7424, 6976, 7360, 7872, },
        {3840, 5760, 6528, 6976, 6528, 6592, 7232, 8000, 8512, 9280, 9088, 8768, },
        {4352, 6912, 6592, 7168, 7936, 8000, 8768, 9792, 9600, 9408, 10048, 10816, },
        {4992, 6976, 6528, 7232, 8256, 9536, 9344, 9408, 10176, 11200, 11968, 12864, },
        {3904, 6272, 7424, 8256, 8192, 8640, 9664, 10816, 11712, 12864, 13056, 13120, },
        {4160, 6912, 7104, 8064, 9216, 9792, 10944, 12224, 12416, 12736, 13760, 14912, },
        {4416, 7552, 7744, 8832, 10112, 10688, 11968, 13504, 13824, 14144, 15296, 16576, },
        {4800, 7232, 7424, 8640, 10048, 11840, 12288, 12864, 14144, 15680, 16960, 18368, },
        {5056, 7744, 7936, 9280, 10944, 12864, 13312, 14016, 15424, 17088, 18496, 20032, },
        {3840, 6784, 8576, 10048, 10624, 11840, 13504, 15168, 16704, 18496, 20160, 20992, },
        {3968, 7168, 9088, 10688, 11392, 12608, 14400, 16320, 17984, 19904, 20864, 21696, },
        {4096, 7552, 9600, 11328, 12160, 13504, 15424, 17472, 19264, 20480, 21440, 23232, },
        {4224, 7936, 9024, 10880, 12928, 14400, 16448, 18624, 19712, 20928, 22848, 24896, },
        {4352, 8320, 9536, 11520, 13696, 15296, 17472, 19776, 20864, 22208, 24256, 26432, },
        {4480, 8704, 9920, 12032, 14336, 15936, 18240, 20800, 22144, 23488, 25664, 27968, },
        {4736, 8256, 9472, 11712, 14144, 16832, 18304, 20032, 22464, 24896, 27200, 29632, },
        {4864, 8512, 9856, 12224, 14784, 17728, 19328, 21056, 23488, 26176, 28608, 31168, },
        {4992, 8896, 10368, 12864, 15552, 18624, 20224, 22080, 24768, 27456, 30016, 32832, },
        {5120, 9152, 10624, 13248, 16192, 19392, 21120, 23104, 25792, 28736, 31424, 34240, },
        {5248, 9536, 11136, 13888, 15872, 19200, 22016, 24128, 27072, 30144, 32960, 35904, },
        {3904, 8320, 11520, 14400, 16384, 19008, 22080, 25152, 28096, 31296, 34368, 36608, },
        {3904, 8576, 11904, 14912, 17152, 19776, 22848, 26176, 29376, 32704, 34944, 37184, },
        {4032, 8832, 12288, 15424, 17664, 20416, 23744, 27200, 30400, 33856, 36352, 38720, },
        {4160, 9216, 12800, 16064, 18432, 21312, 24640, 28224, 31680, 35264, 37760, 40256, },
        {4160, 9344, 13056, 16448, 18944, 21952, 25536, 29248, 32704, 35584, 38208, 41664, },
        {4288, 9728, 13568, 17088, 19584, 22720, 26432, 30272, 33152, 36032, 39616, 43328, },
        {4288, 9856, 12736, 16384, 20224, 23488, 27328, 31296, 34176, 37184, 40896, 44736, },
        {4416, 10240, 13120, 16896, 20864, 24128, 28096, 32320, 35328, 38464, 42304, 46144, },
        {4416, 10368, 13504, 17408, 21504, 25024, 29120, 33344, 36352, 39616, 43584, 47680, },
        {4544, 10752, 13888, 17920, 22144, 25664, 29888, 34368, 37632, 40896, 44992, 49216, },
        {4544, 10880, 14144, 18304, 22656, 26304, 30656, 35264, 38656, 42048, 46272, 50624, },
        {4672, 10304, 13696, 17984, 22464, 27200, 30720, 34496, 38848, 43328, 47680, 52160, },
        {4800, 10560, 13952, 18368, 22976, 27840, 31488, 35392, 40000, 44608, 49088, 53696, },
        {4800, 10688, 14336, 18880, 23616, 28736, 32512, 36416, 41024, 45760, 50368, 55232, },
        {4928, 10944, 14592, 19264, 24128, 29376, 33280, 37312, 42048, 47040, 51776, 56640, },
        {4928, 11200, 14976, 19776, 24896, 30144, 34048, 38336, 43200, 48192, 53056, 58048, },
        {5056, 11456, 15360, 20288, 25408, 30912, 34944, 39232, 44352, 49472, 54464, 59712, },
        {5056, 11584, 15616, 20672, 25920, 31552, 35840, 40256, 45376, 50752, 55872, 61120, },
        {5184, 11840, 15872, 21056, 26560, 32320, 36608, 41152, 46400, 51904, 57152, 62528, },
        {5312, 12224, 16384, 21696, 26240, 32128, 37504, 42176, 47552, 53056, 58560, 64192, },
        {5312, 12352, 16640, 22080, 26752, 32768, 38272, 43072, 48704, 54464, 59968, 65600, },
        {3968, 11136, 16896, 22464, 27264, 32576, 38336, 44096, 49728, 55616, 61248, 66176, },
        {3968, 11264, 17280, 22976, 27776, 33216, 39104, 44992, 50752, 56768, 62656, 67712, },
        {3968, 11520, 17664, 23488, 28544, 33984, 39872, 46016, 51904, 58048, 64064, 69120, },
        {3968, 11648, 17920, 23872, 29056, 34624, 40640, 46912, 53056, 59328, 64512, 69696, },
        {4096, 11904, 18304, 24384, 29568, 35264, 41536, 47936, 54080, 60480, 65792, 71104, },
        {4096, 12032, 18560, 24768, 30080, 35904, 42304, 48832, 55104, 61632, 67200, 72640, },
        {4096, 12288, 18944, 25280, 30720, 36672, 43200, 49856, 56384, 63040, 68608, 74176, },
        {4224, 12544, 19328, 25792, 31360, 37440, 43968, 50752, 57408, 64192, 69888, 75584, },
        {4224, 12672, 19584, 26176, 31872, 38080, 44736, 51648, 58432, 65344, 71168, 76992, },
        {4224, 12800, 19840, 26560, 32384, 38720, 45632, 52672, 59456, 65664, 71616, 78400, },
        {4224, 13056, 20352, 27200, 33024, 39360, 46400, 53696, 60736, 66944, 72896, 79808, },
        {4352, 13312, 20608, 27584, 33536, 40128, 47296, 54592, 60928, 67264, 74304, 81472, },
        {4352, 13440, 19776, 26880, 34176, 40896, 48064, 55488, 61952, 68416, 75584, 82880, },
        {4352, 13568, 20032, 27264, 34688, 41536, 48960, 56512, 62976, 69568, 76864, 84288, },
        {4480, 13952, 20416, 27776, 35328, 42176, 49728, 57536, 64128, 70720, 78144, 85696, },
        {4480, 14080, 20672, 28160, 35840, 42816, 50496, 58432, 65152, 72000, 79552, 87104, },
        {4480, 14208, 21056, 28672, 36352, 43584, 51392, 59328, 66176, 73152, 80832, 88640, },
        {4480, 14336, 21312, 29056, 36992, 44352, 52288, 60352, 67200, 74304, 82112, 90048, },
        {4608, 14720, 21696, 29568, 37632, 44992, 53056, 61376, 68480, 75584, 83520, 91584, },
        {4608, 14848, 21952, 29952, 38144, 45632, 53824, 62272, 69504, 76736, 84800, 92992, },
        {4608, 14976, 22208, 30336, 38656, 46272, 54592, 63168, 70528, 77888, 86080, 94400, },
        {4608, 15104, 22464, 30720, 39168, 46912, 55360, 64064, 71552, 79040, 87360, 95808, },
        {4736, 14528, 22016, 30400, 38976, 47808, 55424, 63296, 71744, 80320, 88768, 97344, },
        {4736, 14656, 22272, 30784, 39488, 48448, 56192, 64192, 72768, 81472, 90048, 98752, },
        {4736, 14784, 22528, 31168, 40000, 49088, 56960, 65088, 73792, 82624, 91328, 100160, },
        {4864, 15040, 22784, 31552, 40512, 49728, 57728, 65984, 74944, 83904, 92736, 101696, },
        {4864, 15168, 23040, 31936, 41152, 50496, 58624, 67008, 75968, 85056, 94016, 103104, },
        {4864, 15296, 23424, 32448, 41664, 51264, 59520, 67904, 76992, 86208, 95296, 104640, },
        {4864, 15424, 23680, 32832, 42176, 51904, 60288, 68800, 78016, 87488, 96704, 106048, },
        {4992, 15680, 23936, 33216, 42688, 52544, 61056, 69696, 79040, 88640, 97984, 107456, },
        {4992, 15936, 24320, 33728, 43328, 53184, 61824, 70720, 80192, 89792, 99264, 108864, },
        {4992, 16064, 24576, 34112, 43968, 53952, 62592, 71616, 81216, 90944, 100544, 110272, },
        {5120, 16320, 24832, 34496, 44480, 54720, 63488, 72512, 82240, 92096, 101952, 111936, },
        {5120, 16448, 25216, 35008, 44992, 55360, 64256, 73408, 83392, 93376, 103232, 113344, },
        {5120, 16576, 25472, 35392, 45504, 56000, 65152, 74432, 84416, 94656, 104640, 114752, },
        {5120, 16704, 25728, 35776, 46016, 56640, 65920, 75328, 85440, 95808, 105920, 116160, },
        {5248, 16960, 25984, 36160, 46656, 57408, 66688, 76224, 86464, 96960, 107200, 117568, },
        {5248, 17088, 26240, 36544, 47168, 58048, 67456, 77120, 87488, 98112, 108480, 118976, },
        {5248, 17344, 26624, 37056, 47808, 58816, 68352, 78144, 88640, 99264, 109888, 120640, },
        {5376, 17600, 27008, 37568, 47360, 58496, 69120, 79040, 89664, 100416, 111168, 122048, },
        {5376, 17728, 27264, 37952, 47872, 59136, 69888, 79936, 90816, 101696, 112448, 123456, },
        {5376, 17856, 27520, 38336, 48384, 59776, 70656, 80832, 91840, 102976, 113856, 124864, },
        {3904, 16512, 27776, 38720, 48896, 60416, 71424, 81728, 92864, 104128, 115136, 126272, },
        {4032, 16768, 28032, 39104, 49408, 60224, 71488, 82752, 93888, 105280, 116416, 126848, },
        {4032, 16896, 28288, 39488, 49920, 60864, 72256, 83648, 94912, 106432, 117696, 128256, },
        {4032, 17024, 28672, 40000, 50432, 61504, 73024, 84544, 95936, 107584, 119104, 129792, },
        {4032, 17280, 29056, 40512, 51200, 62272, 73792, 85568, 97088, 108736, 120384, 131200, },
        {4032, 17408, 29312, 40896, 51712, 62912, 74560, 86464, 98112, 110016, 121792, 132608, },
        {4032, 17536, 29568, 41280, 52224, 63552, 75328, 87360, 99264, 111296, 123072, 134016, },
        {4032, 17664, 29824, 41664, 52736, 64192, 76096, 88256, 100288, 112448, 123520, 134592, },
        {4160, 17920, 30080, 42048, 53248, 64832, 76864, 89152, 101312, 113600, 124800, 136000, },
        {4160, 18048, 30464, 42560, 53760, 65472, 77760, 90176, 102336, 114752, 126080, 137408, },
        {4160, 18176, 30720, 42944, 54272, 66112, 78528, 91072, 103360, 115904, 127488, 138944, },
        {4160, 18304, 30976, 43328, 54784, 66752, 79296, 91968, 104384, 117056, 128768, 140352, },
        {4160, 18560, 31360, 43840, 55424, 67520, 80192, 92992, 105536, 118336, 130176, 141888, },
        {4160, 18688, 31616, 44224, 55936, 68160, 80960, 93888, 106688, 119616, 131456, 143296, },
        {4160, 18816, 32000, 44736, 56576, 68928, 81728, 94784, 107712, 120768, 132736, 144704, },
        {4288, 19072, 32256, 45120, 57088, 69568, 82496, 95680, 108736, 121920, 134016, 146112, },
        {4288, 19200, 32512, 45504, 57600, 70208, 83264, 96576, 109760, 123072, 135296, 147520, },
        {4288, 19328, 32768, 45888, 58112, 70848, 84032, 97472, 110784, 124224, 136576, 148928, },
        {4288, 19456, 33024, 46272, 58624, 71488, 84928, 98496, 111808, 124544, 137024, 150336, },
        {4288, 19584, 33280, 46656, 59136, 72128, 85696, 99392, 112832, 125696, 138304, 151744, },
        {4288, 19840, 33792, 47296, 59776, 72768, 86464, 100416, 113984, 126848, 139584, 153152, },
        {4288, 19968, 34048, 47680, 60288, 73408, 87232, 101312, 115136, 128128, 140864, 154560, },
        {4416, 20224, 34304, 48064, 60800, 74176, 88128, 102208, 116160, 129280, 142144, 156096, },
        {4416, 20352, 34560, 48448, 61312, 74816, 88896, 103104, 116352, 129600, 143552, 157632, },
        {4416, 20480, 34816, 48832, 61952, 75584, 89664, 104000, 117376, 130752, 144832, 159040, },
        {4416, 20608, 33984, 48128, 62464, 76224, 90432, 104896, 118400, 131904, 146112, 160448, },
        {4416, 20736, 34240, 48512, 62976, 76864, 91328, 105920, 119424, 133056, 147392, 161856, },
        {4416, 20864, 34496, 48896, 63488, 77504, 92096, 106816, 120448, 134208, 148672, 163264, },
        {4416, 21120, 34880, 49408, 64128, 78144, 92864, 107840, 121600, 135360, 149952, 164672, },
        {4544, 21376, 35136, 49792, 64640, 78784, 93632, 108736, 122624, 136512, 151232, 166080, },
        {4544, 21504, 35392, 50176, 65152, 79424, 94400, 109632, 123648, 137664, 152512, 167488, },
        {4544, 21632, 35648, 50560, 65664, 80064, 95168, 110528, 124672, 138944, 153920, 168896, },
        {4544, 21760, 36032, 51072, 66176, 80704, 95936, 111424, 125696, 140096, 155200, 170304, },
        {4544, 21888, 36288, 51456, 66688, 81472, 96832, 112320, 126720, 141248, 156480, 171840, },
        {4544, 22016, 36544, 51840, 67328, 82240, 97600, 113216, 127744, 142400, 157760, 173248, },
        {4544, 22144, 36800, 52224, 67840, 82880, 98496, 114240, 128768, 143552, 159040, 174656, },
        {4672, 22528, 37184, 52736, 68480, 83520, 99264, 115264, 130048, 144832, 160320, 176064, },
        {4672, 22656, 37440, 53120, 68992, 84160, 100032, 116160, 131072, 145984, 161728, 177600, },
        {4672, 22784, 37696, 53504, 69504, 84800, 100800, 117056, 132096, 147136, 163008, 179008, },
        {4672, 22912, 37952, 53888, 70016, 85440, 101568, 117952, 133120, 148288, 164288, 180416, },
        {4672, 23040, 38208, 54272, 70528, 86080, 102336, 118848, 134144, 149440, 165568, 181824, },
        {4672, 23168, 38464, 54656, 71040, 86720, 103104, 119744, 135168, 150592, 166848, 183232, },
        {4672, 23296, 38720, 55040, 71552, 87360, 103872, 120640, 136192, 151744, 168128, 184640, },
        {4672, 23424, 38976, 55424, 72064, 88000, 104640, 121536, 137216, 152896, 169408, 186048, },
    };

    for (std::size_t i = 1; i <= FCMP_PLUS_PLUS_MAX_INPUTS; ++i)
    {
        // Uncomment the prints to construct the table
        // printf("{");
        for (std::size_t j = 1; j <= FCMP_PLUS_PLUS_MAX_LAYERS; ++j)
        {
            const std::size_t membership_proof_len = fcmp_pp::membership_proof_len(i, j);
            const std::size_t fcmp_pp_proof_len = fcmp_pp::fcmp_pp_proof_len(i, j);

            EXPECT_EQ(membership_proof_len, PROOF_LEN_TABLE[i-1][j-1]);
            EXPECT_EQ(fcmp_pp_proof_len, ::fcmp_pp_proof_size(i, j));

            // printf("%lu, ", membership_proof_len);
        }
        // printf("},\n");
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(fcmp_pp, output_serialization)
{
    static const crypto::public_key O{0x01};
    static const crypto::ec_point C{0x02};

    static const std::string O_str = "0100000000000000000000000000000000000000000000000000000000000000";
    static const std::string C_str = "0200000000000000000000000000000000000000000000000000000000000000";

    // 1. Check serialized bytes size
    const fcmp_pp::OutputPair legacy_output_pair = fcmp_pp::LegacyOutputPair{{O, C}};
    const fcmp_pp::OutputPair carrot_output_pair = fcmp_pp::CarrotOutputPairV1{{O, C}};
    ASSERT_FALSE(legacy_output_pair == carrot_output_pair);

    const cryptonote::blobdata legacy_serialized = cryptonote::t_serializable_object_to_blob(legacy_output_pair);
    const cryptonote::blobdata carrot_out_v1_ser = cryptonote::t_serializable_object_to_blob(carrot_output_pair);

    ASSERT_EQ(legacy_serialized.size(), (1+32+32));
    ASSERT_EQ(carrot_out_v1_ser.size(), (1+32+32));

    // 2. Check serialized byte string
    const std::string legacy_hex = epee::string_tools::buff_to_hex_nodelimer(legacy_serialized);
    const std::string carrot_hex = epee::string_tools::buff_to_hex_nodelimer(carrot_out_v1_ser);

    static const std::string LEGACY_TAG = "70";
    static const std::string CARROT_TAG = "71";

    ASSERT_EQ(legacy_hex, LEGACY_TAG + O_str + C_str);
    ASSERT_EQ(carrot_hex, CARROT_TAG + O_str + C_str);

    // 3. Check de-serialization
    const std::vector<std::pair<fcmp_pp::OutputPair, cryptonote::blobdata>> op_vec{
            {legacy_output_pair, legacy_serialized},
            {carrot_output_pair, carrot_out_v1_ser}
        };

    for (const auto &serialized : op_vec)
    {
        fcmp_pp::OutputPair output;
        ASSERT_TRUE(::serialization::parse_binary(serialized.second, output));
        ASSERT_EQ(output, serialized.first);
    }

    // 4. Check UnifiedOutput
    const uint64_t output_id = 8321;
    const fcmp_pp::UnifiedOutput legacy_unified_output{output_id, legacy_output_pair};
    const fcmp_pp::UnifiedOutput carrot_unified_output{output_id, carrot_output_pair};

    const cryptonote::blobdata legacy_unified_ser = cryptonote::t_serializable_object_to_blob(legacy_unified_output);
    const cryptonote::blobdata carrot_unified_ser = cryptonote::t_serializable_object_to_blob(carrot_unified_output);

    ASSERT_EQ(legacy_unified_ser.size(), SIZEOF_SERIALIZED_UNIFIED_OUTPUT);
    ASSERT_EQ(carrot_unified_ser.size(), SIZEOF_SERIALIZED_UNIFIED_OUTPUT);

    const std::string legacy_unified_hex = epee::string_tools::buff_to_hex_nodelimer(legacy_unified_ser);
    const std::string carrot_unified_hex = epee::string_tools::buff_to_hex_nodelimer(carrot_unified_ser);
    const std::string outpt_id_hex = epee::string_tools::buff_to_hex_nodelimer(
            cryptonote::t_serializable_object_to_blob(output_id)
        );

    ASSERT_EQ(legacy_unified_hex, outpt_id_hex + LEGACY_TAG + O_str + C_str);
    ASSERT_EQ(carrot_unified_hex, outpt_id_hex + CARROT_TAG + O_str + C_str);

    const std::vector<std::pair<fcmp_pp::UnifiedOutput, cryptonote::blobdata>> unified_vec{
            {legacy_unified_output, legacy_unified_ser},
            {carrot_unified_output, carrot_unified_ser}
        };

    for (const auto &serialized : unified_vec)
    {
        fcmp_pp::UnifiedOutput output;
        ASSERT_TRUE(::serialization::parse_binary(serialized.second, output));
        ASSERT_EQ(output, serialized.first);
    }
}
//----------------------------------------------------------------------------------------------------------------------
