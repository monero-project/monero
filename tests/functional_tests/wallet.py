#!/usr/bin/env python3
#encoding=utf-8

# Copyright (c) 2019-2023, The Monero Project
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

"""Test basic wallet functionality
"""

from __future__ import print_function
import sys
import os
import errno

from framework.wallet import Wallet
from framework.daemon import Daemon

class WalletTest():
    def run_test(self):
      self.reset()
      self.create()
      self.check_main_address()
      self.check_keys()
      self.create_subaddresses()
      self.tags()
      self.attributes()
      self.open_close()
      self.languages()
      self.change_password()
      self.store()

    def remove_file(self, name):
        WALLET_DIRECTORY = os.environ['WALLET_DIRECTORY']
        assert WALLET_DIRECTORY != ''
        try:
            os.unlink(WALLET_DIRECTORY + '/' + name)
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise

    def remove_wallet_files(self, name):
        for suffix in ['', '.keys']:
            self.remove_file(name + suffix)

    def file_exists(self, name):
        WALLET_DIRECTORY = os.environ['WALLET_DIRECTORY']
        assert WALLET_DIRECTORY != ''
        return os.path.isfile(WALLET_DIRECTORY + '/' + name)

    def reset(self):
        print('Resetting blockchain')
        daemon = Daemon()
        res = daemon.get_height()
        daemon.pop_blocks(res.height - 1)
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

        res = wallet.label_account(0, "main")

    def tags(self):
        print('Testing tags')
        wallet = Wallet()
        res = wallet.get_account_tags()
        assert not 'account_tags' in res or len(res.account_tags) == 0
        ok = False
        try: res = wallet.get_accounts('tag')
        except: ok = True
        assert ok or not 'subaddress_accounts' in res or res.subaddress_accounts == 0
        wallet.tag_accounts('tag0', [1])
        res = wallet.get_account_tags()
        assert len(res.account_tags) == 1
        assert res.account_tags[0].tag == 'tag0'
        assert res.account_tags[0].label == ''
        assert res.account_tags[0].accounts == [1]
        res = wallet.get_accounts('tag0')
        assert len(res.subaddress_accounts) == 1
        assert res.subaddress_accounts[0].account_index == 1
        assert res.subaddress_accounts[0].base_address == '82pP87g1Vkd3LUMssBCumk3MfyEsFqLAaGDf6oxddu61EgSFzt8gCwUD4tr3kp9TUfdPs2CnpD7xLZzyC1Ei9UsW3oyCWDf'
        assert res.subaddress_accounts[0].balance == 0
        assert res.subaddress_accounts[0].unlocked_balance == 0
        assert res.subaddress_accounts[0].label == 'idx1_new'
        assert res.subaddress_accounts[0].tag == 'tag0'
        wallet.untag_accounts([0])
        res = wallet.get_account_tags()
        assert len(res.account_tags) == 1
        assert res.account_tags[0].tag == 'tag0'
        assert res.account_tags[0].label == ''
        assert res.account_tags[0].accounts == [1]
        wallet.untag_accounts([1])
        res = wallet.get_account_tags()
        assert not 'account_tags' in res or len(res.account_tags) == 0
        wallet.tag_accounts('tag0', [0])
        wallet.tag_accounts('tag1', [1])
        res = wallet.get_account_tags()
        assert len(res.account_tags) == 2
        x = [x for x in res.account_tags if x.tag == 'tag0']
        assert len(x) == 1
        assert x[0].tag == 'tag0'
        assert x[0].label == ''
        assert x[0].accounts == [0]
        x = [x for x in res.account_tags if x.tag == 'tag1']
        assert len(x) == 1
        assert x[0].tag == 'tag1'
        assert x[0].label == ''
        assert x[0].accounts == [1]
        wallet.tag_accounts('tagA', [0, 1])
        res = wallet.get_account_tags()
        assert len(res.account_tags) == 1
        assert res.account_tags[0].tag == 'tagA'
        assert res.account_tags[0].label == ''
        assert res.account_tags[0].accounts == [0, 1]
        wallet.tag_accounts('tagB', [1, 0])
        res = wallet.get_account_tags()
        assert len(res.account_tags) == 1
        assert res.account_tags[0].tag == 'tagB'
        assert res.account_tags[0].label == ''
        assert res.account_tags[0].accounts == [0, 1]
        wallet.set_account_tag_description('tagB', 'tag B')
        res = wallet.get_account_tags()
        assert len(res.account_tags) == 1
        assert res.account_tags[0].tag == 'tagB'
        assert res.account_tags[0].label == 'tag B'
        assert res.account_tags[0].accounts == [0, 1]
        res = wallet.get_accounts('tagB')
        assert len(res.subaddress_accounts) == 2
        subaddress_accounts = []
        for x in res.subaddress_accounts:
            assert x.balance == 0
            assert x.unlocked_balance == 0
            subaddress_accounts.append((x.account_index, x.base_address, x.label))
        assert sorted(subaddress_accounts) == [(0, '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', 'main'), (1, '82pP87g1Vkd3LUMssBCumk3MfyEsFqLAaGDf6oxddu61EgSFzt8gCwUD4tr3kp9TUfdPs2CnpD7xLZzyC1Ei9UsW3oyCWDf', 'idx1_new')]

    def attributes(self):
        print('Testing attributes')
        wallet = Wallet()

        ok = False
        try: res = wallet.get_attribute('foo')
        except: ok = True
        assert ok
        res = wallet.set_attribute('foo', 'bar')
        res = wallet.get_attribute('foo')
        assert res.value == 'bar'
        res = wallet.set_attribute('foo', 'いっしゅん')
        res = wallet.get_attribute('foo')
        assert res.value == u'いっしゅん'
        ok = False
        try: res = wallet.get_attribute('いちりゅう')
        except: ok = True
        assert ok
        res = wallet.set_attribute('いちりゅう', 'いっぽう')
        res = wallet.get_attribute('いちりゅう')
        assert res.value == u'いっぽう'

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

        ####################################################################################################################
        # This create->close->open pattern reveals if you have code which performs loading correctly but storing incorrectly

        wallet.create_wallet('createcloseopen', password='NIST SP 800-90A -- Dual_EC_DRBG')
        cco_addr = wallet.get_address().address
        assert cco_addr != '44Kbx4sJ7JDRDV5aAhLJzQCjDz2ViLRduE3ijDZu3osWKBjMGkV1XPk4pfDUMqt1Aiezvephdqm6YD19GKFD9ZcXVUTp6BW' # what are the chances haha

        wallet.close_wallet()
        ok = False
        try: wallet.get_address()
        except: ok = True
        assert ok

        wallet.open_wallet('createcloseopen', password='NIST SP 800-90A -- Dual_EC_DRBG')
        res = wallet.get_address()
        assert res.address == cco_addr
        ####################################################################################################################

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
        languages_local = res.languages_local
        for language in languages + languages_local:
            sys.stdout.write('Creating ' + language + ' wallet\n')
            wallet.create_wallet(filename = '', language = language)
            res = wallet.query_key('mnemonic')
            wallet.close_wallet()

    def change_password(self):
        print('Testing password change')
        wallet = Wallet()

        # close the wallet if any, will throw if none is loaded
        try: wallet.close_wallet()
        except: pass

        self.remove_wallet_files('test1')

        seed = 'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted'
        res = wallet.restore_deterministic_wallet(seed = seed, filename = 'test1')
        assert res.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert res.seed == seed

        wallet.close_wallet()
        res = wallet.open_wallet('test1', password = '')
        res = wallet.get_address()
        assert res.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'

        res = wallet.change_wallet_password(old_password = '', new_password = 'foo')
        wallet.close_wallet()

        ok = False
        try: res = wallet.open_wallet('test1', password = '')
        except: ok = True
        assert ok

        res = wallet.open_wallet('test1', password = 'foo')
        res = wallet.get_address()
        assert res.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'

        wallet.close_wallet()

        self.remove_wallet_files('test1')

    def store(self):
        print('Testing store')
        wallet = Wallet()

        # close the wallet if any, will throw if none is loaded
        try: wallet.close_wallet()
        except: pass

        self.remove_wallet_files('test1')

        seed = 'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted'
        res = wallet.restore_deterministic_wallet(seed = seed, filename = 'test1')
        assert res.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert res.seed == seed

        self.remove_file('test1')
        assert self.file_exists('test1.keys')
        assert not self.file_exists('test1')
        wallet.store()
        assert self.file_exists('test1.keys')
        assert self.file_exists('test1')

        wallet.close_wallet()
        self.remove_wallet_files('test1')


if __name__ == '__main__':
    WalletTest().run_test()
