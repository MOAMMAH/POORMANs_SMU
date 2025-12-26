import uart_com
import keysight_opm


keysight_visa = '129.82.224.199'

if __name__ == "__main__":
    dac = uart_com.DACController(port="COM3", baud=115200, verbose=True)
    adc = uart_com.ADCController(port="COM3", baud=115200, verbose=True)
    opm = keysight_opm.KeysightOPM(visa_addr=keysight_visa)

    #test opm 
    print(opm.get_power_all())
    