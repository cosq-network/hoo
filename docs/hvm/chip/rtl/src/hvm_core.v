`include "hvm_pkg.vh"

module hvm_core (
    input  logic              clk,
    input  logic              rst_n,

    output logic [63:0]       instr_addr,
    input  logic [63:0]       instr_data,
    input  logic              instr_valid,
    output logic              instr_ready,

    output logic [63:0]       mem_addr,
    output logic [63:0]       mem_wdata,
    input  logic [63:0]       mem_rdata,
    output logic              mem_valid,
    input  logic              mem_ready,
    output logic              mem_we,
    output logic [7:0]        mem_be,

    input  logic              irq,
    input  logic              nmi,
    input  logic              dbg_break,
    output logic              trap_valid,
    output logic [31:0]       trap_cause,
    output logic [63:0]       trap_pc
);
    logic [63:0] pc_next;
    logic [63:0] pc_current;
    logic [63:0] fetched_instr;
    logic [31:0] decoded_opcode;
    logic [2:0]  decoded_format;
    logic        decoded_valid;
    logic        decoded_extended;
    logic [4:0]  decoded_rd;
    logic [4:0]  decoded_rs1;
    logic [4:0]  decoded_rs2;
    logic [15:0] decoded_imm15;
    logic [19:0] decoded_imm20;
    logic [9:0]  decoded_func;
    logic [63:0] reg_rs1;
    logic [63:0] reg_rs2;
    logic [63:0] reg_wdata;
    logic [4:0]  reg_wa;
    logic        reg_we;
    logic [63:0] alu_y;
    logic [63:0] fpu_y;
    logic [63:0] lsu_rdata;
    logic [63:0] branch_target;
    logic        branch_redirect_valid;
    logic [63:0] branch_link_value;
    logic [4:0]  branch_link_reg;
    logic        branch_write_enable;
    logic        trap_redirect_valid;
    logic [63:0] trap_redirect_pc;

    always_comb begin
        reg_wa = decoded_rd;
        reg_wdata = alu_y;
        reg_we = decoded_valid;

        if (branch_write_enable) begin
            reg_wa = branch_link_reg;
            reg_wdata = branch_link_value;
            reg_we = 1'b1;
        end
    end

    hvm_pc_unit u_pc (
        .clk(clk),
        .rst_n(rst_n),
        .redirect_valid(branch_redirect_valid || trap_redirect_valid),
        .redirect_pc(trap_redirect_valid ? trap_redirect_pc : branch_target),
        .pc_current(pc_current),
        .pc_next(pc_next)
    );

    hvm_ifetch u_ifetch (
        .clk(clk),
        .rst_n(rst_n),
        .pc(pc_current),
        .instr_data(instr_data),
        .instr_valid(instr_valid),
        .instr_ready(instr_ready),
        .instr_addr(instr_addr),
        .decoded_valid(decoded_valid),
        .instr_word(fetched_instr)
    );

    hvm_decoder u_decoder (
        .clk(clk),
        .rst_n(rst_n),
        .instr_word(fetched_instr),
        .decoded_valid(decoded_valid),
        .opcode(decoded_opcode),
        .format(decoded_format),
        .is_extended(decoded_extended),
        .rd(decoded_rd),
        .rs1(decoded_rs1),
        .rs2(decoded_rs2),
        .imm15(decoded_imm15),
        .imm20(decoded_imm20),
        .func(decoded_func)
    );

    hvm_regfile u_regfile (
        .clk(clk),
        .rst_n(rst_n),
        .ra1(decoded_rs1),
        .ra2(decoded_rs2),
        .wa(reg_wa),
        .wd(reg_wdata),
        .we(reg_we),
        .rd1(reg_rs1),
        .rd2(reg_rs2)
    );

    hvm_alu u_alu (
        .a(reg_rs1),
        .b(reg_rs2),
        .op(decoded_opcode[3:0]),
        .y(alu_y)
    );

    hvm_fpu u_fpu (
        .a(reg_rs1),
        .b(reg_rs2),
        .op(decoded_opcode[2:0]),
        .y(fpu_y)
    );

    hvm_lsu u_lsu (
        .clk(clk),
        .rst_n(rst_n),
        .addr(branch_target),
        .wdata(reg_rs2),
        .rdata(mem_rdata),
        .valid(decoded_valid),
        .ready(mem_ready),
        .we(1'b0),
        .be(8'hFF),
        .load_data(lsu_rdata),
        .mem_addr(mem_addr),
        .mem_wdata(mem_wdata),
        .mem_valid(mem_valid),
        .mem_we(mem_we),
        .mem_be(mem_be)
    );

    hvm_branch u_branch (
        .clk(clk),
        .rst_n(rst_n),
        .pc_current(pc_current),
        .opcode(decoded_opcode),
        .rs1(reg_rs1),
        .rs2(reg_rs2),
        .imm15(decoded_imm15),
        .imm20(decoded_imm20),
        .func(decoded_func),
        .redirect_valid(branch_redirect_valid),
        .branch_target(branch_target),
        .link_value(branch_link_value),
        .link_reg(branch_link_reg),
        .write_enable(branch_write_enable)
    );

    hvm_trap u_trap (
        .clk(clk),
        .rst_n(rst_n),
        .irq(irq),
        .nmi(nmi),
        .dbg_break(dbg_break),
        .pc(pc_current),
        .trap_valid(trap_valid),
        .trap_cause(trap_cause),
        .trap_pc(trap_pc),
        .redirect_valid(trap_redirect_valid),
        .redirect_pc(trap_redirect_pc)
    );

endmodule
