#!/usr/bin/env -S vivado -mode batch -source
# https://docs.xilinx.com/r/en-US/ug895-vivado-system-level-design-entry
# https://docs.xilinx.com/r/en-US/ug835-vivado-tcl-commands
cd [file dirname [info script]]
exec mkdir -p build
cd build
set project project
create_project -force -part xczu7ev-ffvc1156-2-i ${project} ${project}
cd $project
# open_project [glob *.xpr]

create_bd_design design_1
create_bd_cell -type ip -vlnv xilinx.com:ip:zynq_ultra_ps_e:3.5 zynq_ultra_ps_e_0
connect_bd_net [get_bd_pins zynq_ultra_ps_e_0/maxihpm0_lpd_aclk] [get_bd_pins zynq_ultra_ps_e_0/pl_clk0]
set_property -dict [list \
  CONFIG.PSU_BANK_0_IO_STANDARD {LVCMOS18} \
  CONFIG.PSU_BANK_1_IO_STANDARD {LVCMOS18} \
  CONFIG.PSU_BANK_2_IO_STANDARD {LVCMOS18} \
  CONFIG.PSU__CAN0__PERIPHERAL__ENABLE {1} \
  CONFIG.PSU__CAN0__PERIPHERAL__IO {MIO 38 .. 39} \
  CONFIG.PSU__CAN1__PERIPHERAL__ENABLE {1} \
  CONFIG.PSU__CAN1__PERIPHERAL__IO {MIO 40 .. 41} \
  CONFIG.PSU__CRF_APB__DP_AUDIO_REF_CTRL__SRCSEL {RPLL} \
  CONFIG.PSU__CRF_APB__DP_STC_REF_CTRL__SRCSEL {RPLL} \
  CONFIG.PSU__CRF_APB__DP_VIDEO_REF_CTRL__SRCSEL {VPLL} \
  CONFIG.PSU__CRF_APB__TOPSW_MAIN_CTRL__SRCSEL {APLL} \
  CONFIG.PSU__CRL_APB__CPU_R5_CTRL__SRCSEL {IOPLL} \
  CONFIG.PSU__CRL_APB__SDIO0_REF_CTRL__SRCSEL {IOPLL} \
  CONFIG.PSU__CRL_APB__SDIO1_REF_CTRL__SRCSEL {IOPLL} \
  CONFIG.PSU__DDRC__DEVICE_CAPACITY {8192 MBits} \
  CONFIG.PSU__DDRC__ROW_ADDR_COUNT {16} \
  CONFIG.PSU__DISPLAYPORT__PERIPHERAL__ENABLE {1} \
  CONFIG.PSU__DPAUX__PERIPHERAL__IO {MIO 27 .. 30} \
  CONFIG.PSU__ENET3__GRP_MDIO__ENABLE {1} \
  CONFIG.PSU__ENET3__PERIPHERAL__ENABLE {1} \
  CONFIG.PSU__GPIO1_MIO__PERIPHERAL__ENABLE {1} \
  CONFIG.PSU__I2C0__PERIPHERAL__ENABLE {1} \
  CONFIG.PSU__I2C0__PERIPHERAL__IO {MIO 34 .. 35} \
  CONFIG.PSU__PCIE__CLASS_CODE_SUB {0x04} \
  CONFIG.PSU__PCIE__DEVICE_PORT_TYPE {Root Port} \
  CONFIG.PSU__PCIE__PERIPHERAL__ENABLE {1} \
  CONFIG.PSU__PCIE__PERIPHERAL__ROOTPORT_IO {MIO 37} \
  CONFIG.PSU__QSPI__GRP_FBCLK__ENABLE {1} \
  CONFIG.PSU__QSPI__PERIPHERAL__ENABLE {1} \
  CONFIG.PSU__QSPI__PERIPHERAL__MODE {Dual Parallel} \
  CONFIG.PSU__SD0__DATA_TRANSFER_MODE {8Bit} \
  CONFIG.PSU__SD0__PERIPHERAL__ENABLE {1} \
  CONFIG.PSU__SD0__PERIPHERAL__IO {MIO 13 .. 22} \
  CONFIG.PSU__SD0__RESET__ENABLE {1} \
  CONFIG.PSU__SD0__SLOT_TYPE {eMMC} \
  CONFIG.PSU__SD1__GRP_CD__ENABLE {1} \
  CONFIG.PSU__SD1__PERIPHERAL__ENABLE {1} \
  CONFIG.PSU__SD1__PERIPHERAL__IO {MIO 46 .. 51} \
  CONFIG.PSU__SD1__SLOT_TYPE {SD 2.0} \
  CONFIG.PSU__TTC0__PERIPHERAL__ENABLE {1} \
  CONFIG.PSU__TTC1__PERIPHERAL__ENABLE {1} \
  CONFIG.PSU__TTC2__PERIPHERAL__ENABLE {1} \
  CONFIG.PSU__TTC3__PERIPHERAL__ENABLE {1} \
  CONFIG.PSU__UART0__PERIPHERAL__ENABLE {1} \
  CONFIG.PSU__UART0__PERIPHERAL__IO {MIO 42 .. 43} \
  CONFIG.PSU__USB0__PERIPHERAL__ENABLE {1} \
  CONFIG.PSU__USB0__REF_CLK_SEL {Ref Clk1} \
  CONFIG.PSU__USB0__RESET__ENABLE {1} \
  CONFIG.PSU__USB0__RESET__IO {MIO 32} \
  CONFIG.PSU__USB3_0__PERIPHERAL__ENABLE {1} \
  CONFIG.PSU__USB3_0__PERIPHERAL__IO {GT Lane1} \
  CONFIG.PSU__USB__RESET__MODE {Shared MIO Pin} \
  CONFIG.SUBPRESET1 {DDR4_MICRON_MT40A256M16GE_083E} \
  CONFIG.PSU__UART1__PERIPHERAL__ENABLE {1} \
  CONFIG.PSU__UART1__PERIPHERAL__IO {EMIO} \
] [get_bd_cells zynq_ultra_ps_e_0]
make_bd_intf_pins_external [get_bd_intf_pins zynq_ultra_ps_e_0/UART_1]

