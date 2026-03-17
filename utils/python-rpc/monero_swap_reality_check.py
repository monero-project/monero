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

"""Classify whether an XMR -> USDT path is valid for the given Monero RPC endpoints.

This tool focuses on the reality-check described in the plan:
- confirm which Monero network each endpoint is on
- explain why testnet/stagenet funds cannot become real USDT
- point to the valid dev/stagenet or real/mainnet branch
"""

from __future__ import annotations

import argparse
import json
import ssl
import sys
from dataclasses import asdict, dataclass
from decimal import Decimal, InvalidOperation
from typing import Any, Dict, List, Optional, Tuple
from urllib.error import HTTPError, URLError
from urllib.parse import urlparse
from urllib.request import (
    HTTPDigestAuthHandler,
    HTTPPasswordMgrWithDefaultRealm,
    HTTPSHandler,
    Request,
    build_opener,
)


MONERO_BASE58_ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"
MONERO_BASE58_INDEX = {char: value for value, char in enumerate(MONERO_BASE58_ALPHABET)}
MONERO_DECODED_BLOCK_SIZES = {
    2: 1,
    3: 2,
    5: 3,
    6: 4,
    7: 5,
    9: 6,
    10: 7,
    11: 8,
}
COMMON_RESTRICTED_RPC_PORTS = {
    18089: "mainnet",
    28089: "testnet",
    38089: "stagenet",
}
ADDRESS_PREFIX_BYTES = {
    18: ("mainnet", "standard"),
    19: ("mainnet", "integrated"),
    42: ("mainnet", "subaddress"),
    24: ("stagenet", "standard"),
    25: ("stagenet", "integrated"),
    36: ("stagenet", "subaddress"),
    53: ("testnet", "standard"),
    54: ("testnet", "integrated"),
    63: ("testnet", "subaddress"),
}
REFERENCES = [
    {
        "label": "Monero network docs",
        "url": "https://docs.getmonero.org/infrastructure/networks/",
    },
    {
        "label": "Monero standard address docs",
        "url": "https://docs.getmonero.org/public-address/standard-address/",
    },
    {
        "label": "Haveno stagenet install guide",
        "url": "https://docs.haveno.exchange/development/installing/",
    },
    {
        "label": "Haveno supported assets",
        "url": "https://docs.haveno.exchange/the-project/assets/",
    },
]


@dataclass
class EndpointConfig:
    raw: str
    display_url: str
    base_url: str
    scheme: str
    hostname: str
    port: int


@dataclass
class EndpointProbe:
    endpoint: EndpointConfig
    ok: bool
    network: Optional[str]
    network_source: Optional[str]
    rpc_route: Optional[str]
    height: Optional[int]
    version: Optional[str]
    error: Optional[str]


@dataclass
class AddressAnalysis:
    address: str
    ok: bool
    network: Optional[str]
    address_type: Optional[str]
    prefix_byte: Optional[int]
    error: Optional[str]


@dataclass
class FeasibilityReport:
    status: str
    mode: str
    target_asset: str
    target_chain: Optional[str]
    amount_atomic: Optional[int]
    amount_xmr: Optional[str]
    source_probe: EndpointProbe
    target_probe: EndpointProbe
    address_analysis: Optional[AddressAnalysis]
    summary: str
    reasons: List[str]
    warnings: List[str]
    next_steps: List[str]
    references: List[Dict[str, str]]


