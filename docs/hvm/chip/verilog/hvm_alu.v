module hvm_alu (
    input  logic [63:0] a,
    input  logic [63:0] b,
    input  logic [3:0]  op,
    output logic [63:0] y
);
    always_comb begin
        y = 64'd0;
        case (op)
            4'd0: y = a + b;
            4'd1: y = a - b;
            4'd2: y = a & b;
            4'd3: y = a | b;
            4'd4: y = a ^ b;
            default: y = 64'd0;
        endcase
    end
endmodule

