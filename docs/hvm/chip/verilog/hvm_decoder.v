module hvm_decoder (
    input  logic        clk,
    input  logic        rst_n,
    input  logic [63:0] instr_word,
    input  logic        decoded_valid,
    output logic [31:0] opcode,
    output logic [2:0]  format,
    output logic        is_extended,
    output logic [4:0]  rd,
    output logic [4:0]  rs1,
    output logic [4:0]  rs2,
    output logic [15:0] imm15,
    output logic [19:0] imm20,
    output logic [9:0]  func
);
    always_comb begin
        opcode = 32'd0;
        format = 3'd0;
        is_extended = 1'b0;
        rd = 5'd0;
        rs1 = 5'd0;
        rs2 = 5'd0;
        imm15 = 16'd0;
        imm20 = 20'd0;
        func = 10'd0;
        if (decoded_valid) begin
            opcode = instr_word[31:0];
            format = instr_word[63:61];
        end
    end
endmodule
