INC=-I ../../install/rdnapi/inc
INC+=-I ../../install/uapi/
INC+=-I ../../install/curl/inc
INC+=-I ../../install/openssl/inc
INC+=-I ../../install/json-c/inc

LIB=-L ../../install/curl/lib -lcurl
LIB+=-L ../../install/json-c/lib -ljson-c
LIB+=-L ../../install/openssl/lib -lssl -lcrypto
LIB+=-L ../../install/rdnapi/lib -lrdnapi

TARGET = rdncloud

all: rdncloud

rdncloud: rdn_cloud.c
	$(CC) -o $@ $^ $(INC) $(LIB)
	
install: $(TARGET)
	$(TARGET_DIR)/usr/bin/
	echo "install $(TARGET) to /usr/bin ok====>"

clean:
	rm -rf *.o
	rm -rf rdncloud

