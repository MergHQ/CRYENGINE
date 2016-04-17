// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// Created by: Scott Peter
//---------------------------------------------------------------------------

#ifndef _CRY_ARRAY_H_
#define _CRY_ARRAY_H_
#pragma once

#include <utility>
#include <type_traits>
#include <CryMemory/IGeneralMemoryHeap.h> // <> required for Interfuscator

//! Stack array helper.
#define ALIGNED_STACK_ARRAY(T, name, size, alignment)         \
  PREFAST_SUPPRESS_WARNING(6255)                              \
  T * name = (T*) alloca((size) * sizeof(T) + alignment - 1); \
  name = Align(name, alignment);

#define STACK_ARRAY(T, name, size)               \
  ALIGNED_STACK_ARRAY(T, name, size, alignof(T)) \

//! Specify semantics for moving objects.
//! If raw_movable() is true, objects will be moved with memmove().
//! If false, with the templated move_init() function.
template<class T>
bool raw_movable(T const&)
{
#if !CRY_PLATFORM_LINUX && !CRY_PLATFORM_ANDROID && !CRY_PLATFORM_APPLE
	return std::is_trivially_move_constructible<T>();
#else
	return false;
#endif
}

//! Generic move function: transfer an existing source object to uninitialised dest address.
//! Addresses must not overlap (requirement on caller).
//! May be specialised for specific types, to provide a more optimal move.
//! For types that can be trivially moved (memcpy), do not specialise move_init, rather specialise raw_movable to return true.
template<class T>
void move_init(T& dest, T& source)
{
	assert(&dest != &source);
	new(&dest)T(std::move(source));
	source.~T();
}

/*---------------------------------------------------------------------------
   Public classes:

    Array<T, [I, STORAGE]>
    StaticArray<T, nSIZE, [I]>
    DynArray<T, [I, STORAGE, ALLOC]>
    FastDynArray<T, [I]>
    FixedDynArray<T, [I]>
    StaticDynArray<T, nSIZE, [I]>

   Support classes are placed in namespaces NArray and NAlloc to reduce global name usage.
   ---------------------------------------------------------------------------*/

namespace NArray
{
// We should never have these defined as macros.
#undef min
#undef max

//! Define our own min, to avoid including entire <algorithm>.
template<class T> inline T min(T a, T b)
{ return a < b ? a : b; }
//! Define our own min, to avoid including entire <algorithm>.
template<class T> inline T max(T a, T b)
{ return a > b ? a : b; }

//! Automatic inference of signed from unsigned int type.
template<class T> struct IntTraits
{
	typedef T TSigned;
};

template<> struct IntTraits<uint>
{
	typedef int TSigned;
};
template<> struct IntTraits<uint64>
{
	typedef int64 TSigned;
};
#if !CRY_PLATFORM_LINUX && !CRY_PLATFORM_ANDROID && !CRY_PLATFORM_APPLE
template<> struct IntTraits<unsigned long>
{
	typedef long TSigned;
};
#endif

/*---------------------------------------------------------------------------
   //! STORAGE prototype for Array<T,I,STORAGE>.
   struct Storage
   {
   template<class T, class I>
   struct Store
   {
    [const] T* begin() [const];
    I size() const;
   };
   };
   ---------------------------------------------------------------------------*/

//! ArrayStorage: Default STORAGE Array<T,I,STORAGE>.
//! Simply contains a pointer and count to an existing array,
//! performs no allocation or deallocation.
struct ArrayStorage
{
	template<class T, class I = int>
	struct Store
	{
		Store()
			: m_aElems(0), m_nCount(0)
		{}
		Store(T* elems, I count)
			: m_aElems(elems), m_nCount(count)
		{}
		Store(T* start, T* finish)
			: m_aElems(start), m_nCount(check_cast<I>(finish - start))
		{}

		void set(T* elems, I count)
		{
			m_aElems = elems;
			m_nCount = count;
		}

		//! Basic storage.
		CONST_VAR_FUNCTION(T * begin(),
		                   { return m_aElems; })
		inline I size() const
		{ return m_nCount; }

