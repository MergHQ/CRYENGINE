#pragma once

#include "Schematyc/Component.h"
#include "Schematyc/Reflection/TypeDesc.h"

namespace Schematyc
{

class CComponentDesc : public CCustomClassDesc
{
};

SCHEMATYC_DECLARE_CUSTOM_CLASS_DESC(CComponent, CComponentDesc, CClassDescInterface)

} // Schematyc
