/**
 * @file fsaiservice.cpp
 * @author simonlsalt
 * @brief Experimental interface for AI back-end
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

#include "llviewerprecompiledheaders.h"
#include "fsaiservice.h"
#include "fsaichatstrings.h"
#include "fsaichatmgr.h"

#include "llcoros.h"
#include "llhttpconstants.h"
#include "httpheaders.h"

// ---------------------------------------------------------
// base class FSAIService

FSAIService::FSAIService(const std::string& name) : mRequestBusy(false),
                                                    mName(name)
{
}


FSAIService::~FSAIService()
{
}


// static simple utility
const LLSD& FSAIService::getAIConfig()
{
	return FSAIChatMgr::getInstance()->getAIConfig();
};


//static
FSAIService* FSAIService::createFSAIService(const std::string& service_name)
{
	// Based on the name, create the proper service class handler
    FSAIService* ai_service = nullptr;
    if (service_name == LLM_KINDROID)
    {
        ai_service = dynamic_cast<FSAIService*>(new FSAIKindroidService(service_name));
    }
    else if (service_name == LLM_LMSTUDIO)
    {
        ai_service = dynamic_cast<FSAIService*>(new FSAILMStudioService(service_name));
    }
    else if (service_name == LLM_OPENAI)
    {
        ai_service = dynamic_cast<FSAIService*>(new FSAIOpenAIService(service_name));
    }
    else if (service_name == LLM_NOMI)
    {
        ai_service = dynamic_cast<FSAIService*>(new FSAINomiService(service_name));
    }


    // else ... add other supported back-ends here

    return ai_service;
};

// ------------------------------------------------
// Use nomi.ai

// curl - X POST - H "Authorization: Bearer ea123456-c9c1-4990-8fed-216ba1c692eb" \
//               - H "Content-Type: application/json" \
//               -d '{"messageText": "Hello there"}' \
//            https://api.nomi.ai/v1/nomis/bb400109-9943-4524-98ca-82f15c044146/chat
//
// response content:
// {
//    "sentMessage" : { "uuid": "98fea2d8-ba17-4fa2-ab5b-2793768ebcad", "text": "Hello there", "sent": "2024-10-30T12:34:56.789Z" },
//    "replyMessage": { "uuid" : "f9e88d1d-f12b-4546-829e-97848ac4d275", "text" : "Good afternoon, human. How are you faring today?", "sent" : "2024-10-30T12:34:57.999Z"}
// }

FSAINomiService::FSAINomiService(const std::string& name) : FSAIService(name)
{
    LL_INFOS("AIChat") << "created FSAINomiService" << LL_ENDL;	
};

FSAINomiService::~FSAINomiService()
{
    LL_INFOS("AIChat") << "deleting FSAINomiService" << LL_ENDL;
};

// Called with new config values before they are stored
bool FSAINomiService::validateConfig(const LLSD& config)
{
    LL_INFOS("AIChat") << "To do - Nomi sanity check for config: " << config << LL_ENDL;
    return true;
};

void FSAINomiService::sendChatToAIService(const std::string& message, bool request_direct)
{  // Send message to the AI service
    std::string url  = getAIConfig().get(AI_ENDPOINT);
    if (url.empty())
	{
        LL_WARNS("AIChat") << "Failed to get URL to back-end AI chat service, check the AI configuration panel" << LL_ENDL;
        LL_WARNS("AIChat") << "getAIConfig() is " << getAIConfig() << LL_ENDL;
    }
	else if (getRequestBusy())
	{   // Already have a chat request in-flight to AI LLM.   Save it to resend
        if (mNextMessage.size() == 0)
        {
            mNextMessage = message;
            mNextMessageDirect = request_direct;
            LL_INFOS("AIChat") << "Saving outgoing message since another is in-flight: " << message << LL_ENDL;
        }
        else
        {
            LL_WARNS("AIChat") << "AI request is busy - dropping message.   Already have one waiting.  Dropping message: " << message << LL_ENDL;
        }
	}
    else if (messageToAIShouldBeDropped(message, request_direct))
    {   // messageToAIShouldBeDropped() will log about anything interesting
        return;
    }
    else
    {   // Send message via coroutine
        mRequestDirect = request_direct;
        LL_INFOS("AIChat") << "Sending " << (mRequestDirect ? "direct" : "regular") << " AI chat request to " << url << ": " << message << LL_ENDL;
        setRequestBusy();
        LLCoros::instance().launch("FSAINomiService::getAIResponseCoro",
                                   boost::bind(&FSAINomiService::getAIResponseCoro, this, url, message));
    }
}


bool FSAINomiService::messageToAIShouldBeDropped(const std::string& message, bool request_direct)
{
    if (!request_direct)
    {  // If someday more incoming message filtering is needed, this is a good bottleneck
        // Reject messages with "OOC:" from user - tbd:  handle differently?
        std::string msg_lower = utf8str_tolower(message);
        if (msg_lower.find("ooc:") != std::string::npos)
        {  // Possibly move some of this to base class to reuse in other implementations
            LL_WARNS("AIChat") << "Dropping regular message with OOC:  " << message << LL_ENDL;
            return true;
        }
        if (msg_lower.find("you are chatting with a bot") == 0)
        {   // @simonlsalt:  Ugly match, but a bot warning comes in from SL servers spoofing as if from the bot account
            // This gets in the way if two bots talk to each other, which is an amusing experiment.  I can't complain too much, I wrote it :(
            LL_WARNS("AIChat") << "Dropping bot warning message:  " << message << LL_ENDL;
            return true;
        }
    }
    return false;
}

bool FSAINomiService::getAIResponseCoro(const std::string& url, const std::string& message)
{
    LLCore::HttpRequest::policy_t               httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("genericPostCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t                  httpRequest(new LLCore::HttpRequest);

    LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders());
    headers->append(HTTP_OUT_HEADER_AUTHORIZATION, getAIConfig().get(AI_API_KEY));
    headers->append(HTTP_OUT_HEADER_CONTENT_TYPE, HTTP_CONTENT_JSON);

	LLSD body = LLSD::emptyMap();
    body["messageText"] = message;
    LLSD req_response = httpAdapter->postJsonAndSuspend(httpRequest, url, body, headers);

    LLSD    http_results = req_response.get(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
    LLCore::HttpStatus status       = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(http_results);

    LL_INFOS("AIChat") << "getAIResponseCoro returned status " << status.getStatus()
                       << " and http_results: " << http_results << LL_ENDL;

	if (status.getStatus() == 0)
	{
        LL_INFOS("AIChat") << "AI response:  " << req_response.get("replyMessage") << LL_ENDL;
        std::string ai_message = req_response.get("replyMessage").get("text");

        if (boost::starts_with(ai_message, "(OOC:"))
        {   // Ensure OOC: removed from reply going to other person
            std::size_t close_paren = ai_message.find(")");
            if (close_paren != std::string::npos)
            {
                std::string ooc_part = ai_message.substr(0, close_paren + 1);
                ai_message.erase(0, close_paren + 1);
                LL_INFOS("AIChat") << "Removing OOC part of chat reply:  " << ooc_part << ", leaving message " << ai_message << LL_ENDL;
                FSAIChatMgr::getInstance()->processIncomingAIResponse(ooc_part, mRequestDirect);  // Handle OOC part
                mRequestDirect = false;     // Pass rest of it to conversation
            }
        }

        if (ai_message.length())
        {
            FSAIChatMgr::getInstance()->processIncomingAIResponse(ai_message, mRequestDirect);
        }
        //'replyMessage' : {
        //    'sent': '2024-11-21T01:15:38.726Z',
        //    'text': 'Okay, enjoy your dinner and let me know when you\'re ready to resume chatting!',
        //    'uuid': '673e89ba-7ec7-d197-7ce9-777000000000'
        //},
	}
	else
	{
        LL_WARNS("AIChat") << "Error status from AI service request:  " << http_results << LL_ENDL;
        setRequestBusy(false);
        return false;
    }

    setRequestBusy(false);

    if (mNextMessage.size() > 0)
    {   // Have another message ready to go
        LL_INFOS("AIChat") << "Sending saved chat request to " << url << LL_ENDL;
        std::string next_message = mNextMessage;
        mNextMessage.clear();
        mRequestDirect = mNextMessageDirect;
        setRequestBusy();
        LLCoros::instance().launch("FSAINomiService::getAIResponseCoro",
                                   boost::bind(&FSAINomiService::getAIResponseCoro, this, url, message));
    }

    return true;
}


void FSAINomiService::sendChatTargetChangeMessage(const std::string& previous_name, const std::string& new_name)
{  // Caller should ensure there is a name change and new_name is not empty
    static std::string chat_ai_previous("  Do not mention [PREVIOUS_NAME].");        // tbd - translate?
    static std::string chat_ai_new("(OOC: You are now talking with [NEW_NAME].  Do not mention any other people you have been talking with.");

    LLStringUtil::format_map_t sub_map;
    sub_map["PREVIOUS_NAME"] = previous_name;
    sub_map["NEW_NAME"]      = new_name;

    std::string new_chat_msg(chat_ai_new);
    if (previous_name.size())
    {  // Replace the tag in the first string
        new_chat_msg.append(chat_ai_previous);
    }
    new_chat_msg.append(")");       // close "(OOC: ..." parenthesis
    LLStringUtil::format(new_chat_msg, sub_map);
    FSAIChatMgr::getInstance()->sendChatToAIService(new_chat_msg, true);  // Send that as a direct message to the LLM
}



// ------------------------------------------------
// Use local LLM via LM Studio
FSAILMStudioService::FSAILMStudioService(const std::string& name) : FSAIService(name)
{
    LL_INFOS("AIChat") << "created FSAILMStudioService" << LL_ENDL;
};


FSAILMStudioService::~FSAILMStudioService()
{
    LL_INFOS("AIChat") << "deleting FSAILMStudioService" << LL_ENDL;
};


// Called with new config values, getAIConfig() still contains old values 
bool FSAILMStudioService::validateConfig(const LLSD& config)
{
    // to do - sanity check config values
    return true;
};

void FSAILMStudioService::sendChatToAIService(const std::string& message, bool request_direct)
{   // Send message to the AI service
    mRequestDirect   = request_direct;
    std::string url;    // to do - get value and fill out request data
    if (!url.empty())
    {
        LLCoros::instance().launch("FSAILMStudioService::getAIResponseCoro",
                                   boost::bind(&FSAILMStudioService::getAIResponseCoro, this, url, message));
    }
}

bool FSAILMStudioService::getAIResponseCoro(const std::string& url, const std::string& message)
{
    // To do - add flag showing a request is in flight
 
    LLCore::HttpRequest::policy_t               httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("genericPostCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t                  httpRequest(new LLCore::HttpRequest);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

    LLSD               httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status      = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

	// to do - fix for proper request - POST ?
    LL_INFOS("AIChat") << "getAIResponseCoro returned status " << status.getStatus() << " and content: "
                       << result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_CONTENT] << LL_ENDL;

    if (!status)
    {
        return false;
    }

 /*   if (result.has(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_CONTENT) &&
        result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_CONTENT].isArray())
    {
        mSkuDescriptions = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_CONTENT];
    }
    else
    {
        LL_WARNS() << "Land SKU description response is malformed" << LL_ENDL;
    }*/

	return true;
}



