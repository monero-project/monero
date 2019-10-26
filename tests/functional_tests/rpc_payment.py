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

from __future__ import print_function
import subprocess
import os

"""Test daemon RPC payment calls
"""

from framework.daemon import Daemon
from framework.wallet import Wallet

class RPCPaymentTest():
    def run_test(self):
        self.make_test_signature = os.environ['MAKE_TEST_SIGNATURE']
        assert len(self.make_test_signature) > 0
        self.secret_key, self.public_key = self.get_keys()
        self.reset()
        self.test_access_tracking()
        self.test_access_mining()
        self.test_access_payment()
        self.test_access_account()
        self.test_free_access()

    def get_keys(self):
        output = subprocess.check_output([self.make_test_signature]).decode('utf-8').rstrip()
        fields = output.split()
        assert len(fields) == 2
        return fields

    def get_signature(self):
        return subprocess.check_output([self.make_test_signature, self.secret_key]).decode('utf-8').rstrip()

    def reset(self):
        print('Resetting blockchain')
        daemon = Daemon(idx=1)
        res = daemon.get_height()
        daemon.pop_blocks(res.height - 1)
        daemon.flush_txpool()

    def test_access_tracking(self):
        print('Testing access tracking')
        daemon = Daemon(idx=1)

        res = daemon.rpc_access_tracking(True)

        res = daemon.rpc_access_tracking()
        data = sorted(res.data, key = lambda k: k['rpc'])
        assert len(data) == 1
        entry = data[0]
        assert entry.rpc == 'rpc_access_tracking'
        assert entry.count == 1
        assert entry.time >= 0
        assert entry.credits == 0

        daemon.get_connections()
        res = daemon.rpc_access_tracking()
        data = sorted(res.data, key = lambda k: k['rpc'])
        assert len(data) == 2
        entry = data[0]
        assert entry.rpc == 'get_connections'
        assert entry.count == 1
        assert entry.time >= 0
        assert entry.credits == 0

        daemon.get_connections()
        res = daemon.rpc_access_tracking()
        data = sorted(res.data, key = lambda k: k['rpc'])
        assert len(data) == 2
        entry = data[0]
        assert entry.rpc == 'get_connections'
        assert entry.count == 2
        assert entry.time >= 0
        assert entry.credits == 0

        daemon.get_alternate_chains()
        res = daemon.rpc_access_tracking()
        data = sorted(res.data, key = lambda k: k['rpc'])
        assert len(data) == 3
        entry = data[0]
        assert entry.rpc == 'get_alternate_chains'
        assert entry.count == 1
        assert entry.time >= 0
        assert entry.credits == 0
        entry = res.data[1]
        assert entry.rpc == 'get_connections'
        assert entry.count == 2
        assert entry.time >= 0
        assert entry.credits == 0

        res = daemon.rpc_access_tracking(True)
        res = daemon.rpc_access_tracking()
        data = sorted(res.data, key = lambda k: k['rpc'])
        assert len(data) == 1
        entry = data[0]
        assert entry.rpc == 'rpc_access_tracking'
        assert entry.count == 1

    def test_access_mining(self):
        print('Testing access mining')
        daemon = Daemon(idx=1)
        wallet = Wallet(idx=3)

        res = daemon.rpc_access_info(client = self.get_signature())
        assert len(res.hashing_blob) > 39
        assert res.height == 1
        assert res.top_hash == '418015bb9ae982a1975da7d79277c2705727a56894ba0fb246adaabb1f4632e3'
        assert res.credits_per_hash_found == 5000
        assert res.diff == 10
        assert res.credits == 0
        cookie = res.cookie

        # Try random nonces till we find one that's valid and one that's invalid
        nonce = 0
        found_valid = 0
        found_invalid = 0
        last_credits = 0
        while found_valid == 0 or found_invalid == 0:
            nonce += 1
            try:
                res = daemon.rpc_access_submit_nonce(nonce = nonce, cookie = cookie, client = self.get_signature())
                found_valid += 1
                assert res.credits == last_credits + 5000
            except Exception as e:
                found_invalid += 1
                res = daemon.rpc_access_info(client = self.get_signature())
                assert res.credits < last_credits or res.credits == 0
            assert nonce < 1000 # can't find both valid and invalid -> the RPC probably fails
            last_credits = res.credits

        # we should now have 1 valid nonce, and a number of bad ones
        res = daemon.rpc_access_info(client = self.get_signature())
        assert len(res.hashing_blob) > 39
        assert res.height > 1
        assert res.top_hash != '418015bb9ae982a1975da7d79277c2705727a56894ba0fb246adaabb1f4632e3' # here, any share matches network diff
        assert res.credits_per_hash_found == 5000
        assert res.diff == 10
        cookie = res.cookie

        res = daemon.rpc_access_data()
        assert len(res.entries) > 0
        e = [x for x in res.entries if x['client'] == self.public_key]
        assert len(e) == 1
        e = e[0]
        assert e.nonces_stale == 0
        assert e.nonces_bad == found_invalid
        assert e.nonces_good == found_valid
        assert e.nonces_dupe == 0

        # Try random nonces till we find one that's valid so we get a load of credits
        while last_credits == 0:
            nonce += 1
            try:
                res = daemon.rpc_access_submit_nonce(nonce = nonce, cookie = cookie, client = self.get_signature())
                found_valid += 1
                last_credits = res.credits
                break
            except:
                found_invalid += 1
            assert nonce < 1000 # can't find a valid none -> the RPC probably fails

        # we should now have at least 5000
        res = daemon.rpc_access_info(client = self.get_signature())
        assert res.credits == last_credits
        assert res.credits >= 5000 # last one was a valid nonce

        res = daemon.rpc_access_data()
        assert len(res.entries) > 0
        e = [x for x in res.entries if x['client'] == self.public_key]
        assert len(e) == 1
        e = e[0]
        assert e.nonces_stale == 0
        assert e.nonces_bad == found_invalid
        assert e.nonces_good == found_valid
        assert e.nonces_dupe == 0
        assert e.balance == 5000
        assert e.credits_total >= 5000

        # find a valid one, then check dupes aren't allowed
        res = daemon.rpc_access_info(client = self.get_signature())
        cookie = res.cookie
        old_cookie = cookie # we keep that so can submit a stale later
        while True:
            nonce += 1
            try:
                res = daemon.rpc_access_submit_nonce(nonce = nonce, cookie = cookie, client = self.get_signature())
                found_valid += 1
                break
            except:
                found_invalid += 1
            assert nonce < 1000 # can't find both valid and invalid -> the RPC probably fails

        res = daemon.rpc_access_data()
        assert len(res.entries) > 0
        e = [x for x in res.entries if x['client'] == self.public_key]
        assert len(e) == 1
        e = e[0]
        assert e.nonces_stale == 0
        assert e.nonces_bad == found_invalid
        assert e.nonces_good == found_valid
        assert e.nonces_dupe == 0

        ok = False
        try:
            res = daemon.rpc_access_submit_nonce(nonce = nonce, cookie = cookie, client = self.get_signature())
        except:
            ok = True
        assert ok

        res = daemon.rpc_access_data()
        assert len(res.entries) > 0
        e = [x for x in res.entries if x['client'] == self.public_key]
        assert len(e) == 1
        e = e[0]
        assert e.nonces_stale == 0
        assert e.nonces_bad == found_invalid
        assert e.nonces_good == found_valid
        assert e.nonces_dupe == 1

        # find stales without updating cookie, one within 5 seconds (accepted), one later (rejected)
        res = daemon.rpc_access_info(client = self.get_signature())
        found_close_stale = 0
        found_late_stale = 0
        while found_close_stale == 0 or found_late_stale == 0:
            nonce += 1
            try:
                res = daemon.rpc_access_submit_nonce(nonce = nonce, cookie = cookie, client = self.get_signature())
                found_close_stale += 1
                found_valid += 1
            except Exception as e:
                #if e[0]['error']['code'] == -18: # stale
                if "'code': -18" in str(e): # stale (ugly version, but also works with python 3)
                    found_late_stale += 1
                else:
                    found_invalid += 1
            assert nonce < 1000 # can't find both valid and invalid -> the RPC probably fails

        res = daemon.rpc_access_data()
        assert len(res.entries) > 0
        e = [x for x in res.entries if x['client'] == self.public_key]
        assert len(e) == 1
        e = e[0]
        assert e.nonces_stale == found_late_stale # close stales are accepted, don't count here
        assert e.nonces_bad == found_invalid
        assert e.nonces_good == found_valid
        assert e.nonces_dupe == 1

        # find very stale with old cookie (rejected)
        res = daemon.rpc_access_info(client = self.get_signature())
        nonce += 1
        ok = False
        try:
            res = daemon.rpc_access_submit_nonce(nonce = nonce, cookie = old_cookie, client = self.get_signature())
        except:
            found_late_stale += 1
            ok = True
        assert ok

        res = daemon.rpc_access_data()
        assert len(res.entries) > 0
        e = [x for x in res.entries if x['client'] == self.public_key]
        assert len(e) == 1
        e = e[0]
        assert e.nonces_stale == found_late_stale
        assert e.nonces_bad == found_invalid
        assert e.nonces_good == found_valid
        assert e.nonces_dupe == 1

    def test_access_payment(self):
        print('Testing access payment')
        daemon = Daemon(idx=1)
        wallet = Wallet(idx=3)

        # Try random nonces till we find one that's valid so we get a load of credits
        res = daemon.rpc_access_info(client = self.get_signature())
        credits = res.credits
        cookie = res.cookie
        nonce = 0
        while credits <= 100:
            nonce += 1
            try:
                res = daemon.rpc_access_submit_nonce(nonce = nonce, cookie = cookie, client = self.get_signature())
                break
            except:
                pass
            assert nonce < 1000 # can't find both valid and invalid -> the RPC probably fails

        res = daemon.rpc_access_info(client = self.get_signature())
        credits = res.credits
        assert credits > 0

        res = daemon.get_info(client = self.get_signature())
        assert res.credits == credits - 1
        credits = res.credits

        res = daemon.generateblocks('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 100)
        block_hashes = res.blocks

        # ask for 1 block -> 1 credit
        res = daemon.getblockheadersrange(0, 0, client = self.get_signature())
        assert res.credits == credits - 1
        credits = res.credits

        # ask for 100 blocks -> >1 credit
        res = daemon.getblockheadersrange(1, 100, client = self.get_signature())
        assert res.credits < credits - 1
        credits = res.credits

        # external users
        res = daemon.rpc_access_pay(payment = 1, paying_for = 'foo', client = self.get_signature())
        assert res.credits == credits - 1
        res = daemon.rpc_access_pay(payment = 4, paying_for = 'bar', client = self.get_signature())
        assert res.credits == credits - 5
        res = daemon.rpc_access_pay(payment = credits, paying_for = 'baz', client = self.get_signature())
        assert "PAYMENT REQUIRED" in res.status
        res = daemon.rpc_access_pay(payment = 2, paying_for = 'quux', client = self.get_signature())
        assert res.credits == credits - 7
        res = daemon.rpc_access_pay(payment = 3, paying_for = 'bar', client = self.get_signature())
        assert res.credits == credits - 10

        # that should be rejected because its cost is massive
        ok = False
        try: res = daemon.get_output_histogram(amounts = [], client = self.get_signature())
        except Exception as e: print('e: ' + str(e)); ok = "PAYMENT REQUIRED" in e.status
        assert ok or "PAYMENT REQUIRED" in res.status

    def test_access_account(self):
        print('Testing access account')
        daemon = Daemon(idx=1)
        wallet = Wallet(idx=3)

        res = daemon.rpc_access_info(client = self.get_signature())
        credits = res.credits
        res = daemon.rpc_access_account(self.get_signature(), 0)
        assert res.credits == credits
        res = daemon.rpc_access_account(self.get_signature(), 50)
        assert res.credits == credits + 50
        res = daemon.rpc_access_account(self.get_signature(), -10)
        assert res.credits == credits + 40
        res = daemon.rpc_access_account(self.get_signature(), -(credits + 50))
        assert res.credits == 0
        res = daemon.rpc_access_account(self.get_signature(), 2**63 - 5)
        assert res.credits == 2**63 - 5
        res = daemon.rpc_access_account(self.get_signature(), 2**63 - 1)
        assert res.credits == 2**64 - 6
        res = daemon.rpc_access_account(self.get_signature(), 2)
        assert res.credits == 2**64 - 4
        res = daemon.rpc_access_account(self.get_signature(), 8)
        assert res.credits == 2**64 - 1
        res = daemon.rpc_access_account(self.get_signature(), -1)
        assert res.credits == 2**64 - 2
        res = daemon.rpc_access_account(self.get_signature(), -(2**63 - 1))
        assert res.credits == 2**64 - 2 -(2**63 - 1)
        res = daemon.rpc_access_account(self.get_signature(), -(2**63 - 1))
        assert res.credits == 0

    def test_free_access(self):
        print('Testing free access')
        daemon = Daemon(idx=0)
        wallet = Wallet(idx=0)

        res = daemon.rpc_access_info(client = self.get_signature())
        assert res.credits_per_hash_found == 0
        assert res.diff == 0
        assert res.credits == 0

        res = daemon.get_info(client = self.get_signature())
        assert res.credits == 0

        # any nonce will do here
        res = daemon.rpc_access_submit_nonce(nonce = 0, cookie = 0, client = self.get_signature())
        assert res.credits == 0


class Guard:
    def __enter__(self):
        for i in range(4):
            Wallet(idx = i).auto_refresh(False)
    def __exit__(self, exc_type, exc_value, traceback):
        for i in range(4):
            Wallet(idx = i).auto_refresh(True)

if __name__ == '__main__':
    with Guard() as guard:
        RPCPaymentTest().run_test()
