// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace stdlib
	{

		//===================================================================================
		//
		// CDeferredEvaluator_Raycast
		//
		//===================================================================================

		class CDeferredEvaluator_Raycast : public client::IDeferredEvaluator
		{
		public:
			struct SParams
			{
				Vec3                   from;
				Vec3                   to;
				bool                   raycastShallSucceed;

				UQS_EXPOSE_PARAMS_BEGIN
					UQS_EXPOSE_PARAM("from", from);
					UQS_EXPOSE_PARAM("to", to);
					UQS_EXPOSE_PARAM("raycastShallSucceed", raycastShallSucceed);
				UQS_EXPOSE_PARAMS_END
			};

		private:
			class CRaycastRegulator
			{
			public:
				explicit               CRaycastRegulator(int maxRequestsPerFrame);
				bool                   RequestRaycast();

			private:
				const int              m_maxRequestsPerFrame;
				int                    m_currentFrame;
				int                    m_numRequestsInCurrentFrame;
			};

		private:
			const SParams              m_params;
			static CRaycastRegulator   s_regulator;

		public:
			explicit                   CDeferredEvaluator_Raycast(const SParams& params);
			virtual EUpdateStatus      Update(const SUpdateContext& updateContext) override;
		};

	}
}
