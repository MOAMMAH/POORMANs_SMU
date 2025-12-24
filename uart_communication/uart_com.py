import serial
import serial.tools.list_ports
import time


def list_available_ports():
    """List all available COM ports."""
    ports = serial.tools.list_ports.comports()
    print("Available COM ports:")
    if ports:
        for port in ports:
            print(f"  - {port.device}: {port.description}")
    else:
        print("  No COM ports found!")
    print()


class DACController:
    """
    Controller class for communicating with STM32 MCU to control MCP4728 DAC via UART.
    """
    
    def __init__(self, port="COM3", baud=115200, auto_connect=True, verbose=True):
        """
        Initialize the DAC controller.
        
        Args:
            port (str): Serial port name (e.g., "COM3", "/dev/ttyUSB0")
            baud (int): Baud rate (default: 115200)
            auto_connect (bool): Automatically connect on initialization
            verbose (bool): Print connection messages
        """
        self.port = port
        self.baud = baud
        self.ser = None
        self.verbose = verbose
        
        if auto_connect:
            self.connect()
    
    def connect(self):
        """Establish serial connection to the MCU."""
        # List available ports
        list_available_ports()
        
        # Check if the requested port is available
        if self.verbose:
            print(f"Checking if {self.port} is available...")
        
        if not self._check_port_available(self.port):
            if self.verbose:
                print(f"⚠️  {self.port} appears to be in use!")
                print("\nCommon causes:")
                print("  • STM32CubeIDE Serial Monitor is open")
                print("  • STM32CubeIDE debugger is running")
                print("  • Another Python script is using the port")
                print("  • Previous script didn't close properly")
                print("\nTrying to release the port...")
            
            if self._try_release_port(self.port):
                if self.verbose:
                    print("  ✓ Port released, trying again...")
                time.sleep(1)
            else:
                if self.verbose:
                    print("  ✗ Could not release port automatically")
                    print("\nManual steps:")
                    print("  1. Close STM32CubeIDE completely (check system tray)")
                    print("  2. Close any other terminals running Python scripts")
                    print("  3. Unplug and replug USB cable")
                    print("  4. Or try a different COM port from the list above")
                    print()
                raise serial.SerialException(f"Port {self.port} is locked by another program")
        
        # Initialize serial connection
        try:
            if self.verbose:
                print(f"Attempting to open {self.port}...")
            self.ser = serial.Serial(self.port, self.baud, timeout=1)
            time.sleep(2)   # Wait for STM32 reset
            if self.verbose:
                print(f"✓ Successfully connected to {self.port}\n")
        except serial.SerialException as e:
            if self.verbose:
                print(f"✗ Error: Could not open serial port {self.port}")
                print(f"Details: {e}")
                print("\n⚠️  The port is locked by another program.")
                print("\nQuick fixes (try in order):")
                print("  1. Close STM32CubeIDE completely (File → Exit)")
                print("  2. Check Task Manager for 'STM32CubeIDE' or 'st-link' processes")
                print("  3. Close any other Python terminals/scripts")
                print("  4. Unplug USB cable, wait 5 seconds, plug back in")
                print("  5. Restart your computer if nothing else works")
                print("\nAlternative: Use a different COM port from the list above")
            raise
    
    def _check_port_available(self, port_name):
        """Check if a port is available by trying to open it."""
        try:
            test_ser = serial.Serial(port_name, self.baud, timeout=0.1)
            test_ser.close()
            return True
        except:
            return False
    
    def _try_release_port(self, port_name):
        """Try to release a port by attempting to close any existing connection."""
        try:
            # Try to open and immediately close to release it
            temp_ser = serial.Serial(port_name, self.baud, timeout=0.1)
            temp_ser.close()
            time.sleep(0.5)
            return True
        except:
            return False
    
    def set_dac(self, channel, dac_value, verbose=None, wait_for_response=True, timeout=2.0):
        """
        Set DAC value for a specific channel and wait for MCU acknowledgment.
        
        Args:
            channel (int): Channel number (0-3)
            dac_value (int): DAC code value (0-4095)
            verbose (bool): Print confirmation message (defaults to self.verbose)
            wait_for_response (bool): Wait for MCU response/acknowledgment
            timeout (float): Timeout in seconds when waiting for response
        
        Returns:
            tuple: (success: bool, response: str) - success indicates if command was sent,
                   response contains MCU acknowledgment or None if timeout/error
        """
        if verbose is None:
            verbose = self.verbose
        
        # Validate inputs
        if channel < 0 or channel > 3:
            print(f"Error: Channel must be 0-3, got {channel}")
            return False, None
        
        if dac_value < 0 or dac_value > 4095:
            print(f"Error: DAC value must be 0-4095, got {dac_value}")
            return False, None
        
        # Clear any leftover data in input buffer
        self.ser.reset_input_buffer()
        
        # Format: "channel,dac_value"
        message = f"{channel},{dac_value}\n"
        self.ser.write(message.encode())
        self.ser.flush()  # Ensure data is sent immediately
        
        if verbose:
            print(f"Sent: Channel {channel} = {dac_value}")
        
        # Wait for MCU response
        if wait_for_response:
            response = self.wait_for_mcu_response(timeout)
            if response:
                if verbose:
                    print(f"  MCU: {response}")
                return True, response
            else:
                if verbose:
                    print(f"  Warning: No response from MCU within {timeout}s")
                return True, None  # Command sent but no response received
        
        return True, None
    
    def set_all_channels(self, dac_value, verbose=None, wait_for_response=True, timeout=2.0):
        """
        Set all DAC channels to the same value and wait for MCU acknowledgment.
        
        Args:
            dac_value (int): DAC code value (0-4095) to set on all channels
            verbose (bool): Print confirmation message (defaults to self.verbose)
            wait_for_response (bool): Wait for MCU response/acknowledgment
            timeout (float): Timeout in seconds when waiting for response
        
        Returns:
            tuple: (success: bool, response: str) - success indicates if command was sent,
                   response contains MCU acknowledgment or None if timeout/error
        """
        if verbose is None:
            verbose = self.verbose
        
        # Validate input
        if dac_value < 0 or dac_value > 4095:
            print(f"Error: DAC value must be 0-4095, got {dac_value}")
            return False, None
        
        # Clear any leftover data in input buffer
        self.ser.reset_input_buffer()
        
        # Format: "set_all,dac_value"
        message = f"set_all,{dac_value}\n"
        self.ser.write(message.encode())
        self.ser.flush()  # Ensure data is sent immediately
        
        if verbose:
            print(f"Sent: All channels = {dac_value}")
        
        # Wait for MCU response
        if wait_for_response:
            response = self.wait_for_mcu_response(timeout)
            if response:
                if verbose:
                    print(f"  MCU: {response}")
                return True, response
            else:
                if verbose:
                    print(f"  Warning: No response from MCU within {timeout}s")
                return True, None  # Command sent but no response received
        
        return True, None
    
    def read_response(self):
        """
        Read a response from the MCU (non-blocking).
        
        Returns:
            str: Response line if available, None otherwise
        """
        if self.ser.in_waiting > 0:
            try:
                line = self.ser.readline().decode().strip()
                return line if line else None
            except (UnicodeDecodeError, serial.SerialException):
                return None
        return None
    
    def wait_for_mcu_response(self, timeout=2.0):
        """
        Wait for a response from the MCU (blocking with timeout).
        
        Args:
            timeout (float): Maximum time to wait in seconds
        
        Returns:
            str: Response line if received, None if timeout
        """
        start_time = time.time()
        
        while (time.time() - start_time) < timeout:
            if self.ser.in_waiting > 0:
                try:
                    line = self.ser.readline().decode().strip()
                    if line:
                        return line
                except (UnicodeDecodeError, serial.SerialException):
                    pass
            time.sleep(0.01)  # Small delay to avoid busy waiting
        
        return None
    
    def check_communication(self, timeout=2.0, verbose=None):
        """
        Check if communication with MCU is working by sending a COMM_OK command.
        
        Args:
            timeout (float): Maximum time to wait for response in seconds
            verbose (bool): Print status messages (defaults to self.verbose)
        
        Returns:
            bool: True if communication is OK, False otherwise
        """
        if verbose is None:
            verbose = self.verbose
        
        if verbose:
            print("Checking communication with MCU...")
        
        # Clear any leftover data in input buffer
        self.ser.reset_input_buffer()
        
        # Send COMM_OK command
        message = "COMM_OK\n"
        self.ser.write(message.encode())
        self.ser.flush()  # Ensure data is sent immediately
        
        # Wait for response
        response = self.wait_for_mcu_response(timeout)
        
        if response:
            # Check if response contains "COMM_OK" or "OK"
            if "COMM_OK" in response.upper() or response.upper() == "OK":
                if verbose:
                    print(f"  ✓ Communication OK: {response}")
                return True
            else:
                if verbose:
                    print(f"  ✗ Unexpected response: {response}")
                return False
        else:
            if verbose:
                print(f"  ✗ No response from MCU within {timeout}s")
            return False
    
    def sweep_channel(self, channel, start_value, end_value, steps, delay=0.1):
        """
        Sweep a DAC channel from start_value to end_value.
        
        Args:
            channel (int): Channel number (0-3)
            start_value (int): Starting DAC value (0-4095)
            end_value (int): Ending DAC value (0-4095)
            steps (int): Number of steps in the sweep
            delay (float): Delay between steps in seconds
        """
        if steps < 1:
            print("Error: Steps must be >= 1")
            return
        
        step_size = (end_value - start_value) / (steps - 1) if steps > 1 else 0
        
        print(f"Sweeping channel {channel} from {start_value} to {end_value} in {steps} steps...")
        
        for i in range(steps):
            dac_value = int(start_value + i * step_size)
            success, response = self.set_dac(channel, dac_value, verbose=False, wait_for_response=True)
            print(f"  Step {i+1}/{steps}: Channel {channel} = {dac_value}", end="")
            if response:
                print(f" - {response}")
            else:
                print()
            time.sleep(delay)
    
    def sweep_all_channels(self, start_values, end_values, steps, delay=0.1):
        """
        Sweep all 4 channels simultaneously.
        
        Args:
            start_values (list): Starting DAC values for each channel [ch0, ch1, ch2, ch3]
            end_values (list): Ending DAC values for each channel [ch0, ch1, ch2, ch3]
            steps (int): Number of steps in the sweep
            delay (float): Delay between steps in seconds
        """
        if len(start_values) != 4 or len(end_values) != 4:
            print("Error: start_values and end_values must have 4 elements")
            return
        
        print(f"Sweeping all channels in {steps} steps...")
        
        for i in range(steps):
            for ch in range(4):
                start_val = start_values[ch]
                end_val = end_values[ch]
                step_size = (end_val - start_val) / (steps - 1) if steps > 1 else 0
                dac_value = int(start_val + i * step_size)
                self.set_dac(ch, dac_value, verbose=False, wait_for_response=False)
            print(f"  Step {i+1}/{steps}: Ch0={int(start_values[0] + i*(end_values[0]-start_values[0])/(steps-1))}, "
                  f"Ch1={int(start_values[1] + i*(end_values[1]-start_values[1])/(steps-1))}, "
                  f"Ch2={int(start_values[2] + i*(end_values[2]-start_values[2])/(steps-1))}, "
                  f"Ch3={int(start_values[3] + i*(end_values[3]-start_values[3])/(steps-1))}")
            time.sleep(delay)
    
    def sweep_channels_independent(self, channel_configs, delay=0.1):
        """
        Sweep multiple channels independently with different directions and ranges simultaneously.
        
        Args:
            channel_configs (list): List of channel configurations, each as a dict:
                {
                    'channel': int (0-3),
                    'start': int (0-4095),
                    'end': int (0-4095),
                    'steps': int
                }
                Channels can sweep upward (start < end) or downward (start > end)
            delay (float): Delay between steps in seconds
        
        Example:
            # Channel 0: 0 to 100 (upward), Channel 1: 1000 to 2000 (upward), Channel 3: 2000 to 10 (downward)
            sweep_channels_independent([
                {'channel': 0, 'start': 0, 'end': 100, 'steps': 50},
                {'channel': 1, 'start': 1000, 'end': 2000, 'steps': 100},
                {'channel': 3, 'start': 2000, 'end': 10, 'steps': 199}
            ])
        """
        if not channel_configs:
            print("Error: No channel configurations provided")
            return
        
        # Validate all configurations
        for config in channel_configs:
            if 'channel' not in config or 'start' not in config or 'end' not in config or 'steps' not in config:
                print(f"Error: Invalid configuration format. Each config needs 'channel', 'start', 'end', 'steps'")
                return
            if config['channel'] < 0 or config['channel'] > 3:
                print(f"Error: Channel must be 0-3, got {config['channel']}")
                return
            if config['start'] < 0 or config['start'] > 4095 or config['end'] < 0 or config['end'] > 4095:
                print(f"Error: DAC values must be 0-4095")
                return
            if config['steps'] < 1:
                print(f"Error: Steps must be >= 1")
                return
        
        # Find maximum number of steps
        max_steps = max(config['steps'] for config in channel_configs)
        
        print(f"Sweeping {len(channel_configs)} channels independently in {max_steps} steps...")
        for config in channel_configs:
            direction = "up" if config['start'] < config['end'] else "down"
            print(f"  Channel {config['channel']}: {config['start']} → {config['end']} ({direction}, {config['steps']} steps)")
        
        # Initialize channel values
        channel_values = {}
        for config in channel_configs:
            channel_values[config['channel']] = config['start']
        
        # Perform sweep
        for step in range(max_steps):
            # Calculate current value for each channel
            for config in channel_configs:
                if config['steps'] > 1:
                    # Calculate progress (0.0 to 1.0)
                    progress = step / (config['steps'] - 1)
                    if progress > 1.0:
                        progress = 1.0
                    
                    # Interpolate value
                    channel_values[config['channel']] = int(
                        config['start'] + progress * (config['end'] - config['start'])
                    )
                else:
                    # Single step - use end value
                    channel_values[config['channel']] = config['end']
            
            # Update all channels simultaneously
            for channel, value in channel_values.items():
                self.set_dac(channel, value, verbose=False, wait_for_response=False)
            
            # Print status
            values_str = ", ".join([f"Ch{ch}={val}" for ch, val in sorted(channel_values.items())])
            print(f"  Step {step+1}/{max_steps}: {values_str}")
            
            time.sleep(delay)
        
        print("Sweep complete!")
    
    def close(self):
        """Close the serial connection."""
        if self.ser and self.ser.is_open:
            self.ser.close()
            if self.verbose:
                print("Serial connection closed.")
    
    def __enter__(self):
        """Context manager entry."""
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit - automatically close connection."""
        self.close()


# ============================================================================
# Main execution - example usage
# ============================================================================

if __name__ == "__main__":
    # Create controller instance
    dac = DACController(port="COM3", baud=115200, verbose=True)
    
    # Print initialization message
    print("Connected to STM32 on", dac.port)
    print("Ready to control DAC channels.")
    
    # Check communication on startup
    if dac.check_communication(verbose=True):
        print("Communication verified!\n")
    else:
        print("Warning: Communication check failed. MCU may not be ready.\n")
    
    print("Available methods:")
    print("  - dac.check_communication()  # Verify MCU communication")
    print("  - dac.set_dac(channel, dac_value)")
    print("  - dac.set_all_channels(dac_value)  # Set all channels to same value")
    print("  - dac.sweep_channel(channel, start, end, steps, delay)")
    print("  - dac.sweep_all_channels([start0,start1,start2,start3], [end0,end1,end2,end3], steps, delay)")
    print("  - dac.sweep_channels_independent([{'channel':0,'start':0,'end':100,'steps':50}, ...])  # Independent sweeps")
    print("  - dac.read_response()")
    print("  - dac.close()")
    print("\nExample usage:")
    print("  dac.check_communication()  # Check if MCU is responding")
    print("  dac.sweep_channel(0, 0, 4095, 100, delay=0.05)  # Sweep channel 0 from 0 to 4095 in 100 steps")
    print("  dac.set_dac(1, 2048)  # Set channel 1 to mid-scale\n")
    
    # ============================================================================
    # YOUR CODE GOES HERE
    # ============================================================================
    
    if dac.check_communication():
        print("Communication verified!")
        time.sleep(1)
        print("Setting channel 1 to 1000")
        dac.set_dac(1, 1000)
    else:
        print("Communication check failed. MCU may not be ready.")
        dac.close()
        exit()
    
    
    # Example 1: Simple sweep on channel 0
    # dac.sweep_channel(0, 0, 4095, 50, delay=0.1)
    
    # Example 2: Sweep multiple channels
    # dac.sweep_channel(0, 0, 2048, 20, delay=0.1)
    # dac.sweep_channel(1, 2048, 4095, 20, delay=0.1)
    
    # Example 3: Sweep all channels simultaneously
    # dac.sweep_all_channels([0, 0, 0, 0], [4095, 2048, 1024, 512], 50, delay=0.1)
    
    # Example 4: Custom algorithm
    # for i in range(10):
    #     dac.set_dac(0, i * 400)
    #     dac.set_dac(1, 4095 - i * 400)
    #     time.sleep(0.2)
    
    # ============================================================================
    # Clean up (optional - connection will close when script exits)
    # ============================================================================
    # dac.close()
