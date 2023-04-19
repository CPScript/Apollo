import time
import os
import sys
import subprocess
from os  import system
from subprocess import call
from platform import platform


# delete
puk = platform()[0], platform()[1],  platform()[2], platform()[3], platform()[4], platform()[5], platform()[6]

if puk == ('W', 'i', 'n', 'd', 'o', 'w', 's'):
    delet = 'cls'
    dr = '\\'
else:
    delet = 'clear'
    dr = '/'

os.system(delet)
time.sleep(1)
print("||============ What would you like to select =============||")
print("||========================================================||")
print("|| /|[1] Site Latency Tester   /|[4] IP address stuff     ||")
print("||  /|[2] Port Sniffer          /|[5] Simple port DDoS    ||")   
print("||   /|[3] BackDoor Creator      /|[6] add something here ||")
print("||========================================================||")
print("||======== Please enter the number of your option ========||")
print(" ")
print(" ")
choice = input( """┌──{User}~[+]
└─> """)

if choice == "1":
  os.system(delet)
  print("       /|||===== Discription =====|||\ ")
  print("<============================================>")
  print("This tool will prompt you to enter a website, once entered it will ping that website then alert you of unwanted traffic or latency.")
  print("<============================================>")
  print(" ")
  time.sleep(7)
  os.system(delet)
  print("Loading...")
  time.sleep(5)
  os.system(delet)
  print("Done!")
  os.system(delet)
  call(["python", "1.py"])
  



if choice == "2":
  os.system(delet)
  print("       /|||===== Discription =====|||\ ")
  print("<============================================>")
  print("This tool will prompt you to enter a IP address and will tell you the open ports on that IP address.")
  print("<============================================>")
  print(" ")
  time.sleep(7)
  os.system(delet)
  print("Loading...")
  time.sleep(5)
  os.system(delet)
  print("Done!")
  os.system(delet)
  call(["python", "2.py"])  



if choice == "3":
  os.system(delet)
  print("       /|||===== Discription =====|||\ ")
  print("<============================================>")
  print("tries to make an backdoor on a IP's selected port... Might not work, if it does... the rest is up to you...")
  print("<============================================>")
  print(" ")
  time.sleep(7)
  os.system(delet)
  print("Loading...")
  time.sleep(5)
  os.system(delet)
  print("Done!")
  os.system(delet)
  os.system('pip install wget')
  os.system(delet)
  call(["python", "3/Saturn.py"]) 


if choice == "4":
  os.system(delet)
  print("       /|||===== Discription =====|||\ ")
  print("<============================================>")
  print("A IPv4 & IPv6 resolver... easy to use")
  print("<============================================>")
  print(" ")
  time.sleep(7)
  os.system(delet)
  print("Loading...")
  time.sleep(5)
  os.system(delet)
  print("Done!")
  os.system('pip install ipapi')
  os.system(delet)
  call(["python", "4/finder.py"]) 


if choice == "5":
  os.system(delet)
  print("       /|||===== Discription =====|||\ ")
  print("<============================================>")
  print("A simple DDoS tool ment to take down unsecure ports on a IPv4 address.")
  print("<============================================>")
  print(" ")
  time.sleep(7)
  os.system(delet)
  print("Loading...")
  time.sleep(5)
  os.system(delet)
  print("Done!")
  os.system(delet)
  call(["python", "5.py"]) 
