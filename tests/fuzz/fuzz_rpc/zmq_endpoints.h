#include "rpc/zmq_pub.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "span.h"
#include <fuzzer/FuzzedDataProvider.h>
#include <zmq.h>
#include <functional>
#include <vector>

using namespace cryptonote;
using namespace cryptonote::listener;

void fuzz_sub_request(zmq_pub&, FuzzedDataProvider&);
void fuzz_send_chain_main(zmq_pub&, FuzzedDataProvider&);
void fuzz_send_miner_data(zmq_pub&, FuzzedDataProvider&);
void fuzz_send_txpool_add(zmq_pub&, FuzzedDataProvider&);

extern std::map<int, std::function<void(zmq_pub&, FuzzedDataProvider&)>> zmq_targets;
