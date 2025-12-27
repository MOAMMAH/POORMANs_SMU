"""
Utility classes for IV curve measurements, optical power measurements, data handling, and plotting.
"""

import numpy as np
import matplotlib.pyplot as plt
import csv
import os
from datetime import datetime
from typing import List, Optional, Dict
import time


class Electrical:
    """
    Class for electrical IV curve calculations and analysis.
    """
    
    def __init__(self, shunt_resistors: List[float] = None):
        """
        Initialize electrical utilities.
        
        Args:
            shunt_resistors: List of shunt resistor values in Ohms [ch0, ch1, ch2, ch3]
        """
        if shunt_resistors is None:
            shunt_resistors = [1.0, 1.0, 1.0, 1.0]
        self.shunt_resistors = shunt_resistors

    def set_shunt_resistors(self, shunt_resistors: List[float]):
        """Set shunt resistor values for each channel."""
        self.shunt_resistors = shunt_resistors

    def get_shunt_resistors(self) -> List[float]:
        """Get shunt resistor values."""
        return self.shunt_resistors

    def calculate_resistance(self, voltage: float, current: float) -> float:
        """Calculate resistance from voltage and current."""
        if current == 0:
            return float('inf')
        return voltage / current

    def calculate_power(self, voltage: float, current: float) -> float:
        """Calculate electrical power (P = V × I)."""
        return voltage * current
    
    def calculate_current_from_shunt(self, voltage: float, channel: int) -> float:
        """Calculate current from shunt resistor voltage drop."""
        if channel < 0 or channel >= len(self.shunt_resistors):
            return 0.0
        shunt_r = self.shunt_resistors[channel]
        if shunt_r == 0:
            return 0.0
        return voltage / shunt_r
    
    def analyze_iv_curve(self, voltages: List[float], currents: List[float]) -> Dict:
        """
        Analyze IV curve and return key parameters.
        
        Returns:
            Dictionary with: resistances, powers, max_power, max_power_voltage,
            max_power_current, open_circuit_voltage, short_circuit_current
        """
        if len(voltages) != len(currents):
            raise ValueError("Voltages and currents must have the same length")
        
        resistances = [self.calculate_resistance(v, i) for v, i in zip(voltages, currents)]
        powers = [self.calculate_power(v, i) for v, i in zip(voltages, currents)]
        
        max_power_idx = np.argmax(powers)
        max_power = powers[max_power_idx]
        
        # Estimate open circuit voltage (point closest to zero current)
        zero_current_idx = np.argmin(np.abs(currents)) if currents else 0
        open_circuit_voltage = voltages[zero_current_idx]
        
        # Estimate short circuit current (point closest to zero voltage)
        zero_voltage_idx = np.argmin(np.abs(voltages)) if voltages else 0
        short_circuit_current = currents[zero_voltage_idx]
        
        return {
            'resistances': resistances,
            'powers': powers,
            'max_power': max_power,
            'max_power_voltage': voltages[max_power_idx],
            'max_power_current': currents[max_power_idx],
            'open_circuit_voltage': open_circuit_voltage,
            'short_circuit_current': short_circuit_current
        }


