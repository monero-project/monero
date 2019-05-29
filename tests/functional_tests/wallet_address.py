#!/usr/bin/env python3
#encoding=utf-8

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

"""Test transaction creation RPC calls

Test the following RPCs:
    - [TODO: many tests still need to be written]

"""

from __future__ import print_function
from framework.wallet import Wallet
from framework.daemon import Daemon

class WalletAddressTest():
    def run_test(self):
      self.reset()
      self.create()
      self.check_main_address()
      self.check_keys()
      self.create_subaddresses()
      self.open_close()
      self.languages()

    def reset(self):
        print('Resetting blockchain')
        daemon = Daemon()
        daemon.pop_blocks(1000)
        daemon.flush_txpool()

    def create(self):
        print('Creating wallet')
        wallet = Wallet()
        # close the wallet if any, will throw if none is loaded
        try: wallet.close_wallet()
        except: pass
        seed = 'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted'
        res = wallet.restore_deterministic_wallet(seed = seed)
        assert res.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert res.seed == seed

    def check_main_address(self):
        print('Getting address')
        wallet = Wallet()
        res = wallet.get_address()
        assert res.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', res
        assert len(res.addresses) == 1
        assert res.addresses[0].address == res.address
        assert res.addresses[0].address_index == 0
        assert res.addresses[0].used == False

    def check_keys(self):
        print('Checking keys')
        wallet = Wallet()
        res = wallet.query_key('view_key')
        assert res.key == '49774391fa5e8d249fc2c5b45dadef13534bf2483dede880dac88f061e809100'
        res = wallet.query_key('spend_key')
        assert res.key == '148d78d2aba7dbca5cd8f6abcfb0b3c009ffbdbea1ff373d50ed94d78286640e'
        res = wallet.query_key('mnemonic')
        assert res.key == 'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted'

    def create_subaddresses(self):
        print('Creating subaddresses')
        wallet = Wallet()
        res = wallet.create_account("idx1")
        assert res.account_index == 1, res
        assert res.address == '82pP87g1Vkd3LUMssBCumk3MfyEsFqLAaGDf6oxddu61EgSFzt8gCwUD4tr3kp9TUfdPs2CnpD7xLZzyC1Ei9UsW3oyCWDf', res
        res = wallet.create_account("idx2")
        assert res.account_index == 2, res
        assert res.address == '8Bdb75y2MhvbkvaBnG7vYP6DCNneLWcXqNmfPmyyDkavAUUgrHQEAhTNK3jEq69kGPDrd3i5inPivCwTvvA12eQ4SJk9iyy', res

        res = wallet.get_address(0, 0)
        assert res.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', res
        assert len(res.addresses) == 1
        assert res.addresses[0].address_index == 0, res
        res = wallet.get_address(1, 0)
        assert res.address == '82pP87g1Vkd3LUMssBCumk3MfyEsFqLAaGDf6oxddu61EgSFzt8gCwUD4tr3kp9TUfdPs2CnpD7xLZzyC1Ei9UsW3oyCWDf', res
        assert len(res.addresses) == 1
        assert res.addresses[0].label == 'idx1', res
        assert res.addresses[0].address_index == 0, res
        res = wallet.get_address(2, 0)
        assert res.address == '8Bdb75y2MhvbkvaBnG7vYP6DCNneLWcXqNmfPmyyDkavAUUgrHQEAhTNK3jEq69kGPDrd3i5inPivCwTvvA12eQ4SJk9iyy', res
        assert len(res.addresses) == 1
        assert res.addresses[0].label == 'idx2', res
        assert res.addresses[0].address_index == 0, res

        res = wallet.create_address(0, "sub_0_1")
        res = wallet.create_address(1, "sub_1_1")
        res = wallet.create_address(1, "sub_1_2")

        res = wallet.get_address(0, [1])
        assert len(res.addresses) == 1
        assert res.addresses[0].address == '84QRUYawRNrU3NN1VpFRndSukeyEb3Xpv8qZjjsoJZnTYpDYceuUTpog13D7qPxpviS7J29bSgSkR11hFFoXWk2yNdsR9WF'
        assert res.addresses[0].label == 'sub_0_1'
        res = wallet.get_address(1, [1])
        assert len(res.addresses) == 1
        assert res.addresses[0].address == '87qyoPVaEcWikVBmG1TaP1KumZ3hB3Q5f4wZRjuppNdwYjWzs2RgbLYQgtpdu2YdoTT3EZhiUGaPJQt2FsykeFZbCtaGXU4'
        assert res.addresses[0].label == 'sub_1_1'
        res = wallet.get_address(1, [2])
        assert len(res.addresses) == 1
        assert res.addresses[0].address == '87KfgTZ8ER5D3Frefqnrqif11TjVsTPaTcp37kqqKMrdDRUhpJRczeR7KiBmSHF32UJLP3HHhKUDmEQyJrv2mV8yFDCq8eB'
        assert res.addresses[0].label == 'sub_1_2'
        res = wallet.get_address(1, [0, 1, 2])
        assert len(res.addresses) == 3
        assert res.addresses[0].address == '82pP87g1Vkd3LUMssBCumk3MfyEsFqLAaGDf6oxddu61EgSFzt8gCwUD4tr3kp9TUfdPs2CnpD7xLZzyC1Ei9UsW3oyCWDf'
        assert res.addresses[0].label == 'idx1'
        assert res.addresses[1].address == '87qyoPVaEcWikVBmG1TaP1KumZ3hB3Q5f4wZRjuppNdwYjWzs2RgbLYQgtpdu2YdoTT3EZhiUGaPJQt2FsykeFZbCtaGXU4'
        assert res.addresses[1].label == 'sub_1_1'
        assert res.addresses[2].address == '87KfgTZ8ER5D3Frefqnrqif11TjVsTPaTcp37kqqKMrdDRUhpJRczeR7KiBmSHF32UJLP3HHhKUDmEQyJrv2mV8yFDCq8eB'
        assert res.addresses[2].label == 'sub_1_2'

        res = wallet.label_address((1, 2), "sub_1_2_new")
        res = wallet.get_address(1, [2])
        assert len(res.addresses) == 1
        assert res.addresses[0].address == '87KfgTZ8ER5D3Frefqnrqif11TjVsTPaTcp37kqqKMrdDRUhpJRczeR7KiBmSHF32UJLP3HHhKUDmEQyJrv2mV8yFDCq8eB'
        assert res.addresses[0].label == 'sub_1_2_new'

        res = wallet.label_account(1, "idx1_new")
        res = wallet.get_address(1, [0])
        assert len(res.addresses) == 1
        assert res.addresses[0].address == '82pP87g1Vkd3LUMssBCumk3MfyEsFqLAaGDf6oxddu61EgSFzt8gCwUD4tr3kp9TUfdPs2CnpD7xLZzyC1Ei9UsW3oyCWDf'
        assert res.addresses[0].label == 'idx1_new'

        res = wallet.get_address_index('87KfgTZ8ER5D3Frefqnrqif11TjVsTPaTcp37kqqKMrdDRUhpJRczeR7KiBmSHF32UJLP3HHhKUDmEQyJrv2mV8yFDCq8eB')
        assert res.index == {'major': 1, 'minor': 2}
        res = wallet.get_address_index('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm')
        assert res.index == {'major': 0, 'minor': 0}
        res = wallet.get_address_index('84QRUYawRNrU3NN1VpFRndSukeyEb3Xpv8qZjjsoJZnTYpDYceuUTpog13D7qPxpviS7J29bSgSkR11hFFoXWk2yNdsR9WF')
        assert res.index == {'major': 0, 'minor': 1}
        res = wallet.get_address_index('82pP87g1Vkd3LUMssBCumk3MfyEsFqLAaGDf6oxddu61EgSFzt8gCwUD4tr3kp9TUfdPs2CnpD7xLZzyC1Ei9UsW3oyCWDf')
        assert res.index == {'major': 1, 'minor': 0}

    def open_close(self):
        print('Testing open/close')
        wallet = Wallet()

        res = wallet.get_address()
        assert res.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'

        wallet.close_wallet()
        ok = False
        try: res = wallet.get_address()
        except: ok = True
        assert ok

        wallet.restore_deterministic_wallet(seed = 'peeled mixture ionic radar utopia puddle buying illness nuns gadget river spout cavernous bounced paradise drunk looking cottage jump tequila melting went winter adjust spout')
        res = wallet.get_address()
        assert res.address == '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW'

        wallet.close_wallet()
        ok = False
        try: wallet.get_address()
        except: ok = True
        assert ok

        wallet.restore_deterministic_wallet(seed = 'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted')
        res = wallet.get_address()
        assert res.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'

    def languages(self):
        print('Testing languages')
        wallet = Wallet()
        res = wallet.get_languages()
        assert 'English' in res.languages
        assert 'English' in res.languages_local
        assert 'Dutch' in res.languages
        assert 'Nederlands' in res.languages_local
        assert 'Japanese' in res.languages
        assert u'日本語' in res.languages_local
        try: wallet.close_wallet()
        except: pass
        languages = res.languages
        for language in languages:
            print('Creating ' + str(language) + ' wallet')
            wallet.create_wallet(filename = '', language = language)
            res = wallet.query_key('mnemonic')
            wallet.close_wallet()


if __name__ == '__main__':
    WalletAddressTest().run_test()
