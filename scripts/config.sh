#!/usr/bin/env bash
set -e
cd "$(dirname "$(readlink -f "$0")")/.."

# initial project-spec/configs/config and project-spec/configs/rootfs_config
petalinux-config --silentconfig
petalinux-config -crootfs --silentconfig

# configure again because some options will occur after `petalinux-config --silentconfig`
for _i in 1 2; do
	for config; do
		scripts/config.pl "$config" project-spec/configs/config
		scripts/config.pl "$config" project-spec/configs/rootfs_config
	done
	# refresh config
	petalinux-config --silentconfig
	petalinux-config -crootfs --silentconfig
done

petalinux-create -t apps -n autostart --enable --force
petalinux-create -t modules -n axi-tangxi --enable --force
# reset changes of `--force`
git restore project-spec/meta-user/{recipes-apps/autostart,recipes-modules}
git clean -fd project-spec/meta-user/{recipes-apps/autostart,recipes-modules}
install -Dm644 project-spec/meta-user/recipes-apps/autostart.old/assets/bin/* -t project-spec/meta-user/recipes-apps/autostart/assets/bin || true
rm -rf project-spec/meta-user/recipes-{app,module}s/*.old