		//! Modifiers, alter range in place.
		void erase_front(I count = 1)
		{
			assert(count >= 0 && count <= m_nCount);
			m_nCount -= count;
			m_aElems += count;
		}

		void erase_back(I count = 1)
		{
			assert(count >= 0 && count <= m_nCount);
			m_nCount -= count;
		}

		void resize(I count)
		{
			assert(count >= 0 && count <= m_nCount);
			m_nCount = count;
		}

	protected:
		T* m_aElems;
		I  m_nCount;
	};
};

//---------------------------------------------------------------------------
//! StaticArrayStorage: STORAGE scheme with a statically sized member array.
template<int nSIZE>
struct StaticArrayStorage
{
	template<class T, class I = int>
	struct Store
	{
		//! Basic storage.
		CONST_VAR_FUNCTION(T * begin(),
		                   { return m_aElems; })
		inline static I size()
		{ return (I)nSIZE; }

	protected:
		T m_aElems[nSIZE];
	};
};
};

//---------------------------------------------------------------------------
//! Array<T,I,S>: Non-growing array.
//! S serves as base class, and implements storage scheme: begin(), size().
template<class T, class I = int, class STORE = NArray::ArrayStorage>
struct Array : STORE::template Store<T, I>
{
	typedef typename STORE::template Store<T, I> S;

	// Tedious redundancy.
	using S::size;
	using S::begin;

	// STL-compatible typedefs.
	typedef T        value_type;
	typedef T*       pointer;
	typedef const T* const_pointer;
	typedef T&       reference;
	typedef const T& const_reference;

	typedef T*       iterator;
	typedef const T* const_iterator;

	typedef I        size_type;
	typedef typename NArray::IntTraits<size_type>::TSigned
	  difference_type;

	typedef Array<T, I>       array;
	typedef Array<const T, I> const_array;

	// Construction.
	Array()
	{}

	//! Forward single- and double-argument constructors.
	template<class In>
	explicit Array(const In& i)
		: S(i)
	{}

	template<class In1, class In2>
	Array(const In1& i1, const In2& i2)
		: S(i1, i2)
	{}

	// Accessors.
	inline bool      empty() const
	{ return size() == 0; }
	inline size_type size_mem() const
	{ return size() * sizeof(T); }

	CONST_VAR_FUNCTION(T * data(),
	                   { return begin(); })

	CONST_VAR_FUNCTION(T * end(),
	                   { return begin() + size(); })

	CONST_VAR_FUNCTION(T * rbegin(),
	                   { return begin() + size() - 1; })

	CONST_VAR_FUNCTION(T * rend(),
	                   { return begin() - 1; })

	CONST_VAR_FUNCTION(T & front(),
	{
		assert(!empty());
		return *begin();
	})

	CONST_VAR_FUNCTION(T & back(),
	{
		assert(!empty());
		return *rbegin();
	})

	CONST_VAR_FUNCTION(T & at(size_type i),
	{
		assert(i >= 0 && i < size());
		return begin()[i];
	})

	CONST_VAR_FUNCTION(T & operator[](size_type i),
	{
		assert(i >= 0 && i < size());
		return begin()[i];
	})

	// Allow both indices and iterators as position indicators
	size_type index(size_type i) const
	{
		assert(i >= 0 && i <= size());
		return i;
	}

	size_type index(const_iterator it) const
	{
		assert(it >= begin() && it <= end());
		return size_type(it - begin());
	}

	//! Conversion to canonical array type.
	operator array()
	{ return array(begin(), size()); }
	operator const_array() const
	{ return const_array(begin(), size()); }

	//! Additional conversion via operator() to full or sub array.
	array operator()(size_type i, size_type count)
	{
		assert(i >= 0 && count >= 0 && i + count <= size());
		return array(begin() + i, count);
	}
	const_array operator()(size_type i, size_type count) const
	{
		assert(i >= 0 && count >= 0 && i + count <= size());
		return const_array(begin() + i, count);
	}

	array       operator()(size_type i = 0)
	{ return (*this)(i, size() - i); }
	const_array operator()(size_type i = 0) const
	{ return (*this)(i, size() - i); }

	// Basic element assignment functions.

