/**
 *  yasli - Serialization Library.
 *  Copyright (C) 2007-2013 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#pragma once
#pragma warning (disable : 4100) 

#include <stdarg.h>

#include <CrySerialization/yasli/Config.h>
#include <CrySerialization/yasli/Helpers.h>
#include <CrySerialization/yasli/Serializer.h>
#include <CrySerialization/yasli/KeyValue.h>
#include <CrySerialization/yasli/TypeID.h>

namespace yasli{

class Archive;
struct CallbackInterface;

template<class T>
bool YASLI_SERIALIZE_OVERRIDE(Archive& ar, T& object, const char* name, const char* label);

class Object;
class KeyValueInterface;
class EnumDescription;
template <class Enum>
EnumDescription& getEnumDescription();
bool serializeEnum(const EnumDescription& desc, Archive& ar, int& value, const char* name, const char* label);

// Context is used to pass arguments to nested serialization functions.
// Example of usage:
//
// void EntityLibrary::serialize(yasli::Archive& ar)
// {
//   yasli::Context libraryContext(ar, this);
//   ar(entities, "entities");
// }
//
// void Entity::serialize(yasli::Archive& ar)
// {
//   if (EntityLibrary* library = ar.context<EntityLibrary>()) {
//     ...
//   }
// }
//
// You may have multiple contexts of different types, but note that the context
// lookup complexity is O(n) in respect to number of contexts.
struct Context {
	void* object;
	TypeID type;
	Context* previousContext;
	Archive* archive;

	Context() : archive(0), object(0), previousContext(0) {}
	template<class T>
	void set(T* object);
	template<class T>
	Context(Archive& ar, T* context);
	template<class T>
	Context(T* context);
	~Context();
};

struct BlackBox;
class Archive{
public:
	enum ArchiveCaps{
		INPUT = 1 << 0,
		OUTPUT = 1 << 1,
		TEXT = 1 << 2,
		BINARY = 1 << 3,
		EDIT = 1 << 4,
		INPLACE = 1 << 5,
		NO_EMPTY_NAMES = 1 << 6,
		VALIDATION = 1 << 7,
		DOCUMENTATION = 1 << 8,
		CUSTOM1 = 1 << 9,
		CUSTOM2 = 1 << 10,
		CUSTOM3 = 1 << 11
	};

	Archive(int caps)
	: lastContext_(0)
	, caps_(caps)
	, filter_(YASLI_DEFAULT_FILTER)
	, modifiedRow_(nullptr)
	{
	}
	virtual ~Archive() {}

	bool isInput() const{ return caps_ & INPUT ? true : false; }
	bool isOutput() const{ return caps_ & OUTPUT ? true : false; }
	bool isEdit() const{
#if YASLI_NO_EDITING
		return false;
#else
		return (caps_ & EDIT)  != 0;
#endif
	}
	bool isInPlace() const { return (caps_ & INPLACE) != 0; }
	bool isValidation() const { return (caps_ & VALIDATION) != 0; }
	bool caps(int caps) const { return (caps_ & caps) == caps; }

	void setFilter(int filter){
		filter_ = filter;
	}
	int getFilter() const{ return filter_; }
	bool filter(int flags) const{
		YASLI_ASSERT(flags != 0 && "flags is supposed to be a bit mask");
		YASLI_ASSERT(filter_ && "Filter is not set!");
		return (filter_ & flags) != 0;
	}

	const char* getModifiedRowName() const { return modifiedRow_ ? modifiedRow_ : ""; }
	void setModifiedRowName(const char* modifiedRow) { modifiedRow_ = modifiedRow; }

	virtual bool operator()(bool& value, const char* name = "", const char* label = 0) { notImplemented(); return false; }
	virtual bool operator()(char& value, const char* name = "", const char* label = 0) { notImplemented(); return false; }
	virtual bool operator()(u8& value, const char* name = "", const char* label = 0) { notImplemented(); return false; }
	virtual bool operator()(i8& value, const char* name = "", const char* label = 0) { notImplemented(); return false; }
	virtual bool operator()(i16& value, const char* name = "", const char* label = 0) { notImplemented(); return false; }
	virtual bool operator()(u16& value, const char* name = "", const char* label = 0) { notImplemented(); return false; }
	virtual bool operator()(i32& value, const char* name = "", const char* label = 0) { notImplemented(); return false; }
	virtual bool operator()(u32& value, const char* name = "", const char* label = 0) { notImplemented(); return false; }
	virtual bool operator()(i64& value, const char* name = "", const char* label = 0) { notImplemented(); return false; }
	virtual bool operator()(u64& value, const char* name = "", const char* label = 0) { notImplemented(); return false; }
	virtual bool operator()(float& value, const char* name = "", const char* label = 0) { notImplemented(); return false; }
	virtual bool operator()(double& value, const char* name = "", const char* label = 0) { notImplemented(); return false; }

	virtual bool operator()(StringInterface& value, const char* name = "", const char* label = 0)    { notImplemented(); return false; }
	virtual bool operator()(WStringInterface& value, const char* name = "", const char* label = 0)    { notImplemented(); return false; }
	virtual bool operator()(const Serializer& ser, const char* name = "", const char* label = 0) { notImplemented(); return false; }
	virtual bool operator()(BlackBox& ser, const char* name = "", const char* label = 0) { notImplemented(); return false; }
	virtual bool operator()(ContainerInterface& ser, const char* name = "", const char* label = 0) { return false; }
	virtual bool operator()(PointerInterface& ptr, const char* name = "", const char* label = 0);
	virtual bool operator()(Object& obj, const char* name = "", const char* label = 0) { return false; }
	virtual bool operator()(KeyValueInterface& keyValue, const char* name = "", const char* label = 0) { return operator()(Serializer(keyValue), name, label); }
	virtual bool operator()(CallbackInterface& callback, const char* name = "", const char* label = 0) { return false; }

	// No point in supporting long double since it is represented as double on MSVC
	bool operator()(long double& value, const char* name = "", const char* label = 0)         { notImplemented(); return false; }

	template<class T>
	bool operator()(const T& value, const char* name = "", const char* label = 0);

	// Error and Warning calls are used for diagnostics and validation of the
	// values. Output depends on the specific implementation of Archive,
	// for example PropertyTree uses it to show bubbles with errors in UI
	// next to the mentioned property.
	template<class T> void error(T& value, const char* format, ...);
	template<class T> void warning(T& value, const char* format, ...);

	void error(const void* value, const yasli::TypeID& type, const char* format, ...);
	// Used to add tooltips in PropertyTree
	void doc(const char* docString);

	// block call are osbolete, please do not use
	virtual bool openBlock(const char* name, const char* label) { return true; }
	virtual void closeBlock() {}

	template<class T>
	T* context() const {
		return (T*)contextByType(TypeID::get<T>());
	}

	void* contextByType(const TypeID& type) const {
		for (Context* current = lastContext_; current != 0; current = current->previousContext)
			if (current->type == type)
				return current->object;
		return 0;
	}

	Context* setLastContext(Context* context) {
		Context* previousContext = lastContext_;
		lastContext_ = context;
		return previousContext;
	}
	Context* lastContext() const{ return lastContext_; }
protected:
	virtual void validatorMessage(bool error, const void* handle, const TypeID& type, const char* message) {}
	virtual void documentLastField(const char* text) {}

	void notImplemented() { YASLI_ASSERT(0 && "Not implemented!"); }

	int caps_;
	int filter_;
	const char* modifiedRow_;
	Context* lastContext_;
};

namespace Helpers{

template<class T>
struct SerializeStruct{
	static bool invoke(Archive& ar, T& value, const char* name, const char* label){
		Serializer serializer(value);
		return ar(serializer, name, label);
	};
};

template<class Enum>
struct SerializeEnum{
	static bool invoke(Archive& ar, Enum& value, const char* name, const char* label){
		const EnumDescription& enumDescription = getEnumDescription<Enum>();
		return serializeEnum(enumDescription, ar, reinterpret_cast<int&>(value), name, label);
	};
};

template<class T>
struct SerializeArray{};

template<class T, int Size>
struct SerializeArray<T[Size]>{
	static bool invoke(Archive& ar, T value[Size], const char* name, const char* label){
		ContainerArray<T, Size> ser(value);
		return ar(static_cast<ContainerInterface&>(ser), name, label);
	}
};

}

template<class T>
Context::Context(Archive& ar, T* context) {
	archive = &ar;
	object = (void*)context;
	type = TypeID::get<T>();
	if (archive)
		previousContext = ar.setLastContext(this);
	else
		previousContext = 0;
}

template<class T>
Context::Context(T* context) {
    archive = 0;
    previousContext = 0;
    set<T>(context);
}

template<class T>
void Context::set(T* object) {
	this->object = (void*)object;
	type = TypeID::get<T>();
}

inline Context::~Context() {
	if (archive)
		archive->setLastContext(previousContext);
}

inline void Archive::doc(const char* docString)
{
#if !YASLI_NO_EDITING
	if (caps_ & DOCUMENTATION)
		documentLastField(docString); 
#endif
}

template<class T>
void Archive::error(T& value, const char* format, ...)
{
#if !YASLI_NO_EDITING
	if ((caps_ & VALIDATION) == 0)
		return;
	va_list args;
	va_start(args, format);
	char buf[1024];
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	validatorMessage(true, &value, TypeID::get<T>(), buf);
#endif
}

inline void Archive::error(const void* handle, const yasli::TypeID& type, const char* format, ...)
{
#if !YASLI_NO_EDITING
	if ((caps_ & VALIDATION) == 0)
		return;
	va_list args;
	va_start(args, format);
	char buf[1024];
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	validatorMessage(true, handle, type, buf);
#endif
}

template<class T>
void Archive::warning(T& value, const char* format, ...)
{
#if !YASLI_NO_EDITING
	if ((caps_ & VALIDATION) == 0)
		return;
	va_list args;
	va_start(args, format);
	char buf[1024];
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	validatorMessage(false, &value, TypeID::get<T>(), buf);
#endif
}

template<class T>
bool Archive::operator()(const T& value, const char* name, const char* label){
    return YASLI_SERIALIZE_OVERRIDE(*this, const_cast<T&>(value), name, label);
}

inline bool Archive::operator()(PointerInterface& ptr, const char* name, const char* label)
{
	Serializer ser(ptr);
	return operator()(ser, name, label);
}

template<class T, int Size>
bool YASLI_SERIALIZE_OVERRIDE(Archive& ar, T object[Size], const char* name, const char* label)
{
	YASLI_ASSERT(0);
	return false;
}

template<class T>
bool YASLI_SERIALIZE_OVERRIDE(Archive& ar, const T& object, const char* name, const char* label)
{
	T::unable_to_serialize_CONST_object();
	YASLI_ASSERT(0);
	return false;
}

// Archives are using yasli integer typedefs (taken from stdint.h). These
// types will be selected directly by Archive::operator() overloads. Those,
// however, do not cover all C++ integer types, as compilers usually have
// multiple integer types of the same type (like int and long in MSVC/x86). We
// would like to handle handle all C++ standard types, but not add duplicated
// and unportable methods to each archive implementations. The remaining types
// are cast to types of the same in functions below. E.g. long will be cast to
// int on MSVC/x86 and long long will be cast to long on GCC/x64.
template<class T, class CastTo>
struct CastInteger
{
	static bool invoke(Archive& ar, T& value, const char* name, const char* label)
	{
		return ar(reinterpret_cast<CastTo&>(value), name, label);
	}
};

template<class T>
bool castInteger(Archive& ar, T& v, const char* name, const char* label)
{
	using namespace Helpers;
	return 
		Select< IsSigned<T>,
			SelectIntSize< sizeof(T),
				CastInteger<T, i8>,
				CastInteger<T, i16>,
				CastInteger<T, i32>,
				CastInteger<T, i64>
			>,
			SelectIntSize< sizeof(T),
				CastInteger<T, u8>,
				CastInteger<T, u16>,
				CastInteger<T, u32>,
				CastInteger<T, u64>
			>
		>::type::invoke(ar, v, name, label);
}
inline bool YASLI_SERIALIZE_OVERRIDE(Archive& ar, unsigned char& v, const char* name, const char* label) { return castInteger(ar, v, name, label); }
inline bool YASLI_SERIALIZE_OVERRIDE(Archive& ar, unsigned short& v, const char* name, const char* label) { return castInteger(ar, v, name, label); }
inline bool YASLI_SERIALIZE_OVERRIDE(Archive& ar, unsigned int& v, const char* name, const char* label) { return castInteger(ar, v, name, label); }
inline bool YASLI_SERIALIZE_OVERRIDE(Archive& ar, unsigned long& v, const char* name, const char* label) { return castInteger(ar, v, name, label); }
inline bool YASLI_SERIALIZE_OVERRIDE(Archive& ar, unsigned long long& v, const char* name, const char* label) { return castInteger(ar, v, name, label); }
inline bool YASLI_SERIALIZE_OVERRIDE(Archive& ar, signed char& v, const char* name, const char* label) { return castInteger(ar, v, name, label); }
inline bool YASLI_SERIALIZE_OVERRIDE(Archive& ar, signed short& v, const char* name, const char* label) { return castInteger(ar, v, name, label); }
inline bool YASLI_SERIALIZE_OVERRIDE(Archive& ar, signed int& v, const char* name, const char* label) { return castInteger(ar, v, name, label); }
inline bool YASLI_SERIALIZE_OVERRIDE(Archive& ar, signed long& v, const char* name, const char* label) { return castInteger(ar, v, name, label); }
inline bool YASLI_SERIALIZE_OVERRIDE(Archive& ar, signed long long& v, const char* name, const char* label) { return castInteger(ar, v, name, label); }
#ifdef _NATIVE_WCHAR_T_DEFINED // MSVC
inline bool YASLI_SERIALIZE_OVERRIDE(Archive& ar, wchar_t& v, const char* name, const char* label) { return castInteger(ar, v, name, label); }
#endif


template<class T>
bool YASLI_SERIALIZE_OVERRIDE(Archive& ar, T& object, const char* name, const char* label)
{
	using namespace Helpers;

	return
		Select< IsClass<T>,
				Identity< SerializeStruct<T> >,
				Select< IsArray<T>,
					Identity< SerializeArray<T> >,
					Identity< SerializeEnum<T> >
				>
		>::type::invoke(ar, object, name, label);
}

}

#include <CrySerialization/yasli/SerializerImpl.h>

// vim: ts=4 sw=4:
