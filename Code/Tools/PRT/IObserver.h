/*
	observer interface definitions
*/
#if defined(OFFLINE_COMPUTATION)

#pragma once

#include "PRTTypes.h"


namespace NSH
{
	interface IObservable;

	//!< observer base implementation
	interface IObserverBase
	{ 
	public: 
		virtual ~IObserverBase(){}; 
		virtual void Update(IObservable* pSubject) = 0; 
	protected: 
		IObserverBase(){}; 
	};

	//use its own list since we need to work with mixed STLs
	typedef prtlist<IObserverBase*> TObserverBaseList;

	/************************************************************************************************************************************************/

	//!< basic interface for all observer interfaces/classes
	//!< the base class for subjects which are observable by IObserverBases
	interface IObservable
	{
		//!< part of observer pattern, attaches an fitting observer
		virtual void Attach(IObserverBase* o) 
		{
			m_Observers.push_back(o); 
		} 

		//!< part of observer pattern, detaches an fitting observer
		virtual void Detach(IObserverBase* o) 
		{ 
			m_Observers.remove(o); 
		} 

		//!< part of observer pattern, notifies all attached observers
		virtual void Notify() 
		{
			const TObserverBaseList::iterator cEnd = m_Observers.end();
			for(TObserverBaseList::iterator iter = m_Observers.begin(); iter != cEnd; ++iter) 
				(*(iter))->Update(this); 
		}

		virtual ~IObservable(){}

	protected:
		IObservable(){};		//!< is not meant to be instantiated explicitly
	private:
		TObserverBaseList m_Observers; //!< the observers to notify, part of observer pattern
	};
}

#endif