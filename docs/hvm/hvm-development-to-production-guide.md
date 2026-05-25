# HVM Development to Commercial Production Guide

Version: `1.0`

This document describes a practical path from first concept to commercial production for an HVM-based CPU, SoC, board, and product line.

It is written for a team building a real hardware product, not just a simulator or RTL prototype.

The guide covers:

- how to start development cleanly
- how to write professional design specifications
- how to simulate and verify the design
- how to work with hardware manufacturers
- how to assemble and test the product
- how to distribute and support it commercially

The HVM-specific requirement is simple:

- the product must remain compatible with the HVM ISA
- the product must remain compatible with the shared JIT/runtime contract
- the product must remain testable in simulation before it is manufactured

## 1. Product Strategy

Start by deciding what you are actually building.

For HVM, there are three realistic commercial targets:

1. **Development platform**  
   A board or system used to prove the CPU, firmware, and software stack.

2. **Reference product**  
   A stable laptop-class or mobile-class platform used for validation and early customers.

3. **Commercial product line**  
   A manufacturable system with a controlled supply chain, support process, and distribution path.

Do not start by trying to build all three at once.

## 2. Recommended Project Stages

Use a staged flow:

1. requirements and architecture
2. ISA and system specification
3. RTL or simulator implementation
4. early verification and co-simulation
5. FPGA or emulation prototype
6. board design and bring-up
7. EVT, DVT, and PVT
8. manufacturing release
9. distribution and support

Each stage should have a clear exit criterion. If the exit criterion is not met, do not move to the next stage.

## 3. How to Write Professional Design Specs

Professional hardware projects fail when the specs are vague.

### 3.1 Required Spec Documents

For an HVM commercial program, create these documents:

- Product Requirements Document
- System Architecture Specification
- ISA Specification
- SoC Integration Specification
- Board Specification
- Firmware Specification
- Verification Plan
- Manufacturing Test Specification
- Power and Thermal Specification
- Security Specification
- Compliance Matrix
- Change Control Policy

### 3.2 Product Requirements Document

The PRD should define:

- target markets
- product categories
- performance goals
- power targets
- thermal envelope
- memory and storage targets
- I/O requirements
- pricing band
- support expectations
- launch constraints

Example PRD items:

- 64-bit HVM CPU
- laptop and mobile board variants
- bootable firmware
- serial debug support
- secure boot path
- commercial manufacturability

### 3.3 System Architecture Specification

This document should define the system at a high level:

- CPU cluster
- memory controller
- cache hierarchy
- interconnect
- interrupt controller
- timers
- debug block
- storage controller
- USB
- display
- audio
- sensors
- power management

It must specify:

- clock domains
- reset domains
- privilege model
- MMIO map
- interrupt map
- boot flow
- firmware responsibilities

### 3.4 ISA Specification

The ISA spec must be normative.

It should define:

- opcode encoding
- instruction lengths
- operand formats
- branch and call semantics
- trap behavior
- memory ordering
- atomic operations
- debug instructions
- system register access
- trap return behavior

For HVM, this spec must stay compatible with the shared JIT profile.

### 3.5 Board Specification

The board spec should define:

- board form factor
- RAM type and topology
- storage type
- display interface
- power delivery
- battery charging
- thermal design
- debug connectors
- manufacturing test access

### 3.6 Verification Plan

The verification plan should include:

- unit tests
- ISA compliance tests
- RTL simulation
- firmware boot tests
- board-level tests
- stress tests
- regression tests
- JIT-vs-hardware comparison tests

### 3.7 Change Control Policy

You need strict change control from the beginning.

Every change should have:

- owner
- rationale
- impact analysis
- test evidence
- approval status

Do not let hardware, firmware, and software drift independently.

## 4. Development Environment

Use a disciplined development setup:

- version control with branch discipline
- issue tracker
- CI for RTL and firmware
- reproducible builds
- artifact storage
- test image storage
- documented tool versions

Typical workspaces:

- one source tree for RTL and firmware
- one tree for simulator/QEMU or JIT work
- one tree for board support and software
- one build tree for FPGA or ASIC prototypes

## 5. Simulation and Verification

Simulation should be the first gate, not the last.

### 5.1 Simulation Layers

Use multiple layers of simulation:

1. **Instruction-level or JIT-level model**  
   Fast functional correctness.

2. **RTL simulation**  
   Timing-neutral correctness of the actual design.

3. **System simulation**  
   CPU, memory, interrupt, and device interactions.

