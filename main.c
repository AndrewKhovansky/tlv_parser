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

TLV_t* parseTlvFromBuffer(uint8_t* buf, uint32_t size, uint32_t* pBytesParsed)
{
	uint8_t tag;
	uint64_t id;
	uint64_t length;

	uint8_t* value;

	int length_type;
	uint64_t length_size;
	uint64_t tag_size;

	uint8_t tmp;
	uint32_t bytesParsed;

	uint8_t class;
	uint8_t type;

	bytesParsed = 0;

	if(size == 0)
	{
		*pBytesParsed = 0;
		return NULL;
	}

//	TLV_t* tlv = TLV_create();


	//Tag parsing
	tag = buf[ bytesParsed++ ];

	class  = (int)((tag >> 6) & 0x03);
	type   = (int)((tag >> 5) & 0x01);

	if((tag & 0x1F) <= 30)
	{
		id = (tag & 0x1F);
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
	}

	tag_size = bytesParsed;

	//Length parsing
	tmp = buf[ bytesParsed++ ];

	if(tmp == 0x80) //Indefinite form
	{
		length_type = Indefinite;
		length_size = 1;
	}
	else	//Definite form
	{
		if((tmp & 0x80) == 0) //Short form
		{
			length = (tmp & 0x7F);
			length_type = Short;

			length_size = 1;
		}
		else	//Long form
		{
			int bytes_to_read = (int)(tmp & 0x7F);

			length_type = Long;

			uint32_t offset = (bytes_to_read - 1) * 8;

			length = 0;
			for(int i=0; i<bytes_to_read; ++i)
			{
				tmp = buf[ bytesParsed++ ];

				length |= ((uint64_t)tmp << offset);

				offset -= 8;
			}

			length_size = bytes_to_read;
		}

		value = &buf[ bytesParsed ];

		bytesParsed += length;
	}

	if(length_type == Indefinite)
	{
		value = &buf[ bytesParsed ];

		length = 0;
		while(1)
		{
			bytesParsed++;

			if((buf[ bytesParsed ] == 0) && (buf[ bytesParsed + 1 ] == 0))
			{
				bytesParsed  += 2;
				break;
			}
			length++;
		}
	}

	TLV_t* tlv = TLV_create();

	tlv->id = id;
	tlv->length = length;
	tlv->length_type = length_type;
	tlv->class = class;
	tlv->type = type;
	tlv->tag_size = tag_size;
	tlv->length_size = length_size;

	*pBytesParsed = bytesParsed;

	return tlv;
}

uint8_t* filebuffer;






int tlvIsFullyParsed(TLV_t* tlv)
{
	TLV_t* tlvChild = NULL;
	TLV_t* tlvNext = NULL;

	uint32_t tlvSize = tlv->length;

	if(tlv->type == Primitive)
	{
		return 1;
	}
	else
	{
		if(tlv->child != NULL)
		{
			tlvChild = tlv->child;
		}
	}




	return 0;
}




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

	uint8_t* parseBuffer = filebuffer;
	uint32_t parseSize = parsedSize;

//	tlv_head = parseTlvFromBuffer(parseBuffer, parseSize, &bytesReadFromBuf);
	//bytesParsed += bytesReadFromBuf;

	//tlv = tlv_head;

//	parseBuffer =

	tlv_head = NULL;
	tlv = NULL;
	while(1)
	{

		TLV_t* new_tlv = parseTlvFromBuffer(parseBuffer, parseSize, &bytesReadFromBuf);
	//	bytesParsed += bytesReadFromBuf;

		if(new_tlv == NULL)
			break;

		if(tlv_head == NULL)
			tlv_head = new_tlv;

		if(tlv == NULL)
		{
			tlv = new_tlv;

			if(tlv->type == Constructed)
			{
				parseBuffer += (tlv->length_size + tlv->tag_size);
				parseSize -= (tlv->length_size + tlv->tag_size);
			}
			else
			{
				parseBuffer += (tlv->length_size + tlv->tag_size + tlv->length);
				parseSize -= (tlv->length_size + tlv->tag_size + tlv->length);
			}

			continue;
		}


		if(new_tlv->type == Constructed)
		{
			parseBuffer += (new_tlv->length_size + new_tlv->tag_size);
			parseSize -= (new_tlv->length_size + new_tlv->tag_size);

		}
		else if(new_tlv->type == Primitive)
		{
			parseBuffer += (new_tlv->length_size + new_tlv->tag_size + new_tlv->length);
			parseSize -= (new_tlv->length_size + new_tlv->tag_size + new_tlv->length);
		}


	/*	TLV_t* tlvParent = NULL;
	//	if (new_tlv->type == Primitive)
		{
			tlvParent = tlv->parent;
			while(tlvParent)
			{
				TLV_t* tlvChild = tlvParent->child;

				int child_len = 0;
				while(tlvChild)
				{
					child_len += (tlvChild->length);

					tlvChild = tlvChild->next;
				}

				child_len += (tlvParent->length_size + tlvParent->tag_size);

				if(child_len == tlvParent->length)
				{
					tlvParent = tlvParent->parent;
				}
				else
				{
					tlvParent = NULL;
					break;
				}

			}
		}
*/

		if(tlv->type == Constructed)
		{
			tlv->child = new_tlv;
			new_tlv->parent = tlv;
		}
		else if(tlv->type == Primitive)
		{
			tlv->next = new_tlv;
			new_tlv->prev = tlv;
		}


		tlv = new_tlv;


	}

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


