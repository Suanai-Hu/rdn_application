INC=-I ../../install/rdnapi/inc
INC+=-I ../../install/uapi/
LIB+=-L ../../install/rdnapi/lib -lrdnapi

TARGET = rdnmonitor

all: rdnmonitor

rdnmonitor: rdn_monitor.c
	$(CC) -o $@ $^ $(INC) $(LIB)
	
install: $(TARGET)
	$(TARGET_DIR)/usr/bin/
	echo "install $(TARGET) to /usr/bin ok====>"

clean:
	rm -rf *.o
	rm -rf rdnmonitor

