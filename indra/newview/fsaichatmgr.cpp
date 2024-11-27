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
#include "llimview.h"

#include "fsaichatmgr.h"
#include "fsaichatstrings.h"
#include "fsaiservice.h"
#include "fsfloateraichat.h"
#include "fsnearbychathub.h"


// Name of config file saved in each av folder
inline constexpr char AVATAR_AI_SETTINGS_FILENAME[] = "avatar_ai_settings.xml ";



// ----------------------------------------------------------------------------------------

FSAIChatMgr::FSAIChatMgr() : mAIService(nullptr)
{
    // Ensure values in case there is no saved settings file
    mAIConfig = LLSD::emptyMap();
    mAIConfig[AI_CHAT_ON]       = false;
    mAIConfig[AI_SERVICE]       = std::string();
    mAIConfig[AI_ENDPOINT]      = std::string();
    mAIConfig[AI_API_KEY]   = std::string();
    mAIConfig[AI_CHARACTER_ID] = std::string();
};


FSAIChatMgr::~FSAIChatMgr()
{
}

void FSAIChatMgr::startupAIChat()
{  // Called at viewer startup time
    loadAvatarAISettings();
    if (mAIConfig[AI_CHAT_ON].asBoolean() && !mAIConfig[AI_SERVICE].asStringRef().empty())
    {
        createAIService(mAIConfig[AI_SERVICE].asStringRef());
    }
}


void FSAIChatMgr::setChatSession(const LLUUID& chat_session)
{   // to do - check for overwrite?
    mChatSession = chat_session;
}

