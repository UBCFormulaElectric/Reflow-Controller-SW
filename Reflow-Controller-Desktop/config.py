USB_READ_T = 200
USB_WRITE_T = 500

DEFAULT_SOAK_RISE_RATE = 0.3
DEFAULT_SOAK_TIME = 200
DEFAULT_SOAK_START_TEMP = 100
DEFAULT_SOAK_END_TEMP = 100
DEFAULT_REFLOW_RISE_RATE = 0.3
DEFAULT_REFLOW_TIME = 10
DEFAULT_REFLOW_TEMP = 150

AMBIENT_TEMP = 25
COOLDOWN_RATE = 10

DEFAULT_Kp = 1
DEFAULT_Ki = 0

COMMAND_HEAT = "Heat\n"
COMMAND_IDLE = "Idle\n"
COMMAND_END = "End\n"
COMMAND_CONNECTED = "Connected\n"
COMMAND_DISCONNECTED = "Disconnected\n"

OFFSET = 0

def setDefaults(mw):
    mw.sb_soakTime.setMaximum(1000)
    mw.sb_soakStartTemp.setMaximum(1000)
    mw.sb_soakEndTemp.setMaximum(1000)
    mw.sb_reflowTime.setMaximum(1000)
    mw.sb_reflowTemp.setMaximum(1000)

    mw.sb_soakRiseRate.setMinimum(0.01)
    mw.sb_reflowRiseRate.setMinimum(0.01)

    mw.sb_soakRiseRate.setValue(DEFAULT_SOAK_RISE_RATE)
    mw.sb_soakTime.setValue(DEFAULT_SOAK_TIME)
    mw.sb_soakStartTemp.setValue(DEFAULT_SOAK_START_TEMP)
    mw.sb_soakEndTemp.setValue(DEFAULT_SOAK_END_TEMP)
    mw.sb_reflowRiseRate.setValue(DEFAULT_REFLOW_RISE_RATE)
    mw.sb_reflowTime.setValue(DEFAULT_REFLOW_TIME)
    mw.sb_reflowTemp.setValue(DEFAULT_REFLOW_TEMP)

    mw.sb_Kp.setValue(DEFAULT_Kp)
    mw.sb_Ki.setValue(DEFAULT_Ki)