4. **Firmware simulation**  
   Boot, handoff, and runtime initialization.

5. **Board simulation**  
   Device maps, interrupts, storage, input, display, and power behavior.

### 5.2 What to Simulate

At minimum simulate:

- CPU register state
- instruction decode
- branch and call behavior
- trap and interrupt entry
- memory and MMIO access
- timer behavior
- debug behavior
- boot ROM execution
- firmware handoff
- runtime hooks

### 5.3 HVM-Specific Simulation Targets

For HVM, compare these behaviors between models:

- instruction size and decode
- base32 vs escape32 behavior
- branch offset handling
- trap return state
- CSR/system register accesses
- atomic semantics
- `BREAK` and debug stop behavior
- JIT runtime hooks

### 5.4 Regression Strategy

Use a layered regression suite:

- unit tests for each instruction family
- instruction stream tests
- boot tests
- trap tests
- MMU tests
- board bring-up tests
- JIT parity tests

## 6. Prototype Strategy

Before a commercial chip, you need a proof point.

### 6.1 FPGA or Emulation Prototype

Use either:

- FPGA-based prototype
- emulator/simulator prototype
- both, if the schedule allows

The prototype should prove:

- the CPU boots
- the firmware runs
- the board layout is sane
- the toolchain works
- the software stack can execute

### 6.2 What the Prototype Should Not Do

Do not try to prove manufacturing yield or final thermal behavior at this stage.

Prototype goals are:

- functional correctness
- interface correctness
- software bootability
- debug visibility

## 7. Working With Hardware Manufacturers

To make a commercial HVM product, you will typically work with multiple vendors.

### 7.1 Vendor Categories

You may need:

- IP vendors
- foundry
- packaging partner
- PCB fab
- OSAT
- EMS / assembly partner
- test equipment vendor
- firmware or board-design consultants

### 7.2 Manufacturer Engagement Flow

Typical flow:

1. NDA
2. requirements review
3. RFQ or proposal request
4. technical alignment
5. DFM/DFT review
6. sample/prototype order
7. pilot run
8. production run

### 7.3 What to Give Manufacturers

Provide a complete data package:

- product brief
- system block diagram
- electrical interface spec
- power budget
- thermal budget
- package requirements
- BOM targets
- compliance targets
- test requirements
- schedule targets

For silicon partners, include:

- RTL or netlist deliverables
- verification summary
- timing targets
- floorplan assumptions
- clock/reset plan
- scan/DFT plan
- memory interface requirements
- package and pinout constraints

### 7.4 How to Work With Them

Treat manufacturers as engineering partners, not as a black box.

You should maintain:

- weekly technical reviews
- issue tracking
- change requests
- sample acceptance criteria
- test result review
- revision control

Never assume a vendor understood an ambiguous spec. Get the requirement into writing.

## 8. DFM, DFT, and Testability

Commercial production depends on being able to build and test the product at scale.

### 8.1 Design for Manufacturing

Include:

- component availability
- PCB routing feasibility
- power integrity
- signal integrity
- thermal feasibility
- package availability
- assembly tolerance

### 8.2 Design for Test

Include:

- boundary scan / JTAG
- manufacturing test mode
- memory test mode
- UART or serial recovery path
- device identification
- firmware recovery mode
- board-level self-test

### 8.3 Test Coverage Goals

Test:

- power rails
- reset behavior
- clocks
- boot ROM
- RAM training or bring-up
- storage detection
- USB enumeration
- display bring-up
- debug access

## 9. Board Bring-Up

Board bring-up is where most hidden problems appear.

### 9.1 Bring-Up Order

Recommended order:

1. verify power rails
2. verify clocks and reset
3. verify UART output
4. verify boot ROM
5. verify RAM
6. verify storage
7. verify interrupt routing
8. verify timer interrupts
9. verify debug access
10. verify firmware handoff

### 9.2 Bring-Up Artifacts

Keep:

- scope traces
- power measurements
- boot logs
- firmware logs
- JTAG logs
- versioned board notes

### 9.3 Bring-Up Rule

If the board cannot print a stable boot log, do not proceed to higher-level software work.

## 10. EVT, DVT, and PVT

Use standard product validation phases.

### 10.1 EVT

Engineering Validation Test:

- prove function
- prove basic assembly
- prove the board boots
- prove the product is close to the intended design

### 10.2 DVT

Design Validation Test:

- prove the design meets spec
- prove thermal behavior
- prove reliability
- prove firmware stability
- prove major peripherals

### 10.3 PVT

Production Validation Test:

