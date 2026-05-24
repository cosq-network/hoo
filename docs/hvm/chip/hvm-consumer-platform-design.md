# HVM Consumer Platform Design

Version: `1.0`

This document defines a consumer laptop/mobile platform built around the HVM microprocessor architecture.

The goal is a practical SoC-centric design that can serve:

- thin-and-light laptops
- tablet-class devices
- mobile handheld devices
- embedded consumer devices with richer OS support

## 1. Platform Principle

The processor is only one part of the system.

The platform must also provide:

- memory controller
- storage controller
- graphics and display pipeline
- audio subsystem
- camera and sensor support
- wireless connectivity
- power management
- security subsystem
- firmware/boot infrastructure

## 2. Recommended Product Shape

For consumer devices, the best implementation is a single SoC with a motherboard or mainboard carrying:

- the HVM processor cluster
- LPDDR memory
- storage devices
- power delivery
- RF modules
- display connectors
- USB and expansion interfaces

For mobile devices, the same design maps to a compact mainboard rather than a classical laptop motherboard.

## 3. SoC Block Overview

### 3.1 Compute

- 2 to 8 HVM CPU cores
- shared last-level cache
- coherent interconnect

### 3.2 Memory

- LPDDR4X or LPDDR5 controller
- optional ECC in higher-end variants
- memory encryption support optional but recommended

### 3.3 Graphics and Display

- integrated GPU or display controller
- display output pipeline
- hardware video decode and encode blocks

### 3.4 Storage and I/O

- PCIe
- NVMe
- USB 3.x / USB4-class controller
- SD/eMMC support for mobile variants
- SPI, I2C, I3C, UART, GPIO

### 3.5 Connectivity

- Wi-Fi
- Bluetooth
- optional cellular modem for mobile products
- secure wireless firmware update support

### 3.6 Audio and Camera

- audio codec interface
- microphone and speaker support
- camera ISP or camera bridge support

### 3.7 Security

- secure boot ROM
- hardware root of trust
- key storage
- debug authentication
- trusted execution environment optional

### 3.8 Power Management

- DVFS
- per-core power gating
- suspend-to-RAM
- deep idle states
- thermal monitoring

## 4. Laptop-Class Platform

Recommended features:

- 2 to 4 CPU performance cores
- optional efficiency cores
- NVMe storage
- USB-C with charging
- external display support
- keyboard, touchpad, and audio codec integration
- fan and thermal management

Recommended board features:

- one or two M.2 slots
- soldered LPDDR for compact designs
- battery charging controller
- firmware recovery mode

## 5. Mobile-Class Platform

Recommended features:

- low-power CPU cluster
- integrated modem support
- LPDDR memory
- UFS or eMMC storage
- display MIPI pipeline
- touchscreen and sensor buses
- aggressive power gating

Recommended board features:

- compact mainboard
- integrated PMIC
- secure boot fuse chain
- strong thermal throttling support

## 6. Motherboard or Mainboard Design

### 6.1 Core Layout

The board should separate:

- compute package
- memory devices
- power delivery
- high-speed I/O
- RF section
- debug and manufacturing interfaces

### 6.2 Connectivity Topology

Suggested high-level connectivity:

- CPU cluster connected to shared interconnect
- memory controller on the same die
- PCIe root complex to storage and expansion
- USB controller to peripherals
- display engine to panel outputs
- audio to codec

### 6.3 Board Constraints

- minimize signal trace lengths for memory and high-speed lanes
- keep RF and power sections isolated
- maintain thermal spread under sustained load
- ensure easy firmware recovery path

## 7. Power and Thermal Strategy

Consumer viability depends on power control.

The platform should support:

- idle power collapse
- DVFS scaling
- thermal throttling
- battery-aware scheduling
- sleep and wake latency controls

## 8. Firmware Stack

Recommended boot chain:

1. ROM boot
2. secure monitor
3. hardware init
4. memory training
5. device enumeration
6. kernel launch

Firmware should also handle:

- recovery mode
- firmware update rollback
- manufacturing test mode
- secure debug unlock

## 9. Software Stack Expectations

The platform should support:

- a modern OS kernel
- user-space drivers where appropriate
- compiler output from the HVM toolchain
- runtime support for threads, traps, and syscalls
- multimedia and graphics acceleration APIs

## 10. Product Tiers

### 10.1 Entry

- 2 cores
- integrated graphics
- LPDDR memory
- NVMe or eMMC

### 10.2 Mainstream

- 4 cores
- larger cache
- better media support
- Wi-Fi and Bluetooth integrated

### 10.3 Premium

- 8 cores or more
- stronger GPU/media blocks
- larger cache
- advanced power and security features

## 11. Manufacturing and Test

The platform should include:

- JTAG or secure debug interface
- boundary scan support
- board bring-up test points
- memory test mode
- thermal validation hooks

## 12. Practical Recommendation

For a first product, use a laptop-first SoC design and reuse the same compute package for mobile by scaling:

- core count
- cache size
- memory width
- modem integration
- battery and thermal envelope

This reduces verification cost while keeping the ISA and firmware stack common across product lines.

