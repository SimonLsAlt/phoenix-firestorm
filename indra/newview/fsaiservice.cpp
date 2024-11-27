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

bool FSAINomiService::sendChatToAIService(const std::string& message, bool request_direct)
{  // Send message to the AI service
    bool        sent = false;
    std::string url  = getAIConfig().get(AI_ENDPOINT);
    if (url.empty())
	{
        LL_WARNS("AIChat") << "Failed to get URL to back-end AI chat service, check the AI configuration panel" << LL_ENDL;
        LL_WARNS("AIChat") << "getAIConfig() is " << getAIConfig() << LL_ENDL;
    }
	else if (getRequestBusy())
	{
        LL_WARNS("AIChat") << "AI request is busy - need to handle this better with a queue?" << LL_ENDL;
	}
    else
    {
        LL_INFOS("AIChat") << "Sending chat request to " << url << LL_ENDL;
        mRequestDirect = request_direct;    // xxx
        setRequestBusy();
        LLCoros::instance().launch("FSAINomiService::getAIResponseCoro",
                                   boost::bind(&FSAINomiService::getAIResponseCoro, this, url, message));
        sent = true;
    }

    return sent;
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
        LL_INFOS("AIChat") << "Do something with:  " << req_response.get("replyMessage") << LL_ENDL;
        const std::string& ai_message = req_response.get("replyMessage").get("text");

		FSAIChatMgr::getInstance()->processIncomingAIResponse(ai_message, mRequestDirect);
        //'replyMessage' : {
        //    'sent': '2024-11-21T01:15:38.726Z',
        //    'text': 'Okay, enjoy your dinner and let me know when you\'re ready to resume testing!',
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
    return true;
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

bool FSAILMStudioService::sendChatToAIService(const std::string& message, bool request_direct)
{   // Send message to the AI service
    mRequestDirect   = request_direct;
    bool        sent = false;
    std::string url;    // to do - get value and fill out request data
    if (!url.empty())
    {
        LLCoros::instance().launch("FSAILMStudioService::getAIResponseCoro",
                                   boost::bind(&FSAILMStudioService::getAIResponseCoro, this, url, message));
        sent = true;
    }

    return sent;
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
bool FSAIOpenAIService::sendChatToAIService(const std::string& message, bool request_direct)
{
    mRequestDirect = request_direct;
    LL_INFOS("AIChat") <<"sendChat called with message: " << message << LL_ENDL;
    // Placeholder logic
    return true;
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
bool FSAIKindroidService::sendChatToAIService(const std::string& message, bool request_direct)
{
    mRequestDirect = request_direct;
    LL_INFOS("AIChat") << "sendChat called with message: " << message << LL_ENDL;
    // Placeholder logic
    return true;
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
