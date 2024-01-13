import socket
import time
import threading
import random
MaxThreads = 8

names = { 21:"Adam Schwartz",
		22:"Mayne Robinson",
		23:"Tommy Alexander",
		24:"Luis Ball",
		25:"Francisco Simons",
		26:"Ralph Edwards"}

server_address = "192.168.1.20"
server_port = 7890
messages = [ 'wrong', 'run', 'mysterious', 'wait', 'friendly',
			 'winter', 'rather', 'nobody', 'fifty', 'select', 
			 'mine', 'pull', 'smoke', 'move', 'cry', 'popular', 
			 'stand', 'society', 'doll', 'useful', 'event', 'perhaps', 
			 'huge', 'amount', 'somebody', 'press', 'war', 'select', 
			 'except', 'got', 'lamp', 'rain', 'pretty', 
			 'mud', 'crew', 'elephant', 'two', 'tell']

def send_hello(thread, username):
	try:
	# Create a socket object
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
			message = ""
			for i in range(random.randint(1,3),random.randint(3, 7)):
				message += random.choice(messages) + ' '

			client_socket.send(message.encode('utf-8'))
			time.sleep(random.uniform(0,1))

	except ConnectionRefusedError:
		time.sleep(1)
		client_socket.connect((server_address, server_port))

def main(names):
	for thread in range(21,22 + MaxThreads):
		time.sleep(0.1)
		if thread < 27:
			username = names[thread]
		else:
			username = f"LinuxKlon #{str(21- thread)}"
		client_thread = threading.Thread(target=send_hello, args=(thread, username, ))
		client_thread.start()

if __name__ == "__main__":
    main(names)
