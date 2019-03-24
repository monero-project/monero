#!/usr/bin/env python3

# Copyright (c) 2019 The Monero Project
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

"""Test speed of weeding out bad txes
"""


import time
from __future__ import print_function
from framework.daemon import Daemon
from framework.wallet import Wallet

N = 100

class RejectSpeedTest():
    def run_test(self):
        self.reset()
        self.mine()
        self.create()
        self.test_speed('noop', '', self.test_noop, N)
        self.test_speed('good', '', self.test_send, N)
        self.test_speed('bad tx version', 'bad-tx-version', self.test_send, N)
        self.test_speed('bad tx fee', 'bad-tx-fee', self.test_send, N)
        self.test_speed('bad ki domain', 'bad-ki-domain', self.test_send, N)
        self.test_speed('bad pseudo out', 'bad-pseudo-out', self.test_send, N)
        self.test_speed('bad mg', 'bad-mg', self.test_send, N)
        self.test_speed('bad bp', 'bad-bp', self.test_send, N)

    def reset(self):
        print 'Resetting blockchain'
        daemon = Daemon()
        res = daemon.get_height()
        daemon.pop_blocks(res.height - 1)
        daemon.flush_txpool()

    def mine(self):
        print('Mining blocks')
        self.daemon = Daemon()
        res = self.daemon.set_log_categories('1,perf*:INFO,verify:INFO')
        self.daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 80)

    def create(self):
        seed = "velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted"
        self.wallet = Wallet()
        self.wallet.restore_deterministic_wallet(seed = seed)
        self.wallet.refresh()
        dst = {'address': '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 'amount': 50000000000000}
        res = self.wallet.transfer(destinations = [dst], do_not_relay = True, get_tx_hex = True, payment_id = '4242424242424242424242424242424424242424242424242424242424242442')
        self.tx_blob = res.tx_blob

    def test_speed(self, msg, debug_invalid, f, N):
        dst = {'address': '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 'amount': 100000000000}
        res = self.wallet.transfer(destinations = [dst]*15, do_not_relay = True, get_tx_hex = True, debug_invalid = debug_invalid)
        self.tx_blob = res.tx_blob
        t = 0
        accepted = 0
        for i in range(N):
            t0 = time.time()
            if f(self.tx_blob):
                accepted += 1
            t1 = time.time()
            t += t1 - t0
            self.daemon.flush_txpool()
        print('%s: %f ms/tx (%u/%u accepted)' % (msg, t * 1000. / N, accepted, N))
        time.sleep(1)

    def test_noop(self, i):
        try: self.daemon.get_version()
        except Exception, e: return False
        return True

    def test_send(self, tx_blob):
        try: return self.daemon.send_raw_transaction(tx_blob, True).status == "OK"
        except Exception, e: return False
        return True

if __name__ == '__main__':
    RejectSpeedTest().run_test()
