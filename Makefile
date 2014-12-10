objects = main.o blowfish.o s_cmd.o r_cmd.o playback.o adpcm.o list.o
INCLUDE_DIR = ./include
CFLAGS = -I./include -lpthread -lasound

client : $(objects) 
	cc -o client $(objects) $(CFLAGS)

main.o : $(INCLUDE_DIR)/blowfish.h $(INCLUDE_DIR)/s_cmd.h $(INCLUDE_DIR)/client.h
blowfish.o : $(INCLUDE_DIR)/blowfish.h 
s_cmd.o: $(INCLUDE_DIR)/blowfish.h $(INCLUDE_DIR)/s_cmd.h 
r_cmd.o: $(INCLUDE_DIR)/blowfish.h $(INCLUDE_DIR)/r_cmd.h $(INCLUDE_DIR)/playback.h $(INCLUDE_DIR)/list.h
adpcm.o: $(INCLUDE_DIR)/adpcm.h $(INCLUDE_DIR)/list.h
playback.o: $(INCLUDE_DIR)/playback.h
list.o: $(INCLUDE_DIR)/list.h

.PHONY : clean 
clean : 
	rm client $(objects) 