	//! Copy values to existing elements.
	void fill(const T& val)
	{
		for (auto& e : * this)
			e = val;
	}

	void copy(const_array source)
	{
		assert(source.size() >= size());
		const T* s = source.begin();
		for (auto& e : * this)
			e = *s++;
	}

	// Raw element construct/destruct functions.
	iterator init()
	{
		for (auto& e : * this)
			new(&e)T;
		return begin();
	}
	iterator init(const T& val)
	{
		for (auto& e : * this)
			new(&e)T(val);
		return begin();
	}
	iterator init(const_array source)
	{
		assert(source.size() >= size());
		assert(source.end() <= begin() || source.begin() >= end());
		const_iterator s = source.begin();
		for (auto& e : * this)
			new(&e)T(*s++);
		return begin();
	}

	iterator move_init(array source)
	{
		iterator s = source.begin();
		if (s != begin())
		{
			if (source.size() < size())
				new(this)Array(begin(), source.size());

			if (raw_movable(*s))
			{
				memmove(begin(), s, size_mem());
			}
			else if (s > begin() || source.end() <= begin())
			{
				for (auto& e : * this)
					::move_init(e, *s++);
			}
			else
			{
				s += size();
				for (iterator it = end(); it > begin(); )
					::move_init(*--it, *--s);
			}
		}
		return begin();
	}

	void destroy()
	{
		// Destroy in reverse order, to complement construction order.
		for (iterator it = end(); it-- > begin(); )
			it->~T();
	}
};

// Type-inferring constructor.

template<class T, class I>
inline Array<T, I> ArrayT(T* elems, I count)
{
	return Array<T, I>(elems, count);
}

template<class T>
inline Array<T, size_t> ArrayT(T* start, T* finish)
{
	return Array<T, size_t>(start, finish);
}

//! A superior alternative to static C arrays.
//! Provides standard STL-like Array interface, including bounds-checking.
//!     standard:		Type array[256];
//!     structured:	StaticArray<Type,256> array;
template<class T, int nSIZE, class I = int>
struct StaticArray : Array<T, I, NArray::StaticArrayStorage<nSIZE>>
{
};

//! Specify allocation for dynamic arrays.
namespace NAlloc
{
// Multi-purpose allocation function prototype
struct AllocArray
{
	void*  data;
	size_t size;

	AllocArray(void* data_ = 0, size_t size_ = 0)
		: data(data_), size(size_) {}

	template<class T, class I>
	AllocArray(Array<T, I> array)
		: data(array.data()), size(array.size_mem()) {}

	template<class T, class I>
	operator Array<T, I>() const
	{ return Array<T, I>((T*)data, check_cast<I>(size / sizeof(T))); }

	void adjust_front(int n)
	{
		if (data)
			(char*&)data += n;
		if (size)
			size -= n;
	}
};
typedef AllocArray (* Allocator)(AllocArray a, size_t nSize, size_t nAlign, bool bSlack);

//! Allocation utilities.
inline size_t realloc_size(size_t nMinSize)
{
	// Choose an efficient realloc size, when growing an existing (non-zero) block.
	// Find the next power-of-two, minus a bit of presumed system alloc overhead.
	static const size_t nMinAlloc = 32;
	static const size_t nOverhead = 16;
	static const size_t nDoubleLimit =
	  sizeof(size_t) < 8 ? 1 << 12    // 32-bit system
	  : 1 << 16;                      // >= 64-bit system

	nMinSize += nOverhead;
	size_t nAlloc = nMinAlloc;
	while (nAlloc < nMinSize)
		nAlloc <<= 1;
	if (nAlloc > nDoubleLimit)
	{
		size_t nAlign = NArray::max(nAlloc >> 3, nDoubleLimit);
		nAlloc = Align(nMinSize, nAlign);
	}
	return nAlloc - nOverhead;
}

template<class A, class T, class I>
Array<T, I> reallocate(A& allocator, Array<T, I> a, I old_size, I& new_size, size_t alignment = 1, bool allow_slack = false)
{
	Array<T, I> aNew = allocator.alloc(a, new_size * sizeof(T), alignment, allow_slack);

	assert(IsAligned(aNew.data(), alignment));
	assert(aNew.size() >= new_size);

	if (aNew.data() && a.data() && aNew.data() != a.data())
	{
		// Move elements.
		aNew(0, NArray::min(old_size, new_size)).move_init(a);

		// Dealloc old.
		allocator.alloc(a, 0, alignment);
	}

	return aNew;
}

struct AllocFunction
{
	Allocator  m_Function;

