// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Client
	{

		//===================================================================================
		//
		// IInputParameterRegistry
		//
		//===================================================================================

		struct IInputParameterRegistry
		{
			struct SParameterInfo
			{
				explicit                        SParameterInfo(const char* _name, const Shared::CTypeInfo& _type, size_t _offset);

				const char*                     name;
				const Shared::CTypeInfo&        type;
				size_t                          offset;    // memory offset of where the parameter is located in the SParams struct of the according generator/function/evaluator class
			};

			virtual                             ~IInputParameterRegistry() {}
			virtual size_t                      GetParameterCount() const = 0;
			virtual SParameterInfo              GetParameter(size_t index) const = 0;
		};

		inline IInputParameterRegistry::SParameterInfo::SParameterInfo(const char* _name, const Shared::CTypeInfo& _type, size_t _offset)
			: name(_name)
			, type(_type)
			, offset(_offset)
		{}

	}
}
