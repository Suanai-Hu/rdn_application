LIB=-lpthread
LIB+=-L../../install/rdnapi/lib -lrdnapi
LIB+=-L../../install/rdnmq/lib -lrdnmq

INC=-I ../../install/uapi
INC+=-I ../../install/rdnapi/inc
INC+=-I ../../install/rdnmq/inc

SRC=main.c
SRC+=rdn_robot_info.c
SRC+=rdn_battery_info.c
SRC+=rdn_temp_info.c
SRC+=rdn_gps.c
SRC+=rdn_error_info.c

TARGET = rdnupdateinfo

all: rdnupdateinfo

rdnupdateinfo: $(SRC)
	$(CC) -o $@ $^ $(INC) $(LIB)

install: $(TARGET)
	$(TARGET_DIR)/usr/bin/
	echo "install $(TARGET) to /usr/bin ok====>"

.PHONY:clean
clean:
	rm -f $(TARGET)  *.o
	rm -f $(TARGET_DIR)/usr/bin/$(TARGET)
	echo "clean $(TARGET) ok====>"