	AllocArray alloc(AllocArray a, size_t nSize, size_t nAlign = 1, bool bSlack = false)
	{
		return m_Function(a, nSize, nAlign, bSlack);
	}
};

//! Adds prefix bytes to allocation, preserving alignment.
template<class A, class Prefix, int nSizeAlign = 1>
struct AllocPrefix : A
{
	AllocArray alloc(AllocArray a, size_t nSize, size_t nAlign = 1, bool bSlack = false)
	{
		// Adjust pointer and size for prefix bytes
		nAlign = NArray::max(nAlign, alignof(Prefix));
		int nPrefixSize = check_cast<int>(Align(sizeof(Prefix), nAlign));

		a.adjust_front(-nPrefixSize);
		if (nSize)
		{
			nSize = Align(nSize, nSizeAlign);
			nSize += nPrefixSize;
		}

		a = A::alloc(a, nSize, nAlign, bSlack);

		a.adjust_front(nPrefixSize);
		return a;
	}
};

//! Stores and retrieves allocator function in memory, for compatibility with diverse allocators.
template<class A>
struct AllocCompatible
{
	AllocArray alloc(AllocArray a, size_t nSize, size_t nAlign = 1, bool bSlack = false)
	{
		AllocPrefix<AllocFunction, Allocator> alloc_prefix;
		if (a.data)
		{
			// Retrieve original allocation function, for realloc or dealloc
			alloc_prefix.m_Function = ((Allocator*)a.data)[-1];
		}
		else
		{
			// Allocate new with this module's base_allocator
			alloc_prefix.m_Function = &A::alloc;
		}
		a = alloc_prefix.alloc(a, nSize, nAlign, bSlack);
		if (a.data)
			// Store the pointer to the allocator
			((Allocator*)a.data)[-1] = alloc_prefix.m_Function;
		return a;
	}
};

//---------------------------------------------------------------------------
// Allocators for DynArray.

//! No reallocation, for use in FixedDynArray.
struct NullAlloc
{
	static AllocArray alloc(AllocArray a, size_t nSize, size_t nAlign = 1, bool bSlack = false)
	{ return a; }
};

//! Standard CryModule memory allocation, using aligned versions.
struct ModuleAlloc
{
	static AllocArray alloc(AllocArray a, size_t nSize, size_t nAlign = 1, bool bSlack = false)
	{
		if (nSize)
		{
			if (nSize != a.size)
			{
				// Alloc new
				if (bSlack)
					nSize = realloc_size(nSize);
				a.data = CryModuleMemalign(nSize, nAlign);
				a.size = nSize;
			}
		}
		else if (a.data)
		{
			// Dealloc
			CryModuleMemalignFree(a.data);
			a = AllocArray();
		}
		return a;
	}
};

//! Standard allocator for DynArray stores a compatibility pointer in the memory.
typedef AllocCompatible<ModuleAlloc> StandardAlloc;

//! Allocator using specific heaps.
struct GeneralHeapAlloc : ModuleAlloc
{
	IGeneralMemoryHeap* m_pHeap;

	GeneralHeapAlloc()
		: m_pHeap(0) {}

	explicit GeneralHeapAlloc(IGeneralMemoryHeap* pHeap)
		: m_pHeap(pHeap) {}

	AllocArray alloc(AllocArray a, size_t nSize, size_t nAlign = 1, bool bSlack = false) const
	{
		if (m_pHeap)
		{
			if (!nSize)
			{
				if (a.data)
					// Dealloc
					m_pHeap->Free(a.data);
				return AllocArray();
			}
			else if (nSize != a.size)
			{
				// Alloc
				if (bSlack)
					nSize = realloc_size(nSize);
				a.data = m_pHeap->Memalign(nAlign, nSize, "");
				a.size = nSize;
			}
			return a;
		}

		return ModuleAlloc::alloc(a, nSize, nAlign, bSlack);
	}
};
};

