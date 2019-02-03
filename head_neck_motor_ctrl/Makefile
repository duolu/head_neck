IDIR = -I../include -I/usr/local/include/
CC = g++
CFLAGS = $(IDIR) -g


LDIR = 

#LIBS = -lpthread -L/usr/local/lib -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_video -lopencv_objdetect -lopencv_calib3d

LIBS = -lpthread

TARGET = head_neck_motor_ctl



OBJ = head_neck_motor_ctl.o serial.o sprotocol.o joystick.o


%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $< 

%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

all : $(TARGET)
	

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^  $(LIBS)

clean:
	rm -f *.o
	rm -f $(TARGET)