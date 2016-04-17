/*
	unit sphere sample generator interface
*/

#pragma once

#include <PRT/PRTTypes.h>
#include "BasicUtils.h"


namespace NSH
{
	namespace NSampleGen
	{
		typedef std::vector<TVec2D, CSHAllocator<TVec2D> >	TRandomSampleVec;			//!< sample vector for random x,y coordinates

		/************************************************************************************************************************************************/

		//!< unit sphere sample generation policy
		interface ISampleGenPolicy
		{
			virtual void GenerateSamples(TRandomSampleVec& rSamples, const uint32 cMaxSamples) = 0; //!< sample generation
			//!< resets organizer, default impl sufficient
			virtual void Reset(){};
		};
		
/************************************************************************************************************************************************/

		interface ISampleOrganizer
		{
			//!< converts NOT NORMALIZED random samples into SH sample and reorganizes all samples, returns number of successfully generated samples
			virtual const uint32 ConvertIntoSHAndReOrganize
			(
				const TVec2D *cpRandomSamples,
				const uint32 cRandomSampleCount,
				const SDescriptor& rSHDescriptor
			) = 0;	
			//!< retrieves all samples
			virtual void GetSamples(TScalarVecVec&)const = 0;
			//!< returns a reference to a sample from a handle
			virtual const CSample_tpl<SCoeffList_tpl<TScalarCoeff> >& GetSample(const TSampleHandle& crHandle)const = 0;
			//!< resets organizer
			virtual void Reset() = 0;
			//!< returns max capacity of samples according to sample handle type
			virtual const uint32 Capacity() = 0;
			//!< returns min capacity of samples according to sample handle type
			virtual const uint32 MinCapacity() = 0;
		};

/************************************************************************************************************************************************/

		//!< unit sphere sample generator interface
		interface ISampleGenerator
		{
			//!< generates cSampleCount SH samples according to sample distribution policy
			//!< reorganizes the vector of SH samples into bins to be able to only work only on a certain hemisphere(defined through vertex normal later on)
			virtual void GenerateSamples(const uint32 cSampleCount) = 0;	
			virtual void GenerateSamples(const uint32 cSampleCount, const SDescriptor& crNewDescriptor) = 0;	
			//!< retrieves the number of constructed samples
			virtual const uint32 Size() const = 0;									
			//!< retrieves the number of ordered constructed samples
			virtual const uint32 OrderedSize() const = 0;									
			//!< retrieves all samples
			virtual void GetSamples(TScalarVecVec&)const = 0;
			//!< returns the descritpor the sample generator is built with
			virtual const SDescriptor& SHDescriptor()const = 0;
			//!< virtual destructor 
			virtual ~ISampleGenerator() = 0 {};											
			//!< resets and restarts sample generator
			virtual void Restart(const uint32 cSampleCount, const SDescriptor& crDescriptor) = 0;
			//!< returns a reference to a sample from a handle
			virtual const CSample_tpl<SCoeffList_tpl<TScalarCoeff> >& ReturnSampleFromHandle(const TSampleHandle& crHandle) const = 0;
		};
	}

}