user1:
	gcc -o user1 user1.c k_socket.c -lpthread
	./user1 127.0.0.1 5000 127.0.0.1 6000

user2:
	gcc -o user2 user2.c k_socket.c -lpthread
	./user2 127.0.0.1 6000 127.0.0.1 5000

clean:
	rm user1 user2 received_input.txt