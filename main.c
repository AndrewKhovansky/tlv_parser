/*
 * main.c
 *
 *  Created on: 8 мая 2024 г.
 *      Author: khova
 */

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

enum TLV_Class
{
	Universal,
	Application,
	ContextSpecific,
	Private
};

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


static uint8_t* ParseBuffer;

void print_with_indent(int indent, char * string)
{
    printf("%*s%s", indent, "", string);
}

int parseTlvFromBuffer(TLV_t* tlv, uint8_t* buf, uint32_t size, uint32_t* pBytesParsed)
{

	uint8_t tag[10];

	uint64_t id;

	int err;
	uint8_t tmp;

	uint32_t bytesParsed;

	bytesParsed = 0;


	if(size == 0)
	{
		*pBytesParsed = 0;
		return 0;
	}

	//Tag parsing
	tag[0] = buf[ bytesParsed++ ];

	tlv->class  = (int)((tag[0] >> 6) & 0x03);
	tlv->type   = (int)((tag[0] >> 5) & 0x01);

	if((tag[0] & 0x1F) <= 30)
	{
		tlv->id = (tag[0] & 0x1F);
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

				tlv->length |= ((uint64_t)tmp >> offset);

				offset -= 8;
			}

			tlv->length_size = bytes_to_read;
		}


		tlv->value = &buf[ bytesParsed ];
	//	bytesParsed += tlv->length;
	}





	if(tlv->length_type == Indefinite)
	{
		tlv->value = &buf[ bytesParsed ];

		tlv->length = 0;
		while(1)
		{
			bytesParsed++;
			if((buf[ bytesParsed ] == 0) && (buf[ bytesParsed + 1 ] == 0))
			{
				bytesParsed  += 2;

		//		return 0;
				break;
			}
			tlv->length++;
		}


	//	tlv->length = 0;
	}
	else
	{

	}

	uint32_t parsed;


	TLV_t* nextTLVp;

	if(tlv->type == Constructed)
	{
		tlv->child = TLV_create();
		tlv->child->parent = tlv;

		tlv->value = &buf[bytesParsed];

		nextTLVp = tlv->child;

		uint32_t parsed;

		parseTlvFromBuffer(tlv->child,tlv->value,tlv->length,&parsed);
		bytesParsed += tlv->length;

		if(bytesParsed < size)
		{
			tlv->next = TLV_create();
			tlv->next->parent = tlv->parent;

			tlv->value = &buf[bytesParsed];

			parseTlvFromBuffer(tlv->next,tlv->value,tlv->length,&parsed);

			bytesParsed += tlv->length;
		}
	}
	else
	{
		bytesParsed += tlv->length;

		nextTLVp = NULL;
		if(tlv->parent != NULL)
		{
			TLV_t* t = tlv->parent->child;

			uint32_t parent_len =  tlv->parent->length;
			uint32_t child_len = 0;

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
	uint32_t bufsize = 1024;

	uint32_t parsedSize;

	parsedSize = 0;
	while( readByteFromHexFile(fp, &filebuffer[parsedSize]) == 0 )
	{
		parsedSize++;

		if(parsedSize >= bufsize)
		{
			bufsize += 1024;
			filebuffer = realloc(filebuffer, bufsize);
		}

	}

	fclose(fp);


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
	{
		char buffer[1024];
		sprintf(buffer, "id: %d, len: %d\r\n", (int)tlv->id, (int)tlv->length);
		print_with_indent(level * 4, buffer);
		fflush(stdout);


		printed_out_count++;


		if((tlv->child != NULL))
		{
			tlv = tlv->child;
			level++;
		}
		else if(tlv->next != NULL)
		{
			tlv = tlv->next;
		}
		else
		{
		//	if(tlv->parent == NULL)
		//		return 0;

			while(1)
			{
				if(tlv->parent == NULL)
				{
					return 0;
				}

				tlv = tlv->parent;
				level--;

				if( !tlv->next )
				{
					continue;
				}
				else
				{
					tlv = tlv->next;
					break;
				}
			}
		}
	}


//	fclose(fp);
}