//---------------------------------------------------------------------------
//! Storage schemes for dynamic arrays.
namespace NArray
{
/*---------------------------------------------------------------------------
   //! STORAGE prototype for DynArray<T,I,STORAGE>.
   //! Extends ArrayStorage with resizing functionality.
   struct DynStorage
   {
   struct Store<T,I>: ArrayStorage<T,I>::Store
   {
    I capacity() const;
    size_t get_alloc_size() const;
    void resize_raw( I new_size, bool allow_slack );
   };
   };
   ---------------------------------------------------------------------------*/

//! FastDynStorage: STORAGE scheme for DynArray<T,I,STORAGE>.
//! Simple extension to ArrayStorage: size & capacity fields are inline, 3 words storage, fast access.
template<class A = NAlloc::StandardAlloc>
struct FastDynStorage
{
	template<class T, class I>
	struct Store : protected A, public ArrayStorage::Store<T, I>
	{
		typedef ArrayStorage::Store<T, I> super_type;

		using super_type::m_aElems;
		using super_type::m_nCount;

		Store()
			: m_nCapacity(0)
		{
			MEMSTAT_REGISTER_CONTAINER(this, EMemStatContainerType::MSC_Vector, T);
		}

		Store(const A& a)
			: A(a), m_nCapacity(0)
		{
			MEMSTAT_REGISTER_CONTAINER(this, EMemStatContainerType::MSC_Vector, T);
		}

		~Store()
		{
			if (m_nCapacity)
			{
				A::alloc(Array<T, I>(m_aElems, m_nCapacity), 0, alignof(T));
				MEMSTAT_BIND_TO_CONTAINER(this, 0);
			}
		}

		I capacity() const
		{ return m_nCapacity; }

		size_t get_alloc_size() const
		{ return capacity() * sizeof(T); }

		void resize_raw(I new_size, bool allow_slack = false)
		{
			if (allow_slack ? new_size > capacity() : new_size != capacity())
			{
				Array<T, I> a = NAlloc::reallocate(static_cast<A&>(*this), Array<T, I>(m_aElems, m_nCapacity), m_nCount, new_size, alignof(T), allow_slack);
				m_aElems = a.data();
				m_nCapacity = a.size();
				MEMSTAT_BIND_TO_CONTAINER(this, m_aElems);
			}
			set_size(new_size);
		}

	protected:

		I    m_nCapacity;

		void set_size(I new_size)
		{
			assert(new_size >= 0 && new_size <= capacity());
			m_nCount = new_size;
			MEMSTAT_USAGE(m_aElems, new_size * sizeof(T));
		}
	};
};

//---------------------------------------------------------------------------
//! SmallDynStorage: STORAGE scheme for DynArray<T,I,STORAGE,ALLOC>.
//! Array is just a single pointer, size and capacity information stored before the array data.
//! Slightly slower than FastDynStorage, optimal for saving space, especially when array likely to be empty.
template<class A = NAlloc::StandardAlloc>
struct SmallDynStorage
{
	template<class T, class I>
	struct Store : protected A
	{
		struct Header
		{
			static const I nCAP_BIT = I(1) << (sizeof(I) * 8 - 1);

			ILINE char*    data() const
			{
				assert(IsAligned(this, sizeof(I)));
				return (char*)(this + 1);
			}
			ILINE bool is_null() const
			{ return m_nSizeCap == 0; }
			ILINE I    size() const
			{ return m_nSizeCap & ~nCAP_BIT; }

			I capacity() const
			{
				I aligned_bytes = Align(size() * sizeof(T), sizeof(I));
				if (m_nSizeCap & nCAP_BIT)
				{
					// Capacity stored in word following data
					return *(I*)(data() + aligned_bytes);
				}
				else
				{
					// Capacity - size < sizeof(I)
					return aligned_bytes / sizeof(T);
				}
			}

