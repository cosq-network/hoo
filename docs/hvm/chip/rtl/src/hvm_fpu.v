module hvm_fpu (
    input  logic [63:0] a,
    input  logic [63:0] b,
    input  logic [2:0]  op,
    output logic [63:0] y
);
    always_comb begin
        y = 64'd0;
    end
endmodule

