CC = gcc
CFLAGS = -g -O0 -Wall

SRCDIR = ./src
LIBS = -lm
SRCS_ = read_nldas.c
WGRIB_SRCS_ = wgrib.c

EXECUTABLE = util/read_nldas
WGRIB = util/wgrib

SRCS = $(patsubst %,$(SRCDIR)/%,$(SRCS_))
WGRIB_SRCS = $(patsubst %,$(SRCDIR)/%,$(WGRIB_SRCS_))
OBJS = $(SRCS:.c=.o)
WGRIB_OBJS = $(WGRIB_SRCS:.c=.o)

.PHONY: all clean help wgrib

all: wgrib read_nldas
	@chmod 755 ./*.sh

read_nldas: $(OBJS)
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(OBJS) $(LIBS)

wgrib: $(WGRIB_OBJS)
	$(CC) $(CFLAGS) -o $(WGRIB) $(WGRIB_OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<  -o $@

clean:
	@rm -f $(OBJS) $(WGRIB_OBJS) $(EXECUTABLE) $(WGRIB)
