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
#include "fsaiservice.h"
#include "fsfloateraichat.h"
#include "fsnearbychathub.h"


// Name of config file saved in each av folder
static const char* AVATAR_AI_SETTINGS_FILENAME = "avatar_ai_settings.xml ";

// Names of supported AI back-ends, must match up with panel_ai_configuration.xml combo box values
static const char* SUPPORTED_LLMS[] = { "Kindroid.ai", "Nomi.ai", "OpenAI", "LMStudio" };


// ----------------------------------------------------------------------------------------

FSAIChatMgr::FSAIChatMgr() : mAIService(nullptr)
{
    // Ensure values in case there is no saved settings file
    mAIConfig = LLSD::emptyMap();
    mAIConfig["chat_enabled"]  = false;
    mAIConfig["service"]       = std::string();
    mAIConfig["endpoint"]      = std::string();
    mAIConfig["account_key"]   = std::string();
    mAIConfig["character_key"] = std::string();

    loadAvatarAISettings();
};


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

    if (full_data.isMap() && full_data.size() > 1 && full_data.has("current_service"))
    {
        mAIConfig.clear();
        std::string cur_service = full_data["current_service"].asString();

        for (LLSD::map_const_iterator it = full_data.beginMap(); it != full_data.endMap(); ++it)
        {
            if (it->first == cur_service && it->second.isMap())
            {
                LLSD config_data = it->second;
                for (LLSD::map_const_iterator config_it = config_data.beginMap(); config_it != config_data.endMap(); ++config_it)
                {
                    mAIConfig[config_it->first] = config_it->second;
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

    // Save the current one
    full_config["current_service"] = service_name;

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
    if (ai_config.has("service"))
    {
        const std::string& ai_service_name = ai_config["service"].asStringRef();
        if (mAIService && mAIService->getName() != ai_service_name)
        {
            LL_INFOS("AIChat") << "Deleting " << mAIService->getName() << " AI service to make new one for " << ai_service_name << LL_ENDL;
            delete mAIService;
            mAIService = nullptr;
        }

        if (!mAIService)
        {   // Create a new one as needed
            mAIService = FSAIService::createFSAIService(ai_service_name);
            if (mAIService)
            {
                LL_INFOS("AIChat") << "Created new " << mAIService->getName() << " AI chat back end service" << LL_ENDL;
            }
            else
            {   // Shouldn't ever happen, except during development when menu and support are not in sync
                LL_WARNS("AIChat") << "Unable to create AI service for " << ai_service_name << LL_ENDL;
            }
        }
    }

    // Copy over all values passed in
    for (LLSD::map_iterator config_iter = ai_config.beginMap(); config_iter != ai_config.endMap(); ++config_iter)
    {
        mAIConfig[config_iter->first] = config_iter->second;
    }
    LL_INFOS("AIChat") << "Set AI config values: " << mAIConfig << LL_ENDL;
    saveAvatarAISettings();     // Save it to the file
}

void FSAIChatMgr::processIncomingChat(const LLUUID& from_id, const std::string& message, const std::string& name, const LLUUID& sessionid)
{
    LL_INFOS("AIChat") << "Handling IM chat from " << from_id << " name " << name << ", session " << sessionid << ", text: " << message
                       << LL_ENDL;

    if (mTargetSession.isNull())
    {  // Start new chat
        // to do - check if AI chat is enabled!
        mTargetSession = sessionid;  // Save info
        mTargetAgent   = from_id;
    }

    // First update the UI with incoming message
    if (mTargetSession.notNull() && mTargetSession == sessionid)
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
            LL_WARNS("AIChat") << "Unable to get AI chat floater" << LL_ENDL;
        }
    }
    else
    {
        LL_INFOS("AIChat") << "No match, skipping Handling IM chat from " << from_id << " name " << name << LL_ENDL;
    }

    if (mAIService)
    {
        LL_INFOS("AIChat") << "Calling AI sendChat()" << LL_ENDL;
        mAIService->sendChat(message);
    }
    else
    {
        LL_WARNS("AIChat") << "No AI service available, unable to send chat" << LL_ENDL;
    }
    // to do - save chat in FSAIChatMgr history for following requests as needed
}


void FSAIChatMgr::processIncomingAIResponse(const std::string& ai_message)
{
    if (mTargetSession.notNull())
    {
        FSFloaterAIChat* floater = FSFloaterAIChat::getAIChatFloater();
        if (floater)
        {
            FSPanelAIConversation* chat_panel = floater->getPanelConversation();
            if (chat_panel)
            {
                chat_panel->processIncomingAIResponse(mAIService->getName(), ai_message);
            }
            else
            {
                LL_WARNS("AIChat") << "Unable to get AI chat conversation panel" << LL_ENDL;
            }
        }
        else
        {
            LL_WARNS("AIChat") << "Unable to get AI chat floater" << LL_ENDL;
        }
    }

    // This skips all the processing for /<number> etc. that's done for local chat
    if (mTargetSession.isNull())
    {
        LL_WARNS("AIChat") << "Unexpected NULL chat session ID, no way to send AI response" << LL_ENDL;
    }
    else
    {   // Send it!
        LLIMModel::sendMessage(ai_message, mTargetSession, mTargetAgent, IM_NOTHING_SPECIAL);
    }
}
