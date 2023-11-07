// 115200 8N1
module uart_tx
    #(
         // MHz
         parameter CLK_FRE = 50,
         parameter BAUD_RATE = 115200,
         parameter BITS = 8
     )
     (
         input clk,
         input rst_n,
         input[7:0] tx_data,
         input tx_data_valid,
         output reg tx_data_ready,
         output tx_pin
     );
    // the clock cycle for a baud/bit
    // make sure CYCLES is even
    localparam CYCLES = CLK_FRE * 1000000 / BAUD_RATE;

    localparam S_IDLE = 1;
    localparam S_START = 2;
    localparam S_SEND_BYTE = 3;
    localparam S_STOP = 4;

    reg[2:0] state;
    reg[2:0] next_state;
    reg[15:0] cycle_cnt;
    reg[2:0] bit_cnt;
    reg[7:0] tx_data_latch;
    reg tx_reg;
    assign tx_pin = tx_reg;

    always@(posedge clk or negedge rst_n)
    begin
        if(rst_n == 1'b0)
            state <= S_IDLE;
        else
            state <= next_state;
    end

    // count cycles of a bit
    always@(posedge clk or negedge rst_n)
    begin
        if(rst_n == 1'b0 || next_state != state || state == S_SEND_BYTE && cycle_cnt == CYCLES - 1)
            cycle_cnt <= 16'd0;
        else
            cycle_cnt <= cycle_cnt + 16'd1;
    end

    // count bits of a frame
    always@(posedge clk or negedge rst_n)
    begin
        if(rst_n == 1'b0 || state != S_SEND_BYTE)
            bit_cnt <= 3'd0;
        else if(cycle_cnt == CYCLES - 1)
            bit_cnt <= bit_cnt + 3'd1;
    end

    always@(*)
    begin
        case(state)
            S_IDLE:
                if(tx_data_valid == 1'b1)
                    next_state <= S_START;
                else
                    next_state <= state;
            S_START:
                if(cycle_cnt == CYCLES - 1)
                    next_state <= S_SEND_BYTE;
                else
                    next_state <= state;
            S_SEND_BYTE:
                if(cycle_cnt == CYCLES - 1 && bit_cnt == BITS - 1)
                    next_state <= S_STOP;
                else
                    next_state <= state;
            S_STOP:
                if(cycle_cnt == CYCLES - 1)
                    next_state <= S_IDLE;
                else
                    next_state <= state;
            default:
                next_state <= S_IDLE;
        endcase
    end

    always@(posedge clk or negedge rst_n)
    begin
        if(rst_n == 1'b0 || state == S_IDLE && tx_data_valid == 1'b1)
            tx_data_ready <= 1'b0;
        else if(state == S_STOP && cycle_cnt == CYCLES - 1 || state == S_IDLE && tx_data_valid == 1'b0)
            tx_data_ready <= 1'b1;
    end

    always@(posedge clk or negedge rst_n)
    begin
        if(rst_n == 1'b0)
            tx_data_latch <= 8'd0;
        else if(state == S_IDLE && tx_data_valid == 1'b1)
            tx_data_latch <= tx_data;
    end

    always@(posedge clk or negedge rst_n)
    begin
        if(rst_n == 1'b0)
            tx_reg <= 1'b1;
        else
        case(state)
            S_START:
                tx_reg <= 1'b0;
            S_SEND_BYTE:
                tx_reg <= tx_data_latch[bit_cnt];
            default:
                tx_reg <= 1'b1;
        endcase
    end
endmodule
