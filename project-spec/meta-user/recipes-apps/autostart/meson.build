project(
  'soc',
  ['c', 'cpp'],
  default_options: ['debug=true'],
)
subdir('src')
subdir('tests')
if get_option('binary')
  bin = 'assets/bin/'
  install_data(
    [bin + 'weight.bin', bin + 'cdf.bin', bin + 'exp.bin'],
    install_dir: '/usr/share/autostart',
  )
endif
systemd_dep = dependency('systemd', method: 'pkg-config', required: false)
systemd_system_unit_dir = systemd_dep.get_variable(
  'systemd_system_unit_dir',
  default_value: '/lib/systemd/system',
)
systemd_system_preset_dir = systemd_dep.get_variable(
  'systemd_system_preset_dir',
  default_value: '/lib/systemd/system-preset',
)
install_data(
  'assets/sysvinit/autostart',
  install_dir: '/etc/init.d',
)
install_data(
  'assets/systemd/system/autostart.service',
  install_dir: systemd_system_unit_dir,
)
install_data(
  'assets/systemd/system-preset/98-autostart.preset',
  install_dir: systemd_system_preset_dir,
)
install_data(
  'scripts/post-burn-emmc.sh',
  install_dir: 'bin',
)
