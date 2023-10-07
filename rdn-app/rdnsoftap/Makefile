LIB=-L../../install/rdnapi/lib -lrdnapi
INC=-I ../../install/rdnapi/inc
INC+=-I ../../install/uapi
TARGET = rdnsoftap

all: rdnsoftap

rdnsoftap: rdn_softap.c
	$(CC) -o $@ $^ $(INC) $(LIB)

install: $(TARGET)
	$(TARGET_DIR)/usr/bin/
	echo "install $(TARGET) to /usr/bin ok====>"

.PHONY:clean
clean:
	rm -f $(TARGET)  *.o
	rm -f $(TARGET_DIR)/usr/bin/$(TARGET)
	echo "clean $(TARGET) ok====>"

