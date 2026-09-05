#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
gui_dir="${root}/apps/xuniaxmr"
gui_repo="${XUNIA_GUI_REPOSITORY:-https://github.com/sonoxo/xuniaxmr.git}"
gui_commit="${XUNIA_GUI_COMMIT:-7691273c6303de9b128c447fb29c437a814e62e6}"

mkdir -p "${root}/apps"
if [[ ! -d "${gui_dir}/.git" ]]; then
  git clone --filter=blob:none --no-checkout "${gui_repo}" "${gui_dir}"
fi
git -C "${gui_dir}" fetch --depth 1 origin "${gui_commit}"
git -C "${gui_dir}" checkout --detach "${gui_commit}"

rm -rf "${gui_dir}/monero"
ln -s ../.. "${gui_dir}/monero"

printf 'XUNIA workspace ready\ncore=%s\ngui=%s\ngui_commit=%s\n' "${root}" "${gui_dir}" "${gui_commit}"
