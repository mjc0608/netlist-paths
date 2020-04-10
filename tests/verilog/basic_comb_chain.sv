module basic_assign_chain
  (
    input logic in,
    output logic out
  );
  logic a;
  logic b;
  // Simple chain of dependencies.
  always_comb begin
    a = in;
    b = a;
    out = b;
  end
endmodule

