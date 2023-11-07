set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]

set_property PACKAGE_PIN N8 [get_ports UART_1_0_rxd]
set_property IOSTANDARD LVCMOS33 [get_ports UART_1_0_rxd]
set_property PACKAGE_PIN N9 [get_ports UART_1_0_txd]
set_property IOSTANDARD LVCMOS33 [get_ports UART_1_0_txd]

# set_property PACKAGE_PIN AJ9 [get_ports sys_clk_p]
# set_property IOSTANDARD DIFF_SSTL12 [get_ports sys_clk_p]
#
# create_clock -period 5.000 -name sys_clk_p -waveform {0.000 2.500} [get_ports sys_clk_p]
#
# set_property PACKAGE_PIN J9 [get_ports rst_n]
# set_property IOSTANDARD LVCMOS33 [get_ports rst_n]
#
# set_property PACKAGE_PIN N8 [get_ports {uart_rx}]
# set_property IOSTANDARD LVCMOS33 [get_ports {uart_rx}]
# set_property PACKAGE_PIN N9 [get_ports {uart_tx}]
# set_property IOSTANDARD LVCMOS33 [get_ports {uart_tx}]