			void set_sizes(I s, I c)
			{
				// Store size, and assert against overflow.
				assert(s <= c);
				m_nSizeCap = s;
				I aligned_bytes = Align(s * sizeof(T), sizeof(I));
				if (c * sizeof(T) >= aligned_bytes + sizeof(I))
				{
					// Has extra capacity, more than word-alignment
					m_nSizeCap |= nCAP_BIT;
					*(I*)(data() + aligned_bytes) = c;
				}
				assert(size() == s);
				assert(capacity() == c);
			}

		protected:
			I m_nSizeCap;           //!< Store allocation size, with last bit indicating extra capacity.

		public:

			static T* null_header()
			{
				// m_aElems is never 0, for empty array points to a static empty header.
				// Declare a big enough static var to account for alignment.
				struct EmptyHeader
				{
					Header head;
					char   pad[alignof(T)];
				};
				static EmptyHeader s_EmptyHeader;

				// The actual header pointer can be anywhere in the struct, it's all initialized to 0.
				static T* s_EmptyElems = (T*)Align(s_EmptyHeader.pad, alignof(T));

				return s_EmptyElems;
			}
		};

		Store()
		{
			set_null();
			MEMSTAT_REGISTER_CONTAINER(this, EMemStatContainerType::MSC_Vector, T);
		}

		Store(const A& a)
			: A(a)
		{
			set_null();
			MEMSTAT_REGISTER_CONTAINER(this, EMemStatContainerType::MSC_Vector, T);
		}

		~Store()
		{
			if (!header()->is_null())
			{
				allocator().alloc(Array<T, I>(m_aElems, capacity()), 0, alignof(T));
				MEMSTAT_BIND_TO_CONTAINER(this, 0);
			}
		}

		// Basic storage.
		CONST_VAR_FUNCTION(T * begin(),
		                   { return m_aElems; })
		inline I size() const
		{ return header()->size(); }
		inline I capacity() const
		{ return header()->capacity(); }
		size_t   get_alloc_size() const
		{ return capacity() * sizeof(T); }

		void resize_raw(I new_size, bool allow_slack = false)
		{
			I new_cap = capacity();
			if (allow_slack ? new_size > new_cap : new_size != new_cap)
			{
				Array<T, I> a(header()->is_null() ? 0 : m_aElems, capacity());
				a = NAlloc::reallocate(allocator(), a, size(), new_size, alignof(T), allow_slack);
				MEMSTAT_BIND_TO_CONTAINER(this, a.data());
				if (!a.data())
				{
					set_null();
					return;
				}
				m_aElems = a.data();
				new_cap = a.size();
			}
			header()->set_sizes(new_size, new_cap);
			MEMSTAT_USAGE(begin(), new_size * sizeof(T));
		}

	protected:

		T* m_aElems;

		CONST_VAR_FUNCTION(Header * header(),
			{
				assert(m_aElems);
				return ((Header*)m_aElems) - 1;
		  })

		void set_null()
		{ m_aElems = Header::null_header(); }
		bool is_null() const
		{ return header()->is_null(); }

		typedef NAlloc::AllocPrefix<A, Header, sizeof(I)> AP;

		AP& allocator()
		{
			COMPILE_TIME_ASSERT(sizeof(AP) == sizeof(A));
			return *(AP*)this;
		}
		const AP& allocator() const
		{
			return *(const AP*)this;
		}
	};
};

//---------------------------------------------------------------------------
//! StaticDynStorage: STORAGE scheme with a statically sized member array.
template<int nSIZE>
struct StaticDynStorage
{
	template<class T, class I = int>
	struct Store : ArrayStorage::Store<T, I>
	{
		Store()
			:           ArrayStorage::Store<T, I>((T*)Align(m_aData, alignof(T)), 0) {}

		static I      capacity()
		{ return (I)nSIZE; }
		static size_t get_alloc_size()
		{ return 0; }

		void resize_raw(I new_size, bool allow_slack = false)
		{
			// cannot realloc, just set size
			assert(new_size >= 0 && new_size <= capacity());
			this->m_nCount = new_size;
		}

	protected:

		char m_aData[nSIZE * sizeof(T) + alignof(T) - 1];       //!< Storage for elems, deferred construction.
	};
};
};

