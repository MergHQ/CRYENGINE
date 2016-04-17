/*
	SH rotation functions and types
*/

#pragma once

namespace NSH
{
	//!< lookup table class which has the lookup table generated at static initialization time
	//!< dim is here the band dimension, not the matrix dimension
	template<uint8 Dim>
	class CCompressedLookupTable_tpl
	{
	public:
		CCompressedLookupTable_tpl();	//!< constructor generating lookup table
		//!< mapping operator which tells the index of the compressed data for a specific row and column index, 
		//!< -1 if wrong access to data which should be 0 by diagonal sparse definition		
		const int32 operator()(const uint8 cRowIndex, const uint8 cColumnIndex)const;	
		//!< tells the index of the compressed data for a specific matrix array index 
		const int32 GetMapping(const uint32 cIndex, const uint8 cElemsInOneRow)const;

	private:
		int32 m_LookupData[Dim*Dim*Dim*Dim];				//!< the lookup data

		void Construct(const uint8 cDim);		//!< constructs one mapping dimension
	};
}

#include "CompressedLookupTable.inl"

