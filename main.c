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
	Constructed
};

struct TLV
{
	uint8_t tag[10];
	uint64_t length;
	uint8_t* value;
	uint8_t number;

	enum TLV_Class class;
	enum TLV_Type type;


	uint64_t id;

	struct TLV* next;
};

typedef struct TLV TLV_t;




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

	*out = ((uint8_t)strtol(hextext, NULL, 16));

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

	/*fseek(fp, 0L, SEEK_END);
	sz = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	char* filebuffer = (char*)calloc(sz,1);

	if(!filebuffer)
	{
		printf("File is too large.");
		return -1;
	}*/



	uint8_t tag[10];
	uint8_t len;


	TLV_t* tlv_list_head;


	TLV_t* tlv;

	tlv = (TLV_t*)calloc(sizeof(TLV_t),1);

	tlv_list_head = tlv;

//	tlv = &tlv_list_head;

	uint64_t id;

	int err;
	uint8_t tmp;
	while(1)
	{
		//Tag parsing
		err = readByteFromHexFile(fp, &tag[0]);

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
				err = readByteFromHexFile(fp, &tmp);

				if(err == -1)
					break;
				else if(err == -2)
				{
					break;
				}

				id |= ((uint64_t)tmp & 0x7F) << offset;
				offset -= 7;

				if((tmp & 0x80) == 0)
					break;
			}

			id = id >> (offset + 7);

			tlv->id = id;
		}


		//Length parsing
		err = readByteFromHexFile(fp, &tmp);


		if(tmp == 0x80) //Indefinite form
		{

		}
		else	//Definite form
		{
			if((tmp & 0x80) == 0) //Short form
			{
				tlv->length = (tmp & 0x7F);
			}
			else	//Long form
			{
				int bytes_to_read = (int)(tmp & 0x7F);

				uint32_t offset = (bytes_to_read - 1) * 8;

				tlv->length = 0;
				for(int i=0; i<bytes_to_read; ++i)
				{
					err = readByteFromHexFile(fp, &tmp);

					tlv->length |= ((uint64_t)tmp >> offset);

					offset -= 8;

				}
			}
		}

	}

	fclose(fp);
}


