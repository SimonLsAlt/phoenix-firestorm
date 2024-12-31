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
* This module contains the main chat processing, while fsfloateraichat.h/.cpp
* create the tabbed window that handles the UI for configuration and interacting
* with the chat process.
*/

#include "llviewerprecompiledheaders.h"

#include "llsdserialize.h"
#include "llavatarnamecache.h"
#include "llimview.h"

#include "fsaichatmgr.h"
#include "fsaichatstrings.h"
#include "fsaiservice.h"
#include "fsfloateraichat.h"
#include "fsnearbychathub.h"


// Name of config file saved in each av folder
inline constexpr char AVATAR_AI_SETTINGS_FILENAME[] = "avatar_ai_settings.xml ";

static constexpr F32 AI_CHAT_AFK_GIVEUP_SECS = (60 * 5);   // idle time to abandon a conversation

static constexpr S32 AI_REPLY_QUEUE_LIMIT = 10;

// ----------------------------------------------------------------------------------------

FSAIChatMgr::FSAIChatMgr() : mAIService(nullptr)
{
    // Ensure values in case there is no saved settings file
    mAIConfig = LLSD::emptyMap();
    mAIConfig[AI_CHAT_ON]      = false;
    mAIConfig[AI_SERVICE]      = std::string();

    mAIConfig[AI_ENDPOINT]     = std::string();    // to do - break apart service specific config settings to live in service class
    mAIConfig[AI_API_KEY]      = std::string();
    mAIConfig[AI_CHARACTER_ID] = std::string();

    mLastChatTimer.resetWithExpiry(AI_CHAT_AFK_GIVEUP_SECS);
};


FSAIChatMgr::~FSAIChatMgr()
{
}

void FSAIChatMgr::startupAIChat()
{  // Called at viewer startup time
    std::string use_saved_name;
    loadAvatarAISettings(use_saved_name);   // Use the service name saved in the config file
    if (mAIConfig[AI_CHAT_ON].asBoolean() && !mAIConfig[AI_SERVICE].asStringRef().empty())
    {
        createAIService(mAIConfig[AI_SERVICE].asStringRef());
    }
}


void FSAIChatMgr::setChatSession(const LLUUID& chat_session)
{   // to do - check for overwrite?
    mChatSession = chat_session;
}


LLSD FSAIChatMgr::readFullAvatarAISettings()
{
    const std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, AVATAR_AI_SETTINGS_FILENAME);

    LLSD data = LLSD::emptyMap();
    if (LLFile::isfile(filename))
    {
        llifstream file(filename.c_str());
        if (file.is_open())
        {
            LLSDSerialize::fromXMLDocument(data, file);
            file.close();
        }
        else
        {
            LL_WARNS("AIChat") << "Found but failed to open avatar AI settings file: " << filename << LL_ENDL;
        }
    }
    return data;
}


void FSAIChatMgr::loadAvatarAISettings(const std::string& use_service)
{   // Loads current selected AI config into mAIConfig
    LLSD full_data = readFullAvatarAISettings();

    if (full_data.isMap() && full_data.size() > 1 && full_data.has(AI_SERVICE))
    {
        std::string new_service = use_service;
        if (new_service.length() == 0)
        {
            new_service = full_data[AI_SERVICE].asString();
        }
        LL_DEBUGS("AIChat") << "use_service is " << (use_service.length() ? use_service : "<empty>") << " and new_service will be " << new_service << LL_ENDL;

        mAIConfig = LLSD::emptyMap();
        mAIConfig[AI_SERVICE] = new_service;
        mAIConfig[AI_CHAT_ON] = (full_data.has(AI_CHAT_ON) && full_data.get(AI_CHAT_ON).asBoolean());

        for (LLSD::map_const_iterator it = full_data.beginMap(); it != full_data.endMap(); ++it)
        {
            if (it->first == new_service && it->second.isMap())
            {
                LLSD config_data = it->second;
                // to do - move service specific config storage into the service class
                for (LLSD::map_const_iterator config_it = config_data.beginMap(); config_it != config_data.endMap(); ++config_it)
                {
                    mAIConfig[config_it->first] = config_it->second;
                    LL_DEBUGS("AIChat") << " setting '" << config_it->first << "' is " << config_it->second.asString() << LL_ENDL;
                }
                break;
            }
        }
        // Don't normally log sensitive data
        LL_DEBUGS("AIChat") << "Read avatar AI configuration for " << new_service << ": " << mAIConfig << LL_ENDL;
    }
}

