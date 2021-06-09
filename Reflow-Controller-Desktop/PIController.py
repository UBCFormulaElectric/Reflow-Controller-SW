from config import *

# PID source from:
# https://www.youtube.com/watch?v=zOByx3Izf5U&ab_channel=Phil%E2%80%99sLab
class Controller:
    def __init__(self):
        self.Kp = DEFAULT_Kp
        self.Ki = DEFAULT_Ki
        self.T = USB_WRITE_T
        self.maxI = 15

        self.integralTerm = 0
        self.prevError = 0

    def update(self, target, measurement):
        # Error signal
        error = target - measurement

        # Proportional term
        proportional = self.Kp * error

        # Integral term
        self.integralTerm = self.integralTerm + 0.5 * self.Ki * self.T / 1000 * (error + self.prevError)

        # Anti-windup
        if self.integralTerm > self.maxI:
            self.integralTerm = self.maxI
        elif self.integralTerm < -self.maxI:
            self.integralTerm = -self.maxI

        # print("P: " + str(proportional) + ", I: " + str(self.integralTerm))

        self.prevError = error
        return proportional + self.integralTerm
