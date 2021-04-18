CC = gcc

TARGET = myls
QUEUE = myQueue

all: $(TARGET)

$(TARGET): $(TARGET).c $(QUEUE).*
	$(CC) -o $(TARGET) $(TARGET).c $(QUEUE).c

clean: 
	$(RM) $(TARGET)