void FSAIChatMgr::saveAvatarAISettings()
{
    if (!mAIService)
    {   // Not set up as a service
        LL_WARNS("AIChat") << "Not saving AI settings, no service selected" << LL_ENDL;
        return;
    }

    // to do - clean up, don't log sensitive data

    LLSD full_config = readFullAvatarAISettings();
    const std::string& service_name = mAIService->getName();
    if (!full_config.has(service_name))
    {   // Have to create an empty entry
        full_config[service_name] = LLSD::emptyMap();
    }

    // Copy service specific values into the config
    for (LLSD::map_const_iterator config_it = mAIConfig.beginMap(); config_it != mAIConfig.endMap(); ++config_it)
    {
        if (config_it->first != AI_CHAT_ON && config_it->first != AI_SERVICE)
        {   // to do - extend service class to maintain its own config data
            full_config[service_name][config_it->first] = config_it->second;
        }
    }

    // Save the current service name and on state
    full_config[AI_SERVICE] = service_name;
    full_config[AI_CHAT_ON] = mAIConfig[AI_CHAT_ON];

    // Write it out to a file
    const std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, AVATAR_AI_SETTINGS_FILENAME);

    llofstream file(filename.c_str());
    if (!file.is_open())
    {
        LL_WARNS() << "Unable to save avatar AI chat settings to " << filename << LL_ENDL;
        return;
    }

    LL_DEBUGS("AIChat") << "Writing avatar AI configuration to " << filename << ": " << mAIConfig << LL_ENDL;
    LLSDSerialize::toPrettyXML(full_config, file);
    file.close();
}


bool FSAIChatMgr::setAIConfigValues(LLSD& ai_config)
{   // Save configuration values for the current service name
    bool changed = false;

    LL_DEBUGS("AIChat") << "Setting new AI config values: " << ai_config << LL_ENDL;
    for (LLSD::map_iterator config_iter = ai_config.beginMap(); config_iter != ai_config.endMap(); ++config_iter)
    {   // Assign all the new values passed in
        if ((mAIConfig[config_iter->first].asString() != config_iter->second.asString()))
        {
            changed = true;
            mAIConfig[config_iter->first] = config_iter->second;
        }
    }

    std::string new_service = ai_config.get(AI_SERVICE).asString();
    if (new_service.length() && (!mAIService || mAIService->getName() != new_service))
    {
        createAIService(new_service);   // Refresh service object
        changed = true;
    }

    if (mAIConfig[AI_CHAT_ON].asBoolean() != ai_config.get(AI_CHAT_ON).asBoolean())
    {
        mAIConfig[AI_CHAT_ON] = ai_config.get(AI_CHAT_ON);
        changed = true;
    }

    if (changed)
    {
        saveAvatarAISettings();  // Save back to file
    }
    else
    {
        LL_INFOS("AIChat") << "AI chat settings not changed, skipping saving to file" << LL_ENDL;
    }
    return changed;
}


void FSAIChatMgr::switchAIConfig(const std::string& service_name)
{   // Switch to different service, loading those config values from file
    loadAvatarAISettings(service_name);

    LL_INFOS("AIChat") << "Switching AI to " << service_name << LL_ENDL;
    createAIService(service_name);
}


void FSAIChatMgr::createAIService(const std::string& ai_service_name)
{  // Creates service object mAIService
    if (mAIService)
    {
        LL_DEBUGS("AIChat") << "Deleting " << mAIService->getName() << " AI service to make new one for " << ai_service_name << LL_ENDL;
        delete mAIService;
        mAIService = nullptr;
    }

    // Create a new one
    mAIService = FSAIService::createFSAIService(ai_service_name);
    if (mAIService)
    {
        LL_INFOS("AIChat") << "Created new " << mAIService->getName() << " AI chat back end service" << LL_ENDL;
    }
    else
    {   // Shouldn't ever happen, except during development when menu and support are not in sync
        LL_WARNS("AIChat") << "Unable to create AI service for " << ai_service_name << LL_ENDL;
    }

    mAIChatHistory.clear();

    while (!mAIReplyQueue.empty())
    {
        mAIReplyQueue.pop();
    }
}


