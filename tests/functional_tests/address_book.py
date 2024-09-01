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

"""Test wallet address book RPC
"""

from __future__ import print_function
from framework.wallet import Wallet

class AddressBookTest():
    def run_test(self):
      self.create()
      self.test_address_book()

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

    def test_address_book(self):
        print('Testing address book')
        wallet = Wallet()

        # empty at start
        res = wallet.get_address_book()
        assert not 'entries' in res or (res.entries) == 0
        ok = False
        try: wallet.get_address_book([0])
        except: ok = True
        assert ok
        ok = False
        try: wallet.delete_address_book(0)
        except: ok = True
        assert ok
        ok = False
        try: wallet.edit_address_book(0, description = '')
        except: ok = True
        assert ok

        # add one
        res = wallet.add_address_book('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', description = 'self')
        assert res.index == 0, res
        for get_all in [True, False]:
            res = wallet.get_address_book() if get_all else wallet.get_address_book([0])
            assert len(res.entries) == 1
            e = res.entries[0]
            assert e.index == 0
            assert e.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', e
            assert e.description == 'self'

        # add a duplicate
        res = wallet.add_address_book('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', description = 'self')
        assert res.index == 1
        res = wallet.get_address_book()
        assert len(res.entries) == 2
        assert res.entries[0].index == 0
        assert res.entries[1].index == 1
        assert res.entries[0].address == res.entries[1].address
        assert res.entries[0].description == res.entries[1].description
        e = res.entries[1]
        res = wallet.get_address_book([1])
        assert len(res.entries) == 1
        assert e == res.entries[0]

        # request (partially) out of range
        ok = False
        try: res = wallet.get_address_book[4, 2]
        except: ok = True
        assert ok
        ok = False
        try: res = wallet.get_address_book[0, 2]
        except: ok = True
        assert ok
        ok = False
        try: res = wallet.get_address_book[2, 0]
        except: ok = True
        assert ok

        # delete first
        res = wallet.delete_address_book(0)
        res = wallet.get_address_book()
        assert len(res.entries) == 1
        assert res.entries[0].index == 0
        assert res.entries[0].address == e.address
        assert res.entries[0].description == e.description

        # delete (new) first
        res = wallet.delete_address_book(0)
        res = wallet.get_address_book()
        assert not 'entries' in res or (res.entries) == 0

        # add non-addresses
        errors = 0
        try: wallet.add_address_book('', description = 'bad')
        except: errors += 1
        try: wallet.add_address_book('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm ', description = 'bad')
        except: errors += 1
        try: wallet.add_address_book('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDn', description = 'bad')
        except: errors += 1
        try: wallet.add_address_book('9ujeXrjzf7bfeK3KZdCqnYaMwZVFuXemPU8Ubw335rj2FN1CdMiWNyFV3ksEfMFvRp9L9qum5UxkP5rN9aLcPxbH1au4WAB', description = 'bad')
        except: errors += 1
        try: wallet.add_address_book('donate@example.com', description = 'bad')
        except: errors += 1
        assert errors == 5
        res = wallet.get_address_book()
        assert not 'entries' in res or len(res.entries) == 0

        # openalias
        res = wallet.add_address_book('donate@getmonero.org', description = 'dev fund')
        assert res.index == 0
        res = wallet.get_address_book()
        assert len(res.entries) == 1
        e = res.entries[0]
        assert e.address == '888tNkZrPN6JsEgekjMnABU4TBzc2Dt29EPAvkRxbANsAnjyPbb3iQ1YBRk1UXcdRsiKc9dhwMVgN5S9cQUiyoogDavup3H'
        assert e.description == 'dev fund'

        # UTF-8
        res = wallet.add_address_book('42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm', description = u'あまやかす')
        assert res.index == 1
        res = wallet.get_address_book([1])
        assert len(res.entries) == 1
        assert res.entries[0].description == u'あまやかす'
        e = res.entries[0]

        # duplicate request
        res = wallet.get_address_book([1, 1])
        assert len(res.entries) == 2
        assert res.entries[0] == e
        assert res.entries[1] == e

        # various address types
        res = wallet.make_integrated_address()
        integrated_address = res.integrated_address
        res = wallet.add_address_book(integrated_address)
        assert res.index == 2
        res = wallet.add_address_book('87KfgTZ8ER5D3Frefqnrqif11TjVsTPaTcp37kqqKMrdDRUhpJRczeR7KiBmSHF32UJLP3HHhKUDmEQyJrv2mV8yFDCq8eB')
        assert res.index == 3

        # get them back
        res = wallet.get_address_book([0])
        assert len(res.entries) == 1
        assert res.entries[0].address == '888tNkZrPN6JsEgekjMnABU4TBzc2Dt29EPAvkRxbANsAnjyPbb3iQ1YBRk1UXcdRsiKc9dhwMVgN5S9cQUiyoogDavup3H'
        assert res.entries[0].description == 'dev fund'
        res = wallet.get_address_book([1])
        assert len(res.entries) == 1
        assert res.entries[0].address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert res.entries[0].description == u'あまやかす'
        res = wallet.get_address_book([2])
        assert len(res.entries) == 1
        assert res.entries[0].address == integrated_address
        res = wallet.get_address_book([3])
        assert len(res.entries) == 1
        assert res.entries[0].address == '87KfgTZ8ER5D3Frefqnrqif11TjVsTPaTcp37kqqKMrdDRUhpJRczeR7KiBmSHF32UJLP3HHhKUDmEQyJrv2mV8yFDCq8eB'

        # edit
        res = wallet.get_address_book([1])
        assert len(res.entries) == 1
        e = res.entries[0]
        assert e.index == 1
        assert e.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert e.description == u'あまやかす'
        res = wallet.get_address_book([1])
        assert len(res.entries) == 1
        e = res.entries[0]
        assert e.index == 1
        assert e.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert e.description == u'あまやかす'
        res = wallet.edit_address_book(1, description = '')
        res = wallet.get_address_book([1])
        assert len(res.entries) == 1
        e = res.entries[0]
        assert e.index == 1
        assert e.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert e.description == ''
        res = wallet.edit_address_book(1, description = 'えんしゅう')
        res = wallet.get_address_book([1])
        assert len(res.entries) == 1
        e = res.entries[0]
        assert e.index == 1
        assert e.address == '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        assert e.description == u'えんしゅう'
        res = wallet.edit_address_book(1, address = '888tNkZrPN6JsEgekjMnABU4TBzc2Dt29EPAvkRxbANsAnjyPbb3iQ1YBRk1UXcdRsiKc9dhwMVgN5S9cQUiyoogDavup3H')
        res = wallet.get_address_book([1])
        assert len(res.entries) == 1
        e = res.entries[0]
        assert e.index == 1
        assert e.address == '888tNkZrPN6JsEgekjMnABU4TBzc2Dt29EPAvkRxbANsAnjyPbb3iQ1YBRk1UXcdRsiKc9dhwMVgN5S9cQUiyoogDavup3H'
        assert e.description == u'えんしゅう'
        ok = False
        try: res = wallet.edit_address_book(1, address = '')
        except: ok = True
        assert ok
        ok = False
        try: res = wallet.edit_address_book(1, address = 'address')
        except: ok = True
        assert ok
        res = wallet.edit_address_book(1)
        res = wallet.get_address_book([1])
        assert len(res.entries) == 1
        assert e == res.entries[0]

        # empty
        wallet.delete_address_book(0)
        res = wallet.get_address_book([0]) # entries above the deleted one collapse one slot up
        assert len(res.entries) == 1
        assert res.entries[0].address == '888tNkZrPN6JsEgekjMnABU4TBzc2Dt29EPAvkRxbANsAnjyPbb3iQ1YBRk1UXcdRsiKc9dhwMVgN5S9cQUiyoogDavup3H'
        assert res.entries[0].description == u'えんしゅう'
        wallet.delete_address_book(2)
        wallet.delete_address_book(0)
        wallet.delete_address_book(0)
        res = wallet.get_address_book()
        assert not 'entries' in res or len(res.entries) == 0


if __name__ == '__main__':
    AddressBookTest().run_test()
