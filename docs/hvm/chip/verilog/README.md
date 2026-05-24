# HVM Verilog Scaffold

This directory contains a starter Verilog file layout for an HVM processor implementation.

For a buildable RTL project layout with a smoke test and Makefile, see [`../rtl/README.md`](../rtl/README.md).

## Files

- `hvm_core.v`: top-level core wrapper
- `hvm_pkg.vh`: shared constants and parameters
- `hvm_pc_unit.v`: program counter and redirect logic
- `hvm_ifetch.v`: instruction fetch and alignment
- `hvm_decoder.v`: opcode and operand decode
- `hvm_regfile.v`: 32 x 64-bit register file
- `hvm_alu.v`: integer execution unit
- `hvm_fpu.v`: floating-point execution unit
- `hvm_lsu.v`: load/store and stack unit
- `hvm_branch.v`: branch, jump, call, and return handling
- `hvm_trap.v`: syscall, breakpoint, and trap handling

## Notes

- The stubs are intentionally minimal.
- The layout is meant to be synthesizable-friendly and easy to extend.
- The current HVM ISA contract assumes base32 and escape32 instruction handling.
