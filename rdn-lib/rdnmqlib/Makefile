.SUFFIXES: .c .o 

LIB_FLAG = -fPIC
TARGET = librdnmq.so
OBJ = rdn_mq_lib.o

INC=-I ../../install/uapi

all:$(TARGET)

$(TARGET):$(OBJ)
	$(CC) -shared -o $@ $(OBJ)

.c.o:
	$(CC) $(LIB_FLAG) $(INC) -c $*.c -o $*.o
	
install: $(TARGET)
	mkdir -p ../../install/rdnmq/lib/
	mkdir -p ../../install/rdnmq/inc/
	install $(TARGET) ../../install/rdnmq/lib/
	echo "install $(TARGET) ok====>"
	
.PRONY:clean
clean:
	rm -f $(OBJ) $(TARGET)
	rm -f ../../install/rdnmq/lib/$(TARGET)
	echo "clean $(TARGET) ok====>"
	
