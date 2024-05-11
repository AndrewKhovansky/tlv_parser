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

uint8_t* filebuffer;

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
	uint8_t* filebuffer = malloc(1024);
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

	if(!filebuffer)
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
			fflush(stdout);
			break;
		case ErrorNoTrailingTLV:
			printf("ERROR: No trailing TLV for indefinite length. Binary offset: %lld\r\n", errb.offset);
			fflush(stdout);
			break;
		case ErrorInvalidClass:
			printf("ERROR: Invalid TLV class. Binary offset: %lld\r\n", errb.offset);
			fflush(stdout);
			break;
		case ErrorInvalidType:
			printf("ERROR: Invalid TLV type. Binary offset: %lld\r\n", errb.offset);
			fflush(stdout);
			break;
		default:
			printf("ERROR. Unknown error.\r\n");
			fflush(stdout);
			break;
		}

		return -1;
	}

	int level  = 0;


	int tlv_numbers[1024];
	for(int i=0; i<sizeof(tlv_numbers)/sizeof(tlv_numbers[0]); ++i)
	{
		tlv_numbers[i] = 1;
	}

	//Print out the TLV tree
	while(tlv)
	{
		char buffer[2048];
		int len = 0;


		len += sprintf(&buffer[len], "Tag #%d: ", tlv_numbers[level]);

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

			tlv_numbers[level]++;
		}
		else	//No child or brother TLV. Move to next parent TLV
		{
			while(1)
			{
				if(tlv->parent == NULL) //No parent? Exit.
				{
					return 0;
				}

				tlv = tlv->parent; //Select parent
				level--;

				if( !tlv->next ) //No more TLVs on this level? Go one level-up more
				{
					continue;
				}
				else
				{
					tlv = tlv->next; //Select next TLV on level
					tlv_numbers[level]++;
					break;
				}
			}
		}
	}

}


