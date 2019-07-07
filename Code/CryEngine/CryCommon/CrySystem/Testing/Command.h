// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once
#include <memory>
#include <vector>
#include <CrySTL/type_traits.h>

namespace CryTest
{
//! Represents a step of work in a feature test.
//! Can hold different categories of objects through type erasure.
//! Supports any callable type such as function pointer, reference or std::function.
//! Supports any class type that implements the following members:
//!
//! //! The main loop to trigger the command's work. Must not block in the function. Returns true when the command is done.
//! //! The function is required for converting the object to CCommand.
//! bool Update();
//!
//! //! Returns additional commands to be run after this one is done. The new commands are inserted to the front of queue.
//! //! The function is optional for converting the object to CCommand.
//! std::vector<CCommand> GetSubCommands() const;
class CCommand
{
public:
	template<typename T>
	CCommand(T&& wrappedCommand)
		: m_Self(new Model<typename std::decay<T>::type>(std::forward<T>(wrappedCommand)))
	{}

	//Copying and moving are regular, i.e. copying would create a deep copy, and moving would transfer the ownership
	~CCommand() = default;
	CCommand(const CCommand& other)
		: m_Self(other.m_Self->Clone())
	{}
	CCommand& operator=(const CCommand& other)
	{
		m_Self.reset(other.m_Self->Clone());
		return *this;
	}
	CCommand(CCommand&& other) = default;
	CCommand& operator=(CCommand&& x) = default;

	bool      Update()
	{
		return m_Self->Update();
	}

	std::vector<CCommand> GetSubCommands() const
	{
		return m_Self->GetSubCommands();
	}

private:

	struct Concept
	{
		virtual ~Concept() = default;
		virtual Concept*              Clone() = 0;
		virtual bool                  Update() = 0;
		virtual std::vector<CCommand> GetSubCommands() const = 0;
	};

	//Helper type predicates to detect method availability of types
	template<class T> using DetectGetSubCommands = decltype(std::declval<T>().GetSubCommands());
	template<class T> using DetectUpdate = decltype(std::declval<T>().Update());

	template<typename T>
	class Model final : public Concept
	{
	public:
		Model(T&& x) : m_Data(std::move(x)) {}
		Model(const T& x) : m_Data(x) {}

	protected:
		virtual Concept*              Clone() override                              { return new Model<T>(*this); }

		inline std::vector<CCommand>  InternalGetSubCommands(std::true_type) const  { return m_Data.GetSubCommands(); }
		inline std::vector<CCommand>  InternalGetSubCommands(std::false_type) const { return {}; }

		virtual std::vector<CCommand> GetSubCommands() const override
		{
			return InternalGetSubCommands(stl::is_detected<DetectGetSubCommands, T>());
		}

		inline bool  InternalUpdate(std::true_type)  { return m_Data.Update(); }
		inline bool  InternalUpdate(std::false_type) { m_Data(); return true; }

		virtual bool Update() override
		{
			return InternalUpdate(stl::is_detected<DetectUpdate, T>());
		}

	private:
		T m_Data;
	};

	std::unique_ptr<Concept> m_Self;
};
}