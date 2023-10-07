INC=-I ../../install/rdnapi/inc
INC+=-I ../../install/uapi
INC+=-I ../../install/json-c/inc

LIB+=-L ../../install/rdnapi/lib -lrdnapi
LIB+=-L ../../install/json-c/lib -ljson-c
LIB+=-lpthread

SRC+=main.c
SRC+=rdn_local_upgrade.c
SRC+=rdn_ota_upgrade.c

TARGET = rdnupgrade

all: rdnupgrade

rdnupgrade: $(SRC)
	$(CC) -o $@ $^ $(INC) $(LIB)
	
install: $(TARGET)
	$(TARGET_DIR)/usr/bin/
	echo "install $(TARGET) to /usr/bin ok====>"

clean:
	rm -rf *.o
	rm -rf rdnupgrade