class Optical:
    """
    Class for optical power measurements using DAC/ADC and OPM.
    """
    
    def __init__(self, dac_controller, adc_controller, opm, electrical: Electrical = None):
        """
        Initialize optical measurement system.
        
        Args:
            dac_controller: DACController instance
            adc_controller: ADCController instance
            opm: KeysightOPM instance
            electrical: Electrical instance for calculations
        """
        self.dac = dac_controller
        self.adc = adc_controller
        self.opm = opm
        self.electrical = electrical or Electrical()
    
    def measure_iv_point(self, dac_channel: int, adc_channel: int, 
                        dac_value: int, opm_channel: int = 1,
                        delay: float = 0.1) -> Dict:
        """
        Measure single IV point with optical power.
        
        Returns:
            Dictionary with: dac_value, voltage, current, power_electrical,
            power_optical_mw, power_optical_dbm
        """
        # Set DAC
        self.dac.set_dac(dac_channel, dac_value, verbose=False)
        time.sleep(delay)
        
        # Read voltage from ADC
        voltage = self.adc.read_voltage(adc_channel, verbose=False)
        if voltage is None:
            voltage = 0.0
        
        # Calculate current
        current = self.electrical.calculate_current_from_shunt(voltage, adc_channel)
        
        # Read optical power
        power_optical_mw = self.opm.get_power_mw(opm_channel)
        power_optical_dbm = self.opm.get_power(opm_channel)
        
        # Calculate electrical power
        power_electrical = self.electrical.calculate_power(voltage, current)
        
        return {
            'dac_value': dac_value,
            'voltage': voltage,
            'current': current,
            'power_electrical': power_electrical,
            'power_optical_mw': power_optical_mw if power_optical_mw is not None else float('nan'),
            'power_optical_dbm': power_optical_dbm if power_optical_dbm is not None else float('nan')
        }
    
    def sweep_iv_curve(self, dac_channel: int, adc_channel: int,
                       start_value: int, end_value: int, steps: int,
                       opm_channel: int = 1, delay: float = 0.1) -> Dict:
        """
        Sweep DAC and measure IV curve with optical power.
        
        Returns:
            Dictionary with: dac_values, voltages, currents, powers_electrical,
            powers_optical_mw, powers_optical_dbm
        """
        dac_values = []
        voltages = []
        currents = []
        powers_electrical = []
        powers_optical_mw = []
        powers_optical_dbm = []
        
        step_size = (end_value - start_value) / (steps - 1) if steps > 1 else 0
        
        print(f"Sweeping DAC ch{dac_channel}, reading ADC ch{adc_channel}, OPM ch{opm_channel}...")
        
        for i in range(steps):
            dac_value = int(start_value + i * step_size)
            point = self.measure_iv_point(dac_channel, adc_channel, dac_value, 
                                        opm_channel, delay)
            
            dac_values.append(point['dac_value'])
            voltages.append(point['voltage'])
            currents.append(point['current'])
            powers_electrical.append(point['power_electrical'])
            powers_optical_mw.append(point['power_optical_mw'])
            powers_optical_dbm.append(point['power_optical_dbm'])
            
            print(f"  Step {i+1}/{steps}: V={point['voltage']:.3f}V, "
                  f"I={point['current']*1000:.3f}mA, "
                  f"P_elec={point['power_electrical']*1000:.3f}mW, "
                  f"P_opt={point['power_optical_mw']:.3f}mW")
        
        return {
            'dac_values': dac_values,
            'voltages': voltages,
            'currents': currents,
            'powers_electrical': powers_electrical,
            'powers_optical_mw': powers_optical_mw,
            'powers_optical_dbm': powers_optical_dbm
        }


