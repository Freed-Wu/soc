// 115200 8N1
module uart_rx
    #(
         // MHz
         parameter CLK_FRE = 50,
         parameter BAUD_RATE = 115200,
         parameter BITS = 8
     )
     (
         input clk,
         input rst_n,
         output reg[7:0] rx_data,
         output reg rx_data_valid,
         input rx_data_ready,
         input rx_pin
     );
    // the clock cycle for a baud/bit
    // make sure CYCLES is even
    localparam CYCLES = CLK_FRE * 1000000 / BAUD_RATE;

    localparam S_IDLE = 1;
    localparam S_START = 2;
    localparam S_REC_BYTE = 3;
    localparam S_STOP = 4;
    localparam S_DATA = 5;

    reg[2:0] state;
    reg[2:0] next_state;
    reg[15:0] cycle_cnt;
    reg[2:0] bit_cnt;
    reg rx_d0;
    reg rx_d1;

    assign rx_negedge = rx_d1 && ~rx_d0;

    // beat delay one cycle to avoid competition
    // https://www.runoob.com/w3cnote/verilog-competition-hazard.html
    always@(posedge clk or negedge rst_n)
    begin
        if(rst_n == 1'b0)
        begin
            rx_d0 <= 1'b0;
            rx_d1 <= 1'b0;
            state <= S_IDLE;
        end
        else
        begin
            rx_d0 <= rx_pin;
            rx_d1 <= rx_d0;
            state <= next_state;
        end
    end

    // count cycles of a bit
    always@(posedge clk or negedge rst_n)
    begin
        if(rst_n == 1'b0 || next_state != state || state == S_REC_BYTE && cycle_cnt == CYCLES - 1)
            cycle_cnt <= 16'd0;
        else
            cycle_cnt <= cycle_cnt + 16'd1;
    end

    // count bits of a frame
    always@(posedge clk or negedge rst_n)
    begin
        if(rst_n == 1'b0 || state != S_REC_BYTE)
            bit_cnt <= 3'd0;
        else if(cycle_cnt == CYCLES - 1)
            bit_cnt <= bit_cnt + 3'd1;
    end

    always@(*)
    begin
        case(state)
            S_IDLE:
                if(rx_negedge)
                    next_state <= S_START;
                else
                    next_state <= state;
            S_START:
                if(cycle_cnt == CYCLES - 1)
                    next_state <= S_REC_BYTE;
                else
                    next_state <= state;
            S_REC_BYTE:
                // receive 8bit data
                if(cycle_cnt == CYCLES - 1 && bit_cnt == BITS - 1)
                    next_state <= S_STOP;
                else
                    next_state <= state;
            S_STOP:
                // half one cycle to avoid missing the next byte receiver
                if(cycle_cnt == CYCLES / 2 - 1)
                    next_state <= S_DATA;
                else
                    next_state <= state;
            S_DATA:
                if(rx_data_ready)
                    next_state <= S_IDLE;
                else
                    next_state <= state;
            default:
                next_state <= S_IDLE;
        endcase
    end

    always@(posedge clk or negedge rst_n)
    begin
        if(rst_n == 1'b0 || state == S_DATA && rx_data_ready)
            rx_data_valid <= 1'b0;
        else if(state == S_STOP && next_state != state)
            rx_data_valid <= 1'b1;
    end

    always@(posedge clk or negedge rst_n)
    begin
        if(rst_n == 1'b0)
            rx_data <= 8'd0;
        else if(state == S_REC_BYTE && cycle_cnt == CYCLES / 2 - 1)
            rx_data[bit_cnt] <= rx_pin;
    end
endmodule
