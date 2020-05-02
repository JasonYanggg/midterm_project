import numpy as np
import serial
import time

name = ["little star", "little bee", "do"]
waitTime = 0.1
little_star = [42, 
  261, 261, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 261,
  392, 392, 349, 349, 330, 330, 294,
  392, 392, 349, 349, 330, 330, 294,
  261, 261, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 261,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2]
little_bee = [49, 
    392, 330, 330, 349, 294, 294,
    261, 294, 330, 349, 392, 392, 392,
    392, 330, 330, 349, 294, 294,
    261, 330, 392, 392, 330,
    294, 294, 294, 294, 294, 330, 349,
    330, 330, 330, 330, 330, 349, 392, 
    392, 330, 330, 349, 294, 294,
    261, 330, 392, 392, 261,
    1, 1, 2, 1, 1, 2, 
    1, 1, 1, 1, 1, 1, 2, 
    1, 1, 2, 1, 1, 2, 
    1, 1, 1, 1, 4, 
    1, 1, 1, 1, 1, 1, 2, 
    1, 1, 1, 1, 1, 1, 2, 
    1, 1, 2, 1, 1, 2, 
    1, 1, 1, 1, 4
]
doo = [1,
  261,
  5
]

# output formatter
formatter = lambda x: "%5d" % x

# send the waveform table to K66F
serdev = '/dev/ttyACM0'
s = serial.Serial(serdev)

while (True):
  print("Wait for signals...")
  song = int(s.readline()) - 1
  print("Start sending the note of %s" % name[song])
  if (song == 0):
    print("It may take about %d seconds ..." % (int(little_star[0] * waitTime)))
    for data in little_star:
      s.write(bytes(formatter(data), 'UTF-8'))
      time.sleep(waitTime)
  elif (song == 1):
    print("It may take about %d seconds ..." % (int(little_bee[0] * waitTime)))
    for data in little_bee:
      s.write(bytes(formatter(data), 'UTF-8'))
      time.sleep(waitTime)
  elif (song == 2):
    print("It may take about %d seconds ..." % (int(doo[0] * waitTime)))
    for data in doo:
      s.write(bytes(formatter(data), 'UTF-8'))
      time.sleep(waitTime)
  print("Signal sended\n\n---------------------------\n")
