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
* This module contains the main chat processing, while fsfloateraichat.h/.cpp
* create the tabbed window that handles the UI for configuration and interacting
* with the chat process.
*/

#ifndef FS_FSAICHATMGR_H
#define FS_FSAICHATMGR_H


#include "stdtypes.h"

#include "lluuid.h"
#include "llsingleton.h"


class FSAIService;

// Last chat messages saved as "SL: <from other avatar>" or "AI: <from LLM>"
typedef std::deque<std::pair<std::string, char *>> ai_chat_history_t;

// To do - make this a config value?  Note this is not the size limit for sending to the AI service
static const S32 AI_HISTORY_MAX_MSGS = 1024;


class FSAIChatMgr : public LLSingleton<FSAIChatMgr>
{
    LLSINGLETON(FSAIChatMgr);
    ~FSAIChatMgr();

public:
    void            startupAIChat();    // Called at viewer startup time

    void            setChatSession(const LLUUID& chat_session);
    const LLUUID&   getChatSession() const { return mChatSession; };

    bool            setAIConfigValues(LLSD & ai_config);    // Supports some subset of config value changes
    void            switchAIConfig(const std::string& service_name);        // Switch to different service, load those config values
    const LLSD&     getAIConfig() const         { return mAIConfig;  }

    void            loadAvatarAISettings(const std::string & use_service);
    void            saveAvatarAISettings();

    // Setting mChattyAgent happens when new session is set up
    const LLUUID&   getChattyAgent() const { return mChattyAgent; };

    const std::string& getChattyDisplayName() const                  { return mChattyDisplayName; };
    void               setChattyDisplayName(const std::string& name) { mChattyDisplayName = name; };

    void            idle();     // Regularly called from main loop

    void            processIncomingChat(const LLUUID& from_id,
                                        const std::string& message,
                                        const std::string& name,
                                        const LLUUID& sessionid);
    void            processIncomingAIResponse(const std::string& ai_message, bool request_direct);
    void            setAIReplyMessagePrompt();

    void            sendChatToAIService(const std::string& message, bool request_direct);

    void            resetChat();

    const ai_chat_history_t& getAIChatHistory() const { return mAIChatHistory; };

  private:
    LLSD            readFullAvatarAISettings();  // Reads the full file possibly with multiple configurations
    void            createAIService(const std::string& ai_service_name);  // Creates service object mAIService
    void            trimAIChatHistoryData();      // Limit history storage
    void            finallyProcessIncomingAIResponse(const std::string& ai_message, bool request_direct);       // Deferred actual processing

    LLUUID          mChatSession;        // Chat session id
    LLUUID          mChattyAgent;        // Other agent
    std::string     mChattyDisplayName;  // Name of other agent to use in conversation

    FSAIService*      mAIService;        // Interface to external AI chat service
    LLSD              mAIConfig;         // Configuration values for AI back end
    LLFrameTimer      mLastChatTimer;    // Track time so we can drop dead chat sessions
    ai_chat_history_t mAIChatHistory;    // Last chat messages saved as "SL: <from other avatar>" or "AI: <from LLM>"

    // Reply data from the AI request coroutine - cached for processing in the main loop
    std::queue<std::pair<std::string, bool>> mAIReplyQueue;        // Has message and "direct" flag
};
#endif
