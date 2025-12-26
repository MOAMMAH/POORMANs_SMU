# POORMANs_SMU

A comprehensive system for controlling 4-channel DAC and ADC via STM32 Nucleo board with UART communication, designed for optical testing and IV curve measurements.

## Overview

This project implements a Source Measurement Unit (SMU) using:
- **STM32 Nucleo board** (STM32F446RE) as the microcontroller
- **MCP4728** 4-channel 12-bit DAC for voltage/current sourcing
- **ADS1115** 16-bit ADC for voltage/current measurement
- **Python control scripts** for instrument automation
- **Keysight OPM** integration for optical power measurements

The system enables simultaneous control of multiple channels, IV curve characterization, and optical power measurements synchronized with electrical sweeps.

## Hardware Components

### STM32 Nucleo (STM32F446RE)
- **I2C1** (PB6/SCL, PB7/SDA): Connected to MCP4728 DAC
- **I2C2** (PB10/SCL, PB11/SDA): Connected to ADS1115 ADC
- **UART2** (PA2/TX, PA3/RX): Serial communication with Python scripts (115200 baud)

### MCP4728 DAC
- 4-channel, 12-bit Digital-to-Analog Converter
- I2C address: 0x60 (A0 pin to GND)
- 5V reference voltage
- Resolution: 4096 steps (0-4095)
- Output range: 0-5V

### ADS1115 ADC
- 16-bit Analog-to-Digital Converter
- I2C address: 0x48 (ADDR pin to GND)
- Programmable Gain Amplifier (PGA): ±6.144V range
- 4 single-ended input channels (AIN0-AIN3 vs GND)
- 128 samples per second (SPS) data rate
- Used for current measurement via shunt resistors

## Software Architecture

### STM32 Firmware (`uart_communication/nucleo/`)

#### `main.c` / `main.h`
**Purpose**: Main application file for STM32 microcontroller

**Key Features**:
- System initialization (I2C1, I2C2, UART2)
- UART command parser for receiving Python commands
- Command handlers:
  - `COMM_OK`: Communication test
  - `channel,dac_value`: Set single DAC channel (0-3, 0-4095)
  - `set_all,dac_value`: Set all DAC channels to same value
  - `read_adc,channel`: Read voltage from ADC channel (0-3)
  - `test_adc`: Test I2C communication with ADC
- ADC voltage reading function `ADS1115_ReadVoltage()`
- Non-blocking UART receive with command buffer

**I2C Configuration**:
- I2C1: 100kHz, for MCP4728 DAC
- I2C2: 100kHz, for ADS1115 ADC

#### `mcp4728.c` / `mcp4728.h`
**Purpose**: MCP4728 DAC driver implementation

**Key Functions**:
- `MCP4728_Init()`: Initialize DAC, send wakeup command
- `MCP4728_WriteChannel()`: Write value to single channel using Sequential Write mode
- `MCP4728_SetAllChannels()`: Write values to all 4 channels simultaneously
- `MCP4728_Write_GeneralCall()`: Send general call commands (wakeup, reset)

**Communication Modes**:
- Fast Write (0x40): Immediate output update
- Sequential Write (0x50): Update all channels with VREF/GAIN settings
- General Call: Wakeup, reset, software update commands

#### `ADS1115.c` / `ADS1115.h`
**Purpose**: ADS1115 ADC driver implementation

**Key Functions**:
- `ADS1115_init()`: Initialize ADC with configuration
- `ADS1115_oneShotMeasure()`: Perform single-shot conversion
- `ADS1115_getData()`: Read conversion result from ADC
- `ADS1115_updateConfig()`: Update ADC configuration (channel, PGA, data rate)
- `ADS1115_setThresholds()`: Set comparator thresholds
- `ADS1115_startContinousMode()` / `ADS1115_stopContinousMode()`: Continuous conversion control

**Configuration**:
- Single-shot mode (power efficient)
- ±6.144V PGA range (supports 0-5V measurements)
- 128 SPS data rate
- MUX settings for single-ended measurements (AINx vs GND)

**Key Fix**: The ADC reading was fixed by:
1. Directly updating the handle's config before starting conversion
2. Using fixed 15ms delay instead of OS bit polling (more reliable)
3. Proper error handling in I2C communication

### Python Control Scripts (`uart_communication/`)

#### `uart_com.py`
**Purpose**: Python interface for STM32 communication and instrument control

**Classes**:

1. **`SerialController`** (Base Class)
   - Serial port management
   - Communication verification (`check_communication()`)
   - Response waiting and parsing
   - Port availability checking and error handling

2. **`DACController`** (Inherits from `SerialController`)
   - `set_dac(channel, dac_value)`: Set single channel
   - `set_all_channels(dac_value)`: Set all channels to same value
   - `sweep_channel(channel, start, end, steps, delay)`: Sweep single channel
   - `sweep_all_channels(start_values, end_values, steps, delay)`: Sweep all channels simultaneously
   - `sweep_channels_independent(channel_configs, delay)`: Independent sweeps on multiple channels