class DataHandler:
    """
    Class for CSV file operations.
    """
    
    @staticmethod
    def save_to_csv(data: Dict, filename: str) -> None:
        """
        Save data dictionary to CSV file.
        
        Args:
            data: Dictionary where keys are column names and values are lists
            filename: Output filename
        """
        if not filename.endswith('.csv'):
            filename += '.csv'
        
        # Get all keys and find maximum length
        keys = list(data.keys())
        max_len = max(len(data[k]) for k in keys) if keys else 0
        
        with open(filename, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(keys)  # Header
            
            for i in range(max_len):
                row = [data[k][i] if i < len(data[k]) else '' for k in keys]
                writer.writerow(row)
        
        print(f"Data saved to {filename}")
    
    @staticmethod
    def load_from_csv(filename: str) -> Dict:
        """
        Load data from CSV file.
        
        Returns:
            Dictionary with column names as keys
        """
        if not os.path.exists(filename):
            raise FileNotFoundError(f"File {filename} not found")
        
        data = {}
        with open(filename, 'r') as csvfile:
            reader = csv.reader(csvfile)
            header = next(reader)
            
            for col in header:
                data[col] = []
            
            for row in reader:
                for i, col in enumerate(header):
                    try:
                        data[col].append(float(row[i]))
                    except (ValueError, IndexError):
                        data[col].append(float('nan'))
        
        return data
    
    @staticmethod
    def create_timestamped_filename(prefix: str, extension: str = 'csv') -> str:
        """Create filename with timestamp."""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        return f"{prefix}_{timestamp}.{extension}"


class Plotter:
    """
    Class for plotting IV curves and related data.
    """
    
    @staticmethod
    def plot_iv_curve(voltages: List[float], currents: List[float],
                      title: str = "IV Curve", save_path: Optional[str] = None,
                      show_plot: bool = True) -> None:
        """Plot IV curve (voltage vs current)."""
        plt.figure(figsize=(10, 6))
        plt.plot(voltages, currents, 'b-', linewidth=2, marker='o', markersize=4)
        plt.xlabel('Voltage (V)', fontsize=12)
        plt.ylabel('Current (A)', fontsize=12)
        plt.title(title, fontsize=14, fontweight='bold')
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"Plot saved to {save_path}")
        
        if show_plot:
            plt.show()
        else:
            plt.close()
    
    @staticmethod
    def plot_power_curve(voltages: List[float], powers: List[float],
                        title: str = "Power Curve", save_path: Optional[str] = None,
                        show_plot: bool = True) -> None:
        """Plot power curve (voltage vs power) with MPP marker."""
        plt.figure(figsize=(10, 6))
        plt.plot(voltages, powers, 'r-', linewidth=2, marker='s', markersize=4)
        plt.xlabel('Voltage (V)', fontsize=12)
        plt.ylabel('Power (W)', fontsize=12)
        plt.title(title, fontsize=14, fontweight='bold')
        plt.grid(True, alpha=0.3)
        
        # Mark maximum power point
        max_power_idx = np.argmax(powers)
        plt.plot(voltages[max_power_idx], powers[max_power_idx], 'ro',
                 markersize=10, label=f'MPP: {powers[max_power_idx]:.4f}W')
        plt.legend()
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"Plot saved to {save_path}")
        
        if show_plot:
            plt.show()
        else:
            plt.close()
    
    @staticmethod
    def plot_optical_power(voltages: List[float], powers_optical: List[float],
                          powers_electrical: List[float] = None,
                          title: str = "Optical vs Electrical Power",
                          save_path: Optional[str] = None, show_plot: bool = True) -> None:
        """Plot optical power vs voltage, optionally with electrical power."""
        plt.figure(figsize=(10, 6))
        
        plt.plot(voltages, powers_optical, 'g-', linewidth=2, marker='o',
                markersize=4, label='Optical Power (mW)')
        
        if powers_electrical:
            # Convert to mW for comparison
            powers_electrical_mw = [p * 1000 for p in powers_electrical]
            plt.plot(voltages, powers_electrical_mw, 'r--', linewidth=2,
                    marker='s', markersize=4, label='Electrical Power (mW)')
        
        plt.xlabel('Voltage (V)', fontsize=12)
        plt.ylabel('Power (mW)', fontsize=12)
        plt.title(title, fontsize=14, fontweight='bold')
        plt.grid(True, alpha=0.3)
        plt.legend()
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"Plot saved to {save_path}")
        
        if show_plot:
            plt.show()
        else:
            plt.close()
    
    @staticmethod
    def plot_from_csv(filename: str, x_col: str, y_col: str,
                     title: Optional[str] = None, save_path: Optional[str] = None,
                     show_plot: bool = True) -> None:
        """Plot data from CSV file."""
        data = DataHandler.load_from_csv(filename)
        
        if x_col not in data or y_col not in data:
            raise ValueError(f"Columns '{x_col}' or '{y_col}' not found in CSV")
        
        x_data = data[x_col]
        y_data = data[y_col]
        
        # Remove NaN values
        valid_indices = [i for i in range(len(x_data))
                        if not (np.isnan(x_data[i]) or np.isnan(y_data[i]))]
        x_data = [x_data[i] for i in valid_indices]
        y_data = [y_data[i] for i in valid_indices]
        
        if title is None:
            title = f"{y_col} vs {x_col}"
        
        plt.figure(figsize=(10, 6))
        plt.plot(x_data, y_data, 'b-', linewidth=2, marker='o', markersize=4)
        plt.xlabel(x_col, fontsize=12)
        plt.ylabel(y_col, fontsize=12)
        plt.title(title, fontsize=14, fontweight='bold')
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"Plot saved to {save_path}")
        
        if show_plot:
            plt.show()
        else:
            plt.close()


# Backward compatibility aliases
class electrical(Electrical):
    """Alias for backward compatibility."""
    pass


# Convenience functions for simple use cases
def quick_iv_sweep(dac_controller, adc_controller, dac_channel: int, adc_channel: int,
                  start_value: int, end_value: int, steps: int,
                  shunt_resistors: List[float] = None) -> Dict:
    """
    Quick IV curve sweep without OPM.
    
    Returns:
        Dictionary with: voltages, currents, and analysis results
    """
    elec = Electrical(shunt_resistors or [1.0, 1.0, 1.0, 1.0])
    
    voltages = []
    currents = []
    
    step_size = (end_value - start_value) / (steps - 1) if steps > 1 else 0
    
    for i in range(steps):
        dac_value = int(start_value + i * step_size)
        dac_controller.set_dac(dac_channel, dac_value, verbose=False)
        time.sleep(0.1)
        
        voltage = adc_controller.read_voltage(adc_channel, verbose=False) or 0.0
        current = elec.calculate_current_from_shunt(voltage, adc_channel)
        
        voltages.append(voltage)
        currents.append(current)
    
    analysis = elec.analyze_iv_curve(voltages, currents)
    
    return {
        'voltages': voltages,
        'currents': currents,
        **analysis
    }
        