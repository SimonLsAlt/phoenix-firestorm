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

    virtual bool sendChatToAIService(const std::string& message, bool request_direct) = 0;

    // Flag indicating a request is processing
    void setRequestBusy(bool busy = true) { mRequestBusy = busy; };
    bool getRequestBusy() const           { return mRequestBusy; };

    // Use this factory to create the correct service object
    static FSAIService * createFSAIService(const std::string& service_name);

protected:

    bool        mRequestBusy;  // Flag that a request is running the coroutine
    bool        mRequestDirect;  // Flag that a request is direct to LLM, not from avatar chat (to do - make a type?)
    std::string mName;         // Name of this AI service
};


// ------------------------------------------------
// Use nomi.ai
class FSAINomiService : public FSAIService
{
  public:
    FSAINomiService(const std::string& name);
    ~FSAINomiService();

    virtual bool sendChatToAIService(const std::string& message, bool request_direct = false) override;

  protected:
    virtual bool validateConfig(const LLSD& config) override;

    bool getAIResponseCoro(const std::string & url, const std::string & message);
};

// ------------------------------------------------
// Use local LLM via LM Studio
class FSAILMStudioService : public FSAIService
{
  public:
    FSAILMStudioService(const std::string& name);
    ~FSAILMStudioService();

    virtual bool sendChatToAIService(const std::string& message, bool request_direct = false) override;

  protected:
    virtual bool validateConfig(const LLSD& config) override;

    bool getAIResponseCoro(const std::string& url, const std::string& message);
};


// ------------------------------------------------
// Use local LLM via OpenAI
class FSAIOpenAIService : public FSAIService
{
  public:
    FSAIOpenAIService(const std::string& name);
    ~FSAIOpenAIService();

    virtual bool sendChatToAIService(const std::string& message, bool request_direct = false) override;

  protected:
    virtual bool validateConfig(const LLSD& config) override;

    bool getAIResponseCoro(const std::string& url, const std::string& message);
};


// ------------------------------------------------
// Use local LLM via Kindroid
class FSAIKindroidService : public FSAIService
{
  public:
    FSAIKindroidService(const std::string& name);
    ~FSAIKindroidService();

    virtual bool sendChatToAIService(const std::string& message, bool request_direct = false) override;

  protected:
    virtual bool validateConfig(const LLSD& config) override;

    bool getAIResponseCoro(const std::string& url, const std::string& message);
};

#endif
