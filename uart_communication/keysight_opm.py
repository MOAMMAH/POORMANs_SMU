import pyvisa
import numpy as np
from time import sleep


class KeysightOPM:
    def __init__(self, visa_addr: str):
        self.rm = pyvisa.ResourceManager()
        self.inst = self.rm.open_resource(visa_addr)
        self.inst.read_termination = "\n"
        self.inst.write_termination = "\n"
        # Increase timeout to handle slow channels (default is often 5 seconds)
        self.inst.timeout = 10000  # 10 seconds in milliseconds
        self.id = self.query("*IDN?").strip()
        
        # Detect number of channels based on device model
        if "N7745" in self.id or "N7744" in self.id:
            self.max_chan = 8  # N7745C/N7744C are 8-channel
        elif "MY61C00155" in self.id:
            self.max_chan = 4
        else:
            self.max_chan = 2  # Default for most 2-channel models

    def write(self, cmd: str):
        self.inst.write(cmd)

    def query(self, cmd: str):
        return self.inst.query(cmd)

    def read_binary(self, datatype="f"):
        return self.inst.read_binary_values(datatype=datatype)

    def _check_channel(self, chan: int) -> bool:
        return 1 <= chan <= self.max_chan

    def set_unit(self, chan: int, unit: str):
        if not self._check_channel(chan): return
        if unit.lower() == "dbm":
            self.write(f"sens{chan}:pow:unit 0")
        elif unit.lower() in ("watt", "w"):
            self.write(f"sens{chan}:pow:unit 1")

    def get_unit(self, chan: int) -> str:
        if not self._check_channel(chan): return "Invalid"
        try:
            unit_code = int(self.query(f"sens{chan}:pow:unit?"))
            return "dBm" if unit_code == 0 else "Watt"
        except:
            return "dBm"  # Default assumption

    def set_wavelength(self, chan: int, wavel: float):
        if not self._check_channel(chan): return
        self.write(f"sens{chan}:pow:wav {wavel}nm")

    def get_wavelength(self, chan: int) -> float:
        if not self._check_channel(chan): return 0.0
        try:
            return float(self.query(f"sens{chan}:pow:wav?")) * 1e9
        except:
            return 0.0

    def set_auto_range(self, chan: int, state: bool):
        if not self._check_channel(chan): return
        self.write(f"sens{chan}:pow:rang:auto {1 if state else 0}")

    def is_auto_range(self, chan: int) -> bool:
        if not self._check_channel(chan): return False
        try:
            return bool(int(self.query(f"sens{chan}:pow:rang:auto?")))
        except:
            return False

    def set_range(self, chan: int, pwr_range: float | str):
        if not self._check_channel(chan): return
        if isinstance(pwr_range, str) and pwr_range.lower() == "auto":
            self.set_auto_range(chan, True)
            return
        self.set_auto_range(chan, False)
        self.write(f"sens{chan}:pow:rang {float(pwr_range)}dbm")

    def get_range(self, chan: int) -> str:
        if not self._check_channel(chan): return "Invalid"
        if self.is_auto_range(chan): return "Auto"
        try:
            return f"{float(self.query(f'sens{chan}:pow:rang?'))} dBm"
        except:
            return "Error"

    def get_power(self, chan: int, retries: int = 2) -> float | None:
        """Measure the optical power on a specific channel in the current unit."""
        if not self._check_channel(chan):
            return None
        
        # Retry logic for channels that timeout
        for attempt in range(retries + 1):
            try:
                # Use standard command format that works across Keysight models
                # Format: read{chan}:pow? (matches working implementation)
                p = float(self.query(f"read{chan}:pow?"))
                return p
            except:
                # If it's the last attempt, return None
                if attempt == retries:
                    return None
                # Otherwise, wait a bit and retry
                sleep(0.1)
        return None

    def get_power_mw(self, chan: int) -> float | None:
        if not self._check_channel(chan): return None
        orig_unit = self.get_unit(chan)
        if orig_unit == "dBm":
            self.set_unit(chan, "Watt")
            sleep(0.05)
        val = self.get_power(chan)
        if orig_unit == "dBm":
            self.set_unit(chan, "dBm")
        return val * 1000 if val is not None else None

    def get_power_all(self) -> list[float]:
        """
        Measure the optical power on all available channels in their current units.
        Uses individual channel reads for reliability (binary read can timeout).
        """
        # Use individual channel reads - more reliable than binary read
        # Binary read can timeout on some channels
        results = []
        for ch in range(1, self.max_chan + 1):
            power = self.get_power(ch, retries=2)
            results.append(power if power is not None else float("nan"))
            # Small delay between channels to avoid overwhelming the instrument
            sleep(0.05)
        return results