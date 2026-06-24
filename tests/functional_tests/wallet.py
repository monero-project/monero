#!/usr/bin/env python3
#encoding=utf-8

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

"""Test basic wallet functionality
"""

import ast
import sys
import util_resources

from framework.wallet import Wallet
from framework.daemon import Daemon

class WalletTest():
    seed = 'velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted'
    address = '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
    ssk = '148d78d2aba7dbca5cd8f6abcfb0b3c009ffbdbea1ff373d50ed94d78286640e'
    svk = '49774391fa5e8d249fc2c5b45dadef13534bf2483dede880dac88f061e809100'
    address_non_deterministic = '4591SBcZXAMQaGfPDgRppX6N1FyyLoYXscRohnAd2MXMhG76o7r2PvYNKvdJdpAZzs4GS4J4bqYuPcfAG479hNrQF2qM6Lr'
    ssk_non_deterministic = 'e4810cd6432e9c59ad1b1e640957960f33d3bbc85f56f0f7631495b7c08a1409'
    svk_non_determinisitc = '10f67f0ecfff7860d978cf88d966f8012ec447799ba36a1da94bd44e314d1206'
    # non-deterministic wallet
    wal_nd = None
    # deterministic wallet from spend-key
    wal_sk = None
    # view-only wallet
    wal_vo = None
    error_codes = {
        -1:"WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR",
        -7:"WALLET_RPC_ERROR_CODE_DENIED",
        -21:"WALLET_RPC_ERROR_CODE_WALLET_ALREADY_EXISTS",
        -22:"WALLET_RPC_ERROR_CODE_INVALID_PASSWORD",
        -23:"WALLET_RPC_ERROR_CODE_NO_WALLET_DIR",
        -29:"WALLET_RPC_ERROR_CODE_WATCH_ONLY",
        -43:"WALLET_RPC_ERROR_CODE_NON_DETERMINISTIC",
        -45:"WALLET_RPC_ERROR_CODE_ATTRIBUTE_NOT_FOUND",
    }

    def run_test(self):
      self.reset()
      self.create_fail()
      self.create()
      self.generate_from_keys()
      self.restore_deterministic()
      self.check_main_address()
      self.check_keys()
      self.create_subaddresses()
      self.tags()
      self.update_lookahead()
      self.attributes()
      self.open_close()
      self.languages()
      self.change_password()
      self.store()

    def reset(self):
        print('Resetting blockchain')
        daemon = Daemon()
        res = daemon.get_height()
        daemon.pop_blocks(res.height - 1)
        daemon.flush_txpool()

    def create_fail(self):
        print('Fail creating wallet')
        wallet_file_name = "tmp_wallet"

        # with --restricted-rpc
        wallet = Wallet(idx=8)
        try:
            res = wallet.create_wallet(filename=wallet_file_name)
        except AssertionError as e:
            res_dict = ast.literal_eval(str(e))
            assert self.error_codes[res_dict["error"]["code"]] == "WALLET_RPC_ERROR_CODE_DENIED"

        # without --wallet-dir
        wallet = Wallet(idx=7)
        try:
            res = wallet.create_wallet(filename=wallet_file_name)
        except AssertionError as e:
            res_dict = ast.literal_eval(str(e))
            assert self.error_codes[res_dict["error"]["code"]] == "WALLET_RPC_ERROR_CODE_NO_WALLET_DIR"

        # attempt to create already existing tmp_wallet
        wallet = Wallet()
        try:
            res = wallet.create_wallet(filename=wallet_file_name)
        except AssertionError as e:
            res_dict = ast.literal_eval(str(e))
            assert self.error_codes[res_dict["error"]["code"]] == "WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR"
            # QUESTION : actually there is a specific error code for this case, should I change it to this?
