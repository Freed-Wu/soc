module uart
    #(
         // MHz
         parameter CLK_FRE = 200
     )
     (
         input sys_clk_p,
         input sys_clk_n,
         input rst_n,
         input uart_rx,
         output uart_tx
     );

    reg[7:0] tx_data;
    reg tx_data_valid;
    wire tx_data_ready;
    wire[7:0] rx_data;
    wire rx_data_valid;
    wire rx_data_ready;
    wire sys_clk;

    IBUFDS
        #(
            .DIFF_TERM("FALSE"), // Differential Termination
            .IBUF_LOW_PWR("TRUE"), // Low power="TRUE", Highest performance="FALSE"
            .IOSTANDARD("DEFAULT") // Specify the input I/O standard
        )
        IBUFDS_inst (
            .O(sys_clk),
            .I(sys_clk_p),
            .IB(sys_clk_n)
        );

    uart_rx
        #(
            .CLK_FRE(CLK_FRE)
        ) uart_rx_inst
        (
            .clk (sys_clk),
            .rst_n (rst_n),
            .rx_data (rx_data),
            .rx_data_valid (rx_data_valid),
            .rx_data_ready (rx_data_ready),
            .rx_pin (uart_rx)
        );

    uart_tx
        #(
            .CLK_FRE(CLK_FRE)
        ) uart_tx_inst
        (
            .clk (sys_clk),
            .rst_n (rst_n),
            .tx_data (tx_data),
            .tx_data_valid (tx_data_valid),
            .tx_data_ready (tx_data_ready),
            .tx_pin (uart_tx)
        );

    // enable receiving
    assign rx_data_ready = 1'b1;

    always@(posedge sys_clk or negedge rst_n)
    begin
        if(rst_n == 1'b0)
        begin
            tx_data <= 8'd0;
            tx_data_valid <= 1'b0;
        end
        else
        begin
            if(rx_data_valid == 1'b1)
            begin
                tx_data_valid <= 1'b1;
                tx_data <= rx_data;
            end
            else if(tx_data_valid && tx_data_ready)
                tx_data_valid <= 1'b0;
        end
    end
endmodule
