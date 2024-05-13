#include "stdint.h"
#include "tlv_parser.h"
#include <stdlib.h>

#include <string.h>

static TLV_t* TLV_create()
{
	return (TLV_t*)calloc(sizeof(TLV_t),1);
}


TLV_t* TLV_parseTlvFromBuffer(uint8_t* buf, uint64_t size, ErrorBlock_t* errblock)
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
	enum TLV_Type type;

	bytesParsed = 0;

	if((size == 0) || !buf) //Nothing to parse
	{
		errblock->errorType = None;
		errblock->offset = 0;

		return NULL;
	}

	//Tag parsing
	rawcount = 0;
	tag = buf[ bytesParsed++ ];

	if((size - bytesParsed) == 0)
	{
		errblock->errorType = ErrorUnexpectedEndOfBuffer;
		errblock->offset = (size-1);
		return NULL;
	}

	tag_raw[rawcount++] = tag;

	class  = (int)((tag >> 6) & 0x03);

	if(class > Private) //Invalid class
	{
		errblock->errorType = ErrorInvalidClass;
		errblock->offset = (bytesParsed-1);
		return NULL;
	}

	type   = (int)((tag >> 5) & 0x01);

	if((type > Constructed)) //Invalid type
	{
		errblock->errorType = ErrorInvalidType;
		errblock->offset = (bytesParsed-1);
		return NULL;
	}


	if((tag & 0x1F) <= 30) //For  0...30 tag is coded by one byte
	{
		id = (tag & 0x1F);

		if((size - bytesParsed) == 0)
		{
			errblock->errorType = ErrorUnexpectedEndOfBuffer;
			errblock->offset = (size-1);
			return NULL;
		}


	}
	else //For >=31 tag is coded by multiple bytes
	{
		int offset = (63 - 7);
		id = 0;

		while(1)
		{
			tmp = buf[ bytesParsed++ ];

			if((size - bytesParsed) == 0)
			{
				errblock->errorType = ErrorUnexpectedEndOfBuffer;
				errblock->offset = (size-1);
				return NULL;
			}

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

		if((type == Primitive) && (length_type == Indefinite))
		{
			errblock->errorType = ErrorPrimitiveIndefinite;
			errblock->offset = (bytesParsed-1);
			return NULL;
		}
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


			if(bytes_to_read > 8)
			{
				errblock->errorType = ErrorInvalidSize;
				errblock->offset = (bytesParsed - 1);
				return NULL;
			}


			if((size - bytesParsed) < bytes_to_read)
			{
				errblock->errorType = ErrorUnexpectedEndOfBuffer;
				errblock->offset = (size - 1);
				return NULL;
			}


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

			if(length > (size - bytesParsed))
			{
				errblock->errorType = ErrorInvalidSize;
				errblock->offset = (bytesParsed-length_size);
				return NULL;
			}
		}

	}


	if((size - bytesParsed) < length)
	{
		errblock->errorType = ErrorUnexpectedEndOfBuffer;
		errblock->offset = (bytesParsed-1);
		return NULL;
	}


	value = &buf[ bytesParsed ];


	TLV_t* tlv = TLV_create(); //TLV is fully parsed and has correct data. Create structure in heap.

	if(!tlv)
	{
		errblock->errorType = ErrorNoFreeHeapSpace;
		errblock->offset = 0;
		return NULL;
	}

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

	return tlv;
}


int TLV_getNumSubTlvs(TLV_t* tlv)
{
	int count = 0;


	tlv = tlv->child;

	while(tlv)
	{
		count++;
		tlv = tlv->next;
	}

	return count;
}


void TLV_deleteTree(TLV_t* tlv)
{
	TLV_t* tlv_temp;
	while(tlv)
	{
		tlv_temp = tlv;

		tlv = tlv->next_through;

		free(tlv_temp);

	}

}