3. **`ADCController`** (Inherits from `SerialController`)
   - `read_voltage(channel)`: Read voltage from ADC channel
   - `read_current(channel)`: Calculate current from shunt resistor
   - `read_all_voltages()`: Read all 4 channels
   - `read_all_currents()`: Read currents from all channels
   - `test_adc_i2c()`: Test I2C communication with ADC
   - `set_shunt_resistor(channel, value)`: Configure shunt resistor values

**Features**:
- Automatic port detection and connection
- Error handling and retry logic
- Verbose mode for debugging
- Context manager support (`with` statement)

#### `keysight_opm.py`
**Purpose**: Keysight Optical Power Meter (OPM) control interface

**Class**: `KeysightOPM`

**Key Methods**:
- `get_power(chan)`: Read power in current unit (dBm or Watt)
- `get_power_mw(chan)`: Read power in milliwatts (auto-converts unit)
- `get_power_all()`: Read all channels simultaneously
- `set_unit(chan, unit)`: Set power unit (dBm or Watt)
- `set_wavelength(chan, wavel)`: Set measurement wavelength
- `set_range(chan, pwr_range)`: Set power range or auto-range
- `measure_power_vs_dac()`: Measure optical power while sweeping DAC values

**Features**:
- Automatic device detection (2 or 4 channel support)
- Unit conversion (dBm ↔ mW)
- Binary data reading for fast multi-channel measurements
- Integration with DAC controller for synchronized sweeps

#### `utils.py`
**Purpose**: Utility classes for data analysis, plotting, and file operations

**Classes**:

1. **`Electrical`**
   - IV curve calculations
   - `calculate_resistance()`, `calculate_power()`, `calculate_current_from_shunt()`
   - `analyze_iv_curve()`: Extract key parameters (max power, open circuit voltage, short circuit current)

2. **`Optical`**
   - Combines DAC, ADC, and OPM for optical measurements
   - `measure_iv_point()`: Single measurement with optical power
   - `sweep_iv_curve()`: Full IV sweep with optical power reading

3. **`DataHandler`**
   - CSV file operations
   - `save_to_csv()`, `load_from_csv()`
   - `create_timestamped_filename()`: Generate unique filenames

4. **`Plotter`**
   - Matplotlib plotting functions
   - `plot_iv_curve()`: Plot voltage vs current
   - `plot_power_curve()`: Plot voltage vs power with MPP marker
   - `plot_optical_power()`: Plot optical vs electrical power
   - `plot_from_csv()`: Plot data from CSV files

**Design Philosophy**: Modular, reusable classes that can be composed together for complex measurements.

## Features

### DAC Control
- ✅ Individual channel control (0-3)
- ✅ Simultaneous multi-channel updates
- ✅ Independent sweeps on different channels
- ✅ Configurable sweep ranges and step counts
- ✅ Real-time feedback and error handling

### ADC Measurement
- ✅ 16-bit voltage measurement (0-5V range)
- ✅ Current calculation via shunt resistors
- ✅ Multi-channel simultaneous reading
- ✅ I2C communication diagnostics
- ✅ Automatic error detection

### Optical Integration
- ✅ Synchronized DAC sweeps with OPM readings
- ✅ Power vs DAC code measurements
- ✅ Multi-channel OPM support
- ✅ Automatic unit conversion

### Data Analysis
- ✅ IV curve characterization
- ✅ Power curve analysis (MPP detection)
- ✅ CSV data export/import
- ✅ Automated plotting
- ✅ Timestamped file management

## Usage Examples

### Basic DAC Control
```python
from uart_communication.uart_com import DACController

dac = DACController(port="COM3", verbose=True)
dac.set_dac(0, 2048)  # Set channel 0 to mid-scale
dac.sweep_channel(0, 0, 4095, 100, delay=0.05)
dac.close()
```

### IV Curve Measurement
```python
from uart_communication.uart_com import DACController, ADCController
from uart_communication.utils import Electrical, Plotter, DataHandler

dac = DACController(port="COM3", verbose=False)
adc = ADCController(port="COM3", shunt_resistors=[200.0, 200.0, 200.0, 200.0])

# Sweep DAC and measure current
voltages = []
currents = []
for dac_val in range(0, 4096, 100):
    dac.set_dac(0, dac_val)
    time.sleep(0.1)
    voltage = adc.read_voltage(0)
    current = adc.read_current(0)
    voltages.append(voltage)
    currents.append(current)

# Analyze and plot
elec = Electrical()
analysis = elec.analyze_iv_curve(voltages, currents)
Plotter.plot_iv_curve(voltages, currents)
DataHandler.save_to_csv({'voltages': voltages, 'currents': currents}, 'iv_curve.csv')
```

