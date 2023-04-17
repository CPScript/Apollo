import time
import requests
import tkinter as tk

# Constants
DELAY = 1 # seconds
ALERT_THRESHOLD = 155 # 100 is bad enuff...

# Function to check server status
def check_server_status(url):
    if not url.startswith('http'): # Add 'https://' prefix if no scheme is provided
        url = 'https://' + url
    response = requests.get(url)
    latency = response.elapsed.total_seconds() * 1000
    if latency > ALERT_THRESHOLD:
        print('|!| WARNING! "{}" Is a high latency '.format(latency))      
    elif latency == 750:
        print("[!!!] Latency has passed 750! please, check your network's traffic!!!", 'WARNING: {}'.format(latency))
        return False
    else:
        print('latency: {} ms'.format(latency))
    return True

# Main loop
while True:
    url = input('Enter website to ping: ')
    if not check_server_status(url):
        break

    while True:
        time.sleep(DELAY)
        if not check_server_status(url):
            break
