TARGET = call_rdn_api
LIB += -L../../../install/xml/lib -lxml2 
LIB += -L../../../install/rdnapi/lib/ -lrdnapi
INC += -I ../../../install/xml/inc
INC += -I ../include/

.PHONY:all

all:$(TARGET) 

call_rdn_api:call_rdn_api.c 
	$(CC) -o $@ $^ $(INC) $(LIB)

install: $(TARGET)
	install $(TARGET) ../install/bin/
	echo "install $(TARGET) ok====>"

.PHONY:clean
clean:
	rm -f $(TARGET)  *.o
	rm -f ../install/bin/$(TARGET)
	echo "clean $(TARGET) ok====>"