### Optical Power vs DAC Sweep
```python
from uart_communication.uart_com import DACController
from uart_communication.keysight_opm import KeysightOPM
from uart_communication.utils import Plotter

dac = DACController(port="COM3", verbose=False)
opm = KeysightOPM("TCPIP::192.168.1.100::INSTR")

# Measure power vs DAC code
results = opm.measure_power_vs_dac(
    dac_controller=dac,
    dac_channel=0,
    start_dac=0,
    end_dac=4095,
    steps=100,
    opm_channel=1,
    delay=0.1,
    verbose=True
)

# Plot results
Plotter.plot_optical_power(results['dac_codes'], results['power'])
```

### Combined Electrical and Optical Measurement
```python
from uart_communication.uart_com import DACController, ADCController
from uart_communication.keysight_opm import KeysightOPM
from uart_communication.utils import Optical, Electrical, Plotter

dac = DACController(port="COM3", verbose=False)
adc = ADCController(port="COM3", shunt_resistors=[200.0, 200.0, 200.0, 200.0])
opm = KeysightOPM("TCPIP::192.168.1.100::INSTR")
elec = Electrical(shunt_resistors=[200.0, 200.0, 200.0, 200.0])

# Create optical measurement system
optical = Optical(dac, adc, opm, elec)

# Sweep IV curve with optical power
data = optical.sweep_iv_curve(
    dac_channel=0, adc_channel=0,
    start_value=0, end_value=4095, steps=100,
    opm_channel=1
)

# Plot results
Plotter.plot_iv_curve(data['voltages'], data['currents'])
Plotter.plot_optical_power(data['voltages'], data['powers_optical_mw'], 
                          data['powers_electrical'])
```

## File Structure

```
POORMANs_SMU/
├── README.md                          # This file
└── uart_communication/
    ├── uart_com.py                    # Python serial communication and controllers
    ├── keysight_opm.py                # Keysight OPM control interface
    ├── utils.py                       # Utility classes (Electrical, Optical, DataHandler, Plotter)
    └── nucleo/
        ├── main.c / main.h            # STM32 main application
        ├── mcp4728.c / mcp4728.h      # MCP4728 DAC driver
        └── ADS1115.c / ADS1115.h      # ADS1115 ADC driver
```

## Setup Instructions

### STM32 Side
1. Open project in STM32CubeIDE
2. Configure I2C1 (PB6/SCL, PB7/SDA) for MCP4728
3. Configure I2C2 (PB10/SCL, PB11/SDA) for ADS1115
4. Configure UART2 (PA2/TX, PA3/RX) at 115200 baud
5. Build and flash to STM32 Nucleo board

### Python Side
1. Install required packages:
   ```bash
   pip install pyserial pyvisa numpy matplotlib
   ```
2. Connect STM32 via USB (creates virtual COM port)
3. Update COM port in Python scripts (default: "COM3" on Windows, "/dev/ttyUSB0" on Linux)
4. For OPM: Configure VISA resource string (e.g., "TCPIP::192.168.1.100::INSTR")

## Hardware Connections

### MCP4728 DAC (I2C1)
- VDD → 5V
- GND → GND
- SCL → PB6
- SDA → PB7
- A0 → GND (I2C address: 0x60)

### ADS1115 ADC (I2C2)
- VDD → 3.3V or 5V
- GND → GND
- SCL → PB11
- SDA → PB10
- ADDR → GND (I2C address: 0x48)
- AIN0-AIN3 → Measurement inputs
- Pull-up resistors: 4.7kΩ on SDA/SCL

### Current Measurement
- Connect shunt resistor (e.g., 200Ω) between ADC input and GND
- Connect device under test between DAC output and ADC input
- Current = Voltage_across_shunt / Shunt_resistance

## Troubleshooting

### ADC Reading Zero
- Check I2C2 connections (SDA=PB10, SCL=PB11)
- Verify ADS1115 power and ground
- Check I2C address (ADDR pin to GND = 0x48)
- Verify pull-up resistors (4.7kΩ)
- Use `test_adc_i2c()` to verify I2C communication

### DAC Not Responding
- Check I2C1 connections (SDA=PB7, SCL=PB6)
- Verify MCP4728 power (5V) and ground
- Check I2C address (A0 pin to GND = 0x60)
- Use `check_communication()` to verify UART connection

### Serial Port Issues
- Close STM32CubeIDE Serial Monitor
- Close any other programs using the COM port
- Unplug and replug USB cable
- Check Device Manager for correct COM port number

## License

This project is provided as-is for research and educational purposes.

## Authors

- STM32 firmware and drivers
- Python control scripts and utilities
- Integration with Keysight OPM

## Acknowledgments

- ADS1115 driver adapted from work by Filip Rak
- MCP4728 driver implementation
- STM32 HAL library by STMicroelectronics
