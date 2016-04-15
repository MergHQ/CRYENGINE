/*
	transfer configurator factory declaration
*/

#if defined(OFFLINE_COMPUTATION)

#pragma once

#include <PRT/PRTTypes.h>
#include <PRT/ITransferConfigurator.h>

namespace NSH
{
	namespace NTransfer
	{
		enum ETransferConfigurator;
		interface ITransferConfigurator;
		struct STransferParameters;

		//!< singleton to abstract transfer configurator factory
		class CTransferConfiguratorFactory
		{
		public:
			//!< singleton stuff
			static CTransferConfiguratorFactory* Instance();

			const ITransferConfiguratorPtr GetTransferConfigurator
			(
				const NSH::NTransfer::STransferParameters& crParameters
			);

		private:
			//!< singleton stuff
			CTransferConfiguratorFactory(){}
			CTransferConfiguratorFactory(const CTransferConfiguratorFactory&);
			CTransferConfiguratorFactory& operator= (const CTransferConfiguratorFactory&);
		};
	}
}

#endif
