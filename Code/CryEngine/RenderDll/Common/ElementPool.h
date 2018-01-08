// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved. 

#pragma once

#include <CryThreading/CryThread.h>
#include <3rdParty/concqueue/concqueue.hpp>

#include <list>
#include <functional>

template<class T>
struct SElementPool
{
	CryCriticalSection         availableFreeElementsLock;
	std::list<T*>              allocatedElements;
	ConcQueue<UnboundMPSC, T*> availableFreeElements;

	std::function<void(T*)>    freeElementFunction;

	SElementPool(std::function<void(T*)> freeFunction)
	{
		freeElementFunction = freeFunction;
	}

	T* GetOrCreateOneElement()
	{
		T* pElement = nullptr;

		AUTO_LOCK(availableFreeElementsLock);
		if (!availableFreeElements.dequeue(pElement))
		{
			pElement = new T;
			allocatedElements.emplace_back(pElement);
		}

		return pElement;
	}

	void ReturnToPool(T* pElement)
	{
		if (pElement)
		{
			AUTO_LOCK(availableFreeElementsLock);
			freeElementFunction(pElement);
			availableFreeElements.enqueue(pElement);
		}
	}

	void ShutDown()
	{
		// Release all elements
		T* pElement;
		while (availableFreeElements.dequeue(pElement));

		while (allocatedElements.size())
		{
			freeElementFunction(allocatedElements.back());
			delete allocatedElements.back();
			allocatedElements.pop_back();
		}
	}
};