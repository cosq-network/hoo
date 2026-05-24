`timescale 1ns/1ps

module tb_hvm_core;
    logic clk;
    logic rst_n;

    logic [63:0] instr_addr;
    logic [63:0] instr_data;
    logic        instr_valid;
    logic        instr_ready;

    logic [63:0] mem_addr;
    logic [63:0] mem_wdata;
    logic [63:0] mem_rdata;
    logic        mem_valid;
    logic        mem_ready;
    logic        mem_we;
    logic [7:0]  mem_be;

    logic irq;
    logic nmi;
    logic dbg_break;
    logic trap_valid;
    logic [31:0] trap_cause;
    logic [63:0] trap_pc;

    hvm_core dut (
        .clk(clk),
        .rst_n(rst_n),
        .instr_addr(instr_addr),
        .instr_data(instr_data),
        .instr_valid(instr_valid),
        .instr_ready(instr_ready),
        .mem_addr(mem_addr),
        .mem_wdata(mem_wdata),
        .mem_rdata(mem_rdata),
        .mem_valid(mem_valid),
        .mem_ready(mem_ready),
        .mem_we(mem_we),
        .mem_be(mem_be),
        .irq(irq),
        .nmi(nmi),
        .dbg_break(dbg_break),
        .trap_valid(trap_valid),
        .trap_cause(trap_cause),
        .trap_pc(trap_pc)
    );

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    initial begin
        rst_n = 1'b0;
        instr_data = 64'h0000_0000_0000_0000;
        instr_valid = 1'b1;
        mem_rdata = 64'h0000_0000_0000_0000;
        mem_ready = 1'b1;
        irq = 1'b0;
        nmi = 1'b0;
        dbg_break = 1'b0;

        repeat (2) @(posedge clk);
        rst_n = 1'b1;

        repeat (4) @(posedge clk);

        irq = 1'b1;
        @(posedge clk);
        irq = 1'b0;

        repeat (2) @(posedge clk);

        $finish;
    end

    always @(posedge clk) begin
        if (rst_n) begin
            if (instr_ready !== instr_valid) begin
                $fatal(1, "instr_ready must mirror instr_valid in the smoke test");
            end

            if (trap_valid !== (irq || nmi || dbg_break)) begin
                $fatal(1, "trap_valid does not match trap inputs");
            end

            if (mem_ready !== 1'b1) begin
                $fatal(1, "mem_ready should stay asserted in the smoke test");
            end
        end
    end
endmodule

