main : *.c
	gcc *.c -lpthread -o server -D __DEBUG__ -g 
clean : 
	rm server
