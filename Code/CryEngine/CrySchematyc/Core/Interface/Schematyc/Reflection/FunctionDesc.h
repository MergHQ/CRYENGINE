#pragma once

#include "Schematyc/Utils/GUID.h"

namespace Schematyc
{

class CCommonFunctionDesc
{
public:

private:

	SGUID       m_guid;
	const char* m_szName = nullptr;
};

template<typename TYPE, TYPE FUNCTION_PTR> class CFunctionDesc : public CCommonFunctionDesc
{
};

} // Schematyc