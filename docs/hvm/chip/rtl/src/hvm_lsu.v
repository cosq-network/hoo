module hvm_lsu (
    input  logic        clk,
    input  logic        rst_n,
    input  logic [63:0] addr,
    input  logic [63:0] wdata,
    input  logic [63:0] rdata,
    input  logic        valid,
    input  logic        ready,
    input  logic        we,
    input  logic [7:0]  be,
    output logic [63:0] load_data,
    output logic [63:0] mem_addr,
    output logic [63:0] mem_wdata,
    output logic        mem_valid,
    output logic        mem_we,
    output logic [7:0]  mem_be
);
    assign mem_addr = addr;
    assign mem_wdata = wdata;
    assign load_data = rdata;
    assign mem_valid = valid;
    assign mem_we = we;
    assign mem_be = be;
endmodule
