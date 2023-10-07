.PHONY:all

TARGET = rdnunload
LIB += -L ../../../install/xml/lib -lxml2
INC += -I ../../../install/xml/inc
INC += -I ../include/

all:$(TARGET) 

rdnunload:rdn_unload_shm.c 
	$(CC) -o $@ $^ $(INC)  $(LIB)

install: $(TARGET)
	install $(TARGET) ../install/bin/
	echo "install $(TARGET) ok====>"

.PHONY:clean
clean:
	rm -f $(TARGET)  *.o
	rm -f ../install/bin/$(TARGET)
	echo "clean $(TARGET) ok====>"


