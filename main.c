/*
 * main.c
 *
 *  Created on: 8 мая 2024 г.
 *      Author: khova
 */

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>


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
	uint8_t tag[2];
	uint8_t len;
	uint8_t* value;



	struct TLV* next;
};

typedef struct TLV TLV_t;

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

	fseek(fp, 0L, SEEK_END);
	sz = ftell(fp);
	fseek(fp, 0L, SEEK_SET);


/*	char* filebuffer = (char*)calloc(sz,1);

	if(!filebuffer)
	{
		printf("File is too large.");
		return -1;
	}

	if(fread(filebuffer,sz,fp) != sz)
	{
		printf("Cannot read file.");
		return -1;
	}

	fclose(fp);*/


	uint8_t tag[2];
	uint8_t len;
	while(1)
	{

		if( fread(tag, 1, 1, fp) != 1 )
		{
			printf("Error reading file.");
			return -1;
		}


		if(tag[0] <= 30)
		{

		}

	}


}