# -top is needed
make_wrapper -files [get_files design_1.bd] -top
add_files project.gen/sources_1/bd/design_1/hdl/design_1_wrapper.v
# open_bd_design [glob *.srcs/sources_1/bd/*/*.bd]

# generate_target all [get_files design_1.bd]
# generate nothing
# config_ip_cache -export [get_ips design_1_zynq_ultra_ps_e_0_0]
# generate project.ip_user_files/bd/
# export_ip_user_files -of_objects [get_files design_1.bd] -no_script -sync
# generate project.ip_user_files/sim_scripts/
# export_simulation -use_ip_compiled_libs \
# -of_objects [get_files design_1.bd] \
# -directory project.ip_user_files/sim_scripts \
# -ip_user_files_dir project.ip_user_files \
# -ipstatic_source_dir project.ip_user_files/ipstatic \
# -lib_map_path [ \
#   list {modelsim=project.cache/compile_simlib/modelsim} \
#   {questa=project.cache/compile_simlib/questa} \
#   {xcelium=project.cache/compile_simlib/xcelium} \
#   {vcs=project.cache/compile_simlib/vcs} \
#   {riviera=project.cache/compile_simlib/riviera} \
# ]

# add_files [glob ../../project.srcs/sources_1/imports/new/*.v]
add_files -fileset constrs_1 [glob ../../project.srcs/constrs_1/imports/new/*.xdc]
import_files

# reset_run impl_1
# will call `launch_runs synth_1` automatically to generate *.dcp
launch_runs impl_1 -to_step write_bitstream
# wait about 3.25 mins
wait_on_runs impl_1

write_hw_platform -force -fixed -include_bit -file system.xsa

exit

# for program
open_hw_manager
# ERROR: [Labtoolstcl 44-235] The labtools system is not initialized. You must execute 'open_hw_manager' to initialize the labtools system prior using this command.
connect_hw_server
# WARNING: [Labtoolstcl 44-128] No matching hw_devices were found.
open_hw_target
set_property PROGRAM.FILE [glob *.runs/impl_1/*.bit] [get_hw_devices xczu7_0]
program_hw_devices [get_hw_devices xczu7_0]
