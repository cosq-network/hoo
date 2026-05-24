module hvm_ifetch (
    input  logic        clk,
    input  logic        rst_n,
    input  logic [63:0] pc,
    input  logic [63:0] instr_data,
    input  logic        instr_valid,
    output logic        instr_ready,
    output logic [63:0] instr_addr,
    output logic        decoded_valid,
    output logic [63:0] instr_word
);
    assign instr_addr = pc;
    assign instr_ready = instr_valid;
    assign decoded_valid = instr_valid;
    assign instr_word = instr_data;
endmodule
