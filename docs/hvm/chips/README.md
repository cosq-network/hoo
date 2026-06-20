# HVM Chip and Board Specifications

This directory contains platform-specific HVM silicon and board specification manuals derived from:

- `docs/hvm/chip/hvm_cpu_specifications.md`
- `docs/hvm/chip/hvm_green_compute_proposal.md`

The documents split the HVM hardware family into deployable reference platforms:

- [HVM Mobile SoC and Board Specifications](./hvm_mobile_soc_board_specifications.md)
- [HVM Desktop Motherboard Specifications](./hvm_desktop_motherboard_specifications.md)
- [HVM Server Motherboard Specifications](./hvm_server_motherboard_specifications.md)
- [HVM Robotics SoC and Board Specifications](./hvm_robotics_soc_board_specifications.md)

## Common HVM Platform Assumptions

All platforms use the HVM 64-bit RISC execution model, HVM-C compressed instruction support, HVM-ARC retain/release acceleration, fine-grained `ICACHE.RNG` invalidation, and the HVM-V vector extension where the thermal envelope allows it.

| Profile | Primary Form | CPU Package | Memory Class | Target Power | Comparable Industry Class |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `HVM-M1` | Mobile SoC + compact board | BGA / PoP | LPDDR5X | 3 W-15 W | Qualcomm Snapdragon 8 Elite, Apple M-series mobile-class SoCs |
| `HVM-D1` | Desktop motherboard | LGA socket | DDR5 UDIMM | 65 W-170 W | Intel Core Ultra desktop, AMD Ryzen 9000 desktop platforms |
| `HVM-S1` | Server motherboard | LGA / dual socket | DDR5 ECC RDIMM | 250 W-800 W | AMD EPYC 9005 and enterprise SP5 server boards |
| `HVM-R1` | Robotics SoM + carrier board | Rugged BGA / LQFP control island | LPDDR5X + ECC SRAM | 8 W-130 W | NVIDIA Jetson AGX Thor / Orin robotics modules |

## Industry Baseline References

These manuals use current public product specifications as reference points for realistic board I/O, power, memory, and mechanical envelopes:

- Qualcomm Snapdragon 8 Elite Mobile Platform: https://www.qualcomm.com/smartphones/products/8-series/snapdragon-8-elite-mobile-platform
- Apple M4 chip announcement: https://www.apple.com/newsroom/2024/05/apple-introduces-m4-chip/
- AMD Ryzen 9 9950X specifications: https://www.amd.com/en/products/processors/desktops/ryzen/9000-series/amd-ryzen-9-9950x.html
- Intel Core Ultra 9 Processor 285K specifications: https://www.intel.com/content/www/us/en/products/sku/241060/intel-core-ultra-9-processor-285k-36m-cache-up-to-5-70-ghz/specifications.html
- AMD EPYC 9005 Series processors: https://www.amd.com/en/products/processors/server/epyc/9005-series.html
- ASUS Pro WS WRX90E-SAGE SE motherboard: https://www.asus.com/motherboards-components/motherboards/workstation/pro-ws-wrx90e-sage-se/techspec/
- Supermicro H13SSL-NT server motherboard: https://www.supermicro.com/en/products/motherboard/h13ssl-nt
- NVIDIA Jetson Thor robotics platform: https://www.nvidia.com/en-us/autonomous-machines/embedded-systems/jetson-thor/
