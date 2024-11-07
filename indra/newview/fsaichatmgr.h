/**
 * @file fsaichatmgr.h
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

#ifndef FS_FSFLOATERAICHAT_H
#define FS_FSFLOATERAICHAT_H


#include "stdtypes.h"

#include "lluuid.h"
#include "llsingleton.h"


class FSAIChatMgr : public LLSingleton<FSAIChatMgr>
{
    LLSINGLETON(FSAIChatMgr);
    ~FSAIChatMgr();

public:
    void            setTargetSession(const LLUUID& chat_session);
    const LLUUID&   getTargetSession() { return mTargetSession; };
    void            clearMatchingTargetSession(const LLUUID& closing_session);

    // Setting mTargetAgent happens when new session is set up
    const LLUUID&   getTargetAgent() { return mTargetAgent; };


    void            processIncomingChat(const LLUUID& from_id,
                                        const std::string& message,
                                        const std::string& name,
                                        const LLUUID& sessionid);

private:
    LLUUID mTargetSession;
    LLUUID mTargetAgent;
};
#endif