TLV_t* TLV_createTreeFromBinaryBuffer(uint8_t* parseBufferBegin, size_t parseSize, ErrorBlock_t* errblock)
{

	TLV_t* tlv = NULL;
	TLV_t* tlv_head = NULL;

	TLV_t* tlv_prev = NULL;

	ErrorBlock_t errb;

	uint8_t* parseBuffer = parseBufferBegin;

	uint32_t offset = 0;

	errb.errorType = None;
	errb.offset = 0;

	if(!errblock) //No error handling structure given.
	{
		return NULL;
	}


	if(!parseBufferBegin || (parseSize == 0)) //No valid data for parsing.
	{
		errblock->errorType = ErrorNothingToParse;
		errblock->offset = 0;
		return NULL;
	}


	while(1)
	{
		tlv = TLV_parseTlvFromBuffer(parseBuffer, parseSize, &errb);

		if(tlv == NULL)
		{
			if(errb.errorType != None) //Error occurred. Pass error code to caller.
			{
				errblock->errorType = errb.errorType;
				errblock->offset = (parseBuffer - parseBufferBegin) + errb.offset;

				TLV_deleteTree(tlv_head);

				return NULL;
			}

			if(tlv_prev->parent)
			{
				if(tlv_prev->parent->length_type == Indefinite)  //No trailing TLV found
				{
					errblock->errorType = ErrorNoTrailingTLV;
					errblock->offset = (parseBuffer - parseBufferBegin) + errb.offset;

					TLV_deleteTree(tlv_head);

					return NULL;
				}
			}

			errblock->errorType = None;
			errblock->offset = 0;

			return tlv_head;
		}


		offset = (tlv->length_size + tlv->tag_size);

		if(tlv->type == Primitive) //Skipping payload of Primitive TLV
			offset += tlv->length;

		parseBuffer += offset;
		parseSize -= offset;

		if(tlv->type == Constructed)
			tlv->unparsed_data_size = tlv->length;

		if(tlv_head == NULL)
		{
			tlv_head = tlv;
			tlv_prev = tlv;
			continue;
		}

		if(tlv_prev->type == Constructed)
		{
			if(tlv_prev->unparsed_data_size == 0) //Previous Constructed TLV is fully parsed for all underlying data.
			{
				tlv_prev->next = tlv; //Put new TLV to same level
				tlv->prev = tlv_prev; //

				tlv->parent = tlv_prev->parent;
			}
			else //Previous TLV has unparsed data
			{
				tlv_prev->child = tlv;  //Put new TLV to child level
				tlv->parent = tlv_prev; //
			}
		}
		else //Previous TLV is Primitive. Put new TLV to same level.
		{
			tlv_prev->next = tlv;
			tlv->prev = tlv_prev;

			tlv->parent = tlv_prev->parent;
		}



		tlv_prev->next_through = tlv; //Put new TLV to through-link chain

		tlv_prev = tlv;


		//Primitive TLV size is subtracted from ALL upper level Constructed TLVs.
		//Constructed TLV with unparsed_data_size==0 considered fully parsed
		if(tlv->type == Primitive)
		{
			TLV_t* tlv_parent = tlv->parent;

			uint32_t parsed_data = 0;

			parsed_data = (tlv->length + tlv->length_size + tlv->tag_size);
			while(tlv_parent)
			{
				if(tlv_parent->length_type != Indefinite)
				{
					tlv_parent->unparsed_data_size -= parsed_data;

					if(tlv_parent->unparsed_data_size == 0) //TLV has no more underlying unparsed data
					{
						tlv_prev = tlv_parent; //Move tree pointer one level up

						if(tlv_parent->parent)
						{
							//TLV is full (no unparsed data), so its header size is subtracted from upper level TLV
							tlv_parent->parent->unparsed_data_size -= (tlv_parent->length_size + tlv_parent->tag_size);
						}
					}

					tlv_parent = tlv_parent->parent;
				}
				else
				{
					if((tlv->length == 0) && (tlv->id == 0)) //Trailing TLV for indefinite length
					{
						tlv_prev = tlv_parent;

						tlv_parent->unparsed_data_size = 0;

						if(tlv_parent->parent)
						{
							if(tlv_parent->parent->length_type != Indefinite)
							{
								if(tlv_parent->parent->unparsed_data_size >= (tlv_parent->length_size + tlv_parent->tag_size))
									tlv_parent->parent->unparsed_data_size -= (tlv_parent->length_size + tlv_parent->tag_size);
							}
						}

						break;
					}

					tlv_parent = tlv_parent->parent;
				}
			}
		}
	}

	errblock->errorType = None;
	errblock->offset = 0;

	return NULL;
}

