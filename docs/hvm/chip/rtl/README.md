# HVM RTL Tree

This directory mirrors the HVM chip design scaffold in a buildable RTL layout.

## Layout

- `src/`: synthesizable RTL modules
- `tb/`: simulation testbenches
- `Makefile`: minimal simulation entrypoint

## Source Files

- `src/hvm_pkg.vh`: shared constants and ISA sizing
- `src/hvm_core.v`: top-level core wrapper
- `src/hvm_pc_unit.v`: PC update and redirect logic
- `src/hvm_ifetch.v`: instruction fetch wrapper
- `src/hvm_decoder.v`: opcode and operand decode
- `src/hvm_regfile.v`: 32 x 64-bit register file
- `src/hvm_alu.v`: integer execution unit
- `src/hvm_fpu.v`: floating-point execution unit
- `src/hvm_lsu.v`: load/store unit
- `src/hvm_branch.v`: branch and call handling
- `src/hvm_trap.v`: traps, interrupts, and debug entry

## Simulation

The default target is a lightweight smoke test that checks:

- reset and clock wiring
- instruction fetch visibility
- trap output plumbing
- basic compile-time integration across all modules

Example:

```sh
make
make run
```

## Scope

This tree is intentionally minimal. It is a starter RTL structure, not a complete CPU implementation.