void FSAIChatMgr::clearMatchingTargetSession(const LLUUID& closing_session)
{   // Clear mChatSession if it matches a closing_session
    if (mChatSession == closing_session)
    {
        LL_INFOS("AIChat") << "Clearing AI chat session " << mChatSession << LL_ENDL;
        mChatSession.setNull();
        mChattyAgent.setNull();

        // to do - close conversation with AI ?
    }
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


void FSAIChatMgr::loadAvatarAISettings()
{   // Loads current selected AI config into mAIConfig
    LLSD full_data = readFullAvatarAISettings();

    if (full_data.isMap() && full_data.size() > 1 && full_data.has(AI_SERVICE))
    {
        mAIConfig = LLSD::emptyMap();
        std::string cur_service = full_data[AI_SERVICE].asString();

        for (LLSD::map_const_iterator it = full_data.beginMap(); it != full_data.endMap(); ++it)
        {
            if (it->first == cur_service && it->second.isMap())
            {
                LLSD config_data = it->second;
                for (LLSD::map_const_iterator config_it = config_data.beginMap(); config_it != config_data.endMap(); ++config_it)
                {
                    mAIConfig[config_it->first] = config_it->second;
                    LL_INFOS("AIChat") << " setting '" << config_it->first << "' is " << mAIConfig[config_it->first].asStringRef() << LL_ENDL;
                }
                break;
            }
        }
        // to do - clean up, don't log sensitive data
        LL_INFOS("AIChat") << "Read avatar AI configuration for " << cur_service << ": " << mAIConfig << LL_ENDL;
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

    // Copy all values into the config
    for (LLSD::map_const_iterator config_it = mAIConfig.beginMap(); config_it != mAIConfig.endMap(); ++config_it)
    {
        full_config[service_name][config_it->first] = config_it->second;
    }

    // Save the current service name and on state
    full_config[AI_SERVICE] = service_name;
    full_config[AI_CHAT_ON] = mAIConfig[AI_CHAT_ON];

    // Write it out to a file
    const std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, AVATAR_AI_SETTINGS_FILENAME);

    llofstream file(filename.c_str());
    if (!file.is_open())
    {
        LL_WARNS() << "Unable to save avatar AI chat settings!" << LL_ENDL;
        return;
    }

    LL_INFOS("AIChat") << "Writing avatar AI configuration to " << filename << ": " << mAIConfig << LL_ENDL;
    LLSDSerialize::toPrettyXML(full_config, file);
    file.close();
}


void FSAIChatMgr::setAIConfig(LLSD& ai_config)
{   // Select AI service to use and save configuration values

    LL_INFOS("AIChat") << "Setting new AI config values: " << ai_config << LL_ENDL;
    if (ai_config.has(AI_SERVICE))
    {
        createAIService(ai_config[AI_SERVICE].asStringRef());
    }

    // Copy over all values passed in
    for (LLSD::map_iterator config_iter = ai_config.beginMap(); config_iter != ai_config.endMap(); ++config_iter)
    {
        mAIConfig[config_iter->first] = config_iter->second;
    }
    LL_INFOS("AIChat") << "Set AI config values: " << mAIConfig << LL_ENDL;
    saveAvatarAISettings();     // Save it to the file
}

void FSAIChatMgr::createAIService(const std::string& ai_service_name)
{  // Creates service object mAIService
    if (mAIService && mAIService->getName() != ai_service_name)
    {
        LL_INFOS("AIChat") << "Deleting " << mAIService->getName() << " AI service to make new one for " << ai_service_name << LL_ENDL;
        delete mAIService;
        mAIService = nullptr;
    }

    if (!mAIService)
    {  // Create a new one as needed
        mAIService = FSAIService::createFSAIService(ai_service_name);
        if (mAIService)
        {
            LL_INFOS("AIChat") << "Created new " << mAIService->getName() << " AI chat back end service" << LL_ENDL;
        }
        else
        {  // Shouldn't ever happen, except during development when menu and support are not in sync
            LL_WARNS("AIChat") << "Unable to create AI service for " << ai_service_name << LL_ENDL;
        }

        mAIChatHistory.clear();
    }
}


void FSAIChatMgr::processIncomingChat(const LLUUID& from_id, const std::string& message, const std::string& name, const LLUUID& sessionid)
{
    LL_INFOS("AIChat") << "Handling IM chat from " << from_id << " name " << name << ", session " << sessionid << ", text: " << message
                       << LL_ENDL;

    if (mChatSession.isNull())
    {  // Start new chat
        // to do - check if AI chat is enabled, this ID is on allow list, etc
        mChatSession = sessionid;  // Save info
        mChattyAgent   = from_id;
    }

    if (mAIConfig.get(AI_CHAT_ON).asBoolean())
    {   // Update UI with incoming IM message
        if (mChatSession.notNull() && mChatSession == sessionid)
        {
            FSFloaterAIChat* floater = FSFloaterAIChat::getAIChatFloater();
            if (floater)
            {
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
                mAIChatHistory.push_front(std::string("SL:").append(message));
                trimAIChatHistoryData();
            }
            else
            {
                LL_WARNS("AIChat") << "No AI service available, unable to send chat" << LL_ENDL;
            }
        }
        else
        {
            LL_INFOS("AIChat") << "AI is in another chat, ignoring IM from " << from_id << " name " << name << LL_ENDL;
        }
    }
    else
    {   // AI chst is off
        LL_INFOS("AIChat") << "AI chat is off" << LL_ENDL;
    }
}


bool FSAIChatMgr::sendChatToAIService(const std::string& message, bool request_direct)
{  // Simple glue
    if (mAIService)
    {  // Send message to external AI chat service
        LL_INFOS("AIChat") << "Sending chat message to " << mAIService->getName() << " AI service" << LL_ENDL;
        return mAIService->sendChatToAIService(message, request_direct);
    }
    return false;
}

void FSAIChatMgr::processIncomingAIResponse(const std::string& ai_message, bool request_direct)
{
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
        {   // Chat from / reply to AV
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

        mAIChatHistory.push_front(std::string("AI:").append(ai_message));
        trimAIChatHistoryData();
    }
}

void FSAIChatMgr::trimAIChatHistoryData()
{  // Limit size of mAIChatHistory
    while (mAIChatHistory.size() > AI_HISTORY_MAX)
    {
        mAIChatHistory.pop_back();
    }
}
