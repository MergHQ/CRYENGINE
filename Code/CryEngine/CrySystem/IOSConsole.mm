/*************************************************************************
 Crytek Source File.
 Copyright (C), Crytek Studios, 2001-2013.
 -------------------------------------------------------------------------
 $Id$
 $DateTime$
 Description:
 Console implementation for iOS, reports back to the main interface.
 -------------------------------------------------------------------------
 History:
 - Jul 19,2013:	Created by Leander Beernaert
 
 *************************************************************************/

#include "StdAfx.h"
#if defined(IOS)
#include "IOSConsole.h"



CIOSConsole::CIOSConsole():
m_isInitialized(false)
{
    
}

CIOSConsole::~CIOSConsole()
{
    
}

// Interface IOutputPrintSink /////////////////////////////////////////////
void CIOSConsole::Print(const char *line)
{
    printf("MSG: %s\n", line);
}
// Interface ISystemUserCallback //////////////////////////////////////////
bool CIOSConsole::OnError(const char *errorString)
{
    printf("ERR: %s\n", errorString);
    return true;
}

void CIOSConsole::OnInitProgress(const char *sProgressMsg)
{
    (void) sProgressMsg;
    // Do Nothing
}
void CIOSConsole::OnInit(ISystem *pSystem)
{
    if (!m_isInitialized)
    {
        IConsole* pConsole = pSystem->GetIConsole();
        if (pConsole != 0)
        {
            pConsole->AddOutputPrintSink(this);
        }
        m_isInitialized = true;
    }
}
void CIOSConsole::OnShutdown()
{
    if (m_isInitialized)
    {
        // remove outputprintsink
        m_isInitialized = false;
    }
}
void CIOSConsole::OnUpdate()
{
    // Do Nothing
}
void CIOSConsole::GetMemoryUsage(ICrySizer *pSizer)
{
    size_t size = sizeof(*this);
    
    
    
    pSizer->AddObject(this, size);
}

// Interface ITextModeConsole /////////////////////////////////////////////
Vec2_tpl<int> CIOSConsole::BeginDraw()
{
    return Vec2_tpl<int>(0,0);
}
void CIOSConsole::PutText( int x, int y, const char * msg )
{
    printf("PUT: %s\n", msg);
}
void CIOSConsole::EndDraw() {
    // Do Nothing
}
#endif // IOS