import socket
import time
import threading
import random

server_address = "192.168.1.20"
server_port = 7890
errors = set()

blue = "\u001b[34m"
green = "\u001b[32m"
color = green
username = "Linux"

def send_hello(username):
	counter = 0
	try:
		while True:
			# Connect to the server
			client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
			client_socket.connect((server_address, server_port))
			
			#Enter username
			text = client_socket.recv(16).decode()
			print(text)
			client_socket.send(username.encode('utf-8'))
			## Authorizaton Message
			text = client_socket.recv(16).decode()
			print(text)
			# Welcome Message
			text = client_socket.recv(25).decode()
			print(text)

			while True:
				message = f"This is Message #{str(counter)}" 
				counter += 1
				client_socket.send(message.encode('utf-8', 'replace'))
				text = client_socket.recv(1024).decode('utf-8','replace')
				print(text)
				time.sleep(0.5)					
				
			print("Disconnecting!")
			client_socket.close()
	
	except Exception as e:
		pass
				
while True:
	send_hello(username)


#client_thread = threading.Thread(target=send_hello, args=(username,))
#client_thread.start()
