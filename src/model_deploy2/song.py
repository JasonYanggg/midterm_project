import numpy as np
import serial
import time

name = ["little star", "little bee", "happy birthday"]
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
  1, 1, 1, 1, 1, 1, 2,
  0, 0, 1, 1, 0, 0, 1,
  0, 0, 1, 1, 0, 0, 1,
  0, 0, 1, 1, 0, 0, 1,
  0, 0, 1, 1, 0, 0, 1,
  0, 0, 1, 1, 0, 0, 1,
  0, 0, 1, 1, 0, 0, 1]
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
    1, 1, 1, 1, 4,
    0, 1, 1, 0, 1, 1,
    0, 0, 1, 1, 0, 0, 0,
    1, 0, 0, 1, 0, 0,
    1, 0, 1, 1, 1,
    0, 0, 0, 0, 0, 1, 1, 
    0, 0, 0, 0, 0, 1, 1, 
    0, 1, 1, 0, 1, 1,
    0, 1, 0, 0, 1
]
happy_birthday = [21,
  293, 330, 293, 392, 369,
  293, 330, 293, 440, 392,
  293, 554, 493, 392, 369, 330,
  523, 493, 392, 440, 392,
  1, 1, 1, 1, 2, 
  1, 1, 1, 1, 2, 
  1, 1, 1, 1, 1, 2, 
  1, 1, 1, 1, 2,
  0, 1, 0, 1, 1, 
  0, 1, 0, 1, 1, 
  0, 1, 0, 1, 1, 0, 
  0, 1, 0, 1, 1
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
    print("It may take about %d seconds ..." % (int(little_star[0] * waitTime * 3)))
    for data in little_star:
      s.write(bytes(formatter(data), 'UTF-8'))
      time.sleep(waitTime)
  elif (song == 1):
    print("It may take about %d seconds ..." % (int(little_bee[0] * waitTime * 3)))
    for data in little_bee:
      s.write(bytes(formatter(data), 'UTF-8'))
      time.sleep(waitTime)
  elif (song == 2):
    print("It may take about %d seconds ..." % (int(happy_birthday[0] * waitTime * 3)))
    for data in happy_birthday:
      s.write(bytes(formatter(data), 'UTF-8'))
      time.sleep(waitTime)
  print("Signal sended\n\n---------------------------\n")