void FSAIChatMgr::processIncomingChat(const LLUUID& from_id, const std::string& message, const std::string& name, const LLUUID& sessionid)
{
    if (mAIConfig.get(AI_CHAT_ON).asBoolean())
    {
        LL_DEBUGS("AIChat") << "Handling IM chat from " << from_id << " name " << name << ", session " << sessionid << ", text: " << message
                           << LL_ENDL;

        if (mLastChatTimer.hasExpired() && mChatSession.notNull() && (mChatSession != sessionid))
        {   // Conversation went dead, reset for new incoming chat
            resetChat();
        }
        if (mChatSession.isNull() && mAIService)
        {   // Start a new chat
            // to do - check if ID is on allow list, etc - have some sort of access control?
            mChatSession = sessionid;  // Save info
            mChattyAgent = from_id;
        }

        if (mChatSession.notNull() && mChatSession == sessionid)
        {   // Get display name to use
            LLAvatarName av_name;
            if (LLAvatarNameCache::get(from_id, &av_name))
            {
                std::string new_name = av_name.getDisplayName(true);
                if (new_name != mChattyDisplayName && new_name.size())
                {
                    std::string previous_name = mChattyDisplayName;
                    mChattyDisplayName        = new_name;
                    LL_INFOS("AIChat") << "Name change detected was chatting with " << previous_name << ", starting chat with "
                                       << mChattyDisplayName << LL_ENDL;
                    if (previous_name != mChattyDisplayName)
                    {  // Service needs to adjust for change conversation target
                        mAIService->aiChatTargetChanged(previous_name, new_name);
                    }
                }
            }

            mLastChatTimer.resetWithExpiry(AI_CHAT_AFK_GIVEUP_SECS);    // Reset dead chat timer

            FSFloaterAIChat* floater = FSFloaterAIChat::getAIChatFloater();
            if (floater)
            {   // Update UI with incoming IM message
                FSPanelAIConversation* chat_panel = floater->getPanelConversation();
                if (chat_panel)
                {
                    chat_panel->processIncomingChat(name, message);
                }
                else
                {
                    LL_WARNS("AIChat") << "Unable to get AI chat conversation panel" << LL_ENDL;
                }
            }
            else
            {
                // to do - ensure message gets added to history and displayed later
                LL_WARNS("AIChat") << "Unable to get AI chat floater" << LL_ENDL;
            }

            if (mAIService)
            {   // Send message to external AI chat service
                LL_INFOS("AIChat") << "Sending chat from avatar to " << mAIService->getName() << " AI service" << LL_ENDL;
                mAIService->sendChatToAIService(message, false);
                if (mAIService->saveChatHistory())
                {
                    mAIChatHistory.push_front(std::string("SL:").append(message));
                    trimAIChatHistoryData();
                }
            }
            else
            {
                LL_WARNS("AIChat") << "No AI service available, unable to send chat" << LL_ENDL;
            }
        }
        else
        {
            LL_INFOS("AIChat") << "AI is in another chat session " << mChatSession << " with " << mChattyAgent
                               << ", ignoring IM from " << from_id << " name " << name << LL_ENDL;
        }
    }
    else
    {   // AI chst is off
        LL_INFOS("AIChat") << "AI chat is off" << LL_ENDL;
    }
}


void FSAIChatMgr::sendChatToAIService(const std::string& message, bool request_direct)
{  // Simple glue
    if (mAIService)
    {  // Send message to external AI chat service
        LL_INFOS("AIChat") << "Sending chat message to " << mAIService->getName() << " AI service" << LL_ENDL;
        return mAIService->sendChatToAIService(message, request_direct);
    }
}


