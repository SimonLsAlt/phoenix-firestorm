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
{   // to do - replace with config storaged in service
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


// virtual but useful base class
bool FSAIService::okToSendChatToAIService(const std::string& message, bool request_direct)
{
    // to do - better config testing, use validateConfig()
    std::string base_url = getAIConfig().get(AI_ENDPOINT);
    if (base_url.empty())
    {
        LL_WARNS("AIChat") << "Failed to get URL to back-end AI chat service, check the AI configuration panel" << LL_ENDL;
        LL_WARNS("AIChat") << "getAIConfig() is " << getAIConfig() << LL_ENDL;
        return false;
    }

    if (getRequestBusy())
    {  // Already have a chat request in-flight to AI LLM.   Save it to resend
        if (mNextMessage.size() == 0)
        {
            mNextMessage       = message;
            mNextMessageDirect = request_direct;
            LL_INFOS("AIChat") << "Saving outgoing message since another is in-flight: " << message << LL_ENDL;
        }
        else
        {
            LL_WARNS("AIChat") << "AI request is busy - dropping message.   Already have one waiting.  Dropping message: " << message
                               << LL_ENDL;
        }
        return false;
    }

    if (messageToAIShouldBeDropped(message, request_direct))
    {  // messageToAIShouldBeDropped() will log about anything interesting
        return false;
    }

    // OK to send message to AI
    return true;
}

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
    LL_DEBUGS("AIChat") << "created FSAINomiService" << LL_ENDL;
};

FSAINomiService::~FSAINomiService()
{
    LL_DEBUGS("AIChat") << "deleting FSAINomiService" << LL_ENDL;
};

// Called with new config values before they are stored
bool FSAINomiService::validateConfig(const LLSD& config)
{
    LL_INFOS("AIChat") << "To do - Nomi sanity check for config: " << config << LL_ENDL;
    return true;
};

