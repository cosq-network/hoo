# HVM Mobile SoC and Board Specifications

This document defines the `HVM-M1` mobile SoC and its compact reference board. The profile targets smartphones, handheld gaming devices, tablets, wearables, and battery-powered edge nodes.

The design inherits the base HVM CPU profile of 2 big HVM-V-capable cores plus 4 little efficiency cores, HVM-C compressed opcodes, HVM-ARC non-trapping retain/release instructions, `ICACHE.RNG`, and an integrated low-power graphics and AI subsystem.

## 1. Industry Reference Envelope

The `HVM-M1` platform is sized against current premium mobile SoCs such as Qualcomm Snapdragon 8 Elite and Apple M-series mobile-class chips. Public Snapdragon 8 Elite specifications list an Oryon 64-bit CPU up to 4.47 GHz, Adreno graphics, Hexagon AI acceleration, X80 5G modem support, Wi-Fi 7, Vulkan 1.3, OpenGL ES 3.2, and OpenCL 3.0 FP. Apple M4 public specifications establish a useful integrated SoC baseline with a 10-core CPU option, 10-core GPU, 16-core Neural Engine, and second-generation 3 nm process target.

`HVM-M1` should not copy those architectures. It should match their platform class: compact package, high memory bandwidth per watt, integrated display/camera/media paths, aggressive sleep states, and always-on sensor processing.

## 2. SoC Summary

| Item | `HVM-M1` Specification |
| :--- | :--- |
| CPU topology | 2 big HVM cores + 4 little HVM cores |
| ISA extensions | HVM-C, HVM-ARC, `ICACHE.RNG`, HVM-V 128-bit `VLEN` |
| Process target | TSMC N4P or equivalent 4 nm-class FinFET |
| Package | BGA-1218 bottom package with optional LPDDR PoP top package |
| Memory | 8 GB-24 GB LPDDR5X, 128-bit aggregate bus |
| Storage | UFS 4.0, 2-lane, soldered NAND package |
| Display | 1x MIPI-DSI 4-lane, optional embedded DisplayPort bridge |
| Camera | 2x MIPI-CSI 4-lane ports |
| Wireless attachment | PCIe Gen 4 x1/x2 modem interface plus SDIO low-power radio option |
| TDP / sustained power | 3 W-8 W phone mode, 8 W-15 W handheld/tablet mode |
| Peak CPU clock | Big cores up to 3.6 GHz, little cores up to 2.2 GHz |

## 3. Compute Blocks

### 3.1 CPU Cluster

- Big cores implement out-of-order HVM execution, HVM-V 128-bit vector lanes, branch fusion, and full HVM-ARC atomic refcount operations.
- Little cores implement in-order HVM execution with HVM-C decompression and HVM-ARC support, but may omit high-throughput vector pipelines.
- Shared L3 cache is 8 MB-12 MB, tuned for mobile leakage rather than desktop bandwidth.
- Per-core L1 cache is 64 KB instruction plus 64 KB data. Per-core L2 is 512 KB on big cores and 256 KB on little cores.

### 3.2 AI, GPU, and Media

- Integrated GPU supports Vulkan 1.3, OpenGL ES 3.2, and basic compute shaders for desktop composition and handheld gaming.
- AI accelerator target is 20-40 TOPS INT8 equivalent, exposed through HVM-A doorbell queues and shared virtual memory.
- Media block supports H.265, AV1 decode, H.264 encode/decode, secure display path, and low-power camera ISP pre-processing.
- The AI/GPU/media complex must be independently power-gated from the CPU cluster.

## 4. Package and Memory Stacking

```
+---------------------------------------------+
| LPDDR5X PoP memory top package              |
+---------------------------------------------+
                    ||
+---------------------------------------------+
| HVM-M1 SoC bottom BGA package               |
+---------------------------------------------+
                    ||
+---------------------------------------------+
| 10-14 layer HDI mobile board                |
+---------------------------------------------+
```

| Package Element | Requirement |
| :--- | :--- |
| Bottom package | BGA-1218, 0.4 mm pitch, 15 mm x 15 mm target body |
| Top memory package | LPDDR5X PoP, 0.3 mm pitch |
| Underfill | Required for handheld shock and thermal cycling |
| Thermal interface | Graphite spreader plus vapor chamber option above 8 W sustained |
| Board technology | HDI stackup with blind/buried microvias |

