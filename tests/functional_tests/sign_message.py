#!/usr/bin/env python3

# Copyright (c) 2019-2024, The Monero Project
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

"""Test message signing/verification RPC calls

Test the following RPCs:
    - sign
    - verify

"""

from framework.wallet import Wallet

class MessageSigningTest():
    def run_test(self):
      self.create()
      self.check_signing(False, False)
      self.check_signing(False, True)
      self.check_signing(True, False)
      self.check_signing(True, True)

    def create(self):
        print('Creating wallets')
        seeds = [
            'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted',
            'peeled mixture ionic radar utopia puddle buying illness nuns gadget river spout cavernous bounced paradise drunk looking cottage jump tequila melting went winter adjust spout',
        ]
        self.address = [
            '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm',
            '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW',
        ]
        self.wallet = [None, None]
        for i in range(2):
            self.wallet[i] = Wallet(idx = i)
            # close the wallet if any, will throw if none is loaded
            try: self.wallet[i].close_wallet()
            except: pass
            res = self.wallet[i].restore_deterministic_wallet(seed = seeds[i])
            assert res.address == self.address[i]
            assert res.seed == seeds[i]

    def check_signing(self, subaddress, spend_key):
        print('Signing/verifing messages with ' + ('subaddress' if subaddress else 'standard address') + ' ' + ('spend key' if spend_key else 'view key'))
        messages = ['foo', '']
        if subaddress:
            address = []
            for i in range(2):
                res = self.wallet[i].create_account()
                if i == 0:
                    account_index = res.account_index
                res = self.wallet[i].create_address(account_index = account_index)
                if i == 0:
                    address_index = res.address_index
                address.append(res.address)
        else:
            address = [self.address[0], self.address[1]]
            account_index = 0
            address_index = 0
        for message in messages:
            res = self.wallet[0].sign(message, account_index = account_index, address_index = address_index, signature_type = 'spend' if spend_key else 'view')
            signature = res.signature
            for i in range(2):
                res = self.wallet[i].verify(message, address[0], signature)
                assert res.good
                assert not res.old
                assert res.version == 2
                assert res.signature_type == 'spend' if spend_key else 'view'
                res = self.wallet[i].verify('different', address[0], signature)
                assert not res.good
                res = self.wallet[i].verify(message, address[1], signature)
                assert not res.good
                res = self.wallet[i].verify(message, address[0], signature + 'x')
                assert not res.good
                res = self.wallet[i].verify(message, address[0], signature.replace('SigV2','SigV1'))
                assert not res.good

if __name__ == '__main__':
    MessageSigningTest().run_test()
