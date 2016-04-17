/*
	vegetation transfer interface inline implementations
*/

#if defined(OFFLINE_COMPUTATION)

#include <PRT/SHFrameworkBasis.h>
#include <PRT/ISHMaterial.h>

inline NSH::NTransfer::CVegetationTransferConfigurator::CVegetationTransferConfigurator(const STransferParameters& crParameters) : m_TransferParameters(crParameters)
{}

inline const bool NSH::NTransfer::CVegetationTransferConfigurator::ProcessOnlyUpperHemisphere(const NMaterial::EMaterialType cMatType)const
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

inline const bool NSH::NTransfer::CVegetationTransferConfigurator::UseCoefficientLookupMode()const
{
	return true;	
}

inline NSH::ITransferConfiguratorPtr NSH::NTransfer::CVegetationTransferConfigurator::Clone()const
{
	return ITransferConfiguratorPtr(new CVegetationTransferConfigurator(m_TransferParameters));
}

#endif