#include "StdAfx.h"
#include "NativeEntityBase.h"

#include "Utility/StringConversions.h"

const char *CNativeEntityBase::GetPropertyValue(int index) const
{
	IEntity *pEntity = GetEntity();
	if (pEntity == nullptr)
	{
		CryLog("[Warning] Failed to get entity property %i, entity was null!", index);
		return "";
	}

	IEntityClass *pEntityClass = pEntity->GetClass();

	IEntityPropertyHandler *pPropertyHandler = pEntityClass->GetPropertyHandler();
	if (pPropertyHandler)
	{
		return pPropertyHandler->GetProperty(pEntity, index);
	}

	CryLog("[Warning] Failed to get entity property %i (class %s), property handler was null!", index, pEntity->GetClass()->GetName());
	return "";
}

void CNativeEntityBase::SetPropertyValue(int index, const char *value)
{
	IEntity *pEntity = GetEntity();
	if (pEntity == nullptr)
		return;

	IEntityClass *pEntityClass = pEntity->GetClass();

	IEntityPropertyHandler *pPropertyHandler = pEntityClass->GetPropertyHandler();
	if (pPropertyHandler)
	{
		pPropertyHandler->SetProperty(pEntity, index, value);

		// Notify the entity of its properties having changed
		pPropertyHandler->PropertiesChanged(pEntity);
	}
}

float CNativeEntityBase::GetPropertyFloat(int index) const
{
	return (float)atof(GetPropertyValue(index));
}

int CNativeEntityBase::GetPropertyInt(int index) const
{
	return atoi(GetPropertyValue(index));
}

ColorF CNativeEntityBase::GetPropertyColor(int index) const
{
	return StringToColor(GetPropertyValue(index), false);
}

bool CNativeEntityBase::GetPropertyBool(int index) const
{
	return GetPropertyInt(index) != 0;
}

Vec3 CNativeEntityBase::GetPropertyVec3(int index) const
{
	return StringToVec3(GetPropertyValue(index));
}

void CNativeEntityBase::SetPropertyFloat(int index, float value)
{
	string valueString;
	valueString.Format("%f", value);

	SetPropertyValue(index, valueString.c_str());
}

void CNativeEntityBase::SetPropertyInt(int index, int value)
{
	string valueString;
	valueString.Format("%i", value);

	SetPropertyValue(index, valueString.c_str());
}

void CNativeEntityBase::SetPropertyBool(int index, bool value)
{
	string valueString;
	valueString.Format(value ? "1" : "0");

	SetPropertyValue(index, valueString.c_str());
}

void CNativeEntityBase::SetPropertyVec3(int index, Vec3 value)
{
	string valueString;
	valueString.Format("%f,%f,%f", value.x, value.y, value.z);

	SetPropertyValue(index, valueString.c_str());
}