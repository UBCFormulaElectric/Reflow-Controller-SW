from config import *

# PID source from:
# https://www.youtube.com/watch?v=zOByx3Izf5U&ab_channel=Phil%E2%80%99sLab
class Controller:
    def __init__(self):
        self.Kp = DEFAULT_Kp
        self.Ki = DEFAULT_Ki
        self.Kd = DEFAULT_Kd
        self.T = USB_WRITE_T
        self.maxI = 15

        self.integralTerm = 0
        self.derivativeTerm = 0
        self.prevError = 0

    def update(self, target, measurement):
        # Error signal
        error = target - measurement

        # Proportional term
        proportional = self.Kp * error

        # Integral term
        self.integralTerm = (self.integralTerm + error) * self.Ki

        # Anti-windup
        if self.integralTerm > self.maxI:
            self.integralTerm = self.maxI
        elif self.integralTerm < -self.maxI:
            self.integralTerm = -self.maxI

        # Derivative
        self.derivativeTerm = (error - self.prevError) * self.Kd

        # print("P: " + str(proportional) + ", I: " + str(self.integralTerm))
        self.prevError = error
        return proportional + self.integralTerm
