#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-
# title: I2C Master Script
# desription: Periodically requests information from Safe Shutdown v2.0
# author: Camble
# date 2016-11-15
# version: 0.1
# source: https://github.com/Camble/Safe-Shutdown-Microcontroller

import smbus
import struct
import time

bus = smbus.SMBus(1)
system_state = ""
voltage = 0
warn_voltage = 3.3
shutdown_voltage = 3.2
address = 0x04

def configSlave(value):
  # write some settings to the slave via I2C
  # bus.write_byte(address, value)
  return -1

def getState():
  system_state = ""
  for i in range(len(system_state)):
    system_state[i] += chr(bus.read_byte(address))

def compareVoltage():
  # warn if warn_voltage reached
  # shutdown shutdown_voltage reached

while True:
  time.sleep(1)
  getVoltage()
  compareVoltage()

  print "system_state contents: " + str(system_state)
