# Hardware Schematic: USB-C Smart Reading Lamp (v2)

This directory contains the complete KiCad 10 schematic project for the **USB-C Smart Reading Lamp**, representing everything implemented and tested up to **Step 5** of the development plan.

---

## 1. Project Files

| File | Description |
| :--- | :--- |
| [`smart_lamp.kicad_pro`](file:///c:/DRIVE%20E/MY%20CHORES/DIY%20BED%20LAMP/hardware/smart_lamp.kicad_pro) | KiCad project file (open this in KiCad 10 to launch the project) |
| [`smart_lamp.kicad_sch`](file:///c:/DRIVE%20E/MY%20CHORES/DIY%20BED%20LAMP/hardware/smart_lamp.kicad_sch) | KiCad schematic file |
| [`smart_lamp.kicad_sym`](file:///c:/DRIVE%20E/MY%20CHORES/DIY%20BED%20LAMP/hardware/smart_lamp.kicad_sym) | Local project symbol library (`ESP32-C3-SuperMini`, `TTP223_Module`, `IRLZ44N`) |
| [`sym-lib-table`](file:///c:/DRIVE%20E/MY%20CHORES/DIY%20BED%20LAMP/hardware/sym-lib-table) | Symbol library table mapping `${KIPRJMOD}/smart_lamp.kicad_sym` |
| [`smart_lamp.pdf`](file:///c:/DRIVE%20E/MY%20CHORES/DIY%20BED%20LAMP/hardware/smart_lamp.pdf) | Ready-to-print vector PDF export of the full schematic |
| [`smart_lamp.svg`](file:///c:/DRIVE%20E/MY%20CHORES/DIY%20BED%20LAMP/hardware/smart_lamp.svg) | Scalable Vector Graphic (SVG) for web or document embedding |
| [`smart_lamp_bom.csv`](file:///c:/DRIVE%20E/MY%20CHORES/DIY%20BED%20LAMP/hardware/smart_lamp_bom.csv) | Bill of Materials (BOM) export |
| [`smart_lamp_erc.rpt`](file:///c:/DRIVE%20E/MY%20CHORES/DIY%20BED%20LAMP/hardware/smart_lamp_erc.rpt) | Electrical Rules Check (ERC) validation report (**0 Errors, 0 Warnings**) |

---

## 2. Implemented Subsystems (Steps 1 to 5)

The schematic is organized into five functional blocks:

### Block 1: 5V USB-C Power Input & Bulk Filtering
- **Input (`J1`)**: 2-pin screw terminal / header (`+5V`, `GND`) fed by USB-C 5V.
- **Bulk Capacitor (`C1`)**: 100 µF 16V electrolytic capacitor across +5V and GND to buffer peak current pulses from 20 kHz LED PWM switching.
- **HF Bypass (`C2`)**: 100 nF ceramic capacitor on 5V rail for high-frequency noise suppression.
- **3.3V Filtering (`C3`, `C4`)**: 10 µF bulk + 100 nF ceramic bypass capacitors on the ESP32-C3 onboard 3.3V LDO output.

### Block 2: Microcontroller (`U1` - ESP32-C3 SuperMini)
- **Power**: 5V input to Pin 9, GND to Pin 10, 3.3V output from Pin 11.
- **PWM Outputs**:
  - `GPIO0` (Pin 16) -> Net `PWM_WARM` (Warm white channel)
  - `GPIO1` (Pin 15) -> Net `PWM_COOL` (Cool white channel)
- **Touch Input**:
  - `GPIO3` (Pin 13) -> Net `TOUCH_IN` (TTP223 capacitive touch input, active HIGH)
- **Reserved / Expansion Pins**:
  - `GPIO4` (Pin 12) -> Net `I2C_SDA`
  - `GPIO5` (Pin 1)  -> Net `I2C_SCL`
  - `GPIO6` (Pin 2)  -> Net `BTN_PAGE`
- **Strapping Pins (`GPIO8`, `GPIO9`)**:
  - Marked with `no_connect` and explicit warnings per `PLAN.md` (never use for external I/O).

### Block 3: Touch Sensor (`MOD1` - TTP223 Capacitive Module)
- **Power**: `VCC` -> +3.3V rail, `GND` -> GND.
- **Signal**: `OUT` -> Net `TOUCH_IN` (ESP32-C3 GPIO3).
- **Configuration Note**: Solder jumpers A & B are left open (default factory state) for active-HIGH momentary operation.

### Block 4: Dual Low-Side MOSFET LED Driver (20 kHz PWM)
- **Warm White Channel**:
  - Gate Resistor `R1`: 100 Ω in series from `PWM_WARM` (damps ringing and protects GPIO).
  - Gate Pulldown `R2`: 10 kΩ to GND (keeps Q1 firmly OFF during ESP32 boot/reset).
  - MOSFET `Q1`: IRLZ44N N-channel logic-level power MOSFET (TO-220).
    - Gate -> node between R1 and R2.
    - Source -> GND.
    - Drain -> Net `LED_WARM_NEG` (Warm White cathode).
- **Cool White Channel**:
  - Gate Resistor `R3`: 100 Ω in series from `PWM_COOL`.
  - Gate Pulldown `R4`: 10 kΩ to GND.
  - MOSFET `Q2`: IRLZ44N N-channel logic-level power MOSFET (TO-220).
    - Gate -> node between R3 and R4.
    - Source -> GND.
    - Drain -> Net `LED_COOL_NEG` (Cool White cathode).
- **LED Connector (`J2`)**: 3-pin connector:
  - Pin 1: `+5V` (Common Anode from 5V power rail)
  - Pin 2: `W-` (Warm cathode from Q1 Drain)
  - Pin 3: `C-` (Cool cathode from Q2 Drain)

### Block 5: Expansion Interfaces (For Upcoming Steps)
- Delineated in a dashed box with clean breakout headers so future peripherals can be plugged in without redesigning the core board:
  - **`J3` (`I2C Bus`)**: 4-pin header (`+3.3V`, `GND`, `I2C_SDA` [GPIO4], `I2C_SCL` [GPIO5]) for Step 7 OLED (0x3C), Step 8 RTC DS3231 (0x68), Step 11 BH1750 (0x23), Step 12 AHT20 (0x38).
  - **`J4` (`Page Button`)**: 2-pin header (`BTN_PAGE` [GPIO6], `GND`) for Step 9 push button (active LOW to GND with internal pull-up).

---

## 3. Bill of Materials (BOM)

| Ref | Value | Package / Footprint | Description |
| :--- | :--- | :--- | :--- |
| **U1** | ESP32-C3-SuperMini | PinHeader_2x08_P2.54mm | ESP32-C3 SuperMini MCU module |
| **MOD1** | TTP223_Touch | PinHeader_1x03_P2.54mm | TTP223 capacitive touch module |
| **Q1, Q2** | IRLZ44N | TO-220-3_Vertical | Logic-Level N-MOSFET (55V, 47A, Vgs(th) 1-2V) |
| **R1, R3** | 100 Ω | Axial DIN0207 (P=7.62mm) | 1/4W gate series resistor |
| **R2, R4** | 10 kΩ | Axial DIN0207 (P=7.62mm) | 1/4W gate pulldown resistor |
| **C1** | 100 µF 16V | Radial D6.3mm P2.5mm | Bulk electrolytic capacitor |
| **C2, C4** | 100 nF | Disc D5mm P5mm | Ceramic decoupling capacitor |
| **C3** | 10 µF | Disc D5mm P5mm | Ceramic bulk capacitor |
| **J1** | Conn_01x02_5V_IN | TerminalBlock MKDS-1,5 2-pin | Main 5V power input terminal |
| **J2** | Conn_01x03_LED | TerminalBlock MKDS-1,5 3-pin | 5V CCT LED strip connector |
| **J3** | Conn_01x04_I2C | PinHeader 1x04 P2.54mm | I2C sensor bus expansion |
| **J4** | Conn_01x02_BTN | PinHeader 1x02 P2.54mm | Page button expansion |

---

## 4. Verification & Validation

The schematic has been verified using the KiCad 10 command-line engine:
- **Electrical Rules Check (ERC)**: `0 Errors, 0 Warnings` (see [`smart_lamp_erc.rpt`](file:///c:/DRIVE%20E/MY%20CHORES/DIY%20BED%20LAMP/hardware/smart_lamp_erc.rpt))
- **Grid alignment**: All wire endpoints, junctions, and symbol pins are aligned to the standard 1.27 mm / 2.54 mm KiCad grid.