- prove the line can build at scale
- prove yield
- prove test time
- prove packaging and logistics
- prove QA procedures

## 11. Commercial Production Flow

### 11.1 Pre-Production

Before mass production:

- freeze the design
- freeze the BOM or approved alternates
- freeze firmware release candidates
- freeze the factory test plan
- freeze the packaging specification

### 11.2 Pilot Run

Use a pilot run to validate:

- assembly quality
- firmware programming
- test throughput
- packaging
- defect rates
- rework flow

### 11.3 Mass Production

Mass production requires:

- approved supplier list
- inventory planning
- serialized tracking
- factory acceptance tests
- shipment QA
- return handling

## 12. Assembly and Factory Test

### 12.1 Assembly Flow

The EMS partner usually handles:

- PCB assembly
- solder paste and placement
- reflow
- inspection
- board-level test
- programming
- final assembly

### 12.2 Factory Test Requirements

Factory tests should verify:

- board identity
- CPU response
- memory detect
- UART output
- storage presence
- power sequencing
- thermal sensors
- debug port access

### 12.3 Programming and Provisioning

The manufacturing line should support:

- firmware flashing
- secure key provisioning if applicable
- serial number programming
- calibration data injection
- test result recording

## 13. Packaging and Productization

Commercial products need more than working hardware.

### 13.1 Product Assets

Prepare:

- packaging artwork
- regulatory labels
- serial number scheme
- user documentation
- quick-start guide
- recovery instructions
- warranty terms

### 13.2 Logistics

Plan:

- inbound component logistics
- factory inventory
- outbound product logistics
- spare parts
- RMAs and replacements

## 14. Distribution Strategy

### 14.1 Sales Channels

Possible channels:

- direct online sales
- enterprise sales
- distributor sales
- partner sales
- regional reseller channels

### 14.2 Distribution Readiness

Before launch, define:

- product SKU structure
- regional compliance
- localization requirements
- support policy
- firmware update policy
- end-of-life policy

### 14.3 Support and RMA

You need:

- customer support channel
- bug reporting path
- return material authorization flow
- board replacement strategy
- firmware update mechanism
- security update policy

## 15. Compliance and Certification

Commercial hardware usually needs regulatory work.

Typical areas:

- EMC / EMI
- electrical safety
- radio certification if wireless is included
- battery safety if batteries are included
- environmental compliance
- import/export constraints

You should engage a compliance lab early enough that late changes do not invalidate the design.

## 16. Security and Update Policy

Commercial hardware needs a security posture from day one.

### 16.1 Required Security Planning

- secure boot
- debug lockout
- signed firmware updates
- rollback strategy
- key management
- vulnerability handling process

### 16.2 Update Policy

Define:

- who can sign firmware
- how updates are distributed
- how rollback is prevented or controlled
- how critical security fixes are prioritized

## 17. HVM-Specific Commercial Rules

For HVM, the commercial hardware, the simulator, and the JIT must stay aligned.

### 17.1 Shared Contract

- instruction encoding must match
- trap and interrupt behavior must match
- memory semantics must match
- ABI semantics must match
- runtime hook semantics must match

### 17.2 Validation Rule

Every hardware revision should be validated against:

- ISA tests
- firmware boot tests
- QEMU system simulation
- HVM JIT execution
- board-level regression tests

If the simulator and JIT diverge, fix the contract before shipping hardware.

## 18. Team Structure

You usually need separate owners for:

- architecture
- RTL or silicon implementation
- firmware
- board design
- verification
- manufacturing
- supply chain
- mechanical design
- compliance
- product support

The work is too broad for a single undisciplined group.

## 19. Practical Deliverable Checklist

Before a commercial launch, ensure you have:

- architecture specification
- ISA specification
- board specification
- verification plan
- simulation regressions
- firmware images
- bring-up logs
- manufacturing test flow
- compliance evidence
- packaging and support materials
- update and security policy

## 20. Recommended Starting Point

For HVM, the best starting commercial path is:

1. finish the CPU and ISA specification
2. prove the design in JIT and system simulation
3. bring up a dev board or FPGA prototype
4. build a minimal laptop-class board
5. validate firmware and OS boot
6. only then move toward production

That sequence reduces risk and keeps the design aligned with the software contract.

## 21. Final Rule

Do not move into manufacturing until:

- the simulator is stable
- the JIT agrees with the ISA
- the board boots repeatedly
- the test plan is complete
- the factory test flow exists
- the support model exists

Commercial production is a systems problem, not just a hardware problem.

