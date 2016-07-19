#include "vco.h"
#if defined(HAS_VCO)
	#include "vco_orbis.inl"
#endif

extern "C"
{
	void OptimizeIndexBufferForOrbis(uint32_t* pIndices, uint32_t numIndices)
	{
#if defined(HAS_VCO)
		OptimizeIndexBufferForOrbisImpl(pIndices, numIndices);
#endif
	}
}
