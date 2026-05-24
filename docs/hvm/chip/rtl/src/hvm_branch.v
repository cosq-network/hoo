module hvm_branch (
    input  logic        clk,
    input  logic        rst_n,
    input  logic [63:0] pc_current,
    input  logic [31:0] opcode,
    input  logic [63:0] rs1,
    input  logic [63:0] rs2,
    input  logic [15:0] imm15,
    input  logic [19:0] imm20,
    input  logic [9:0]  func,
    output logic        redirect_valid,
    output logic [63:0] branch_target,
    output logic [63:0] link_value,
    output logic [4:0]  link_reg,
    output logic        write_enable
);
    assign redirect_valid = 1'b0;
    assign branch_target = pc_current + 64'd4;
    assign link_value = pc_current + 64'd4;
    assign link_reg = 5'd29;
    assign write_enable = 1'b0;
endmodule