void FSAINomiService::sendChatToAIService(const std::string& message, bool request_direct)
{  // Send message to the AI service
    if (!okToSendChatToAIService(message, request_direct))
    {
        return;
    }

    mRequestDirect  = request_direct;

    std::string url = getAIConfig().get(AI_ENDPOINT);
    // Send message via coroutine
    LL_DEBUGS("AIChat") << "Sending " << (mRequestDirect ? "direct" : "regular") << " AI chat request to " << url << ": " << message
                        << LL_ENDL;
    setRequestBusy();
    LLCoros::instance().launch("FSAINomiService::sendMessageToAICoro",
                                boost::bind(&FSAINomiService::sendMessageToAICoro, this, url, message));
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

bool FSAINomiService::sendMessageToAICoro(const std::string& url, const std::string& message)
{
    LLCore::HttpRequest::policy_t               http_policy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t http_adapter(new LLCoreHttpUtil::HttpCoroutineAdapter("genericPostCoro", http_policy));
    LLCore::HttpRequest::ptr_t                  http_request(new LLCore::HttpRequest);

    LLCore::HttpHeaders::ptr_t headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders());
    headers->append(HTTP_OUT_HEADER_AUTHORIZATION, getAIConfig().get(AI_API_KEY));
    headers->append(HTTP_OUT_HEADER_CONTENT_TYPE, HTTP_CONTENT_JSON);

	LLSD body = LLSD::emptyMap();
    body["messageText"] = message;
    LLSD req_response = http_adapter->postJsonAndSuspend(http_request, url, body, headers);

    LLSD    http_results = req_response.get(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
    LLCore::HttpStatus status       = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(http_results);

    LL_DEBUGS("AIChat") << "sendMessageToAICoro returned status " << status.getStatus()
                       << " and http_results: " << http_results << LL_ENDL;

	if (status.getStatus() == 0)
	{
        LL_DEBUGS("AIChat") << "AI response:  " << req_response.get("replyMessage") << LL_ENDL;
        std::string ai_message = req_response.get("replyMessage").get("text");

        if (boost::starts_with(ai_message, "(OOC:"))
        {   // Ensure OOC: removed from reply going to other person
            std::size_t close_paren = ai_message.find(")");
            if (close_paren != std::string::npos)
            {
                std::string ooc_part = ai_message.substr(0, close_paren + 1);
                ai_message.erase(0, close_paren + 1);
                LL_DEBUGS("AIChat") << "Removing OOC part of chat reply:  " << ooc_part << ", leaving message " << ai_message << LL_ENDL;
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
        LLCoros::instance().launch("FSAINomiService::sendMessageToAICoro",
                                   boost::bind(&FSAINomiService::sendMessageToAICoro, this, url, message));
    }

    return true;
}


void FSAINomiService::aiChatTargetChanged(const std::string& previous_name, const std::string& new_name)
{   // Caller should ensure there is a name change and new_name is not empty
    // Send an OOC: message about the new conversation
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
        LLCoros::instance().launch("FSAILMStudioService::sendMessageToAICoro",
                                   boost::bind(&FSAILMStudioService::sendMessageToAICoro, this, url, message));
    }
}

bool FSAILMStudioService::sendMessageToAICoro(const std::string& url, const std::string& message)
{
    LLCore::HttpRequest::policy_t               http_policy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t http_adapter(new LLCoreHttpUtil::HttpCoroutineAdapter("genericPostCoro", http_policy));
    LLCore::HttpRequest::ptr_t                  http_request(new LLCore::HttpRequest);

    LLSD result = http_adapter->getJsonAndSuspend(http_request, url);

    LLSD               httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status      = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

	// to do - fix for proper request - POST ?
    LL_INFOS("AIChat") << "sendMessageToAICoro returned status " << status.getStatus() << " and content: "
                       << result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_CONTENT] << LL_ENDL;

	return (bool) status;
}



// ------------------------------------------------
// Use OpenAI

// Create new thread with each new chat av / session
// To send a message:
//    Add a message to the thread with role ‘user’ and incoming chat as ‘content’
//    Run it
//    Get run
//      loop - check status to be completed or error
//    Get the last message

FSAIOpenAIService::FSAIOpenAIService(const std::string& name) : FSAIService(name)
{  // Call base class constructor
    LL_INFOS("AIChat") <<"FSAIOpenAIService created with name: " << name << LL_ENDL;
}

// Destructor
FSAIOpenAIService::~FSAIOpenAIService()
{
	LL_INFOS("AIChat") <<"FSAIOpenAIService destroyed"<< LL_ENDL;
}

// sendChat override
void FSAIOpenAIService::sendChatToAIService(const std::string& message, bool request_direct)
{
    if (!okToSendChatToAIService(message, request_direct))
    {
        return;
    }

    LL_DEBUGS("AIChat") << "sendChat called with message: " << message << LL_ENDL;
    mRequestDirect = request_direct;
    
    std::string base_url = getAIConfig().get(AI_ENDPOINT);

    // Send message via coroutine
    LL_DEBUGS("AIChat") << "Sending " << (mRequestDirect ? "direct" : "regular") << " AI chat request to " << base_url << ": "
                        << message
                        << LL_ENDL;
    setRequestBusy();
    LLCoros::instance().launch("FSAIOpenAIService::sendMessageToAICoro",
                               boost::bind(&FSAIOpenAIService::sendMessageToAICoro, this, base_url, message));
}

// aiChatTargetChanged override
void FSAIOpenAIService::aiChatTargetChanged(const std::string& previous_name, const std::string& new_name)
{
    LL_INFOS("AIChat") << "aiChatTargetChanged called chat switch from " << previous_name << " to " << new_name << LL_ENDL;
    // to do - create new thread, zap old one?
}


// validateConfig override
bool FSAIOpenAIService::validateConfig(const LLSD& config)
{
    LL_INFOS("AIChat") <<"validateConfig called" << LL_ENDL;
    // Placeholder logic: Assume config is always valid
    return true;
}

// sendMessageToAICoro
bool FSAIOpenAIService::sendMessageToAICoro(const std::string& url, const std::string& message)
{
    LL_INFOS("AIChat") <<"sendMessageToAICoro called with URL: " << url << " and message: " << message<< LL_ENDL;

    LLCore::HttpRequest::policy_t               http_policy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t http_adapter(new LLCoreHttpUtil::HttpCoroutineAdapter("genericPostCoro", http_policy));
    LLCore::HttpRequest::ptr_t                  http_request(new LLCore::HttpRequest);

    // Headers used for most of the POST operations
    LLCore::HttpHeaders::ptr_t post_headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders());
    std::string auth_header = getAIConfig().get(AI_API_KEY).asString();
    if (!boost::starts_with(auth_header, "Bearer "))
    {   // openAI wants "Authorization: Bearer <api key>"
        auth_header = std::string("Bearer ").append(auth_header);
    }
    post_headers->append(HTTP_OUT_HEADER_AUTHORIZATION, auth_header);
    post_headers->append(HTTP_OUT_HEADER_CONTENT_TYPE, HTTP_CONTENT_JSON);
    post_headers->append("OpenAI-Beta", "assistants=v2");

    // Headers used for the GET operations
    LLCore::HttpHeaders::ptr_t get_headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders());
    get_headers->append(HTTP_OUT_HEADER_AUTHORIZATION, auth_header);
    get_headers->append("OpenAI-Beta", "assistants=v2");

    LLSD               body = LLSD::emptyMap();
    LLSD               req_response;
    LLSD               http_results;
    LLCore::HttpStatus status;
    std::string        openai_url;
    std::string        base_url(url);
    if (!boost::ends_with(base_url, "/"))
    {
        base_url.append("/");
    }
    std::string run_status;

    // Step 1 : create thread if there isn't one yet
    if (mOpenAIThread.empty())
    {
        openai_url = base_url;
        openai_url.append("threads");
        req_response = http_adapter->postJsonAndSuspend(http_request, openai_url, body, post_headers);

        http_results = req_response.get(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
        status       = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(http_results);

        LL_DEBUGS("AIChat") << "creating openAI thread returned status " << status.getStatus() << " and http_results: " << http_results
                            << LL_ENDL;

        if (status.getStatus() != 0)
        {
            LL_WARNS("AIChat") << "Unable to make OpenAI thread from " << openai_url << ", fix configuration settings?" << LL_ENDL;
            setRequestBusy(false);
            return false;
        }

        // { "id" : "thread_Wup24tssocSFdvYDiN8xibvc", "object" : "thread", "created_at" : 1733957473, "metadata" : {}, "tool_resources": {} }
        mOpenAIThread = req_response.get("id").asString();
        LL_DEBUGS("AIChat") << "OpenAI thread set to " << mOpenAIThread << LL_ENDL;
    }

    // Step 2 : add message to the thread

    openai_url = base_url;     // build url:  api.openai.com/v1/threads/$OPENAI_THREAD/messages
    openai_url.append("threads/");
    openai_url.append(mOpenAIThread);
    openai_url.append("/messages");

    body = LLSD::emptyMap();
    body["role"] = "user";
    body["content"] = message;

    req_response = http_adapter->postJsonAndSuspend(http_request, openai_url, body, post_headers);
    http_results = req_response.get(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
    status       = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(http_results);

    LL_DEBUGS("AIChat") << "OpenAI adding message to thread returned status " << status.getStatus() << " and http_results: " << http_results << LL_ENDL;

    if (status.getStatus() != 0)
    {
        LL_WARNS("AIChat") << "Unable to add message to OpenAI thread from " << openai_url << LL_ENDL;
        setRequestBusy(false);
        return false;
    }

    //{     Need to use any of these values?
    //    "id": "msg_D2jTGwoI6TAzc4zgYkZGOEfs",
    //    "object": "thread.message",
    //    "created_at": 1733957632,
    //    "assistant_id": null,
    //    "thread_id": "thread_Wup04tssocSFdvYDiN8xibvc",
    //    "run_id": null,
    //    "role": "user",
    //    "content": [{ "type": "text", "text": { "value": "What is the best way to cook shrimp?", "annotations": [] } }],
    //    "attachments": [],
    //    "metadata": {}
    //}

    // Step 3:  get assistant instructions if we don't have it already
    if (mAssistantInstructions.empty())
    {
        openai_url = base_url;  // build url: //api.openai.com/v1/assistants/$OPENAI_ASST_ID
        openai_url.append("assistants/");
        openai_url.append(getAIConfig().get(AI_CHARACTER_ID));

        req_response = http_adapter->getJsonAndSuspend(http_request, openai_url, post_headers);
        http_results = req_response.get(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
        status       = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(http_results);

        LL_DEBUGS("AIChat") << "OpenAI getting assistant info returned " << status.getStatus() << " and req_response: " << req_response
                            << LL_ENDL;

        if (status.getStatus() != 0)
        {
            LL_WARNS("AIChat") << "Unable to get OpenAI assistant from " << openai_url << " and id " << getAIConfig().get(AI_CHARACTER_ID)
                               << ", fix configuration settings?" << LL_ENDL;
            setRequestBusy(false);
            return false;
        }

        //{  results
        //  "id" : "asst_3sSIOJIb3PnQTENXne5jD8t4",
        //  "object" : "assistant",
        //  "created_at" : 1733793284,
        //  "name" : "MrHap",
        //  "description" : null,
        //  "model" : "gpt-4o-mini",
        //  "instructions" : "You are a creative and intelligent AI <lots more>",
        //  "tools" : [],
        //  "top_p" : 1.0,
        //  "temperature" : 1.0,
        //  "tool_resources" : {},
        //  "metadata" : {},
        //  "response_format" : "auto"
        //  }

        mAssistantInstructions = req_response.get("instructions").asString();
        if (mAssistantInstructions.empty())
        {   // Ensure there is something
            mAssistantInstructions = "You are a helpful and friendly AI assistant";
        }
    }

    // Step 4 :  create a run

    openai_url = base_url;  // build url:  api.openai.com/v1/threads/$OPENAI_THREAD/runs
    openai_url.append("threads/");
    openai_url.append(mOpenAIThread);
    openai_url.append("/runs");

    body = LLSD::emptyMap();
    body["assistant_id"] = getAIConfig().get(AI_CHARACTER_ID);
    addAssistantInstructions(body);

    req_response = http_adapter->postJsonAndSuspend(http_request, openai_url, body, post_headers);
    http_results = req_response.get(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
    status       = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(http_results);

    LL_DEBUGS("AIChat") << "OpenAI creating a run returned status " << status.getStatus()
                        << " and http_results: " << http_results << LL_ENDL;

    if (status.getStatus() != 0)
    {
        LL_WARNS("AIChat") << "Unable to create OpenAI run from " << openai_url << ", fix configuration settings?" << LL_ENDL;
        setRequestBusy(false);
        return false;
    }

    //{     results
    //    "id": "run_3fBoJ5ipXZPeieGMZhtJQa27",
    //    "object": "thread.run",
    //    "created_at": 1733957999,
    //    "assistant_id": "asst_3sSIOJIb5PnQTvwxne5218t4",
    //    "thread_id": "thread_Wup04tssqrpwdvYDiN8xibvc",
    //    "status": "queued",
    //    "started_at": null,
    //    "expires_at": 1733958599,
    //    "cancelled_at": null,
    //    "failed_at": null,
    //    "completed_at": null,
    //    "required_action": null,
    //    "last_error": null,
    //    "model": "gpt-4o-mini",
    //    "instructions": "",
    //    "tools": [],
    //    "tool_resources": {},
    //    "metadata": {},
    //    "temperature": 1.0,
    //    "top_p": 1.0,
    //    "max_completion_tokens": null,
    //    "max_prompt_tokens": null,
    //    "truncation_strategy": { "type": "auto", "last_messages": null },
    //    "incomplete_details": null,
    //    "usage": null,
    //    "response_format": "auto",
    //    "tool_choice": "auto",
    //    "parallel_tool_calls": true
    //}

    run_status = req_response.get("status").asString();
    if (run_status != "queued" && run_status != "in_progress")
    {
        LL_WARNS("AIChat") << "Expected OpenAI run to be 'queued' or 'in_progress': " << req_response << LL_ENDL;
        setRequestBusy(false);
        return false;
    }

    mOpenAIRun = req_response.get("id").asString();
    LL_DEBUGS("AIChat") << "OpenAI run set to " << mOpenAIRun << LL_ENDL;

    // Step 5 : check and wait for run to complete

    openai_url = base_url;  // build url:  https://api.openai.com/v1/threads/$OPENAI_THREAD/runs/$OPENAI_RUN_ID
    openai_url.append("threads/");
    openai_url.append(mOpenAIThread);
    openai_url.append("/runs/");
    openai_url.append(mOpenAIRun);

    const F32 SECS_BETWEEN_REQUESTS = 1.f;
    const F32 AI_EXPIRED_TIMEOUT = 30.f;  // seconds

    LLTimer ai_wait_timer;
    ai_wait_timer.setTimerExpirySec(AI_EXPIRED_TIMEOUT);
    S32 ai_wait_req_count = 0;

    while (true)
    {
        llcoro::suspendUntilTimeout(SECS_BETWEEN_REQUESTS);

        ai_wait_req_count++;
        req_response = http_adapter->getJsonAndSuspend(http_request, openai_url, post_headers);
        http_results = req_response.get(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
        status       = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(http_results);

        LL_DEBUGS("AIChat") << "OpenAI getting run info count " << ai_wait_req_count << " to " << openai_url << " returned status "
                            << status.getStatus() << " and req_response: " << req_response << LL_ENDL;

        if (status.getStatus() != 0)
        {
            LL_WARNS("AIChat") << "Unable to get OpenAI run status " << openai_url << " after " << ai_wait_req_count << " checks"
                               << " and " << ai_wait_timer.getElapsedTimeF32() << " seconds" << LL_ENDL;
            setRequestBusy(false);
            return false;
        }
        // results
        //{
        //    "id": "run_3fBoJ5ipXZPeieGMZhtJQa27",
        //    "object": "thread.run",
        //    "created_at": 1733957999,
        //    "assistant_id": "asst_3sSIOJIb3PnQTENXne5jD8t4",
        //    "thread_id": "thread_Wup04tssocSFdvYDiN8xibvc",
        //    "status": "completed",
        //    "started_at": 1733957999,
        //    "expires_at": null,
        //    "cancelled_at": null,
        //    "failed_at": null,
        //    "completed_at": 1733958005,
        //    "required_action": null,
        //    "last_error": null,
        //    "model": "gpt-4o-mini",
        //    "tools": [],
        //    "tool_resources": {},
        //    "metadata": {},
        //    "temperature": 1.0,
        //    "top_p": 1.0,
        //    "max_completion_tokens": null,
        //    "max_prompt_tokens": null,
        //    "truncation_strategy": { "type": "auto", "last_messages": null },
        //    "incomplete_details": null,
        //    "usage":
        //        { "prompt_tokens": 51, "completion_tokens": 339, "total_tokens": 390, "prompt_token_details": { "cached_tokens": 0 } },
        //    "response_format": "auto",
        //    "tool_choice": "auto",
        //    "parallel_tool_calls": true
        //}

        // Possible status values:  queued, in_progress, requires_action, cancelling, cancelled, failed, completed, incomplete, expired
        // https://platform.openai.com/docs/api-reference/runs/object
        run_status = req_response.get("status").asString();
        if (run_status == "completed")
        {
            LL_INFOS("AIChat") << "OpenAI run completed after " << ai_wait_req_count << " checks and "
                               << ai_wait_timer.getElapsedTimeF32() << " seconds" << LL_ENDL;
            break;      // yay!
        }

        LL_DEBUGS("AIChat") << "OpenAI run status response after " << ai_wait_req_count << " checks and "
                            << ai_wait_timer.getElapsedTimeF32() << " seconds: " << req_response << LL_ENDL;

        // Check for problems
        if (run_status == "requires_action" || run_status == "cancelling" || run_status == "cancelled" ||
            run_status == "failed" || run_status == "expired")
        {
            LL_WARNS("AIChat") << "Error OpenAI run status " << run_status << " after " << ai_wait_req_count << " checks"
                               << " and " << ai_wait_timer.getElapsedTimeF32() << " seconds" << LL_ENDL;
            setRequestBusy(false);
            return false;
        }

        if (ai_wait_timer.hasExpired())
        {   // Check for our own timeout
            LL_WARNS("AIChat") << "OpenAI request run timeout, still running after " << ai_wait_req_count << " checks and "
                               << ai_wait_timer.getElapsedTimeF32() << " seconds.  Last req_response:" << req_response << LL_ENDL;
            setRequestBusy(false);
            return false;
        }
    }   // End loop sleeping and checking run status

    // Step 6 : get the most recent message

    openai_url = base_url;  // build url:  curl https://api.openai.com/v1/threads/$OPENAI_THREAD/messages 
    openai_url.append("threads/");
    openai_url.append(mOpenAIThread);
    openai_url.append("/messages?limit=1&order=desc&run_id=");
    openai_url.append(mOpenAIRun);

    req_response = http_adapter->getJsonAndSuspend(http_request, openai_url, get_headers);

    http_results = req_response.get(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
    status       = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(http_results);

    LL_DEBUGS("AIChat") << "OpenAI fetching most recent message from run returned status " << status.getStatus()
                        << " and http_results: " << req_response << LL_ENDL;

    if (status.getStatus() != 0)
    {
        LL_WARNS("AIChat") << "Unable to get last message from OpenAI run via " << openai_url << LL_ENDL;
        setRequestBusy(false);
        return false;
    }

    std::string ai_message;  // Dig text out of response:   data[0].content[0].text.value
    const LLSD& chat_data = req_response.get("data");
    if (chat_data.isArray() && chat_data.size() > 0)
    {
        const LLSD& chat_message_full = chat_data[0];
        if (chat_message_full.isMap())
        {
            const LLSD& chat_message_content = chat_message_full.get("content");
            if (chat_message_content.isArray() && chat_message_content.size() > 0)
            {
                const LLSD& content_first_reply = chat_message_content[0];
                if (content_first_reply.isMap())        //  && content_first_reply.get("type").asString() == "text"
                {
                    const LLSD& first_reply_text = content_first_reply.get("text");
                    if (first_reply_text.isMap())
                    {   // Phew!
                        ai_message = first_reply_text.get("value").asString();
                    }
                }
            }
        }
    }

    if (ai_message.length())
    {
        FSAIChatMgr::getInstance()->processIncomingAIResponse(ai_message, mRequestDirect);
    }
    else
    {
        LL_WARNS("AIChat") << "Unable to extract chat response from OpenAI results " << req_response << LL_ENDL;
    }

    setRequestBusy(false);

    if (mNextMessage.size() > 0)
    {  // Have another message ready to go
        LL_INFOS("AIChat") << "Sending saved chat request to " << url << LL_ENDL;
        std::string next_message = mNextMessage;
        mNextMessage.clear();
        mRequestDirect = mNextMessageDirect;
        setRequestBusy();
        LLCoros::instance().launch("FSAIOpenAIService::sendMessageToAICoro",
                                   boost::bind(&FSAIOpenAIService::sendMessageToAICoro, this, url, message));
    }

    return true;
}


// Called from coroutine
bool FSAIOpenAIService::addAssistantInstructions(LLSD& body)
{   // Add "instructions" if needed
    static S32 reply_size = 0;

    bool added = false;
    if (reply_size != 0)
    {
        std::string extra;
        switch (reply_size)
        {
            case 2:
                extra = "If appropriate, give a medium reponse of a few sentences.";
                break;
            case 3:
                extra = "If appropriate, give a reponse with a few small paragraphs.";
                break;
            case 1:
            default:
                extra = "Give a short reponse.";
                break;
        }

        LL_DEBUGS("AIChat") << "Adding extra instructions: " << extra << LL_ENDL;
        extra.append("\n");
        body["instructions"] = extra.append(mAssistantInstructions);
        added = true;
    }

    return added;
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

// sendMessageToAICoro
bool FSAIKindroidService::sendMessageToAICoro(const std::string& url, const std::string& message)
{
    LL_INFOS("AIChat") << "sendMessageToAICoro called with URL: " << url << " and message: " << message << LL_ENDL;
    // Placeholder logic
    return true;
}
