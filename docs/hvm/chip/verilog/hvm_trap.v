module hvm_trap (
    input  logic        clk,
    input  logic        rst_n,
    input  logic        irq,
    input  logic        nmi,
    input  logic        dbg_break,
    input  logic [63:0] pc,
    output logic        trap_valid,
    output logic [31:0] trap_cause,
    output logic [63:0] trap_pc,
    output logic        redirect_valid,
    output logic [63:0] redirect_pc
);
    assign trap_valid = irq | nmi | dbg_break;
    assign trap_cause = irq ? 32'd1 : (nmi ? 32'd2 : (dbg_break ? 32'd3 : 32'd0));
    assign trap_pc = pc;
    assign redirect_valid = trap_valid;
    assign redirect_pc = 64'h0000_0000_0000_1000;
endmodule
