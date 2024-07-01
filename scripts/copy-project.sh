#!/usr/bin/env bash
set -e
cd "$(dirname "$(readlink -f "$0")")/.."
pwd=$PWD

[[ -n $1 ]] || set -- ../soc

cd "$(dirname "$1")"
basename="$(basename "$1")"
petalinux-create -tproject -n"$basename" --template zynqMP
cd "$basename"
cp -r "$pwd/.git" .
git restore .

install -Dm644 "$pwd"/project-spec/hw-description/system.xsa -t project-spec/hw-description
install -Dm644 "$pwd"/project-spec/meta-user/recipes-apps/autostart/assets/bin/{weight,cdf,exp}.bin -t project-spec/meta-user/recipes-apps/autostart/assets/bin
