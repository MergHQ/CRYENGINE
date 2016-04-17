/*
	SH rotation functions and types implementation
*/

template<uint8 Dim>
NSH::CCompressedLookupTable_tpl<Dim>::CCompressedLookupTable_tpl()
{
	//reset to -1, only valid fields will contain mapped data
	for(int i=0; i<Dim*Dim*Dim*Dim; ++i)
		m_LookupData[i] = -1;
	//now build recursively, remember first element is not stored for rotation
	Construct(Dim);
}

template<uint8 Dim>
inline const int32 NSH::CCompressedLookupTable_tpl<Dim>::operator()(const uint8 cRowIndex, const uint8 cColumnIndex)const
{
	return m_LookupData[cRowIndex * Dim*Dim + cColumnIndex];
}

template<uint8 Dim>
inline const int32 NSH::CCompressedLookupTable_tpl<Dim>::GetMapping(const uint32 cIndex, const uint8 cElemsInOneRow)const
{
	const uint8 cRowIndex = (uint8)(cIndex / (uint32)cElemsInOneRow);
	const uint8 cColumnIndex = (uint8)(cIndex - cRowIndex * cElemsInOneRow);
	return m_LookupData[cRowIndex * Dim*Dim + cColumnIndex];
}

template<uint8 Dim>
void NSH::CCompressedLookupTable_tpl<Dim>::Construct(const uint8 cDim)
{
	//first compute row start
	uint32 rowStart = 0;
	uint8 x = 1;
	int dim = cDim;
	int dataOffset = 0;//number of already stored elements
	while(--dim)
	{
		dataOffset += (int)x*(int)x;
		rowStart += x;
		x += 2;
	}
	//now build mapping
	//x = mapping count per row and column
	//rowStart = row and column offset for this matrix
	//dataOffset=already stored data
	for(int i=0; i<x; ++i)//iterate each row
		for(int j=0; j<x; ++j)//iterate each column
			m_LookupData[(rowStart+i)*Dim*Dim+rowStart+j] = dataOffset + i*x+j - 1/*non stored first element*/;

	if(cDim > 2)
		Construct(cDim-1);	//continue recursion
}
