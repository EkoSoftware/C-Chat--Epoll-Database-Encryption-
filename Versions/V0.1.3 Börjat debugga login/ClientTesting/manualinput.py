import socket
import time
import threading
import random
from os import system

server_address = "192.168.1.20"
server_port = 7890
lock = threading.Lock()

def sending(client_socket):
	try:
		username = input()
		username = username + "\0"
		client_socket.send(username.encode('utf-8'))
	# Create a socket object
		while True:
			message = input()
			message = message + "\0"
			client_socket.send(message.encode('utf-8'))
		# Send "Hello" to the server

	except Exception as e:
		print(e)
		
def recieve(client_socket):
	while True:
		lock.acquire()
		text = client_socket.recv(1024).decode('utf-8','ignore')
		if text.startswith("Welcome"):
			system("clear")
			print(text)
			lock.release()
			continue
		elif text.startswith("Invalid"):
			system("clear")
			print(text)
			lock.release()
			continue
		elif text:
			print(text)
			lock.release()
		
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client_socket.connect((server_address, server_port))

sending_thread = threading.Thread(target=sending, args=(client_socket,))
recieve_thread = threading.Thread(target=recieve,args=(client_socket,))
recieve_thread.start()
sending_thread.start()

