#pragma once 

#include <CrySerialization/yasli/Archive.h>

namespace yasli
{

template<typename Type>
struct DefaultSinglestep
{
	static double value()
	{
		return std::is_integral<Type>::value ? 1.0 : 0.1;
	}
};

template<class T>
struct RangeDecorator
{
	T* value;
	T hardMin;
	T hardMax;
	T singleStep;

	void YASLI_SERIALIZE_METHOD(Archive& ar) {}
};

template<class T>
RangeDecorator<T> Range(T& value, T hardMin, T hardMax, T singleStep = (T)DefaultSinglestep<T>::value())
{
	RangeDecorator<T> r;
	r.value = &value;
	r.hardMin = hardMin;
	r.hardMax = hardMax;
	r.singleStep = singleStep;
	return r;
}

template<class T>
bool YASLI_SERIALIZE_OVERRIDE(Archive& ar, RangeDecorator<T>& value, const char* name, const char* label)
{
	if (ar.isEdit())
		return ar(Serializer(value), name, label);
	else
		return ar(*value.value, name, label);
}

}