//! Legacy base class of DynArray, only used for read-only access.
#define DynArrayRef DynArray

//---------------------------------------------------------------------------
//! DynArray<T,I,S>: Extension of Array allowing dynamic allocation.
//! S specifies storage scheme, as with Array, but adds resize(), capacity(), ...
//! A specifies the actual memory allocation function: alloc().
//! Behaves in almost all respects like std::vector. However:.
//!     When growing a DynArray (resize, insert, etc) without specifiying an initialization value,.
//!     new elements are initialized with T, not T(). This means if a default constructor exists,.
//!     it is called, otherwise (for example, basic types) the elements are not initialized.
template<class T, class I = int, class STORE = NArray::SmallDynStorage<>>
struct DynArray : Array<T, I, STORE>
{
	typedef Array<T, I, STORE>                   super_type;
	typedef typename STORE::template Store<T, I> S;

	// Tedious redundancy for non-MS compilers
	using_type(super_type, size_type);
	using_type(super_type, iterator);
	using_type(super_type, const_iterator);
	using_type(super_type, array);
	using_type(super_type, const_array);

	using super_type::size;
	using super_type::capacity;
	using super_type::begin;
	using super_type::end;
	using super_type::index;
	using super_type::copy;
	using super_type::init;
	using super_type::destroy;

	DynArray()
	{}

	//! Init from basic storage type.
	DynArray(const S& s)
		: super_type(s)
	{}

	DynArray(size_type count)
	{
		grow(count);
	}
	DynArray(size_type count, const T& val)
	{
		grow(count, val);
	}

	//! Copying from a generic array type.
	DynArray(const_array a)
	{
		push_back(a);
	}
	DynArray& operator=(const_array a)
	{
		if (a.begin() >= begin() && a.end() <= end())
		{
			// Assigning from (partial) self; remove undesired elements.
			size_type front_elems = index(a.begin());
			pop_back(size() - index(a.end()));
			erase(0, front_elems);
		}
		else
		{
			// Assert no overlap.
			assert(a.end() <= begin() || a.begin() >= end());
			if (a.size() == size())
			{
				// If same size, perform element copy.
				copy(a);
			}
			else
			{
				// If different sizes, destroy then copy init elements.
				pop_back(size());
				push_back(a);
			}
		}
		return *this;
	}

	//! Copy init/assign.
	inline DynArray(const DynArray& a)
	{
		push_back(a());
	}
	inline DynArray& operator=(const DynArray& a)
	{
		return *this = a();
	}

	super_type& super()
	{
		return *this;
	}

	//! Move init/assign, by copying and clearing array storage structure; does not realloc or copy elements.
	DynArray(DynArray&& a)
		: super_type(a)
	{
		// Reset source state to avoid destruction
		new(&a)super_type;
	}
	DynArray& operator=(DynArray&& a)
	{
		this->~DynArray();
		new(this)DynArray(std::move(a));
		return *this;
	}

	inline ~DynArray()
	{
		destroy();
	}

	void swap(DynArray& a)
	{
		// Swap storage structures, no element copying
		super_type temp = super();
		super() = a.super();
		a.super() = temp;

		// Disable destructor on temp
		new(&temp)S;
		MEMSTAT_SWAP_CONTAINERS(this, &a);
	}

	inline size_type available() const
	{
		return capacity() - size();
	}

	// Allocation modifiers.

	void reserve(size_type count)
	{
		if (count > capacity())
		{
			I s = size();
			S::resize_raw(count, false);
			S::resize_raw(s, true);
		}
	}

	//! Grow array, return iterator to new raw elems.
	iterator grow_raw(size_type count = 1, bool allow_slack = true)
	{
		S::resize_raw(size() + count, allow_slack);
		return end() - count;
	}
	Array<T, I> append_raw(size_type count = 1, bool allow_slack = true)
	{
		return Array<T, I>(grow_raw(count, allow_slack), count);
	}

	iterator grow(size_type count)
	{
		return append_raw(count).init();
	}
	iterator grow(size_type count, const T& val)
	{
		return append_raw(count).init(val);
	}

