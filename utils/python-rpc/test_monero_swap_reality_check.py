#!/usr/bin/env python3

# Copyright (c) 2026, The Monero Project
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

import unittest
from typing import Optional

import monero_swap_reality_check as checker


class MoneroSwapRealityCheckTests(unittest.TestCase):
    def make_probe(self, raw: str, ok: bool, network: Optional[str], source: Optional[str]):
        endpoint = checker.parse_endpoint(raw)
        return checker.EndpointProbe(
            endpoint=endpoint,
            ok=ok,
            network=network,
            network_source=source,
            rpc_route="/json_rpc" if ok else None,
            height=123,
            version="18.3.4.0",
            error=None if ok else "unreachable",
        )

    def test_infer_network_from_info_prefers_explicit_flags(self):
        info = {"mainnet": False, "testnet": True, "stagenet": False}
        self.assertEqual(checker.infer_network_from_info(info), "testnet")

    def test_parse_endpoint_normalizes_scheme(self):
        endpoint = checker.parse_endpoint("dev2.monerodevs.org:28089")
        self.assertEqual(endpoint.base_url, "http://dev2.monerodevs.org:28089")
        self.assertEqual(endpoint.port, 28089)

    def test_analyze_address_detects_mainnet_standard_address(self):
        donation_address = (
            "44AFFq5kSiGBoZ4NMDwYtN18obc8AemS33DBLWs3H7otXft3XjrpDtQGv7SqSsaBYBb98uNbr2VBBEt7f2wfn3RVGQBEP3A"
        )
        analysis = checker.analyze_address(donation_address)
        self.assertIsNotNone(analysis)
        self.assertTrue(analysis.ok)
        self.assertEqual(analysis.network, "mainnet")
        self.assertEqual(analysis.address_type, "standard")
        self.assertEqual(analysis.prefix_byte, 18)

    def test_build_report_rejects_testnet_to_real_usdt(self):
        source_probe = self.make_probe("dev2.monerodevs.org:28089", True, "testnet", "rpc")
        target_probe = self.make_probe("dev2.monerodevs.org:18089", True, "mainnet", "rpc")

        report = checker.build_report(
            source_probe=source_probe,
            target_probe=target_probe,
            address_analysis=None,
            amount_atomic=3532212,
            target_asset="USDT",
            target_chain="TRC20",
            mode="auto",
        )

        self.assertEqual(report.status, "not_possible_as_stated")
        self.assertIn("testnet", report.summary)
        self.assertTrue(any("mainnet node" in reason for reason in report.reasons))

    def test_build_report_accepts_mainnet_with_real_venue(self):
        source_probe = self.make_probe("mainnet-node.example:18089", True, "mainnet", "rpc")
        target_probe = self.make_probe("mainnet-node.example:18089", True, "mainnet", "rpc")

        report = checker.build_report(
            source_probe=source_probe,
            target_probe=target_probe,
            address_analysis=None,
            amount_atomic=1257467472,
            target_asset="USDT",
            target_chain="TRC20",
            mode="auto",
        )

        self.assertEqual(report.status, "possible_with_real_venue")
        self.assertIn("Mainnet XMR", report.summary)
        self.assertTrue(any("USDT-TRC20" in step for step in report.next_steps))


if __name__ == "__main__":
    unittest.main()
