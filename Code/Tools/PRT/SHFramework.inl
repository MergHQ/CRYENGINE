/*
	sh framework inline implementation
*/

namespace NSH
{
	template<typename TCoeffType, typename TIteratorSink>
	inline const CSHCompressor<TCoeffType, TIteratorSink> NFramework::GetSHCompressor
	(
		const typename CSHCompressor<TCoeffType, TIteratorSink>::SCompressionProp& crProp, 
		TIteratorSink iterBegin, 
		const TIteratorSink cIterEnd, 
		const uint32 cBands
	)
	{
		return CSHCompressor<TCoeffType, TIteratorSink>(crProp, iterBegin, cIterEnd, cBands);
	}
	
}//NSH
