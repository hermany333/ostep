CC = gcc
CFLAGS = -Wall -pthread

SRCS = pzip.c jobbuf.c
OBJS = $(SRCS:.c=.o)

TARGET = pzip

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# To make something.o you need something.c. Then compile src -> obj
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