	void shrink()
	{
		// Realloc memory to exact array size.
		S::resize_raw(size());
	}

	void resize(size_type new_size)
	{
		size_type s = size();
		if (new_size > s)
			append_raw(new_size - s, false).init();
		else if (new_size < s)
			pop_back(s - new_size, false);
	}
	void resize(size_type new_size, const T& val)
	{
		size_type s = size();
		if (new_size > s)
			append_raw(new_size - s, false).init(val);
		else if (new_size < s)
			pop_back(s - new_size, false);
	}

	void assign(size_type n, const T& val)
	{
		resize(n);
		fill(val);
	}

	void assign(const_iterator start, const_iterator finish)
	{
		*this = const_array(start, finish);
	}

	// Append
	iterator push_back()
	{
		return new(grow_raw())T;
	}
	iterator push_back(const T& val)
	{
		return new(grow_raw())T(val);
	}
	iterator push_back(T&& val)
	{
		return new(grow_raw())T(std::move(val));
	}
	iterator push_back(const_array a)
	{
		return append_raw(a.size()).init(a);
	}

	// Insert
	template<class Pos>
	iterator insert_raw(Pos pos, size_type count = 1)
	{
		// Grow array, return iterator to inserted raw elems.
		size_type p = index(pos);
		append_raw(count);
		(*this)(p + count).move_init((*this)(p));
		return begin() + p;
	}

	template<class Pos>
	iterator insert(Pos pos, const T& val)
	{
		return new(insert_raw(pos))T(val);
	}
	template<class Pos>
	iterator insert(Pos pos, T&& val)
	{
		return new(insert_raw(pos))T(std::move(val));
	}
	template<class Pos>
	iterator insert(Pos pos, size_type count)
	{
		return ArrayT(insert_raw(pos, count), count).init();
	}
	template<class Pos>
	iterator insert(Pos pos, size_type count, const T& val)
	{
		return ArrayT(insert_raw(pos, count), count).init(val);
	}
	template<class Pos>
	iterator insert(Pos pos, const_array a)
	{
		return ArrayT(insert_raw(pos, a.size()), a.size()).init(a);
	}
	template<class Pos>
	iterator insert(Pos pos, const_iterator start, const_iterator finish)
	{
		return insert(pos, const_array(start, finish));
	}

	// Erase
	void pop_back(size_type count = 1, bool allow_slack = true)
	{
		// Destroy erased elems, change size without reallocing.
		assert(count >= 0 && count <= size());
		if (count)
		{
			size_type new_size = size() - count;
			(*this)(new_size).destroy();
			S::resize_raw(new_size, allow_slack);
		}
	}

	template<class Pos>
	iterator erase(Pos pos, size_type count = 1)
	{
		size_type p = index(pos);
		if (count)
		{
			(*this)(p, count).destroy();
			(*this)(p).move_init((*this)(p + count));
			S::resize_raw(size() - count, true);
		}
		return begin() + p;
	}

	iterator erase(iterator start, iterator finish)
	{
		return erase(start, check_cast<size_type>(finish - start));
	}

	void clear()
	{
		destroy();
		S::resize_raw(0, false);
	}
};

//---------------------------------------------------------------------------

template<class T, class I = int, class A = NAlloc::StandardAlloc>
struct FastDynArray : DynArray<T, I, NArray::FastDynStorage<A>>
{
};

template<class T, class I = int>
struct FixedDynArray : DynArray<T, I, NArray::FastDynStorage<NAlloc::NullAlloc>>
{
	typedef NArray::ArrayStorage::Store<T, I> S;

	void set(void* elems, I mem_size)
	{
		this->m_aElems = (T*)elems;
		this->m_nCapacity = mem_size / sizeof(T);
		this->m_nCount = 0;
	}
	void set(Array<T, I> a)
	{
		this->m_aElems = a.begin();
		this->m_nCapacity = a.size();
		this->m_nCount = 0;
	}
};

template<class T, int nSIZE, class I = int>
struct StaticDynArray : DynArray<T, I, NArray::StaticDynStorage<nSIZE>>
{
};

#include <Cry3DEngine/CryPodArray.h>

#endif
