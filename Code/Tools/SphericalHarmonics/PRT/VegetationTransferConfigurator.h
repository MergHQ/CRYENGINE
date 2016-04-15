/*
	vegetation transfer interface definition 
*/

#if defined(OFFLINE_COMPUTATION)

#include <PRT/ITransferConfigurator.h>
#include <PRT/TransferParameters.h>
#include <PRT/ISHMaterial.h>
#include <PRT/SHFrameworkBasis.h>

namespace NSH
{
	namespace NTransfer
	{
		//!< implements ITransferConfigurator for default transfers
		class CVegetationTransferConfigurator : public ITransferConfigurator
		{
		public:
			INSTALL_CLASS_NEW(CVegetationTransferConfigurator)

			CVegetationTransferConfigurator(const STransferParameters& crParameters);	//!< ctor with transfer parameters
			virtual const bool ProcessOnlyUpperHemisphere(const NMaterial::EMaterialType)const;
			virtual const bool UseCoefficientLookupMode()const;
			virtual ~CVegetationTransferConfigurator(){}
			virtual ITransferConfiguratorPtr Clone()const;

		protected:
			STransferParameters m_TransferParameters;	//!< copied transfer parameters

			CVegetationTransferConfigurator(){};					//!< only valid with transfer parameters
		};
	}
}

#include "VegetationTransferConfigurator.inl"

#endif