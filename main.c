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


	//uint32_t level;

	uint8_t tag_raw[10];
	uint8_t length_raw[10];

	uint32_t real_data_len;

	uint32_t unparsed_data_size;

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


enum TLV_ErrorType
{
	None,
	InvalidClass,
	InvalidType,
	InvalidSize,
	NoTrailingTLV,
};

typedef struct
{
	uint32_t offset;
	enum TLV_ErrorType errorType;
}ErrorBlock_t;


TLV_t* parseTlvFromBuffer(uint8_t* buf, uint32_t size, ErrorBlock_t* errblock)
{
	uint8_t tag;
	uint64_t id;
	uint64_t length = 0;

	uint8_t* value;

	uint8_t tag_raw[10];
	uint8_t length_raw[10];

	int rawcount = 0;

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
		return NULL;
	}

	//Tag parsing
	rawcount = 0;
	tag = buf[ bytesParsed++ ];

	tag_raw[rawcount++] = tag;

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

			tag_raw[rawcount++] = tmp;

			id |= ((uint64_t)tmp & 0x7F) << offset;
			offset -= 7;

			if((tmp & 0x80) == 0)
				break;
		}

		id = id >> (offset + 7);
	}

	tag_size = bytesParsed;

	//Length parsing
	rawcount = 0;

	tmp = buf[ bytesParsed++ ];

	length_raw[rawcount++] = tmp;

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

				length_raw[rawcount++] = tmp;

				length |= ((uint64_t)tmp << offset);

				offset -= 8;
			}

			length_size = bytes_to_read + 1;
		}
	}

	value = &buf[ bytesParsed ];


	TLV_t* tlv = TLV_create();

	tlv->id = id;

	if(length_type != Indefinite)
		tlv->length = length;
	else
		tlv->length = UINT64_MAX;

	tlv->value = value;
	tlv->length_type = length_type;
	tlv->class = class;
	tlv->type = type;
	tlv->tag_size = tag_size;
	tlv->length_size = length_size;


	memcpy(tlv->tag_raw, tag_raw, tag_size);
	memcpy(tlv->length_raw, length_raw, length_size);

	if(tlv->type == Primitive)
	{
		bytesParsed += tlv->length;
	}

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


	TLV_t* tlv_prev = (TLV_t*)calloc(sizeof(TLV_t), 1);

	TLV_t* tlv_head = tlv_prev;

	ParseBuffer = filebuffer;

	uint8_t* parseBuffer = filebuffer;
	uint32_t parseSize = parsedSize;

	tlv_head = NULL;
	tlv_prev = NULL;

	TLV_t* tlv = NULL;

	uint32_t offset = 0;

	ErrorBlock_t errb;
	while(1)
	{
		tlv = parseTlvFromBuffer(parseBuffer, parseSize, &errb);

		if(tlv == NULL)
			break;

		offset = (tlv->length_size + tlv->tag_size);

		if(tlv->type == Primitive)
			offset += tlv->length;

		parseBuffer += offset;
		parseSize -= offset;


		if(tlv->type == Constructed)
		{
			tlv->unparsed_data_size = tlv->length;
		}

		if(tlv_head == NULL)
		{
			tlv_head = tlv;
			tlv_prev = tlv;
			continue;
		}

		if(tlv_prev->type == Constructed)
		{

			if(tlv_prev->unparsed_data_size == 0)
			{
				tlv_prev->next = tlv;
				tlv->prev = tlv_prev;


				tlv->parent = tlv_prev->parent;
			}
			else
			{
				tlv_prev->child = tlv;
				tlv->parent = tlv_prev;
			}
		}
		else
		{
			tlv_prev->next = tlv;
			tlv->prev = tlv_prev;

			tlv->parent = tlv_prev->parent;
		}

		tlv_prev = tlv;

		if(tlv->type == Primitive)
		{
			TLV_t* tp = tlv->parent;

			uint32_t parsed_data = 0;

			parsed_data = (tlv->length + tlv->length_size + tlv->tag_size);
			while(tp)
			{
				if(tp->length_type != Indefinite)
				{
					tp->unparsed_data_size -= parsed_data;

					if(tp->unparsed_data_size == 0)
					{
						tlv_prev = tp;

						if(tp->parent)
						{
							tp->parent->unparsed_data_size -= (tp->length_size + tp->tag_size);
						}
					}

					tp = tp->parent;
				}
				else
				{
					if((tlv->length == 0) && (tlv->id == 0))
					{
						tlv_prev = tp;

						tp->unparsed_data_size = 0;

						if(tp->parent)
						{
							if(tp->parent->length_type != Indefinite)
							{
								if(tp->parent->unparsed_data_size >= (tp->length_size + tp->tag_size))
									tp->parent->unparsed_data_size -= (tp->length_size + tp->tag_size);
							}
						}

						break;
					}

					tp = tp->parent;
				}
			}
		}
	}



	tlv_prev = tlv_head;

	tlv = NULL;

	tlv = tlv_head;


	int level  = 0;
	while(tlv)
	{
		char buffer[1024];
		int len = 0;

		len += sprintf(&buffer[len], "Tag: ");

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


		len += sprintf(&buffer[len], "[0x");
		for(int i=0; i<tlv->tag_size; ++i)
		{
			len += sprintf(&buffer[len], "%x", tlv->tag_raw[i]);
		}
		len += sprintf(&buffer[len], "]\r\n");

		print_with_indent(level * 2, buffer);

		len = 0;

		switch(tlv->type)
		{
		case Indefinite:
			len += sprintf(&buffer[len], "Length: INDEFINITE ");
			break;
		default:
			len += sprintf(&buffer[len], "Length: %u ", (uint32_t)tlv->length);
			break;
		}

		len += sprintf(&buffer[len], "[0x");
		for(int i=0; i<tlv->length_size; ++i)
		{
			len += sprintf(&buffer[len], "%x", tlv->length_raw[i]);
		}
		len += sprintf(&buffer[len], "]\r\n");

		print_with_indent(level * 2, buffer);






		len = 0;

		fflush(stdout);

		if((tlv->child != NULL)) //Select child TLV
		{
			tlv = tlv->child;
			level++;
		}
		else if(tlv->next != NULL)	//Select next TLV on this level
		{
			tlv = tlv->next;
		}
		else	//Select parent TLV
		{
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

}