## 5. Power and Clock Domains

| Rail | Voltage Range | Max Current | Notes |
| :--- | :---: | :---: | :--- |
| `VDD_CORE_BIG` | 0.60 V-1.05 V | 18 A | Big core DVFS island |
| `VDD_CORE_LITTLE` | 0.50 V-0.90 V | 8 A | Efficiency core DVFS island |
| `VDD_GPU_AI` | 0.65 V-0.95 V | 18 A | GPU, NPU, vector-adjacent accelerators |
| `VDD_SRAM` | 0.80 V-0.90 V | 6 A | Cache arrays, retention capable |
| `VDD_LPDDR` | per LPDDR5X vendor spec | 6 A | PoP memory rails |
| `VDD_IO_1V8` | 1.8 V | 2 A | MIPI, GPIO, low-speed controls |
| `VDD_AON` | 0.75 V-0.90 V | 150 mA | Always-on sensor island |

Low-power state requirements:

- Suspend-to-RAM entry below 2 ms.
- Wake from always-on sensor interrupt below 150 microseconds.
- Deep retention leakage below 150 microamps at board level excluding radios.
- PMIC must support per-island power sequencing and brownout reporting to the HVM supervisor.

## 6. Board Specification

| Feature | Requirement |
| :--- | :--- |
| Board class | 80 mm x 60 mm reference SoM or phone mainboard section |
| Stackup | 10-14 layers, HDI, impedance-controlled |
| Memory routing | PoP or shortest-path LPDDR5X, length matched by byte lane |
| Storage | 1x UFS 4.0 soldered package, optional microSD debug slot |
| Display | 1x MIPI-DSI 4-lane at 100 ohm differential |
| Cameras | 2x MIPI-CSI 4-lane at 100 ohm differential |
| USB | 1x USB 3.2 / USB4-capable Type-C controller |
| Debug | USB-C debug accessory mode, 1.8 V UART pads, JTAG test pads |
| Security | On-die secure boot ROM, TRNG, fuses, optional SPI TPM/HSM on dev board |

## 7. Firmware and Boot Flow

1. Mask ROM validates the first-stage bootloader using fused root keys.
2. FSBL initializes PMIC, LPDDR training, clocks, and secure monitor state.
3. OpenSBI-compatible supervisor firmware exposes HVM machine services.
4. U-Boot or a mobile boot chain loads the kernel from UFS.
5. Runtime advertises HVM-C, HVM-ARC, HVM-V, and HVM-A feature bits through device tree.

## 8. Layout Rules

- MIPI DSI/CSI differential impedance: 100 ohm +/- 10%.
- USB high-speed differential impedance: 90 ohm +/- 10%.
- RF keepout: 3.5 mm minimum from memory and MIPI routing.
- LPDDR routes must avoid layer transitions where possible; any unavoidable via must use backdrill or microvia construction.
- Place PMIC within 20 mm of the SoC power balls and use local high-frequency decoupling on every CPU/GPU rail.

## 9. Validation Checklist

- Boot ROM secure boot passes with production fuses and development fuses.
- LPDDR5X training passes across voltage, temperature, and process corners.
- Sustained 8 W workload maintains junction temperature below 95 C with handheld cooling.
- HVM-ARC retain/release operations pass atomic stress tests on all CPU clusters.
- HVM-V vector context switch lazy-save behavior passes suspend/resume and multitasking tests.
- MIPI display and camera links pass eye margin and EMI pre-scan.

## 10. References

- Qualcomm Snapdragon 8 Elite Mobile Platform: https://www.qualcomm.com/smartphones/products/8-series/snapdragon-8-elite-mobile-platform
- Apple M4 chip announcement: https://www.apple.com/newsroom/2024/05/apple-introduces-m4-chip/
- HVM CPU baseline: `docs/hvm/chip/hvm_cpu_specifications.md`
- HVM green-compute proposal: `docs/hvm/chip/hvm_green_compute_proposal.md`
