/*
	transfer interface definitions
*/
#if defined(OFFLINE_COMPUTATION)

#pragma once

#include <PRT/IObserver.h>
#include <PRT/PRTTypes.h>

class CSimpleIndexedMesh;//forward declaration

namespace NSH
{
	namespace NSampleGen
	{
		interface ISampleGenerator;//forward declaration
	}

	namespace NTransfer
	{
		//forward declarations
		struct STransferParameters;				
		interface ITransferConfigurator;

		//!< transfer status
		struct STransferStatus
		{
			uint32 meshIndex;						//!< current mesh index processing
			uint32 vertexIndex;					//!< current vertex index processing within current mesh
			uint32 overallVertexIndex;	//!< current vertex index processing within current mesh
			uint32 passIndex;						//!< current processed pass
			uint32 totalMeshCount;			//!< total mesh count to process
			uint32 totalVertexCount;		//!< total vertex count within processing mesh to process
			uint32 overallVertexCount;	//!< overall total vertex count 
			uint32 totalPassCount;			//!< passes to process
			uint32 totalRCThreads;			//!< ray casting threads to start, 0 indicates no multithreading
			uint32 rcThreadsRunning;		//!< ray casting threads currently running

			STransferStatus()		//!< constructs with initial state
			{
				Reset();
			}
			
			void Reset()					//!< resets state
			{
				rcThreadsRunning = meshIndex = vertexIndex = overallVertexIndex = passIndex = totalMeshCount = totalVertexCount = overallVertexCount = 0;
				totalRCThreads = totalPassCount = 1;
			}
		};

		/************************************************************************************************************************************************/

		//!< basic interface for all transfer types, including observer pattern
		//!< it is all about incident radiance which can be vector quantize later on and be used as incident SH lighting for meso scale
		//!< no virtual destructor yet needed since no memory gets allocated explicitely
		interface IObservableTransfer : public IObservable
		{
			//!< transfer specific process function
			//!< works on vertex base and computes coefficients per vertex according to crSHDescriptor(number of bands) and material
			//!< add additional vertices prior if mesh is to coarse
			virtual const bool Process
			(
				const NSampleGen::ISampleGenerator& crSampleGen,		//!< samples generator where to get samples from
				const TGeomVec& crMeshes,														//!< vector of meshes to work on
				const SDescriptor& crSHDescriptor,									//!< descriptor for spherical harmonic setup
				const STransferParameters& crParameters,						//!< parameters to control the computation
				const ITransferConfigurator& crConfigurator					//!< configurator for transfer process
			) = 0;

			//!< retrieves the current and total processing state
			virtual void GetProcessingState(STransferStatus& rState) = 0;
			virtual ~IObservableTransfer(){}

		protected:
			IObservableTransfer(){};		//!< is not meant to be instantiated explicitely
		};
	}
}

#endif