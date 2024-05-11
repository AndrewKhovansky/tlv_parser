CC = gcc
RM = rm
CFLAGS = -Itlv_parser

TLV_SRC_DIR := tlv_parser
TLV_INC_DIR := tlv_parser

TLV_SRC_FILE := $(TLV_SRC_DIR)/tlv_parser.c

tlv_test: main.o tlv_parser.o
	$(CC) main.o tlv_parser.o -o tlv_test
	$(RM) -f *.o 

main.o: main.c
	$(CC) $(INCLUDES) $(INC_SOURCE) $(CXXFLAGS) -I$(TLV_INC_DIR) -std=c99 -c main.c -o main.o
    
tlv_parser.o: $(TLV_SRC_FILE)
	$(CC) $(CXXFLAGS) $(INC_SOURCE) -I$(TLV_INC_DIR) -c $(TLV_SRC_FILE) -std=c99 -o tlv_parser.o

