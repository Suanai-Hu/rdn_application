.SUFFIXES: .c .o 

LIB_FLAG = -fPIC
TARGET = librdnapi.so
OBJ = rdn_api_lib.o

all:$(TARGET)

$(TARGET):$(OBJ)
	$(CC) -shared -o $@ $(OBJ)

.c.o:
	$(CC) $(LIB_FLAG) $(INC) -c $*.c -o $*.o
	
install: $(TARGET)
	mkdir -p ../install/lib
	mkdir -p ../install/bin
	mkdir -p ../../../install/rdnapi/lib/
	mkdir -p ../../../install/rdnapi/inc/
	install $(TARGET) ../install/lib/
	install $(TARGET) ../../../install/rdnapi/lib/
	echo "install $(TARGET) ok====>"
	
.PRONY:clean
clean:
	rm -f $(OBJ) $(TARGET)
	rm -f ../install/lib/$(TARGET)
	rm -f ../../../install/rdnapi/lib/$(TARGET)
	echo "clean $(TARGET) ok====>"
	
