import sys
from PyQt5 import QtWidgets as qtw
import PyQt5.QtCore as core
import matplotlib
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg, NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure
import serial
import serial.tools.list_ports
from time import time

from config import *
from PIController import Controller


class MplCanvas(FigureCanvasQTAgg):

    def __init__(self, parent=None, width=5, height=5, dpi=100):
        fig = Figure(figsize=(width, height), dpi=dpi)
        fig.subplots_adjust(left=0.1, right=0.95, top=0.95, bottom=0.1)

        self.axes = fig.add_subplot(111)
        super(MplCanvas, self).__init__(fig)


class MainWindow(qtw.QMainWindow):

    def __init__(self, *args, **kwargs):
        super(MainWindow, self).__init__(*args, **kwargs)

        graph = self.GUI_createPlot()
        profile = self.GUI_createProfileSettings()
        serialport = self.GUI_createSerialSettings()
        pi = self.GUI_createPiControlSettings()
        status = self.GUI_createStatus()

        settingsTabs = qtw.QTabWidget()
        settingsTabs.addTab(profile, "Reflow Profile")
        settingsTabs.addTab(serialport, "Serial Port")
        settingsTabs.addTab(pi, "PI Controller")
        statusTabs = qtw.QTabWidget()
        statusTabs.addTab(status, "Status")
        graphTabs = qtw.QTabWidget()
        graphTabs.addTab(graph, "Reflow Graph")

        container = qtw.QWidget()
        container.setLayout(qtw.QGridLayout())
        container.layout().addWidget(graphTabs, 0, 0, 1, 1)
        container.layout().addWidget(settingsTabs, 0, 1, 1, 1)
        container.layout().addWidget(statusTabs, 1, 0, 1, 2)

        container.layout().setColumnStretch(0, 30)
        container.layout().setColumnStretch(1, 15)
        container.layout().setRowStretch(0, 60)
        container.layout().setRowStretch(1, 10)

        self.ser = None
        self.isReflowing = False
        self.startTime = 0
        self.elapsedTime = 0
        self.currentTemp = 0
        self.sentEndCommand = False
        self.xMeasured = []
        self.yMeasured = []
        self.xProfile = []
        self.yProfile = []

        setDefaults(self)
        self.control = Controller()
        self.callback_updateControllerConstants()
        self.callback_updateProfilePlotData()
        self.init_MakeBindings()

        timerRead = core.QTimer(self)
        timerRead.timeout.connect(self.update_readTemp)
        timerRead.start(USB_READ_T)

        timerWrite = core.QTimer(self)
        timerWrite.timeout.connect(self.update_write)
        timerWrite.start(USB_WRITE_T)

        self.setCentralWidget(container)
        self.show()

    def update_readTemp(self):
        if not self.ser_IsConnected():
            return

        readSuccessful, inputData = self.ser_Read()
        if not readSuccessful:
            return

        parsedTemp = 0.0
        try:
            _, tempStr, _ = inputData.split(" ")
            parsedTemp = float(tempStr)
        except:
            self.writeToConsole("Parsing message from device failed")
            return

        self.currentTemp = parsedTemp
        self.le_currentTemp.setText(str(parsedTemp))

        if not self.isReflowing:
            return

        self.elapsedTime = time() - self.startTime
        self.le_elapsedTime.setText("%.2f" % self.elapsedTime)
        progress = round(self.elapsedTime / self.endReflowTime * 100)
        progress = min(progress, 100)
        self.pb_reflowProgress.setValue(progress)
        self.yMeasured = self.yMeasured + [self.currentTemp]
        self.xMeasured = self.xMeasured + [self.elapsedTime]
        self.updatePlot()

    def update_write(self):
        if not self.isReflowing:
            return

        # Find correct temperature set point based on elapsed time
        temp, stage = self.getTargetData()
        self.le_targetTemp.setText("%.2f" % temp)
        self.le_stage.setText(stage)

        # Do PI control loop and write result to controller
        if not self.sentEndCommand:
            control = self.control.update(temp, self.currentTemp)
            self.setSSR(control >= 0)

    # ------------------ Reflow Helper Functions -----------------#

    def stopReflow(self):
        if self.isReflowing:
            self.writeToConsole("Reflow stopped")

        self.ser_Write(COMMAND_IDLE)
        self.pb_reflowProgress.setValue(0)
        self.b_start.setText("Start")
        self.isReflowing = False

    def setSSR(self, isOpen):
        if not self.ser_IsConnected():
            self.writeToConsole("Can't set SSR because device is not connected")
            return

        if isOpen:
            msg = COMMAND_HEAT
            status = "SSR closed, heating up."
        else:
            msg = COMMAND_IDLE
            status = "SSR open, cooling down."

        self.le_status.setText(status)
        if not self.ser_Write(msg):
            self.writeToConsole("Reflow failed: Writing to device failed")

    def getTargetData(self):
        t = time() - self.startTime + OFFSET
        stage = ""
        temp = 0

        if 0 <= t < self.startSoakTime:
            temp = self.sb_soakRiseRate.value() * t + AMBIENT_TEMP
            stage = "Heat up to soak"
        elif t < self.endSoakTime:
            m = (self.sb_soakEndTemp.value() - self.sb_soakStartTemp.value()) / (self.endSoakTime - self.startSoakTime)
            temp = m * (t - self.startSoakTime) + self.sb_soakStartTemp.value()
            stage = "Soak"
        elif t < self.startReflowTime:
            temp = self.sb_reflowRiseRate.value() * (t - self.endSoakTime) + self.sb_soakEndTemp.value()
            stage = "Heat up to reflow"
        elif t < self.endReflowTime:
            temp = self.sb_reflowTemp.value()
            stage = "Reflow"
        elif t > self.endReflowTime:
            temp = AMBIENT_TEMP
            stage = "Reflow finished, open door"

            if not self.sentEndCommand:
                self.ser_Write(COMMAND_END)
                self.sentEndCommand = True

        return temp, stage

    def updatePlot(self):
        self.canvas.axes.cla()
        self.canvas.axes.plot(self.xMeasured, self.yMeasured, 'r')
        self.canvas.axes.plot(self.xProfile, self.yProfile, 'b')
        self.canvas.axes.set_title("Reflow Oven Temperature")
        self.canvas.axes.set_xlabel("Time (s)")
        self.canvas.axes.set_ylabel("Temperature (C)")
        self.canvas.draw()

    def writeToConsole(self, msg):
        if self.te_console is not None:
            self.te_console.append(msg)

    # ----------- Serial Communication Functions -----------------#
    # Help with connecting and communicating with the device via USB serial. Uses PySerial library.

    def ser_Read(self):
        inputData = ""
        try:
            inputData = self.ser.readline().decode("utf-8")
            inputData = inputData.replace("\n", "")
            self.writeToConsole("In: " + inputData)
        except:
            self.writeToConsole("Reading from device failed")
            return False, ""

        return True, inputData

    def ser_Write(self, msg):
        try:
            self.ser.write(msg.encode('utf-8'))
        except:
            self.writeToConsole("Writing to device failed")
            return False

        return True

    def ser_IsConnected(self):
        return self.ser is not None and self.ser.isOpen()

    def ser_TryConnect(self):
        if self.ser_IsConnected():
            self.ser_Disconnect()
            return

        try:
            port = str(self.cb_port.currentText())
            self.ser = serial.Serial(port, baudrate=9600, timeout=1)
            self.le_portstatus.setText("Connected to " + port)
            self.le_stage.setText("Connected to " + port)
            self.le_status.setText("")
            self.writeToConsole("Connected to " + port)
            self.b_connectButton.setText("Disconnect")
            self.ser_Write(COMMAND_CONNECTED)
        except:
            self.ser_Disconnect()
            self.writeToConsole("Could not connect to " + port)

    def ser_Disconnect(self):
        if self.ser is not None:
            self.ser_Write(COMMAND_DISCONNECTED)
            self.ser.close()

        self.stopReflow()
        self.le_portstatus.setText("Disconnected")
        self.le_stage.setText("Disconnected")
        self.le_status.setText("")
        self.writeToConsole("Disconnected from port")
        self.b_connectButton.setText("Connect")

    def ser_GetComPorts(self):
        self.cb_port.clear()

        for port, _, _ in sorted(serial.tools.list_ports.comports()):
            self.cb_port.addItem(port)

    # --------------------- Callback Functions -------------------#
    # Functions called when GUI elements are interacted with, such as buttons or text fields.

    def callback_tryStartReflow(self):
        if self.isReflowing:
            self.ser_Write(COMMAND_CONNECTED)
            self.stopReflow()
            return

        if not self.ser_IsConnected():
            self.writeToConsole("Reflow could not start: Device is not connected")
            return

        self.ser_Write(COMMAND_CONNECTED)
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()
        self.startTime = time()
        self.xMeasured = []
        self.yMeasured = []
        self.writeToConsole("Reflow started")
        self.b_start.setText("Stop")
        self.sentEndCommand = False
        self.isReflowing = True

    def callback_changeTime(self, timeAmount):
        if not self.ser_IsConnected():
            self.writeToConsole("Can't increment time because device is not connected")
            return

        if not self.isReflowing:
            self.writeToConsole("Can't increment time because not currently reflowing")
            return

        self.startTime -= timeAmount

    def callback_updateProfilePlotData(self):
        self.startSoakTime = (self.sb_soakStartTemp.value() - AMBIENT_TEMP) / self.sb_soakRiseRate.value()
        self.endSoakTime = self.startSoakTime + self.sb_soakTime.value()
        self.startReflowTime = (
                                           self.sb_reflowTemp.value() - self.sb_soakEndTemp.value()) / self.sb_reflowRiseRate.value() \
                               + self.endSoakTime
        self.endReflowTime = self.startReflowTime + self.sb_reflowTime.value()
        self.coolDownTime = (self.sb_reflowTemp.value() - AMBIENT_TEMP) / COOLDOWN_RATE + self.endReflowTime

        self.xProfile = [0, self.startSoakTime, self.endSoakTime, self.startReflowTime, self.endReflowTime,
                         self.coolDownTime]
        self.yProfile = [AMBIENT_TEMP, self.sb_soakStartTemp.value(), self.sb_soakEndTemp.value(),
                         self.sb_reflowTemp.value(),
                         self.sb_reflowTemp.value(), AMBIENT_TEMP]

        self.updatePlot()

    def callback_updateControllerConstants(self):
        if self.control is None:
            self.control = Controller()

        self.control.Kp = self.sb_Kp.value()
        self.control.Ki = self.sb_Ki.value()
        self.control.Kd = self.sb_Kd.value()

    # ----------------- Initialization Functions -----------------#

    def init_MakeBindings(self):
        self.sb_soakRiseRate.valueChanged.connect(self.callback_updateProfilePlotData)
        self.sb_soakTime.valueChanged.connect(self.callback_updateProfilePlotData)
        self.sb_soakStartTemp.valueChanged.connect(self.callback_updateProfilePlotData)
        self.sb_soakEndTemp.valueChanged.connect(self.callback_updateProfilePlotData)
        self.sb_reflowRiseRate.valueChanged.connect(self.callback_updateProfilePlotData)
        self.sb_reflowTime.valueChanged.connect(self.callback_updateProfilePlotData)
        self.sb_reflowTemp.valueChanged.connect(self.callback_updateProfilePlotData)

        self.sb_Kp.valueChanged.connect(self.callback_updateControllerConstants)
        self.sb_Ki.valueChanged.connect(self.callback_updateControllerConstants)
        self.sb_Kd.valueChanged.connect(self.callback_updateControllerConstants)

    # ----------- GUI Element Creation Functions -----------------#
    # These functions only create the actual GUI elements using PyQt5.

    def GUI_createPlot(self):
        self.canvas = MplCanvas(self, width=10, height=10, dpi=100)
        toolbar = NavigationToolbar(self.canvas, self)

        container = qtw.QWidget()
        container.setLayout(qtw.QGridLayout())
        container.layout().addWidget(toolbar)
        container.layout().addWidget(self.canvas)
        return container

    def GUI_createProfileSettings(self):
        heat2SoakForm, [self.sb_soakRiseRate] = self.GUI_createFormBox("Heat to Soak", ["Rise rate (C/s)"])
        soakForm, [self.sb_soakTime, self.sb_soakStartTemp, self.sb_soakEndTemp] = \
            self.GUI_createFormBox("Soak", ["Time (s): ", "Start temperature (C): ", "End temperature (C): "])
        heat2ReflowForm, [self.sb_reflowRiseRate] = self.GUI_createFormBox("Heat to Reflow", ["Rise rate (C/s)"])
        reflowForm, [self.sb_reflowTime, self.sb_reflowTemp] = \
            self.GUI_createFormBox("Reflow", ["Time (s): ", "Temperature (C): "])

        layout = qtw.QVBoxLayout()
        layout.addWidget(heat2SoakForm)
        layout.addWidget(soakForm)
        layout.addWidget(heat2ReflowForm)
        layout.addWidget(reflowForm)
        layout.addWidget(self.GUI_growSpace())
        container = qtw.QWidget()
        container.setLayout(layout)
        return container

    def GUI_createPiControlSettings(self):
        piForm, [self.sb_Kp, self.sb_Ki, self.sb_Kd] = \
            self.GUI_createFormBox("PI Controller Constants", ["K proportional", "K integral", "K derivative"])

        layout = qtw.QVBoxLayout()
        layout.addWidget(piForm)
        layout.addWidget(self.GUI_growSpace())
        container = qtw.QWidget()
        container.setLayout(layout)
        return container

    def GUI_createSerialSettings(self):
        self.cb_port = qtw.QComboBox()
        refreshButton = qtw.QPushButton("Refresh")
        refreshButton.clicked.connect(self.ser_GetComPorts)
        self.b_connectButton = qtw.QPushButton("Connect")
        self.b_connectButton.clicked.connect(self.ser_TryConnect)
        self.le_portstatus = qtw.QLineEdit("Disconnected")
        self.le_portstatus.setReadOnly(True)

        serialPortBox = qtw.QGroupBox("Select Port")
        serialPortBox.setLayout(qtw.QGridLayout())
        serialPortBox.layout().addWidget(qtw.QLabel("Port: "), 0, 0)
        serialPortBox.layout().addWidget(self.cb_port, 0, 1)
        serialPortBox.layout().addWidget(refreshButton, 0, 2)
        serialPortBox.layout().addWidget(qtw.QLabel("Status: "), 1, 0)
        serialPortBox.layout().addWidget(self.le_portstatus, 1, 1, 1, 2)
        serialPortBox.layout().addWidget(self.b_connectButton, 2, 0, 1, 3)
        serialPortBox.layout().setColumnStretch(0, 10)
        serialPortBox.layout().setColumnStretch(1, 20)
        serialPortBox.layout().setColumnStretch(2, 10)

        self.te_console = qtw.QTextEdit()
        self.te_console.setReadOnly(True)
        self.te_console.setLineWrapMode(qtw.QTextEdit.NoWrap)
        clearConsoleButton = qtw.QPushButton("Clear")
        clearConsoleButton.clicked.connect(lambda: self.te_console.setText(""))

        consoleBox = qtw.QGroupBox("Port Console")
        consoleBox.setLayout(qtw.QVBoxLayout())
        consoleBox.layout().addWidget(self.te_console)
        consoleBox.layout().addWidget(clearConsoleButton)

        container = qtw.QWidget()
        container.setLayout(qtw.QVBoxLayout())
        container.layout().addWidget(serialPortBox)
        container.layout().addWidget(consoleBox)
        return container

    def GUI_createStatus(self):
        self.le_currentTemp = qtw.QLineEdit("0")
        self.le_currentTemp.setReadOnly(True)
        self.le_targetTemp = qtw.QLineEdit("0")
        self.le_targetTemp.setReadOnly(True)
        self.le_elapsedTime = qtw.QLineEdit("0")
        self.le_elapsedTime.setReadOnly(True)
        self.le_stage = qtw.QLineEdit("Disconnected")
        self.le_stage.setReadOnly(True)
        self.le_status = qtw.QLineEdit("")
        self.le_status.setReadOnly(True)
        self.pb_reflowProgress = qtw.QProgressBar()
        self.pb_reflowProgress.setValue(0)

        self.b_start = qtw.QPushButton("Start")
        self.b_start.clicked.connect(self.callback_tryStartReflow)
        self.b_start.setSizePolicy(
            qtw.QSizePolicy.Preferred,
            qtw.QSizePolicy.Expanding)

        b_plus5 = qtw.QPushButton("+5s")
        b_plus5.clicked.connect(lambda: self.callback_changeTime(5))
        b_minus5 = qtw.QPushButton("-5s")
        b_minus5.clicked.connect(lambda: self.callback_changeTime(-5))

        b_heat = qtw.QPushButton("Heat up (close SSR)")
        b_heat.clicked.connect(lambda: self.setSSR(True))
        b_cool = qtw.QPushButton("Cool down (open SSR)")
        b_cool.clicked.connect(lambda: self.setSSR(False))

        currentValues = qtw.QWidget()
        currentValues.setLayout(qtw.QGridLayout())
        currentValues.layout().addWidget(qtw.QLabel("Current temperature (C): "), 0, 0)
        currentValues.layout().addWidget(self.le_currentTemp, 0, 1)
        currentValues.layout().addWidget(qtw.QLabel("Target temperature (C): "), 1, 0)
        currentValues.layout().addWidget(self.le_targetTemp, 1, 1)
        currentValues.layout().addWidget(qtw.QLabel("Elapsed time (s): "), 2, 0)
        currentValues.layout().addWidget(self.le_elapsedTime, 2, 1)
        currentValues.layout().setColumnStretch(0, 2)
        currentValues.layout().setColumnStretch(1, 2)

        status = qtw.QWidget()
        status.setLayout(qtw.QGridLayout())
        status.layout().addWidget(qtw.QLabel("Current stage: "), 0, 0)
        status.layout().addWidget(self.le_stage, 0, 1)
        status.layout().addWidget(qtw.QLabel("Current status: "), 1, 0)
        status.layout().addWidget(self.le_status, 1, 1)
        status.layout().addWidget(self.pb_reflowProgress, 2, 0, 1, 2)

        controls = qtw.QWidget()
        controls.setLayout(qtw.QGridLayout())
        controls.layout().addWidget(b_plus5, 0, 0)
        controls.layout().addWidget(b_minus5, 1, 0)
        controls.layout().addWidget(b_heat, 2, 0)
        controls.layout().addWidget(b_cool, 3, 0)

        layout = qtw.QGridLayout()
        layout.addWidget(currentValues, 0, 0, 3, 1)
        layout.addWidget(status, 0, 1, 3, 1)
        layout.addWidget(controls, 0, 2, 3, 1)
        layout.addWidget(self.b_start, 0, 3, 3, 1)

        layout.setColumnStretch(0, 10)
        layout.setColumnStretch(1, 12)
        layout.setColumnStretch(2, 5)
        layout.setColumnStretch(3, 5)

        container = qtw.QWidget()
        container.setLayout(layout)
        return container

    def GUI_createFormBox(self, title, fieldTitles=[]):
        form = qtw.QGroupBox(title)
        form.setLayout(qtw.QGridLayout())
        fieldWidgets = [None] * len(fieldTitles)

        for i in range(0, len(fieldTitles)):
            sb = qtw.QDoubleSpinBox()
            form.layout().addWidget(qtw.QLabel(fieldTitles[i]), i, 0)
            form.layout().addWidget(sb, i, 1)
            fieldWidgets[i] = sb

        return form, fieldWidgets

    def GUI_growSpace(self):
        spacer = qtw.QWidget()
        spacer.setSizePolicy(
            qtw.QSizePolicy.Preferred,
            qtw.QSizePolicy.Expanding)
        return spacer


# TODO
# Document

# Implement end condition
# Anti-flicker and anti-windup
# Pausing?

matplotlib.use('Qt5Agg')
app = qtw.QApplication(sys.argv)
app.setStyle('Fusion')
w = MainWindow()
w.resize(1500, 800)
app.exec_()
