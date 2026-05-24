module hvm_regfile (
    input  logic        clk,
    input  logic        rst_n,
    input  logic [4:0]  ra1,
    input  logic [4:0]  ra2,
    input  logic [4:0]  wa,
    input  logic [63:0] wd,
    input  logic        we,
    output logic [63:0] rd1,
    output logic [63:0] rd2
);
    logic [63:0] regs [0:31];

    integer i;
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            for (i = 0; i < 32; i++) begin
                regs[i] <= 64'd0;
            end
        end else begin
            regs[0] <= 64'd0;
            if (we && wa != 5'd0) begin
                regs[wa] <= wd;
            end
        end
    end

    assign rd1 = (ra1 == 5'd0) ? 64'd0 : regs[ra1];
    assign rd2 = (ra2 == 5'd0) ? 64'd0 : regs[ra2];
endmodule
