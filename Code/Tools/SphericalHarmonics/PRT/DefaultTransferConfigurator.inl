/*
	default transfer interface inline implementations
*/

#if defined(OFFLINE_COMPUTATION)

#include <PRT/SHFrameworkBasis.h>
#include <PRT/ISHMaterial.h>


inline NSH::NTransfer::CDefaultTransferConfigurator::CDefaultTransferConfigurator(const STransferParameters& crParameters) 
	: m_TransferParameters(crParameters)
{}

inline const bool NSH::NTransfer::CDefaultTransferConfigurator::ProcessOnlyUpperHemisphere(const NMaterial::EMaterialType cMatType)const
{
	switch(cMatType)
	{
	case NSH::NMaterial::MATERIAL_TYPE_DEFAULT:
		return true;
		break;
	case NSH::NMaterial::MATERIAL_TYPE_BASETEXTURE:
		return true;
		break;
	case NSH::NMaterial::MATERIAL_TYPE_ALPHATEXTURE:
	case NSH::NMaterial::MATERIAL_TYPE_ALPHA_DEFAULT:
		return false;
		break;
	case NSH::NMaterial::MATERIAL_TYPE_BACKLIGHTING:
	case NSH::NMaterial::MATERIAL_TYPE_BACKLIGHTING_DEFAULT:
		return false;
		break;
	default:
		return true;
	}
}

inline NSH::ITransferConfiguratorPtr NSH::NTransfer::CDefaultTransferConfigurator::Clone()const
{
	return ITransferConfiguratorPtr(new CDefaultTransferConfigurator(m_TransferParameters));
}

inline const bool NSH::NTransfer::CDefaultTransferConfigurator::UseCoefficientLookupMode()const
{
	return false;	
}

#endif