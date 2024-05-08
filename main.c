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
	uint8_t number;

	enum TLV_LengthType length_type;
	enum TLV_Class class;
	enum TLV_Type type;

	uint64_t id;

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

	*out = ((uint8_t)strtol(hextext, NULL, 16));

	return 0;
}



int parseTlvFromFile(TLV_t* tlv, FILE* fp)
{
	uint8_t tag[10];

	//if(tlv == NULL)
//	{
//		tlv = (TLV_t*)calloc(sizeof(TLV_t),1);
	//}
	/*else
	{
		tlv->next = (TLV_t*)calloc(sizeof(TLV_t),1);
		tlv = tlv->next;
	}*/

//	tlv = (TLV_t*)calloc(sizeof(TLV_t),1);



	uint64_t id;

	int err;
	uint8_t tmp;

	uint8_t end[2];
	if(tlv->parent->type == Constructed)
	{
		long int pos = ftell(fp);

		readByteFromHexFile(fp, &end[0]);
		readByteFromHexFile(fp, &end[1]);

		if((end[0] == 0) && (end[1] == 0))
		{
			return -3;
		}
		else
		{
			fseek(fp,pos,SEEK_SET);
		}

	}


	//Tag parsing
	err = readByteFromHexFile(fp, &tag[0]);

	if(err == -2)
		return -2;	//End of file

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
		tlv->length_type = Indefinite;
	}
	else	//Definite form
	{
		if((tmp & 0x80) == 0) //Short form
		{
			tlv->length = (tmp & 0x7F);
			tlv->length_type = Short;
		}
		else	//Long form
		{
			int bytes_to_read = (int)(tmp & 0x7F);

			tlv->length_type = Long;

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


	if(tlv->type == Constructed)
	{
		tlv->child = TLV_create();

		tlv->child->parent = tlv;
		parseTlvFromFile(tlv->child, fp);
	}
	else
	{
		uint8_t* databuf = NULL;

		if(tlv->length_type == Indefinite)
		{
			uint32_t bufsize = 1024;

			databuf =  (uint8_t*)malloc(bufsize);
			uint32_t bytes_read = 0;
			uint8_t* dataPtr = databuf;
			while(readByteFromHexFile(fp, &tmp) == 0)
			{
				bytes_read++;
				if(bytes_read >= bufsize)
				{
					databuf = (uint8_t*)realloc(databuf, bufsize+1024);
					dataPtr = (databuf + bufsize);
					bufsize += 1024;
				}

				uint16_t end_sequence;

				if(bytes_read >= 2)
				{
					memcpy(&end_sequence, &databuf[bytes_read - 2], 2);
				}

				if(end_sequence == 0x0000)
					break;

				*(dataPtr++) = tmp;

			}

			tlv->value = databuf;
		}
		else
		{
			databuf =  (uint8_t*)malloc(tlv->length);
			uint32_t bytes_read = 0;
			uint8_t* dataPtr = databuf;
			while(readByteFromHexFile(fp, &tmp) == 0)
			{
				bytes_read++;
				*(dataPtr++) = tmp;

				if(bytes_read >= tlv->length)
				{
					break;
				}

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

	while(1)
	{

		if(parseTlvFromFile(tlv, fp) != 0)
		{
			break;
		}


		tlv->next = (TLV_t*)calloc(sizeof(TLV_t), 1);
		tlv = tlv->next;

	}

	tlv = tlv_head;


//	fclose(fp);
}


