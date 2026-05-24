`ifndef HVM_PKG_VH
`define HVM_PKG_VH

localparam int HVM_XLEN = 64;
localparam int HVM_REG_COUNT = 32;
localparam int HVM_INSTR_BASE_BYTES = 4;
localparam int HVM_INSTR_EXT_BYTES = 8;

localparam logic [7:0] HVM_ESCAPE_PREFIX = 8'hFE;

`endif
