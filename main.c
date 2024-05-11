/*
 * main.c
 *
 *  Created on: 8 мая 2024 г.
 *      Author: khova
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "tlv_parser.h"

#define INDENTS_PER_LEVEL 2

<<<<<<< HEAD
enum TLV_Type
{
	Primitive,
	Constructed,
	Terminator
};

enum TLV_LengthType
{
	Long,
	Short,
	Indefinite
};



struct TLV
{
	uint8_t tag[10];
	uint64_t length;
	uint8_t* value;
	uint64_t id;

	uint32_t tag_size;
	uint32_t length_size;
	uint32_t subsequent_len;

	enum TLV_LengthType length_type;
	enum TLV_Class class;
	enum TLV_Type type;


	uint64_t length_estimated;

	struct TLV* parent;
	struct TLV* child;
	struct TLV* prev;
	struct TLV* next;
};

typedef struct TLV TLV_t;



TLV_t* TLV_create()
{
	return (TLV_t*)calloc(sizeof(TLV_t),1);
}
=======
>>>>>>> non_recursive


int readByteFromHexFile(FILE* fp, uint8_t* out)
{
	char hextext[3];
	hextext[2] = 0x00;

	int idx = 0;

	char tmp;

	if(!fp)
		return -1;

	while( idx < 2 )
	{
		if(fread(&tmp, 1, 1, fp) == 0)
		{
			return -2;
		}

		if((tmp >= '0') && (tmp <= '9'))
		{
			hextext[idx++] = tmp;
		}
		else if((tmp >= 'A') && (tmp <= 'F'))
		{
			hextext[idx++] = tmp;
		}
		else if((tmp >= 'a') && (tmp <= 'f'))
		{
			hextext[idx++] = tmp;
		}
	}

	if(idx < 2)
		return -3;

	*out = ((uint8_t)strtol(hextext, NULL, 16));

	return 0;
}


uint8_t* createBinaryBufferFromHexFile(FILE* fp, size_t* parsedBinarySize)
{
<<<<<<< HEAD
    printf("%*s%s", indent, "", string);
}

int parseTlvFromBuffer(TLV_t* tlv, uint8_t* buf, uint32_t size, uint32_t* pBytesParsed)
{

	uint8_t tag;
	uint64_t id;
	uint8_t tmp;
	uint32_t bytesParsed;

	bytesParsed = 0;

	if(size == 0)
	{
		*pBytesParsed = 0;
		return 0;
	}

	/*if(tlv->parent != NULL)
	{
		if(tlv->parent->length == UINT64_MAX)
		{
			if( (buf[0] == 0) && (buf[1] == 0) )
			{
				tlv->parent->length = tlv->parent->length_estimated;
				*pBytesParsed = 2;
				return 0;
			}
		}
	}*/




	//Tag parsing
	tag = buf[ bytesParsed++ ];

	tlv->class  = (int)((tag >> 6) & 0x03);
	tlv->type   = (int)((tag >> 5) & 0x01);

	if((tag & 0x1F) <= 30)
	{
		tlv->id = (tag & 0x1F);
	}
	else
	{
		int offset = (63 - 7);
		id = 0;

		while(1)
		{
			tmp = buf[ bytesParsed++ ];

			id |= ((uint64_t)tmp & 0x7F) << offset;
			offset -= 7;

			if((tmp & 0x80) == 0)
				break;
		}

		id = id >> (offset + 7);

		tlv->id = id;
	}

	tlv->tag_size = bytesParsed;

	//Length parsing
	tmp = buf[ bytesParsed++ ];

	if(tmp == 0x80) //Indefinite form
	{
		tlv->length_type = Indefinite;
		tlv->length_size = 1;
		tlv->value = &buf[ bytesParsed ];
		tlv->length = UINT64_MAX;
	}
	else	//Definite form
	{
		if((tmp & 0x80) == 0) //Short form
		{
			tlv->length = (tmp & 0x7F);
			tlv->length_type = Short;

			tlv->length_size = 1;
		}
		else	//Long form
		{
			int bytes_to_read = (int)(tmp & 0x7F);

			tlv->length_type = Long;

			uint32_t offset = (bytes_to_read - 1) * 8;

			tlv->length = 0;
			for(int i=0; i<bytes_to_read; ++i)
			{
				tmp = buf[ bytesParsed++ ];

				tlv->length |= ((uint64_t)tmp << offset);

				offset -= 8;
			}

			tlv->length_size = (bytes_to_read + 1);
		}

		tlv->value = &buf[ bytesParsed ];
	}

	/*if(tlv->length_type == Indefinite)
	{
		tlv->value = &buf[ bytesParsed ];

		tlv->length = 0;

		uint8_t* seek = tlv->value;

		while(1)
		{
		//	bytesParsed++;

			if((seek[0] == 0) && (seek[1] == 0))
			{
				//bytesParsed  += 2;
				break;
			}

			seek++;

			tlv->length++;
		}
	}*/

	uint32_t parsed;

	if(tlv->length == 0)
	{
		sleep(0);
	}


	if(tlv->id == 16)
	{
		sleep(0);
	}

	if(tlv->type == Constructed)
	{
		tlv->child = TLV_create();
		tlv->child->parent = tlv;

		tlv->value = &buf[bytesParsed];

		uint32_t parsed;

		parseTlvFromBuffer(tlv->child, &buf[bytesParsed], tlv->length,&parsed);
		bytesParsed += parsed;

		if(bytesParsed < size)
		{
			tlv->next = TLV_create();
			tlv->next->parent = tlv->parent;

			tlv->value = &buf[bytesParsed];

			parseTlvFromBuffer(tlv->next, tlv->value, tlv->length, &parsed);
			bytesParsed += parsed;
		}
	}
	else
	{
		bytesParsed += tlv->length;


		if(tlv->length == 0)
		{
			sleep(0);
	//		*pBytesParsed = bytesParsed;
	//		return 0;
		}


		if(tlv->parent != NULL)
		{
			TLV_t* t = tlv->parent->child;

			uint32_t parent_len =  tlv->parent->length;
			uint32_t child_len = 0;


			if(tlv->length == 0)
			{
	//			sleep(0);
				*pBytesParsed = bytesParsed;
				return 0;
			}



			while(t)
			{
				child_len += (t->length + t->length_size + t->tag_size);
				t = t->next;
			}

			if(parent_len == child_len)
			{
				*pBytesParsed = bytesParsed;
				return 0;
			}
			else
			{
				tlv->next = TLV_create();
				tlv->next->parent = tlv->parent;
				parseTlvFromBuffer(tlv->next, &buf[bytesParsed], size - bytesParsed, &parsed);

				bytesParsed += parsed;
			}
		}
	}

	*pBytesParsed = bytesParsed;
	return 0;
}