void FSAIChatMgr::resetChat()
{
    mChatSession.setNull();  // Chat session id
    mChattyAgent.setNull();  // Other agent
    mChattyDisplayName.clear();
    mAIChatHistory.clear();  // Last chat messages saved as "SL: <from other avatar>" or "AI: <from LLM>"
    mLastChatTimer.resetWithExpiry(AI_CHAT_AFK_GIVEUP_SECS);

    FSFloaterAIChat* floater = FSFloaterAIChat::getAIChatFloater();
    if (floater)
    {
        FSPanelAIDirect2LLM* direct_chat_panel = floater->getPanelDirect2LLM();
        if (direct_chat_panel)
        {
            direct_chat_panel->resetChat();
        }
        else
        {
            LL_WARNS("AIChat") << "Unable to get AI direct chat panel for reset" << LL_ENDL;
        }
        FSPanelAIConversation* chat_panel = floater->getPanelConversation();
        if (chat_panel)
        {
            chat_panel->resetChat();
        }
        else
        {
            LL_WARNS("AIChat") << "Unable to get AI chat conversation panel for reset" << LL_ENDL;
        }
    }
}


void FSAIChatMgr::idle()
{   // Handle any incoming messages from the AI that have been saved for main thread processing
    while (!mAIReplyQueue.empty())
    {
        std::pair<std::string, bool> ai_message = mAIReplyQueue.front();
        mAIReplyQueue.pop();  // Remove from the front
        finallyProcessIncomingAIResponse(ai_message.first, ai_message.second);
    }
}

void FSAIChatMgr::processIncomingAIResponse(const std::string& ai_message, bool request_direct)
{   // Just save message data - called from coroutine
    if (mAIReplyQueue.size() < AI_REPLY_QUEUE_LIMIT)
    {
        mAIReplyQueue.push({ ai_message, request_direct });  // Push on the back.
    }
    else
    {
        LL_WARNS("AIChat") << "AI message reply queue is full with " << mAIReplyQueue.size() << ", dropping message: " << ai_message
                           << LL_ENDL;
    }
}


void FSAIChatMgr::finallyProcessIncomingAIResponse(const std::string& ai_message, bool request_direct)
{   // to do - can't do UI stuff from the coro, or it may get an assert.   Buffer responses
    // and add an idle() call from the main loop to handle reponses
    llassert(LLCoros::on_main_thread_main_coro());
    FSFloaterAIChat* floater = FSFloaterAIChat::getAIChatFloater();
    if (floater)
    {
        if (request_direct)
        {  // Direct Chat to / from LLM
            FSPanelAIDirect2LLM* direct_chat_panel = floater->getPanelDirect2LLM();
            if (direct_chat_panel)
            {
                direct_chat_panel->displayIncomingAIResponse(ai_message);
                direct_chat_panel->setAIServiceNamePrompts(mAIService->getName());
            }
            else
            {
                LL_WARNS("AIChat") << "Unable to get AI direct chat panel" << LL_ENDL;
            }
        }
        else
        {   //  reply to other avatar's chat
            FSPanelAIConversation* chat_panel = floater->getPanelConversation();
            if (chat_panel)
            {
                chat_panel->displayIncomingAIResponse(ai_message);
                chat_panel->setAIReplyMessagePrompt(mAIService->getName());
            }
            else
            {
                LL_WARNS("AIChat") << "Unable to get AI chat conversation panel" << LL_ENDL;
            }
        }
    }
    else
    {
        LL_WARNS("AIChat") << "Unable to get AI chat floater" << LL_ENDL;
    }

    if (!request_direct)
    {   // This skips all the processing for /<number> etc. that's done for local chat
        // Send it!
        LLIMModel::sendMessage(ai_message, mChatSession, mChattyAgent, IM_NOTHING_SPECIAL);

        if (mAIService->saveChatHistory())
        {
            mAIChatHistory.push_front(std::string("AI:").append(ai_message));
            trimAIChatHistoryData();
        }
    }
}



void FSAIChatMgr::trimAIChatHistoryData()
{  // Limit size of mAIChatHistory
    while (mAIChatHistory.size() > AI_HISTORY_MAX)
    {
        mAIChatHistory.pop_back();
    }
}
