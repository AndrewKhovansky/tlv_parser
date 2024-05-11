Test utility for TLV (Tag-Length-Value) parsing.

Parser uses binary input. For hex-to-bin converting and filtering see example in main.c

API:
	
	1) TLV_t* TLV_parseTlvFromBuffer(uint8_t* buf, uint64_t size, ErrorBlock_t* errblock)
		Create single TLV structure from binary buffer. 
		Arguments:
			buf - binary buffer pointer
			size - buffer size
			errblock - pointer to ErrorBlock_t structure for error handling.
			
	2) TLV_t* TLV_createTreeFromBinaryBuffer(uint8_t* parseBufferBegin, size_t parseSize, ErrorBlock_t* errblock)
		Create TLV tree from buffer. 
		Arguments:
			parseBufferBegin - binary buffer pointer
			parseSize - buffer size
			errblock - pointer to ErrorBlock_t structure for error handling.
			
	3) int TLV_getNumSubTlvs(TLV_t* tlv)
		Get number of TLV members of Constructed TLV structure. 
		Arguments: 	
			tlv - pointer to Constructed TLV structure.
			
			
	4) void TLV_deleteTree(TLV_t* tlv)
		Delete TLV tree and free allocated memory.
		Arguments:
			tlv - pointer to TLV tree head structure.