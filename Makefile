objects = main.o blowfish.o 
INCLUDE_DIR = ./include
CFLAGS = -I./include

client : $(objects) 
	cc -o client $(objects) $(CFLAGS)

main.o : $(INCLUDE_DIR)/blowfish.h
blowfish.o : $(INCLUDE_DIR)/blowfish.h 

.PHONY : clean 
clean : 
	rm client $(objects) 
