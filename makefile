#makefile for Homework 5

hw5: hw5.c
	gcc -o hw5 hw5.c

clean:
	rm hw5
    
tar:
	tar -cf hw5.tar hw5.c makefile
