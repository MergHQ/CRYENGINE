// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Serialize.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		bool Serialize(Serialization::IArchive& ar, CTimeValue& timeValue, const char* name, const char* label)
		{
			int64 value = 0;

			if (ar.isOutput())
			{
				// serialize
				value = timeValue.GetValue();
				return ar(value, name);
			}
			else if (ar.isInput())
			{
				// de-serialize
				const bool result = ar(value, name);
				timeValue.SetValue(value);
				return result;
			}

			return false;
		}

		bool Serialize(Serialization::IArchive& ar, OBB& obb, const char* name, const char* label)
		{
			typedef f32(&Array)[3][3];

			stack_string tmpName;
			stack_string tmpLabel;

			if (!name)
				name = "";

			if (!label)
				label = "";

			tmpName.Format("%s.m33", name);
			tmpLabel.Format("%s.m33", label);
			if (!ar((Array)obb.m33, tmpName.c_str(), tmpLabel.c_str()))
				return false;

			tmpName.Format("%s.h", name);
			tmpLabel.Format("%s.h", label);
			if (!ar(obb.h, tmpName.c_str(), tmpLabel.c_str()))
				return false;

			tmpName.Format("%s.c", name);
			tmpLabel.Format("%s.c", label);
			if (!ar(obb.c, tmpName.c_str(), tmpLabel.c_str()))
				return false;

			return true;
		}

		template<typename T>
		struct SSharedPtrSerializer
		{
			std::shared_ptr<T>& ptr;

			SSharedPtrSerializer(std::shared_ptr<T>& ptr_) : ptr(ptr_) {}

			void Serialize(Serialization::IArchive& ar)
			{
				bool ptrExists = (ptr != nullptr);
				if (ar.isInput())
				{
					if (ar(ptrExists, "exists"))
					{
						if (ptrExists)
						{
							ptr.reset(new T);
							ar(*ptr, "ptr");
						}
						else
						{
							ptr.reset();
						}
					}
				}
				else
				{
					ar(ptrExists, "exists");
					if (ptrExists)
					{
						ar(*ptr, "ptr");
					}
				}
			}
		};

		bool Serialize(Serialization::IArchive& ar, std::shared_ptr<CHistoricQuery>& ptr, const char* szName, const char* szLabel)
		{
			SSharedPtrSerializer<CHistoricQuery> ser(ptr);
			return ar(ser, szName, szLabel);
		}

	}
}
