/*
	transfer configurator factory implementation
*/
#include "stdafx.h"

#if defined(OFFLINE_COMPUTATION)

#include "TransferConfiguratorFactory.h"
#include "DefaultTransferConfigurator.h"
#include "VegetationTransferConfigurator.h"
#include <PRT/TransferParameters.h>
#include <PRT/ITransferConfigurator.h>

NSH::NTransfer::CTransferConfiguratorFactory* NSH::NTransfer::CTransferConfiguratorFactory::Instance()
{
	static CTransferConfiguratorFactory inst;
	return &inst;
}

const NSH::ITransferConfiguratorPtr NSH::NTransfer::CTransferConfiguratorFactory::GetTransferConfigurator
(
	const NSH::NTransfer::STransferParameters& crParameters
)
{
	switch(crParameters.configType)
	{
	case TRANSFER_CONFIG_VEGETATION:
		return ITransferConfiguratorPtr(new CVegetationTransferConfigurator(crParameters));
		break;
	case TRANSFER_CONFIG_DEFAULT:
	default:
		return ITransferConfiguratorPtr(new CDefaultTransferConfigurator(crParameters));
		break;
	}
}

#endif