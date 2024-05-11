CC = gcc
RM = rm

CFLAGS = -O2 -std=c99

TLV_SRC_DIR := tlv_parser
TLV_INC_DIR := tlv_parser

TLV_SRC_FILE := $(TLV_SRC_DIR)/tlv_parser.c

tlv_test: main.o tlv_parser.o
	$(CC) main.o tlv_parser.o -o tlv_test
	$(RM) -f *.o 

main.o: main.c
	$(CC) $(INCLUDES) $(INC_SOURCE) $(CFLAGS) -I$(TLV_INC_DIR) -c main.c -o main.o
    
tlv_parser.o: $(TLV_SRC_FILE)
	$(CC) $(CFLAGS) $(INC_SOURCE) -I$(TLV_INC_DIR) -c $(TLV_SRC_FILE) -o tlv_parser.o