#            assert self.error_codes[res_dict["error"]["code"]] == "WALLET_RPC_ERROR_CODE_WALLET_ALREADY_EXISTS"
            assert res_dict["error"]["message"] == "attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting."

        # invalid filename
        try:
            res = wallet.create_wallet(filename="/invalid/filename")
        except AssertionError as e:
            res_dict = ast.literal_eval(str(e))
            assert self.error_codes[res_dict["error"]["code"]] == "WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR"
            assert res_dict["error"]["message"] == "Invalid filename"

        # invalid seed language
        language = "Newspeak"
        try:
            res = wallet.create_wallet(language=language)
        except AssertionError as e:
            res_dict = ast.literal_eval(str(e))
            assert self.error_codes[res_dict["error"]["code"]] == "WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR"
            assert res_dict["error"]["message"] == "Unknown language: " + language

    def create(self):
        print('Creating wallet')
        wallet = Wallet()
        # close the wallet if any, will throw if none is loaded
        try: wallet.close_wallet()
        except: pass
        res = wallet.create_wallet()
        assert res == {}

    def generate_from_keys(self):
        print('Generating wallet from keys')
        # view-key + spend-key (determinisitc)
        wallet = Wallet()
        try: wallet.close_wallet()
        except: pass
        res = wallet.generate_from_keys(address = self.address, spendkey = self.ssk, viewkey = self.svk)
        assert res.address == self.address
        assert res.info == "Wallet has been generated successfully."
        assert wallet.query_key("mnemonic")["key"] == self.seed

        # view-key + spend-key (non-determinisitc)
        self.wal_nd = Wallet(idx=1)
        try: wallet.close_wallet()
        except: pass
        res = self.wal_nd.generate_from_keys(address = self.address_non_deterministic, spendkey = self.ssk_non_deterministic, viewkey = self.svk_non_determinisitc)
        assert res.address == self.address_non_deterministic
        assert res.info == "Non-deterministic Wallet has been generated successfully."

        # fail spend-key only (deterministic)
        self.wal_sk = Wallet(idx=2)
        try: self.wal_sk.close_wallet()
        except: pass
        try:
            res = wallet.generate_from_keys(address = self.address, spendkey = self.ssk)
        except Exception as e:
            res_dict = ast.literal_eval(str(e))
            assert self.error_codes[res_dict["error"]["code"]] == "WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR"
            assert res_dict["error"]["message"] == "field 'viewkey' is mandatory. Please provide a view key you want to restore from."

        # view-only
        self.wal_vo = Wallet(idx=3)
        try: self.wal_vo.close_wallet()
        except: pass
        res = self.wal_vo.generate_from_keys(address = self.address, viewkey = self.svk)
        assert res.address == self.address
        assert res.info == "View-only Wallet has been generated successfully."

    def restore_deterministic(self):
        print('Restoring deterministic wallet')
        wallet = Wallet()
        util_resources.remove_wallet_files("tmp_wallet_copy")
        # close the wallet if any, will throw if none is loaded
        try: wallet.close_wallet()
        except: pass
        res = wallet.restore_deterministic_wallet(filename = "tmp_wallet_copy", seed = self.seed)
        assert res.address == self.address
        assert res.seed == self.seed

        # Note : restoring multisig is covered in multisig.py

    def check_main_address(self):
        print('Getting address')
        wallet = Wallet()
        res = wallet.get_address()
        assert res.address == self.address, res
        assert len(res.addresses) == 1
        assert res.addresses[0].address == res.address
        assert res.addresses[0].address_index == 0
        assert res.addresses[0].used == False

    def check_keys(self):
        print('Checking keys')
        wallet = Wallet()
        res = wallet.query_key('view_key')
        assert res.key == self.svk
        res = wallet.query_key('spend_key')
        assert res.key == self.ssk
        res = wallet.query_key('mnemonic')
        assert res.key == self.seed

        # non-deterministic
        res = self.wal_nd.query_key('view_key')
        assert res.key == self.svk_non_determinisitc
        res = self.wal_nd.query_key('spend_key')
        assert res.key == self.ssk_non_deterministic
        try:
            self.wal_nd.query_key('mnemonic')
        except Exception as e:
            res_dict = ast.literal_eval(str(e))
            assert self.error_codes[res_dict["error"]["code"]] == "WALLET_RPC_ERROR_CODE_NON_DETERMINISTIC"
            assert res_dict["error"]["message"] == "The wallet is non-deterministic. Cannot display seed."

        # view-only
        res = self.wal_vo.query_key('view_key')
        assert res.key == self.svk
        try:
            self.wal_vo.query_key('spend_key')
        except Exception as e:
            res_dict = ast.literal_eval(str(e))
            assert self.error_codes[res_dict["error"]["code"]] == "WALLET_RPC_ERROR_CODE_WATCH_ONLY"
            assert res_dict["error"]["message"] == "The wallet is watch-only. Cannot retrieve spend key."
        try:
            self.wal_vo.query_key("mnemonic")
        except Exception as e:
            res_dict = ast.literal_eval(str(e))
            assert self.error_codes[res_dict["error"]["code"]] == "WALLET_RPC_ERROR_CODE_WATCH_ONLY"
            assert res_dict["error"]["message"] == "The wallet is watch-only. Cannot retrieve seed."

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
        assert res.address == self.address, res
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
        res = wallet.get_address_index(self.address)
        assert res.index == {'major': 0, 'minor': 0}
        res = wallet.get_address_index('84QRUYawRNrU3NN1VpFRndSukeyEb3Xpv8qZjjsoJZnTYpDYceuUTpog13D7qPxpviS7J29bSgSkR11hFFoXWk2yNdsR9WF')
        assert res.index == {'major': 0, 'minor': 1}
        res = wallet.get_address_index('82pP87g1Vkd3LUMssBCumk3MfyEsFqLAaGDf6oxddu61EgSFzt8gCwUD4tr3kp9TUfdPs2CnpD7xLZzyC1Ei9UsW3oyCWDf')
        assert res.index == {'major': 1, 'minor': 0}

        res = wallet.label_account(0, "main")

    def update_lookahead(self):
        print('Updating subaddress lookahead')
        wallet = Wallet()
        address_0_999 = '8BQKgTSSqJjP14AKnZUBwnXWj46MuNmLvHfPTpmry52DbfNjjHVvHUk4mczU8nj8yZ57zBhksTJ8kM5xKeJXw55kCMVqyG7' # this is the address for address 999 of the main account in the test wallet
        try:  # assert address_1_999 is not in the current pubkey table
            wallet.get_address_index(address_0_999)
        except Exception as e:
            assert str(e) ==  "{'error': {'code': -2, 'message': \"Address doesn't belong to the wallet\"}, 'id': '0', 'jsonrpc': '2.0'}"
        # update the lookahead and assert the high index address is now in the table
        wallet.set_subaddress_lookahead(50, 1000)
        r = wallet.get_address_index(address_0_999)
        assert r['index']['major'] == 0
        assert r['index']['minor'] == 999

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
        assert sorted(subaddress_accounts) == [(0, self.address, 'main'), (1, '82pP87g1Vkd3LUMssBCumk3MfyEsFqLAaGDf6oxddu61EgSFzt8gCwUD4tr3kp9TUfdPs2CnpD7xLZzyC1Ei9UsW3oyCWDf', 'idx1_new')]

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
        except Exception as e:
            res_dict = ast.literal_eval(str(e))
            ok = self.error_codes[res_dict["error"]["code"]] == "WALLET_RPC_ERROR_CODE_ATTRIBUTE_NOT_FOUND"
        assert ok
        res = wallet.set_attribute('いちりゅう', 'いっぽう')
        res = wallet.get_attribute('いちりゅう')
        assert res.value == u'いっぽう'

    def open_close(self):
        print('Testing open/close')
        wallet = Wallet()

        res = wallet.get_address()
        assert res.address == self.address

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

        wallet.open_wallet(filename = "tmp_wallet_copy")
        res = wallet.get_address()
        assert res.address == self.address

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

        util_resources.remove_wallet_files('test1')

        res = wallet.restore_deterministic_wallet(seed = self.seed, filename = 'test1')
        assert res.address == self.address
        assert res.seed == self.seed

        wallet.close_wallet()
        res = wallet.open_wallet('test1', password = '')
        res = wallet.get_address()
        assert res.address == self.address

        # invalid old password
        ok = False
        try:
            wallet.change_wallet_password(old_password = 'foo', new_password = 'foo')
        except Exception as e:
            res_dict = ast.literal_eval(str(e))
            ok = self.error_codes[res_dict["error"]["code"]] == "WALLET_RPC_ERROR_CODE_INVALID_PASSWORD" and res_dict["error"]["message"] == "Invalid original password."
        assert ok

        res = wallet.change_wallet_password(old_password = '', new_password = 'foo')
        wallet.close_wallet()

        try: res = wallet.open_wallet('test1', password = '')
        except Exception as e:
            res_dict = ast.literal_eval(str(e))
            ok = self.error_codes[res_dict["error"]["code"]] == "WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR" and res_dict["error"]["message"] == "invalid password"
        assert ok

        res = wallet.open_wallet('test1', password = 'foo')
        res = wallet.get_address()
        assert res.address == self.address

        wallet.close_wallet()

        util_resources.remove_wallet_files('test1')

    def store(self):
        print('Testing store')
        wallet = Wallet()

        # close the wallet if any, will throw if none is loaded
        try: wallet.close_wallet()
        except: pass

        util_resources.remove_wallet_files('test1')

        res = wallet.restore_deterministic_wallet(seed = self.seed, filename = 'test1')
        assert res.address == self.address
        assert res.seed == self.seed

        util_resources.remove_file('test1')
        assert util_resources.file_exists('test1.keys')
        assert not util_resources.file_exists('test1')
        wallet.store()
        assert util_resources.file_exists('test1.keys')
        assert util_resources.file_exists('test1')

        wallet.close_wallet()

        wallet.open_wallet(filename = 'test1', password = '')
        wallet.close_wallet()

        util_resources.remove_wallet_files('test1')


if __name__ == '__main__':
    WalletTest().run_test()
