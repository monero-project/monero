# XUNIA XMR unified platform

This repository is the canonical control plane for the combined XUNIA XMR platform.

## Layout

- Root: Monero-compatible core, daemon, wallet RPC, libraries, and tests.
- `apps/xuniaxmr`: pinned GUI workspace created by the bootstrap script.
- `platform/ontology`: Palantir Foundry object, link, and action contract.
- `scripts/xunia`: workspace bootstrap, node health, validation, and GPT-Doug defensive diagnosis.

The bootstrap process pins the GUI to a reviewed commit and links its expected `monero` source directory to this repository root. That eliminates duplicate core source during combined builds while preserving upstream project structure.

## Local start

```bash
./scripts/xunia/bootstrap_workspace.sh
python3 scripts/xunia/validate_platform.py
python3 scripts/xunia/node_health.py | tee /tmp/xunia-health.json
python3 scripts/xunia/diagnose.py < /tmp/xunia-health.json
```

The health probe is read-only and defaults to the local daemon RPC. Do not expose unrestricted wallet RPC or daemon administrative RPC to public networks.

## Projection

1. **Foundation:** unified source workspace, contract validation, core/GUI compatibility gate.
2. **Operations:** redacted node telemetry and human-approved GPT-Doug recommendations in Foundry.
3. **Delivery:** matched core/GUI release manifest, hashes, signatures, and rollback metadata.
4. **Experience:** unified launcher and operator dashboard consuming only approved RPC methods.
5. **Scale:** multi-node fleet views, staged deployments, and policy-controlled AIP workflows.

Consensus changes remain outside this platform layer and require the upstream cryptographic review process.
