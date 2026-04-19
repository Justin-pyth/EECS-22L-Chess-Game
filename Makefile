CC = gcc
CFLAGS = -Wall -Wextra -O2

TARGET = chess

SRCS = Chess.c Engine.c Moves.c terminalTestingFunctions.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)