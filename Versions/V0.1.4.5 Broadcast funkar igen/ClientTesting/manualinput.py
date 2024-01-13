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
		message = ""
		while True:
			message = input()
			message = message + "\0"
			if message == "quit":
				client_socket.send(message.encode('utf-8'))
				client_socket.close()
				exit()
			elif message:
				client_socket.send(message.encode('utf-8'))
		# Send "Hello" to the server
		exit()
	except Exception as e:
		print(e)
		
def recieve(client_socket):
	try:
		while True:
			#lock.acquire()
			text = client_socket.recv(1024).decode('utf-8','ignore')
			if text.startswith("Welcome"):
				system("clear")
				print(text)
			#	lock.release()
			elif text.startswith("Invalid"):
				system("clear")
				print(text)
			#	lock.release()
			elif text:
				print(text)
			#	lock.release()
			else:
				pass
			#	lock.release()
	except Exception as e:
		print(e)

		
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client_socket.connect((server_address, server_port))

sending_thread = threading.Thread(target=sending, args=(client_socket,))
recieve_thread = threading.Thread(target=recieve,args=(client_socket,))
sending_thread.start()
recieve_thread.start()

