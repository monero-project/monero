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

"""Test URI RPC
"""

try:
  from urllib import quote as urllib_quote
except:
  from urllib.parse import quote as urllib_quote

from framework.wallet import Wallet

class URITest():
    def run_test(self):
      self.create()
      self.test_monero_uri()
      self.test_monero_multi_uri()

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

    def test_monero_uri(self):
        print('Testing monero: URI - legacy functions')
        wallet = Wallet()

        utf8string = [u'えんしゅう', u'あまやかす']
        quoted_utf8string = [urllib_quote(x.encode('utf8')) for x in utf8string]

        ok = False
        try: res = wallet.make_uri()
        except: ok = True
        assert ok
        ok = False
        try: res = wallet.make_uri(address = '')
        except: ok = True
        assert ok
        ok = False
        try: res = wallet.make_uri(address = 'kjshdkj')
        except: ok = True
        assert ok

        for address in [
            '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm',
            '4BxSHvcgTwu25WooY4BVmgdcKwZu5EksVZSZkDd6ooxSVVqQ4ubxXkhLF6hEqtw96i9cf3cVfLw8UWe95bdDKfRQeYtPwLm1Jiw7AKt2LY',
            '8AsN91rznfkBGTY8psSNkJBg9SZgxxGGRUhGwRptBhgr5XSQ1XzmA9m8QAnoxydecSh5aLJXdrgXwTDMMZ1AuXsN1EX5Mtm'
        ]:
            res = wallet.make_uri(address = address)
            assert res.uri == 'monero:' + address
            res = wallet.parse_uri(res.uri)
            assert res.uri.address == address
            assert res.uri.payment_id == ''
            assert res.uri.amount == 0
            assert res.uri.tx_description == ''
            assert res.uri.recipient_name == ''
            assert not 'unknown_parameters' in res or len(res.unknown_parameters) == 0
            res = wallet.make_uri(address = address, amount = 11000000000)
            assert res.uri == 'monero:' + address + '?tx_amount=0.011' or res.uri == 'monero:' + address + '?tx_amount=0.011000000000'
            res = wallet.parse_uri(res.uri)
            assert res.uri.address == address
            assert res.uri.payment_id == ''
            assert res.uri.amount == 11000000000
            assert res.uri.tx_description == ''
            assert res.uri.recipient_name == ''
            assert not 'unknown_parameters' in res or len(res.unknown_parameters) == 0

        address = '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'

        res = wallet.make_uri(address = address, tx_description = utf8string[0])
        assert res.uri == 'monero:' + address + '?tx_description=' + quoted_utf8string[0]
        res = wallet.parse_uri(res.uri)
        assert res.uri.address == address
        assert res.uri.payment_id == ''
        assert res.uri.amount == 0
        assert res.uri.tx_description == utf8string[0]
        assert res.uri.recipient_name == ''
        assert not 'unknown_parameters' in res or len(res.unknown_parameters) == 0

        res = wallet.make_uri(address = address, recipient_name = utf8string[0])
        assert res.uri == 'monero:' + address + '?recipient_name=' + quoted_utf8string[0]
        res = wallet.parse_uri(res.uri)
        assert res.uri.address == address
        assert res.uri.payment_id == ''
        assert res.uri.amount == 0
        assert res.uri.tx_description == ''
        assert res.uri.recipient_name == utf8string[0]
        assert not 'unknown_parameters' in res or len(res.unknown_parameters) == 0

        res = wallet.make_uri(address = address, recipient_name = utf8string[0], tx_description = utf8string[1])
        assert res.uri == 'monero:' + address + '?recipient_name=' + quoted_utf8string[0] + '&tx_description=' + quoted_utf8string[1]
        res = wallet.parse_uri(res.uri)
        assert res.uri.address == address
        assert res.uri.payment_id == ''
        assert res.uri.amount == 0
        assert res.uri.tx_description == utf8string[1]
        assert res.uri.recipient_name == utf8string[0]
        assert not 'unknown_parameters' in res or len(res.unknown_parameters) == 0

        res = wallet.make_uri(address = address, recipient_name = utf8string[0], tx_description = utf8string[1], amount = 1000000000000)
        assert res.uri == 'monero:' + address + '?tx_amount=1.000000000000&recipient_name=' + quoted_utf8string[0] + '&tx_description=' + quoted_utf8string[1]
        res = wallet.parse_uri(res.uri)
        assert res.uri.address == address
        assert res.uri.payment_id == ''
        assert res.uri.amount == 1000000000000
        assert res.uri.tx_description == utf8string[1]
        assert res.uri.recipient_name == utf8string[0]
        assert not 'unknown_parameters' in res or len(res.unknown_parameters) == 0

        # external payment ids are not supported anymore
        ok = False
        try: res = wallet.make_uri(address = address, recipient_name = utf8string[0], tx_description = utf8string[1], amount = 1000000000000, payment_id = '1' * 64)
        except: ok = True
        assert ok

        # spaces must be encoded as %20
        res = wallet.make_uri(address = address, tx_description = ' ' + utf8string[1] + ' ' + utf8string[0] + ' ', amount = 1000000000000)
        assert res.uri == 'monero:' + address + '?tx_amount=1.000000000000&tx_description=%20' + quoted_utf8string[1] + '%20' + quoted_utf8string[0] + '%20'
        res = wallet.parse_uri(res.uri)
        assert res.uri.address == address
        assert res.uri.payment_id == ''
        assert res.uri.amount == 1000000000000
        assert res.uri.tx_description == ' ' + utf8string[1] + ' ' + utf8string[0] + ' '
        assert res.uri.recipient_name == ''
        assert not 'unknown_parameters' in res or len(res.unknown_parameters) == 0

        # the example from the docs
        res = wallet.parse_uri('monero:46BeWrHpwXmHDpDEUmZBWZfoQpdc6HaERCNmx1pEYL2rAcuwufPN9rXHHtyUA4QVy66qeFQkn6sfK8aHYjA3jk3o1Bv16em?tx_amount=239.39014&tx_description=donation')
        assert res.uri.address == '46BeWrHpwXmHDpDEUmZBWZfoQpdc6HaERCNmx1pEYL2rAcuwufPN9rXHHtyUA4QVy66qeFQkn6sfK8aHYjA3jk3o1Bv16em'
        assert res.uri.amount == 239390140000000
        assert res.uri.tx_description == 'donation'
        assert res.uri.recipient_name == ''
        assert res.uri.payment_id == ''
        assert not 'unknown_parameters' in res or len(res.unknown_parameters) == 0

        # malformed/invalid
        for uri in [
            '',
            ':',
            'monero',
            'notmonero:42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm',
            'MONERO:42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm',
            'MONERO::42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm',
            'monero:',
            'monero:badaddress',
            'monero:tx_amount=10',
            'monero:?tx_amount=10',
            'monero:42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm?tx_amount=-1',
            'monero:42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm?tx_amount=1e12',
            'monero:42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm?tx_amount=+12',
            'monero:42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm?tx_amount=1+2',
            'monero:42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm?tx_amount=A',
            'monero:42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm?tx_amount=0x2',
            'monero:42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm?tx_amount=222222222222222222222',
            'monero:42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDn?tx_amount=10',
            'monero:42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm&',
            'monero:42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm&tx_amount',
            'monero:42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm&tx_amount=',
            'monero:42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm&tx_amount=10=',
            'monero:42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm&tx_amount=10=&',
            'monero:42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm&tx_amount=10=&foo=bar',
            'monero:42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm?tx_amount=10&tx_amount=20',
            'monero:42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm?tx_payment_id=1111111111111111',
            'monero:4BxSHvcgTwu25WooY4BVmgdcKwZu5EksVZSZkDd6ooxSVVqQ4ubxXkhLF6hEqtw96i9cf3cVfLw8UWe95bdDKfRQeYtPwLm1Jiw7AKt2LY?tx_payment_id=' + '1' * 64,
            'monero:9ujeXrjzf7bfeK3KZdCqnYaMwZVFuXemPU8Ubw335rj2FN1CdMiWNyFV3ksEfMFvRp9L9qum5UxkP5rN9aLcPxbH1au4WAB',
            'monero:5K8mwfjumVseCcQEjNbf59Um6R9NfVUNkHTLhhPCmNvgDLVS88YW5tScnm83rw9mfgYtchtDDTW5jEfMhygi27j1QYphX38hg6m4VMtN29',
            'monero:7A1Hr63MfgUa8pkWxueD5xBqhQczkusYiCMYMnJGcGmuQxa7aDBxN1G7iCuLCNB3VPeb2TW7U9FdxB27xKkWKfJ8VhUZthF',
        ]:
            ok = False
            try: res = wallet.parse_uri(uri)
            except: ok = True
            assert ok, res

        # unknown parameters but otherwise valid
        res = wallet.parse_uri('monero:' + address + '?tx_amount=239.39014&foo=bar')
        assert res.uri.address == address
        assert res.uri.amount == 239390140000000
        assert res.unknown_parameters == ['foo=bar'], res
        res = wallet.parse_uri('monero:' + address + '?tx_amount=239.39014&foo=bar&baz=quux')
        assert res.uri.address == address
        assert res.uri.amount == 239390140000000
        assert res.unknown_parameters == ['foo=bar', 'baz=quux'], res
        res = wallet.parse_uri('monero:' + address + '?tx_amount=239.39014&%20=%20')
        assert res.uri.address == address
        assert res.uri.amount == 239390140000000
        assert res.unknown_parameters == ['%20=%20'], res
        res = wallet.parse_uri('monero:' + address + '?tx_amount=239.39014&unknown=' + quoted_utf8string[0])
        assert res.uri.address == address
        assert res.uri.amount == 239390140000000
        assert res.unknown_parameters == [u'unknown=' + quoted_utf8string[0]], res


    def test_monero_multi_uri(self):
        print('Testing monero: URI - v2 functions')
        self.wallet = Wallet()
        self.addr1 = '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'
        self.addr2 = '4BxSHvcgTwu25WooY4BVmgdcKwZu5EksVZSZkDd6ooxSVVqQ4ubxXkhLF6hEqtw96i9cf3cVfLw8UWe95bdDKfRQeYtPwLm1Jiw7AKt2LY'
        self.addr3 = '8AsN91rznfkBGTY8psSNkJBg9SZgxxGGRUhGwRptBhgr5XSQ1XzmA9m8QAnoxydecSh5aLJXdrgXwTDMMZ1AuXsN1EX5Mtm'
        self.addr4 = '4C1oV8aJf1kKsyR2U1HdpWzGm9S6Th4QcyPzL2uUNWvZ7QfL3PGdZ3M1vExdUEfM1GtxkwVbHWr6k9VdPsoJ1vYoLt1m6a5Hg3nCzPjVtU'
        self.utf8string = [u'えんしゅう', u'あまやかす']
        
        self.test_multi_recipient_basic()
        self.test_parse_uri_v2_with_currency()
        self.test_make_uri_v2_with_currency()
        self.test_make_uri_v2_with_single_recipient_and_currency()
        self.test_parse_uri_rejects_multi_recipient_v2()
        self.test_parse_uri_v2_with_integrated_addresses()
        self.test_v2_with_mismatched_amounts_and_special_cases()
        self.test_unknown_parameters_and_compatibility()
    
    def _make_and_parse_v2(self, addresses, amounts, recipient_names=None, tx_description=''):
        res = self.wallet.make_uri_v2(addresses=addresses, amounts=amounts, recipient_names=(recipient_names or [''] * len(addresses)), tx_description=tx_description)
        return self.wallet.parse_uri_v2(res.uri)

    def _assert_multi_parsed(self, parsed, expected_addresses, expected_amounts, expected_currencies, expected_names=None, expected_desc=None,):
        to_list = lambda x: x if isinstance(x, (list, tuple)) else [x]

        addrs = to_list(expected_addresses)
        amts = to_list(expected_amounts)
        curs = to_list(expected_currencies)
        names = None if expected_names is None else to_list(expected_names)

        assert len(parsed.uri.addresses) == len(addrs)
        for i, (addr, amt, cur) in enumerate(zip(addrs, amts, curs)):
            assert parsed.uri.addresses[i] == addr
            
            assert parsed.uri.amounts[i] == {"amount": amt, "currency": cur}
            if names:
                assert parsed.uri.recipient_names[i] == names[i]

        if expected_desc:
            assert parsed.uri.tx_description == expected_desc

    def test_multi_recipient_basic(self):
        # two recipients
        addresses = [self.addr1, self.addr2]
        amounts = [500_000_000_000, 200_000_000_000]
        names = [self.utf8string[0], self.utf8string[1]]
        parsed = self._make_and_parse_v2(addresses, amounts, names, 'Multi recipient URI test with two payments')
        self._assert_multi_parsed(parsed, addresses, amounts, "XMR", names, 'Multi recipient URI test with two payments')

        # three recipients (one empty name)
        addresses = [self.addr1, self.addr2, self.addr3]
        amounts = [1_000_000_000_000, 500_000_000_000, 250_000_000_000]
        names = [self.utf8string[0], self.utf8string[1], '']
        parsed = self._make_and_parse_v2(addresses, amounts, names, 'Multi recipient URI test with three payments')
        self._assert_multi_parsed(parsed, addresses, amounts, "XMR", names, 'Multi recipient URI test with three payments')

    def test_parse_uri_v2_with_currency(self):
        from decimal import Decimal
        eth_str = "12345.67890123456789"
        cases = [
            (f"monero:{self.addr1}?version=2.0&amount=1BTC", 100_000_000, "BTC"),
            (f"monero:{self.addr1}?version=2.0&amount=0.5BTC", 50_000_000, "BTC"),
            (f"monero:{self.addr1}?version=2.0&amount=12.34EUR", 1234, "EUR"),
            (f"monero:{self.addr1}?version=2.0&amount=100ETH", 100 * 10**18, "ETH"),
            (f"monero:{self.addr1}?version=2.0&amount={eth_str}ETH", int(Decimal(eth_str) * (Decimal(10) ** 18)), "ETH")
        ]
        for uri, expected_amount, expected_currency in cases:
            parsed = self.wallet.parse_uri_v2(uri)
            self._assert_multi_parsed(parsed, self.addr1, expected_amount, expected_currency)

    def test_make_uri_v2_with_currency(self):
        expected = 1 * 10**18
        amounts = [{ 'amount': expected, 'currency': 'ETH' }]
        parsed = self._make_and_parse_v2([self.addr1, self.addr2], amounts, ['Alice', 'Bob'], 'eth payment')
        self._assert_multi_parsed(parsed, [self.addr1, self.addr2], expected, 'ETH', ['Alice', 'Bob'], 'eth payment')

    def _assert_raises(self, fn, msg):
        ok = False
        try:
            fn()
        except Exception:
            ok = True
        assert ok, msg

    def test_make_uri_v2_with_single_recipient_and_currency(self):
        amounts = [{ 'amount': 1, 'currency': 'ETH' }]
        uri = self._make_and_parse_v2([self.addr1], amounts, ['Alice'], 'eth payment')
        self._assert_raises(lambda: self.wallet.parse_uri(uri), "Expected rejection for single-recipient URI with currency in legacy parse_uri function.")

    def test_parse_uri_rejects_multi_recipient_v2(self):
        uri = 'monero:{a1}?version=2.0&amount=0.5&address={a2}&amount=0.2'.format(a1=self.addr1, a2=self.addr2)
        self._assert_raises(lambda: self.wallet.parse_uri(uri), "Expected rejection for multi-recipient v2 URIs by parse_uri (old parser)")

    def test_parse_uri_v2_with_integrated_addresses(self):
        uri = 'monero:{a1}?version=2.0&amount=0.1&address={a2}&amount=0.2'.format(a1=self.addr2, a2=self.addr4)
        self._assert_raises(lambda: self.wallet.parse_uri_v2(uri), f"Expected rejection for multiple integrated addresses but it parsed: {uri}")

    def test_v2_with_mismatched_amounts_and_special_cases(self):
        # mismatched amounts / missing second amount => decodes to 0
        uri = 'monero:{a1}?version=2.0&amount=0.5&label=Alice&address={a2}&label=Bob'.format(a1=self.addr1, a2=self.addr2)
        parsed = self.wallet.parse_uri_v2(uri)
        self._assert_multi_parsed(parsed, [self.addr1, self.addr2], [500_000_000_000, 0], 'XMR', ['Alice', 'Bob'])

        # trailing delimiter must not create empty payment
        uri_trailing = 'monero:{a1}?version=2.0&amount=0.5&label=Alice&address={a2}&amount=0.2&label=Bob&'.format(a1=self.addr1, a2=self.addr2)
        parsed = self.wallet.parse_uri_v2(uri_trailing)
        assert len(parsed.uri.addresses) == 2
        assert parsed.uri.addresses[0] == self.addr1
        assert parsed.uri.addresses[1] == self.addr2

        # special characters should round-trip (RPC should URL-encode)
        special_name = "A&B=Test?"
        special_desc = "Desc with spaces & symbols!"
        parsed = self._make_and_parse_v2([self.addr1, self.addr2], [750_000_000_000, 250_000_000_000], [special_name, special_name], special_desc)
        for n in parsed.uri.recipient_names:
            assert n == special_name
        assert parsed.uri.tx_description == special_desc
      
    def test_unknown_parameters_and_compatibility(self):
        # unknown parameters are collected by v2
        v1 = self.wallet.make_uri(address=self.addr1, amount=239_390_140_000_000, tx_description='donation')
        v1_with_unknown = v1.uri + '&foo=bar&baz=quux'
        parsed_v2 = self.wallet.parse_uri_v2(v1_with_unknown)
        assert parsed_v2.unknown_parameters == ['foo=bar', 'baz=quux']
    
        uri = f"monero:{self.addr1}?version=2.0&amount=239.39014&foo=bar&baz=quux"
        parsed_decimal = self.wallet.parse_uri_v2(uri)
        assert parsed_decimal.uri.addresses[0] == self.addr1
        assert parsed_decimal.uri.amounts[0]["amount"] == 239_390_140_000_000
        assert parsed_decimal.unknown_parameters == ['foo=bar', 'baz=quux']
        
        # compatibility: single-recipient old vs new
        amount = 250_000_000_000
        name = "Eve"
        desc = "compatibility test"
        old = self.wallet.make_uri(address=self.addr1, amount=amount, recipient_name=name, tx_description=desc)
        new = self.wallet.make_uri_v2(addresses=[self.addr1], amounts=[amount], recipient_names=[name], tx_description=desc)
        assert old.uri
        assert new.uri
        assert old.uri == new.uri

        # old -> parse v2
        old = self.wallet.make_uri(address=self.addr2, amount=11_000_000_000, recipient_name="Bob", tx_description="old->new parse test")
        parsed = self.wallet.parse_uri_v2(old.uri)
        self._assert_multi_parsed(parsed, self.addr2, 11_000_000_000, 'XMR', 'Bob', 'old->new parse test')
        assert parsed.get("unknown_parameters", []) == []

        # new -> old parse
        new = self.wallet.make_uri_v2(addresses=[self.addr3], amounts=[500_000_000_000], recipient_names=["Trent"], tx_description="new->old parse test")
        parsed_old = self.wallet.parse_uri(new.uri)
        self._assert_multi_parsed(parsed_old, self.addr3, 500_000_000_000, 'XMR', 'Trent', 'new->old parse test')
        assert parsed_old.uri.payment_id == ""
        assert parsed_old.get("unknown_parameters", []) == []
        
if __name__ == '__main__':
    URITest().run_test()
