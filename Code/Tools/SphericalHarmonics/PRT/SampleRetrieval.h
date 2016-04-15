/*
	sample retrieval class
*/

#pragma once

#include <PRT/PRTTypes.h>
#include <PRT/SHMath.h>
#include "BasicUtils.h"


namespace NSH
{
	template< class T >
	class vectorpvector;

	namespace NSampleGen
	{
		//!< sample retrieval class to just retrieve certain samples from a whole sample vector (to be able to process with less than all samples)
		template <class SampleCoeffType>
		class CSampleRetrieval_tpl
		{
		public:
			CSampleRetrieval_tpl(vectorpvector<CSample_tpl<SCoeffList_tpl<SampleCoeffType> > >& rVectorVector, const int32 cTotalSampleCount);
			CSample_tpl<SCoeffList_tpl<SampleCoeffType> >& Next();	//!< retrieves next sample
			void ResetForNextVertex();//!< resets for next sample retrieval
			const bool End() const;		//!< true if all samples have been retrieved
			const uint32 SampleCount() const;	//!< returns the number of samples to retrieve
		private:
			typename vectorpvector<CSample_tpl<SCoeffList_tpl<SampleCoeffType> > >::iterator m_Iterator;	//!< iterator used in case of full sample retrieval
			typedef std::set<TUint32Pair, std::less<TUint32Pair>, CSHAllocator<TUint32Pair> > TUint32PairSet;


			bool m_RetrievePartially;					//!< indicates whether any partially retrieval should be performed
			int32 m_SampleCountToRetrieve;		//!< numbers to retrieve
			int32 m_AlreadyRetrieved;					//!< counts the samples already been retrieved
			//specific members for partial retrieval
			TUint32PairSet	m_RandomTable;		//!< table generated in constructor containing each vector index and vector element member index
			TUint32PairSet::iterator	m_RandomTableIter;//!< iterator working on the random table

			vectorpvector<CSample_tpl<SCoeffList_tpl<SampleCoeffType> > >& m_rSampleData;

			CSampleRetrieval_tpl();								//!< no default constructor available
			const CSampleRetrieval_tpl& operator=(const CSampleRetrieval_tpl&);//!< no assignment operator possible
		};
	}
}

#include "SampleRetrieval.inl"