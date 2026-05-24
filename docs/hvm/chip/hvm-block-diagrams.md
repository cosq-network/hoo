# HVM Block Diagrams

Version: `1.0`

This document provides simple block diagrams for the HVM microprocessor and the consumer SoC that surrounds it.

## 1. Microprocessor Block Diagram

```mermaid
flowchart TB
    PC[PC Unit]
    IF[Instruction Fetch]
    ID[Decoder]
    RF[Register File]
    ALU[Integer ALU]
    FPU[FPU]
    LSU[Load/Store Unit]
    BR[Branch Unit]
    TRAP[Trap / Syscall Unit]
    MMU[MMU / TLB Wrapper]
    CTL[Control State Machine]
    IC[L1 Instruction Cache]
    DC[L1 Data Cache]

    PC --> IF --> ID
    ID --> RF
    ID --> ALU
    ID --> FPU
    ID --> LSU
    ID --> BR
    ID --> TRAP
    ID --> CTL
    IF --> IC
    LSU --> DC
    MMU --> IF
    MMU --> LSU
    BR --> PC
    TRAP --> PC
    CTL --> PC
    CTL --> RF
    CTL --> ALU
    CTL --> FPU
    CTL --> LSU
```

### Notes

- The processor is intentionally load/store based.
- Branch and call control flow is explicit.
- The MMU wrapper is optional for the first RTL revision, but the interface should be planned early.
- Caches can be added around the same core without changing the ISA contract.

## 2. Consumer SoC Block Diagram

```mermaid
flowchart LR
    CPU[HVM CPU Cluster]
    LLC[Shared L2 / LLC]
    MEM[LPDDR Controller]
    GPU[Graphics / Display]
    MEDIA[Video Encode / Decode]
    IO[PCIe / USB / SD / UART / I2C / GPIO]
    NET[Wi-Fi / BT / Modem]
    AUDIO[Audio Subsystem]
    CAM[Camera / Sensor Block]
    PWR[Power Management / PMIC]
    SEC[Secure Boot / Root of Trust]
    FW[Firmware / Boot ROM]
    DEBUG[Debug / JTAG / Trace]

    CPU <---> LLC
    LLC <---> MEM
    LLC <---> GPU
    LLC <---> MEDIA
    LLC <---> IO
    LLC <---> NET
    LLC <---> AUDIO
    LLC <---> CAM
    PWR --> CPU
    PWR --> MEM
    PWR --> GPU
    PWR --> IO
    SEC --> FW
    DEBUG --> CPU
    DEBUG --> SEC
```

### Notes

- The SoC is organized around a shared interconnect and a coherent memory hierarchy.
- Laptop and mobile variants share the same core compute design.
- Peripheral blocks scale by product tier rather than by ISA changes.

## 3. Design Principle Summary

- Keep the core small.
- Keep the interconnect simple.
- Keep the firmware path explicit.
- Keep the ISA stable while scaling product features around it.

