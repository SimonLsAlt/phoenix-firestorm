/**
 * @file fsaichatmgr.cpp
 * @author simonlsalt
 * @brief Experimental hooks for AI chat
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Firestorm Viewer Source Code
 * Copyright (C) 2024, Firestorm Viewer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * $/LicenseInfo$
 */

/*
* This is experimental code to integrate external chat systems to the SL / FS 
* chat experience.
* 
* This module contains the main chat processing, while llfloateraichat.h/.cpp 
* create the tabbed window that handles the UI for configuration and interacting
* with the chat process.
*/

#include "llviewerprecompiledheaders.h"

#include "fsaichatmgr.h"


//////////////////////////////////////////////////////////////////////////////

FSAIChatMgr::FSAIChatMgr()
{
}

FSAIChatMgr::~FSAIChatMgr()
{
}


void FSAIChatMgr::setTargetSession(const LLUUID& chat_session)
{   // to do - check for overwrite?
    mTargetSession = chat_session;
}

void FSAIChatMgr::clearMatchingTargetSession(const LLUUID& closing_session)
{   // Clear mTargetSession if it matches a closing_session
    if (mTargetSession == closing_session)
    {
        LL_INFOS("AIChat") << "Clearing AI chat session " << mTargetSession << LL_ENDL;
        mTargetSession.setNull();
        mTargetAgent.setNull();

        // to do - close conversation with AI ?
    }
}


void FSAIChatMgr::processIncomingChat(const LLUUID& from_id, const std::string& message, const std::string& name, const LLUUID& sessionid)
{
    LL_INFOS("AIChat") << "Handling IM chat from " << from_id << " name " << name
        << ", session " << sessionid << ", text: " << message << LL_ENDL;

    if (mTargetSession.isNull())
    {   // Start new chat
        // to do - check if AI chat is enabled!
        mTargetSession = sessionid;     // Save info
        mTargetAgent   = from_id;
    }

    if (mTargetSession.notNull() && mTargetSession == sessionid)
    {
        // find / open AI chat window
        // pass in / set incoming chat message text in conversation tab
        // send chat message to LLM API
        //      callback routine / coro result needs to be processed
    }
    else
    {
        LL_INFOS("AIChat") << "No match, skipping Handling IM chat from " << from_id << " name " << name << LL_ENDL;
    }
}
