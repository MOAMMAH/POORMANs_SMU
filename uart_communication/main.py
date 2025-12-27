import keysight_opm
import uart_com
from time import sleep

# VISA resource string for Keysight OPM
keysight_visa = 'TCPIP::129.82.224.199::INSTR'

if __name__ == "__main__":
    try:
        # Initialize OPM (doesn't use serial port)
        opm = keysight_opm.KeysightOPM(keysight_visa)
        print(f"Connected to OPM: {opm.id}\n")
        
        # Initialize DAC controller
        dac = uart_com.DACController(port="COM3", baud=115200, verbose=True)
        
        # Check communication
        if not dac.check_communication():
            print("Warning: DAC communication check failed.")
            dac.close()
            exit()
        
        # Set DAC channel 0
        dac_value = 1000
        print(f"Setting DAC channel 0 to {dac_value}...")
        dac.set_dac(0, dac_value)
        sleep(0.2)  # Wait for DAC to settle
        
        # Close DAC controller to free the serial port
        dac.close()
        sleep(0.5)  # Brief pause before reopening port
        
        # Initialize ADC controller (uses same port, so we need to close DAC first)
        adc = uart_com.ADCController(port="COM3", baud=115200, 
                                     shunt_resistors=[200.0, 200.0, 200.0, 200.0],
                                     verbose=True)
        
        # Check communication
        if not adc.check_communication():
            print("Warning: ADC communication check failed.")
            adc.close()
            exit()
        
        # Read current from ADC channel 0
        print("\nReading current from ADC channel 0...")
        current = adc.read_current(0, verbose=True, timeout=3.0)
        
        # Close ADC controller
        adc.close()
        
        # Read power from OPM channel 1 (OPM uses 1-based indexing)
        print("\nReading power from OPM channel 1...")
        power = opm.get_power(1)
        unit = opm.get_unit(1)
        
        # Print results
        print("\n" + "="*50)
        print("Results:")
        print("="*50)
        print(f"DAC Channel 0: {dac_value}")
        if current is not None:
            print(f"ADC Channel 0 Current: {current*1000:.3f} mA")
        else:
            print(f"ADC Channel 0 Current: N/A")
        if power is not None:
            print(f"OPM Channel 1: {power:.6f} {unit}")
        else:
            print(f"OPM Channel 1: N/A")
                
    except Exception as e:
        print(f"Error: {e}")