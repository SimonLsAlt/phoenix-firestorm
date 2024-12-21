/**
 * @file fsaiservice.h
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

#ifndef FS_FSAISERVICE_H
#define FS_FSAISERVICE_H

class LLSD;


// Abstract (mostly) base class
class FSAIService
{
public:
    FSAIService(const std::string& name);
    ~FSAIService();

    // Set and get the configuration values
    virtual bool validateConfig(const LLSD& config) = 0;

    static const LLSD& getAIConfig();

    const std::string& getName() { return mName; };

    virtual void sendChatToAIService(const std::string& message, bool request_direct) = 0;
    virtual bool okToSendChatToAIService(const std::string& message, bool request_direct);

    virtual void aiChatTargetChanged(const std::string& previous_name, const std::string& new_name) {};

    // Check if message should be dropped
    virtual bool messageToAIShouldBeDropped(const std::string& message, bool request_direct) { return false; };

    // Save in history?
    virtual bool saveChatHistory() const { return false; };

    // Flag indicating a request is processing
    void setRequestBusy(bool busy = true) { mRequestBusy = busy; };
    bool getRequestBusy() const           { return mRequestBusy; };

    // Use this factory to create the correct service object
    static FSAIService * createFSAIService(const std::string& service_name);

protected:

    bool        mRequestBusy;   // Flag that a request is running the coroutine
    bool        mRequestDirect; // Flag that a request is direct to LLM, not from avatar chat (to do - make a type?)
    std::string mName;          // Name of this AI service

    // Outgoing to AI message queue
    std::queue<std::pair<std::string, bool>> mAIOutboxQueue;  // Contains message and "direct" flag
};


// ------------------------------------------------
// Use nomi.ai
class FSAINomiService : public FSAIService
{
  public:
    FSAINomiService(const std::string& name);
    ~FSAINomiService();

    virtual void sendChatToAIService(const std::string& message, bool request_direct = false) override;
    virtual void aiChatTargetChanged(const std::string& previous_name, const std::string& new_name) override;
    virtual bool messageToAIShouldBeDropped(const std::string& message, bool request_direct) override;

  protected:
    virtual bool validateConfig(const LLSD& config) override;

    bool sendMessageToAICoro(const std::string & message);
};



// ------------------------------------------------
// Use local LLM via LM Studio
class FSAILMStudioService : public FSAIService
{
  public:
    FSAILMStudioService(const std::string& name);
    ~FSAILMStudioService();

    virtual void sendChatToAIService(const std::string& message, bool request_direct = false) override;

  protected:
    virtual bool validateConfig(const LLSD& config) override;

    bool sendMessageToAICoro(const std::string& message);
};


// ------------------------------------------------
// Use local LLM via OpenAI
class FSAIOpenAIService : public FSAIService
{
  public:
    FSAIOpenAIService(const std::string& name);
    ~FSAIOpenAIService();

    virtual void sendChatToAIService(const std::string& message, bool request_direct = false) override;
    virtual void aiChatTargetChanged(const std::string& previous_name, const std::string& new_name) override;

  protected:
    virtual bool validateConfig(const LLSD& config) override;

    bool sendMessageToAICoro(const std::string& message);
    bool addAssistantInstructions(LLSD& body);

    std::string mAssistantInstructions; // Value fetched from openAI
    std::string mOpenAIThread;          // OpenAI thread ID for conversation with assistant
    std::string mOpenAIRun;             // OpenID run ID for a the current message
};


// ------------------------------------------------
// Use local LLM via Kindroid
class FSAIKindroidService : public FSAIService
{
  public:
    FSAIKindroidService(const std::string& name);
    ~FSAIKindroidService();

    virtual void sendChatToAIService(const std::string& message, bool request_direct = false) override;

  protected:
    virtual bool validateConfig(const LLSD& config) override;

    bool sendMessageToAICoro(const std::string& message);
};

#endif
