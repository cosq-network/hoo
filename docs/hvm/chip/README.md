# HVM Chip Design Docs

This directory contains the design notes for a new HVM-based processor stack.

## Documents

- [HVM Microprocessor Architecture](./hvm-microprocessor-architecture.md)
- [HVM Consumer Platform Design](./hvm-consumer-platform-design.md)
- [HVM Verilog Implementation Guide](./hvm-verilog-implementation-guide.md)
- [HVM Block Diagrams](./hvm-block-diagrams.md)
- [Verilog Scaffold](./verilog/README.md)
- [RTL Tree](./rtl/README.md)

## Scope

- 64-bit RISC-style HVM processor inspired by modern ARM64-class systems
- Consumer laptop and mobile platform integration around the processor
- Minimal, implementation-oriented Verilog plan with a clear module breakdown
- Minimal RTL project layout with a smoke-testbench and Makefile

## Canonical References

- `docs/hvm/hvm-spec.md`
- `docs/hvm/instructions.md`
- `docs/hvm/hvm_instruction_set.csv`
- `docs/hvm/hvm_register_set.csv`
