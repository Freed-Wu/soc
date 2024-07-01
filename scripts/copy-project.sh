#!/usr/bin/env bash
set -e
cd "$(dirname "$(readlink -f "$0")")/.."
pwd=$PWD

cd "$(dirname "${1:-../soc}")"
basename="$(basename "${1:-../soc}")"
petalinux-create -tproject -n"$basename" --template zynqMP
cd "$basename"
cp -r "$pwd/.git" .
git restore .

install -Dm644 "${2:-"$pwd"/project-spec/hw-description/system.xsa}" -t project-spec/hw-description
install -Dm644 "$pwd"/project-spec/meta-user/recipes-apps/autostart/assets/bin/{weight,cdf,exp}.bin -t project-spec/meta-user/recipes-apps/autostart/assets/bin
