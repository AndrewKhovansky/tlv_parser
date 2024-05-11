/*
 * tlv_parser.h
 *
 *  Created on: 11 мая 2024 г.
 *      Author: khova
 */

#ifndef TLV_PARSER_TLV_PARSER_H_
#define TLV_PARSER_TLV_PARSER_H_

enum TLV_ErrorType
{
	None,
	ErrorInvalidClass,
	ErrorInvalidType,
	ErrorInvalidSize,
	ErrorPrimitiveIndefinite,
	ErrorNoTrailingTLV,
	ErrorNoFreeHeapSpace,
	ErrorNothingToParse,
	ErrorUnexpectedEndOfBuffer
};

typedef struct
{
	uint64_t offset;
	enum TLV_ErrorType errorType;
}ErrorBlock_t;


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

	uint32_t tag_size; 		//Variables for offset calculation and for raw value printing
	uint32_t length_size;	//

	uint8_t tag_raw[10];
	uint8_t length_raw[10];

	uint32_t unparsed_data_size; //Internal use variable for underlying TLVs parsing

	enum TLV_LengthType length_type;
	enum TLV_Class class;
	enum TLV_Type type;

	struct TLV* parent; //Upper level TLV
	struct TLV* child;  //Lower level TLV
	struct TLV* prev;   //Previous TLV on this level
	struct TLV* next;   //Next TLV on this level

	struct TLV* prev_through;	//Through-link for memory freeing
	struct TLV* next_through;	//
};

typedef struct TLV TLV_t;


int TLV_getNumSubTlvs(TLV_t* tlv);
TLV_t* TLV_parseTlvFromBuffer(uint8_t* buf, uint64_t size, ErrorBlock_t* errblock);
int TLV_getNumSubTlvs(TLV_t* tlv);

void TLV_deleteTree(TLV_t* tlv);
TLV_t* TLV_createTreeFromBinaryBuffer(uint8_t* parseBufferBegin, size_t parseSize, ErrorBlock_t* errblock);

#endif /* TLV_PARSER_TLV_PARSER_H_ */
