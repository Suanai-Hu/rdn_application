LIB=-L../../install/rdnapi/lib -lrdnapi
LIB+=-L../../install/json-c/lib -ljson-c
LIB+=-L../../install/mosquitto/lib -lmosquitto
LIB+=-L../../install/rdnmq/lib -lrdnmq
LIB+=-lpthread

INC=-I ../../install/uapi
INC+=-I ../../install/rdnapi/inc
INC+=-I ../../install/json-c/inc
INC+=-I ../../install/mosquitto/inc
INC+=-I ../../install/rdnmq/inc

SRC=main.c
SRC+=rdn_sub_client.c
SRC+=rdn_sub_msg.c
SRC+=rdn_sub_debug_msg.c
SRC+=rdn_sub_remotectrl_msg.c
SRC+=rdn_sub_request_msg.c
SRC+=rdn_sub_setting_msg.c
SRC+=rdn_sub_ota_msg.c

SRC+=rdn_pub_client.c
SRC+=rdn_pub_msg.c

SRC+=rdn_mqtt_utility.c

TARGET = rdnmqtt

all:rdnmqtt

rdnmqtt: $(SRC)
	$(CC) -o $@ $^ $(INC) $(LIB)

install: $(TARGET)
	$(TARGET_DIR)/usr/bin/
	echo "install $(TARGET) to /usr/bin ok====>"

.PHONY:clean
clean:
	rm -f $(TARGET)  *.o
	rm -f $(TARGET_DIR)/usr/bin/$(TARGET)
	echo "clean $(TARGET) ok====>"