def parse_args(argv: List[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Probe Monero RPC endpoints and classify whether an XMR -> USDT path is valid."
    )
    parser.add_argument(
        "--source-rpc",
        required=True,
        help="Source Monero RPC endpoint, for example dev2.monerodevs.org:28089 or https://host:port",
    )
    parser.add_argument(
        "--target-rpc",
        required=True,
        help="Target Monero RPC endpoint, for example dev2.monerodevs.org:18089",
    )
    parser.add_argument(
        "--amount-atomic",
        type=parse_atomic_units,
        help="Optional XMR amount in atomic units (piconero).",
    )
    parser.add_argument(
        "--source-address",
        help="Optional Monero address to cross-check the network via the address prefix byte.",
    )
    parser.add_argument(
        "--target-asset",
        default="USDT",
        help="Destination asset. Defaults to USDT.",
    )
    parser.add_argument(
        "--target-chain",
        choices=("ERC20", "TRC20"),
        help="Desired USDT chain for the valid branch.",
    )
    parser.add_argument(
        "--mode",
        choices=("auto", "dev", "real"),
        default="auto",
        help="How to frame the recommendation. 'auto' follows the detected source network.",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=10.0,
        help="Per-request timeout in seconds. Defaults to 10.",
    )
    parser.add_argument(
        "--rpc-user",
        help="Optional Monero RPC username if the node requires digest auth.",
    )
    parser.add_argument(
        "--rpc-password",
        help="Optional Monero RPC password if the node requires digest auth.",
    )
    parser.add_argument(
        "--insecure",
        action="store_true",
        help="Skip TLS certificate validation for HTTPS endpoints.",
    )
    parser.add_argument(
        "--format",
        choices=("text", "json", "markdown"),
        default="text",
        help="Output format. Defaults to text.",
    )
    parser.add_argument(
        "--report-file",
        help="Optional path to write the rendered report.",
    )
    return parser.parse_args(argv)


def parse_atomic_units(value: str) -> int:
    try:
        parsed = int(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("amount must be an integer number of atomic units") from exc
    if parsed < 0:
        raise argparse.ArgumentTypeError("amount must be non-negative")
    return parsed


def parse_endpoint(raw: str) -> EndpointConfig:
    normalized = raw.strip()
    if "://" not in normalized:
        normalized = f"http://{normalized}"

    parsed = urlparse(normalized)
    if not parsed.hostname:
        raise ValueError(f"invalid endpoint '{raw}': missing hostname")
    if parsed.port is None:
        raise ValueError(f"invalid endpoint '{raw}': missing port")

    path = parsed.path.rstrip("/")
    if path.endswith("/json_rpc"):
        path = path[: -len("/json_rpc")]
    elif path.endswith("/get_info"):
        path = path[: -len("/get_info")]

    base_url = f"{parsed.scheme}://{parsed.hostname}:{parsed.port}{path}"
    display_url = f"{parsed.scheme}://{parsed.hostname}:{parsed.port}"
    return EndpointConfig(
        raw=raw,
        display_url=display_url,
        base_url=base_url,
        scheme=parsed.scheme,
        hostname=parsed.hostname,
        port=parsed.port,
    )


def build_rpc_opener(
    endpoint: EndpointConfig,
    username: Optional[str],
    password: Optional[str],
    insecure: bool,
):
    handlers = []
    if username and password:
        password_manager = HTTPPasswordMgrWithDefaultRealm()
        password_manager.add_password(None, endpoint.base_url, username, password)
        handlers.append(HTTPDigestAuthHandler(password_manager))

    if endpoint.scheme == "https":
        tls_context = ssl.create_default_context()
        if insecure:
            tls_context.check_hostname = False
            tls_context.verify_mode = ssl.CERT_NONE
        handlers.append(HTTPSHandler(context=tls_context))

    return build_opener(*handlers)


def post_json_rpc(
    endpoint: EndpointConfig,
    username: Optional[str],
    password: Optional[str],
    insecure: bool,
    timeout: float,
) -> Tuple[Dict[str, Any], str]:
    opener = build_rpc_opener(endpoint, username, password, insecure)
    payload = {
        "jsonrpc": "2.0",
        "id": "0",
        "method": "get_info",
    }
    request = Request(
        f"{endpoint.base_url}/json_rpc",
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with opener.open(request, timeout=timeout) as response:
        return json.loads(response.read().decode("utf-8")), "/json_rpc"


def get_info_json(
    endpoint: EndpointConfig,
    username: Optional[str],
    password: Optional[str],
    insecure: bool,
    timeout: float,
) -> Tuple[Dict[str, Any], str]:
    opener = build_rpc_opener(endpoint, username, password, insecure)
    request = Request(
        f"{endpoint.base_url}/get_info",
        headers={"Accept": "application/json"},
        method="GET",
    )
    with opener.open(request, timeout=timeout) as response:
        return json.loads(response.read().decode("utf-8")), "/get_info"


def probe_endpoint(
    endpoint_raw: str,
    username: Optional[str],
    password: Optional[str],
    insecure: bool,
    timeout: float,
) -> EndpointProbe:
    endpoint = parse_endpoint(endpoint_raw)
    port_network = infer_network_from_port(endpoint.port)
    errors: List[str] = []

    for fetcher in (post_json_rpc, get_info_json):
        try:
            payload, route = fetcher(endpoint, username, password, insecure, timeout)
            info = payload.get("result", payload)
            rpc_network = infer_network_from_info(info)
            network_source = "rpc" if rpc_network else "port_heuristic" if port_network else None
            return EndpointProbe(
                endpoint=endpoint,
                ok=True,
                network=rpc_network or port_network,
                network_source=network_source,
                rpc_route=route,
                height=safe_int(info.get("height")),
                version=safe_string(info.get("version")),
                error=None,
            )
        except (HTTPError, URLError, TimeoutError, OSError, ValueError, json.JSONDecodeError) as exc:
            errors.append(f"{fetcher.__name__}: {exc}")

    return EndpointProbe(
        endpoint=endpoint,
        ok=False,
        network=port_network,
        network_source="port_heuristic" if port_network else None,
        rpc_route=None,
        height=None,
        version=None,
        error=" | ".join(errors) if errors else "RPC probe failed",
    )


def safe_int(value: Any) -> Optional[int]:
    if value is None:
        return None
    try:
        return int(value)
    except (TypeError, ValueError):
        return None


def safe_string(value: Any) -> Optional[str]:
    if value is None:
        return None
    return str(value)


def infer_network_from_info(info: Dict[str, Any]) -> Optional[str]:
    nettype = safe_string(info.get("nettype"))
    if nettype in {"mainnet", "testnet", "stagenet"}:
        return nettype

    for candidate in ("mainnet", "testnet", "stagenet"):
        if info.get(candidate) is True:
            return candidate

    return None


def infer_network_from_port(port: int) -> Optional[str]:
    return COMMON_RESTRICTED_RPC_PORTS.get(port)


def decode_monero_base58(address: str) -> bytes:
    if len(address) not in (95, 106):
        raise ValueError("Monero addresses are usually 95 or 106 characters long")
    if any(char not in MONERO_BASE58_INDEX for char in address):
        raise ValueError("address contains characters outside the Monero Base58 alphabet")

    decoded = bytearray()
    for offset in range(0, len(address), 11):
        block = address[offset : offset + 11]
        decoded_block_size = MONERO_DECODED_BLOCK_SIZES.get(len(block))
        if decoded_block_size is None:
            raise ValueError(f"unsupported block length {len(block)}")

        value = 0
        for char in block:
            value = value * 58 + MONERO_BASE58_INDEX[char]

        if value >= 1 << (8 * decoded_block_size):
            raise ValueError("Base58 block overflow")
        decoded.extend(value.to_bytes(decoded_block_size, byteorder="big"))

    return bytes(decoded)


def analyze_address(address: Optional[str]) -> Optional[AddressAnalysis]:
    if not address:
        return None

    cleaned = address.strip()
    try:
        decoded = decode_monero_base58(cleaned)
        prefix_byte = decoded[0]
        network, address_type = ADDRESS_PREFIX_BYTES.get(prefix_byte, (None, "unknown"))
        return AddressAnalysis(
            address=cleaned,
            ok=True,
            network=network,
            address_type=address_type,
            prefix_byte=prefix_byte,
            error=None,
        )
    except ValueError as exc:
        return AddressAnalysis(
            address=cleaned,
            ok=False,
            network=None,
            address_type=None,
            prefix_byte=None,
            error=str(exc),
        )


def atomic_units_to_xmr(amount_atomic: Optional[int]) -> Optional[str]:
    if amount_atomic is None:
        return None
    try:
        amount = Decimal(amount_atomic) / Decimal(10**12)
    except (InvalidOperation, ValueError):
        return None
    return format(amount.normalize(), "f")


def choose_source_network(
    source_probe: EndpointProbe, address_analysis: Optional[AddressAnalysis]
) -> Tuple[Optional[str], List[str], List[str]]:
    reasons = []
    warnings = []

    if source_probe.network:
        source_text = source_probe.network
        if source_probe.network_source == "rpc":
            reasons.append(
                f"Source endpoint {source_probe.endpoint.display_url} reports {source_text} via {source_probe.rpc_route}."
            )
        elif source_probe.network_source == "port_heuristic":
            warnings.append(
                f"Source endpoint {source_probe.endpoint.display_url} could not be verified live; using the port {source_probe.endpoint.port} heuristic for {source_text}."
            )

    if address_analysis and address_analysis.ok and address_analysis.network:
        reasons.append(
            f"Source address prefix byte {address_analysis.prefix_byte} maps to {address_analysis.network} ({address_analysis.address_type})."
        )
        if source_probe.network and address_analysis.network != source_probe.network:
            warnings.append(
                "The source address network does not match the source RPC network. Verify the wallet network before moving funds."
            )

    return source_probe.network or (address_analysis.network if address_analysis else None), reasons, warnings


def choose_target_network(target_probe: EndpointProbe) -> Tuple[Optional[str], List[str], List[str]]:
    reasons = []
    warnings = []
    if target_probe.network:
        if target_probe.network_source == "rpc":
            reasons.append(
                f"Target endpoint {target_probe.endpoint.display_url} reports {target_probe.network} via {target_probe.rpc_route}."
            )
        elif target_probe.network_source == "port_heuristic":
            warnings.append(
                f"Target endpoint {target_probe.endpoint.display_url} could not be verified live; using the port {target_probe.endpoint.port} heuristic for {target_probe.network}."
            )
    return target_probe.network, reasons, warnings


def build_next_steps_for_dev(target_chain: Optional[str]) -> List[str]:
    chain_text = f"USDT-{target_chain}" if target_chain else "USDT-ERC20 or USDT-TRC20"
    return [
        "Stop treating the current testnet balance as economically convertible.",
        "Provision a stagenet wallet and a stagenet daemon or restricted RPC endpoint.",
        "Fund the stagenet wallet from a stagenet faucet before testing the trade flow.",
        f"Run the proof on Haveno stagenet and choose {chain_text}.",
        "Keep wallet, daemon, and trading venue on stagenet for the entire test.",
    ]


def build_next_steps_for_real(target_chain: Optional[str]) -> List[str]:
    venue_text = (
        f"USDT-{target_chain} withdrawals"
        if target_chain
        else "the exact USDT withdrawal chain you need"
    )
    return [
        "Source real XMR on mainnet; do not reuse testnet or stagenet balances.",
        "Connect the wallet to a mainnet daemon or trusted remote node before depositing anywhere.",
        f"Choose a venue that explicitly supports XMR deposits and {venue_text}.",
        "Confirm the deposit network, withdrawal chain, fees, and compliance requirements before sending funds.",
        "Treat the Monero RPC endpoint as node infrastructure only, not as the swap venue.",
    ]


def build_report(
    source_probe: EndpointProbe,
    target_probe: EndpointProbe,
    address_analysis: Optional[AddressAnalysis],
    amount_atomic: Optional[int],
    target_asset: str,
    target_chain: Optional[str],
    mode: str,
) -> FeasibilityReport:
    target_asset_normalized = target_asset.upper()
    source_network, source_reasons, source_warnings = choose_source_network(source_probe, address_analysis)
    target_network, target_reasons, target_warnings = choose_target_network(target_probe)
    reasons = source_reasons + target_reasons
    warnings = source_warnings + target_warnings

    if not source_probe.ok:
        warnings.append(f"Source RPC probe failed: {source_probe.error}")
    if not target_probe.ok:
        warnings.append(f"Target RPC probe failed: {target_probe.error}")
    if address_analysis and not address_analysis.ok:
        warnings.append(f"Source address could not be decoded: {address_analysis.error}")

    inferred_mode = mode
    if mode == "auto":
        if source_network in {"testnet", "stagenet"}:
            inferred_mode = "dev"
        elif source_network == "mainnet":
            inferred_mode = "real"

    summary = "Unable to classify the route with confidence."
    status = "needs_manual_confirmation"
    next_steps = [
        "Re-run the probe against a reachable Monero node and capture get_info output.",
        "Confirm the wallet network locally before moving funds.",
    ]

    if target_asset_normalized != "USDT":
        warnings.append(
            f"This checker is opinionated for USDT. You passed {target_asset_normalized}, so treat the next steps as a network check only."
        )

    if source_network == "testnet":
        status = "not_possible_as_stated"
        summary = "The source balance is on Monero testnet, so there is no direct path from that XMR to real, spendable USDT."
        reasons.append("Monero testnet, stagenet, and mainnet are separate blockchains; RPC endpoints do not bridge funds between them.")
        if target_network == "mainnet":
            reasons.append("The target endpoint is a mainnet node, not a conversion bridge from testnet to mainnet.")
        next_steps = build_next_steps_for_dev(target_chain) + build_next_steps_for_real(target_chain)
    elif source_network == "stagenet" and inferred_mode == "dev":
        status = "dev_only"
        summary = "A stagenet-only proof is valid, but it will not settle into real USDT."
        reasons.append("Stagenet is appropriate for testing the workflow, but not for acquiring spendable assets.")
        next_steps = build_next_steps_for_dev(target_chain)
    elif source_network == "stagenet" and inferred_mode == "real":
        status = "not_possible_as_stated"
        summary = "Stagenet XMR cannot be swapped into real USDT. Start from mainnet XMR for the real-funds path."
        reasons.append("The real-funds branch requires mainnet XMR from end to end.")
        next_steps = build_next_steps_for_real(target_chain)
    elif source_network == "mainnet":
        status = "possible_with_real_venue"
        summary = "Mainnet XMR can be swapped into USDT, but not by moving coins between Monero RPC endpoints alone."
        reasons.append("A Monero RPC endpoint provides node access only; the actual swap must happen on a venue that supports XMR and your chosen USDT chain.")
        if target_network and target_network != "mainnet":
            warnings.append("The target endpoint does not look like mainnet. Use a mainnet-only flow for real funds.")
        next_steps = build_next_steps_for_real(target_chain)
    elif source_network is None:
        status = "network_unconfirmed"
        summary = "The source network could not be confirmed, so the XMR -> USDT path remains untrusted."
        warnings.append("Do not move funds until the wallet and node both confirm the same network.")

    return FeasibilityReport(
        status=status,
        mode=inferred_mode,
        target_asset=target_asset_normalized,
        target_chain=target_chain,
        amount_atomic=amount_atomic,
        amount_xmr=atomic_units_to_xmr(amount_atomic),
        source_probe=source_probe,
        target_probe=target_probe,
        address_analysis=address_analysis,
        summary=summary,
        reasons=reasons,
        warnings=warnings,
        next_steps=next_steps,
        references=REFERENCES,
    )


def render_text(report: FeasibilityReport) -> str:
    lines = [
        "Monero XMR -> USDT Reality Check",
        f"Status: {report.status}",
        f"Mode: {report.mode}",
        f"Summary: {report.summary}",
    ]

    if report.amount_atomic is not None:
        lines.append(f"Amount: {report.amount_atomic} atomic units ({report.amount_xmr} XMR)")

    lines.extend(
        [
            f"Source RPC: {report.source_probe.endpoint.display_url}",
            f"Source network: {report.source_probe.network or 'unknown'}",
            f"Target RPC: {report.target_probe.endpoint.display_url}",
            f"Target network: {report.target_probe.network or 'unknown'}",
        ]
    )

    if report.address_analysis:
        lines.append(
            f"Source address analysis: {report.address_analysis.network or 'unknown'} ({report.address_analysis.address_type or 'unknown'}), prefix byte={report.address_analysis.prefix_byte}"
            if report.address_analysis.ok
            else f"Source address analysis: failed ({report.address_analysis.error})"
        )

    if report.reasons:
        lines.append("")
        lines.append("Reasons:")
        for reason in report.reasons:
            lines.append(f"- {reason}")

    if report.warnings:
        lines.append("")
        lines.append("Warnings:")
        for warning in report.warnings:
            lines.append(f"- {warning}")

    if report.next_steps:
        lines.append("")
        lines.append("Next steps:")
        for index, step in enumerate(report.next_steps, start=1):
            lines.append(f"{index}. {step}")

    lines.append("")
    lines.append("References:")
    for reference in report.references:
        lines.append(f"- {reference['label']}: {reference['url']}")

    return "\n".join(lines)


def render_markdown(report: FeasibilityReport) -> str:
    lines = [
        "# Monero XMR -> USDT Reality Check",
        "",
        f"- Status: `{report.status}`",
        f"- Mode: `{report.mode}`",
        f"- Summary: {report.summary}",
        f"- Source RPC: `{report.source_probe.endpoint.display_url}`",
        f"- Source network: `{report.source_probe.network or 'unknown'}`",
        f"- Target RPC: `{report.target_probe.endpoint.display_url}`",
        f"- Target network: `{report.target_probe.network or 'unknown'}`",
    ]

    if report.amount_atomic is not None:
        lines.append(f"- Amount: `{report.amount_atomic}` atomic units (`{report.amount_xmr}` XMR)")
    if report.target_chain:
        lines.append(f"- Target chain: `USDT-{report.target_chain}`")
    if report.address_analysis and report.address_analysis.ok:
        lines.append(
            f"- Source address: `{report.address_analysis.network or 'unknown'}` `{report.address_analysis.address_type or 'unknown'}`"
        )

    if report.reasons:
        lines.extend(["", "## Reasons"])
        for reason in report.reasons:
            lines.append(f"- {reason}")

    if report.warnings:
        lines.extend(["", "## Warnings"])
        for warning in report.warnings:
            lines.append(f"- {warning}")

    if report.next_steps:
        lines.extend(["", "## Next Steps"])
        for step in report.next_steps:
            lines.append(f"- {step}")

    lines.extend(["", "## References"])
    for reference in report.references:
        lines.append(f"- [{reference['label']}]({reference['url']})")

    return "\n".join(lines)


def render_json(report: FeasibilityReport) -> str:
    return json.dumps(asdict(report), indent=2, sort_keys=True)


def render_report(report: FeasibilityReport, output_format: str) -> str:
    if output_format == "json":
        return render_json(report)
    if output_format == "markdown":
        return render_markdown(report)
    return render_text(report)


def write_report(report_text: str, report_file: Optional[str]) -> None:
    if not report_file:
        return
    with open(report_file, "w", encoding="utf-8") as handle:
        handle.write(report_text)
        if not report_text.endswith("\n"):
            handle.write("\n")


def main(argv: List[str]) -> int:
    args = parse_args(argv)
    try:
        source_probe = probe_endpoint(
            endpoint_raw=args.source_rpc,
            username=args.rpc_user,
            password=args.rpc_password,
            insecure=args.insecure,
            timeout=args.timeout,
        )
        target_probe = probe_endpoint(
            endpoint_raw=args.target_rpc,
            username=args.rpc_user,
            password=args.rpc_password,
            insecure=args.insecure,
            timeout=args.timeout,
        )
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    address_analysis = analyze_address(args.source_address)
    report = build_report(
        source_probe=source_probe,
        target_probe=target_probe,
        address_analysis=address_analysis,
        amount_atomic=args.amount_atomic,
        target_asset=args.target_asset,
        target_chain=args.target_chain,
        mode=args.mode,
    )
    rendered = render_report(report, args.format)
    write_report(rendered, args.report_file)
    print(rendered)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
