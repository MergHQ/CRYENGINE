/*
	simple print observer declaration
*/
#if defined(OFFLINE_COMPUTATION)

#include <PRT/IObserver.h>

namespace NSH
{
	namespace NTransfer
	{
		interface IObservableTransfer;
	}

	//!< implements a simple observer which prints out the current state to std output
	class CPrintTransferObserver: public IObserverBase
	{
	public: 
		CPrintTransferObserver(NTransfer::IObservableTransfer *pS);//!< constructor with -subject to observe- argument

		~CPrintTransferObserver();												//!< detaches from subject

		virtual void Update(IObservable *pChangedSubject);	//!< update function called from subject

		virtual void Display();	//!< simple print function, output percentage of progress

		void ResetDisplayState();	//!< forces display to a complete output

	private: 
		NTransfer::IObservableTransfer *m_pSubject;		//!< subject to observe
		float m_fLastProgress;												//!< last progress
		uint32 m_LastRunningThreads;									//!< last number of running theads
		bool m_FirstTime;															//!< indicates whether something has been displayed already or not	
		bool m_Progress;															//!< indicates whether progress was outputted so far or not
		
		CPrintTransferObserver();	//!< standard constructor forbidden
	}; 
}

#endif