uint8_t* filebuffer;

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		printf("No file given.");
		return -1;
	}


	char* filename = argv[1];

	FILE* fp = fopen(filename,"rb");

	if(!fp)
	{
		printf("Cannot open file.");
		return -1;
	}

	uint32_t sz;

//	fseek(fp, 0L, SEEK_END);
//	sz = ftell(fp);
//	fseek(fp, 0L, SEEK_SET);


	filebuffer = malloc(1024);
=======
	uint8_t* filebuffer = malloc(1024);
>>>>>>> non_recursive
	uint32_t bufsize = 1024;

	size_t parsedSize;

	if( !filebuffer )
	{
		*parsedBinarySize = 0;
		return NULL;
	}

	parsedSize = 0;
	while( readByteFromHexFile(fp, &filebuffer[parsedSize]) == 0 )
	{
		parsedSize++;

		if(parsedSize >= bufsize)
		{
			bufsize += 1024;
			filebuffer = realloc(filebuffer, bufsize);

			if(!filebuffer)
			{
				*parsedBinarySize = 0;
				return NULL;
			}
		}
	}

	*parsedBinarySize = parsedSize;
	return filebuffer;
}


void print_with_indent(int indent, char * string)
{
    printf("%*s%s", indent, "", string);
}


int main(int argc, char* argv[])
{

	char* filename;

	FILE* fp;
	size_t parsedSize;

	TLV_t* tlv = NULL;

	ErrorBlock_t errb;

	uint8_t* filebuffer;

	if(argc < 2)
	{
		printf("No file given.");
		return -1;
	}

	filename = argv[1];
	fp = fopen(filename,"rb");

	if(!fp)
	{
		printf("Cannot open file.");
		return -1;
	}


	filebuffer = createBinaryBufferFromHexFile(fp, &parsedSize);
	fclose(fp);

<<<<<<< HEAD

	TLV_t* tlv = (TLV_t*)calloc(sizeof(TLV_t), 1);

	TLV_t* tlv_head = tlv;

	uint32_t bytesReadFromBuf;

	ParseBuffer = filebuffer;

	uint32_t bytesParsed = 0;




	parseTlvFromBuffer(tlv, filebuffer + bytesParsed, (parsedSize - bytesParsed), &bytesReadFromBuf);
	bytesParsed += bytesReadFromBuf;


	tlv = tlv_head;

	TLV_t* printed_out_list[1024];
	int printed_out_count;

	int level = 0;

	printed_out_count = 0;
	while(tlv)
=======
	if(!filebuffer)
>>>>>>> non_recursive
	{
		printf("Cannot parse HEX.");
		return -1;
	}

	tlv = TLV_createTreeFromBinaryBuffer(filebuffer, parsedSize, &errb);

	if(tlv == NULL)
	{
		switch(errb.errorType)
		{
		case ErrorPrimitiveIndefinite:
			printf("ERROR: Indefinite length for primitive TLV. Binary offset: %lld\r\n", errb.offset);
			break;
		case ErrorNoTrailingTLV:
			printf("ERROR: No trailing TLV for indefinite length. Binary offset: %lld\r\n", errb.offset);
			break;
		case ErrorInvalidClass:
			printf("ERROR: Invalid TLV class. Binary offset: %lld\r\n", errb.offset);
			break;
		case ErrorInvalidType:
			printf("ERROR: Invalid TLV type. Binary offset: %lld\r\n", errb.offset);
			break;
		case ErrorInvalidSize:
			printf("ERROR: Invalid TLV length. Binary offset: %lld\r\n", errb.offset);
			break;
		case ErrorNoFreeHeapSpace:
			printf("ERROR: No heap space for TLV structure. Binary offset: %lld\r\n", errb.offset);
			break;
		default:
			printf("ERROR. Unknown error. Binary offset: %lld\r\n", errb.offset);
			break;
		}

		fflush(stdout);

		free(filebuffer);

		return -1;
	}


	int level  = 0;

	//Variables for printing TLV numbers
	int tlv_numbers[1024];
	for(int i=0; i<sizeof(tlv_numbers)/sizeof(tlv_numbers[0]); ++i)
	{
		tlv_numbers[i] = 1;
	}

	//Print out the TLV tree
	TLV_t* tlv_head = tlv;
	while(tlv)
	{
		char buffer[2048];
		int len = 0;

		len += sprintf(&buffer[len], "TLV #%d: ", tlv_numbers[level]);

		switch(tlv->class)
		{
		case Universal:
			len += sprintf(&buffer[len], "Class: Universal, ");
			break;
		case Application:
			len += sprintf(&buffer[len], "Class: Application, ");
			break;
		case ContextSpecific:
			len += sprintf(&buffer[len], "Class: Context-Specific, ");
			break;
		case Private:
			len += sprintf(&buffer[len], "Class: Private, ");
			break;
		default:
			len += sprintf(&buffer[len], "Class: Unknown, ");
			break;
		}

		switch(tlv->type)
		{
		case Primitive:
			len += sprintf(&buffer[len], "Type: Primitive, ");
			break;
		case Constructed:
			len += sprintf(&buffer[len], "Type: Constructed, ");
			break;
		default:
			len += sprintf(&buffer[len], "Class: Unknown, ");
			break;
		}


		len += sprintf(&buffer[len], "ID: %d ", (int)tlv->id);

		len += sprintf(&buffer[len], "[");
		for(int i=0; i<tlv->tag_size; ++i)
		{
			len += sprintf(&buffer[len], "%02x", tlv->tag_raw[i]);
		}
		len += sprintf(&buffer[len], "]\r\n");

		print_with_indent(level * INDENTS_PER_LEVEL, buffer);

		len = 0;

		switch(tlv->length_type)
		{
		case Indefinite:
			len += sprintf(&buffer[len], " Length: INDEFINITE ");
			break;
		default:
			len += sprintf(&buffer[len], " Length: %lld ", tlv->length);
			break;
		}

		len += sprintf(&buffer[len], "[");
		for(int i=0; i<tlv->length_size; ++i)
		{
			len += sprintf(&buffer[len], "%02x", tlv->length_raw[i]);
		}
		len += sprintf(&buffer[len], "]\r\n");

		print_with_indent(level * INDENTS_PER_LEVEL, buffer);

		len = 0;

		if(tlv->type == Primitive)
		{
			len += sprintf(&buffer[len], " Value: [");
			for(int i=0; i<tlv->length; ++i)
			{
				len += sprintf(&buffer[len], "%02x", tlv->value[i]);
			}
			len += sprintf(&buffer[len], "]\r\n");
		}
		else
		{
			int sub_tlvs = TLV_getNumSubTlvs(tlv);
			len += sprintf(&buffer[len], " Value: Constructed (%d TLVs)\r\n", sub_tlvs);
		}
		print_with_indent(level * INDENTS_PER_LEVEL, buffer);

		len = 0;

		fflush(stdout);

		if((tlv->child != NULL)) //Select child TLV
		{
			tlv = tlv->child;
			level++;
		}
		else if(tlv->next != NULL)	//No child. Select brother TLV
		{
			tlv = tlv->next;

			tlv_numbers[level]++; //Increment Tag# counter
		}
		else	//No child or brother TLV. Move to next parent TLV
		{
			while(1)
			{
				if(tlv->parent == NULL) //No parent? Exit.
				{
					tlv = NULL;
					break;
				}

				tlv = tlv->parent; //Select parent
				level--;

				if( !tlv->next ) //No more TLVs on parent level? Go up one level more
				{
					continue;
				}
				else
				{
					tlv = tlv->next; //Select next TLV on level
					tlv_numbers[level]++; //Increment Tag# counter
					break;
				}
			}
		}
	}

	free(filebuffer);
	TLV_deleteTree(tlv_head);

	return 0;

}


