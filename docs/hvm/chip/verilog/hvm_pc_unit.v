module hvm_pc_unit (
    input  logic        clk,
    input  logic        rst_n,
    input  logic        redirect_valid,
    input  logic [63:0] redirect_pc,
    output logic [63:0] pc_current,
    output logic [63:0] pc_next
);
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            pc_current <= 64'd0;
        end else if (redirect_valid) begin
            pc_current <= redirect_pc;
        end else begin
            pc_current <= pc_next;
        end
    end

    assign pc_next = pc_current + 64'd4;
endmodule