// ------------------------------------------------
// Use local LLM via OpenAI
FSAIOpenAIService::FSAIOpenAIService(const std::string& name) : FSAIService(name)
{  // Call base class constructor
    LL_INFOS("AIChat") <<"FSAIOpenAIService created with name: " << name<< LL_ENDL;
}

// Destructor
FSAIOpenAIService::~FSAIOpenAIService()
{
	LL_INFOS("AIChat") <<"FSAIOpenAIService destroyed"<< LL_ENDL;
}

// sendChat override
void FSAIOpenAIService::sendChatToAIService(const std::string& message, bool request_direct)
{
    mRequestDirect = request_direct;
    LL_INFOS("AIChat") <<"sendChat called with message: " << message << LL_ENDL;
}

// validateConfig override
bool FSAIOpenAIService::validateConfig(const LLSD& config)
{
    LL_INFOS("AIChat") <<"validateConfig called" << LL_ENDL;
    // Placeholder logic: Assume config is always valid
    return true;
}

// getAIResponseCoro
bool FSAIOpenAIService::getAIResponseCoro(const std::string& url, const std::string& message)
{
    LL_INFOS("AIChat") <<"getAIResponseCoro called with URL: " << url << " and message: " << message<< LL_ENDL;
    // Placeholder logic
    return true;
}


// ------------------------------------------------
// Use local LLM via Kindroid
FSAIKindroidService::FSAIKindroidService(const std::string& name) : FSAIService(name)
{  // Call base class constructor
    LL_INFOS("AIChat") << "FSAIKindroidService created with name: " << name << LL_ENDL;
}

// Destructor
FSAIKindroidService::~FSAIKindroidService()
{
	LL_INFOS("AIChat") << "FSAIKindroidService destroyed" << LL_ENDL;
}

// sendChat override
void FSAIKindroidService::sendChatToAIService(const std::string& message, bool request_direct)
{
    mRequestDirect = request_direct;
    LL_INFOS("AIChat") << "sendChat called with message: " << message << LL_ENDL;
    // Placeholder logic
}

// validateConfig override
bool FSAIKindroidService::validateConfig(const LLSD& config)
{
    LL_INFOS("AIChat") << "validateConfig called" << LL_ENDL;
    // Placeholder logic: Assume config is always valid
    return true;
}

// getAIResponseCoro
bool FSAIKindroidService::getAIResponseCoro(const std::string& url, const std::string& message)
{
    LL_INFOS("AIChat") << "getAIResponseCoro called with URL: " << url << " and message: " << message << LL_ENDL;
    // Placeholder logic
    return true;
}
