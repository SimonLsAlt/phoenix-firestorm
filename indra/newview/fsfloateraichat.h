/**
 * @file fsfloateraichat.h
 * @author simonlsalt
 * @brief Floating window for AI chat control
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

#ifndef FS_FSFLOATERAICHAT_H
#define FS_FSFLOATERAICHAT_H

#include <vector>
#include "llagent.h"
#include "llassettype.h"
#include "llfloater.h"
#include "llhost.h"
#include "llpanel.h"
#include "llextendedstatus.h"
#include "llvlcomposition.h"

#include "lleventcoro.h"

class LLComboBox;
class LLDispatcher;
class LLLineEditor;
class LLTabContainer;
class LLViewerRegion;
class LLViewerTextEditor;
class LLCheckBoxCtrl;
class LLLineEditor;
class LLTextBox;
class LLTextEditor;

class FSPanelAIConfiguration;
class FSPanelAIConversation;
class FSPanelAIDirect2LLM;
class FSPanelAIInfo;


// ------------------------------------------------------
// Main floater class

class FSFloaterAIChat : public LLFloater
{
public:

    FSFloaterAIChat(const LLSD& seed);
    ~FSFloaterAIChat();


    void onOpen(const LLSD& key) override;
    void onClose(bool app_quitting) override;
    bool postBuild() override;

    FSPanelAIConfiguration * getPanelConfiguration();
    FSPanelAIConversation  * getPanelConversation();
    FSPanelAIDirect2LLM    * getPanelDirect2LLM();

    static FSFloaterAIChat* getAIChatFloater();

protected:
    // member data

    typedef std::vector<FSPanelAIInfo*> ai_info_panels_t;
    ai_info_panels_t                    mAIInfoPanels;

private:
};


// ------------------------------------------------------
// Base class for all AI information panels.

class FSPanelAIInfo : public LLPanel
{
public:
    FSPanelAIInfo();

    // to do - clean up after more implementation, get rid of unused functions
    bool postBuild() override;

    virtual bool enableUIElement(const std::string& ui_name, const std::string& service_name) const { return true; };
    virtual void resetChat() {};

  protected:
    void updateUIElement(const std::string& lineEditorName, const std::string& textBoxName,
                         const LLSD& ai_config, const std::string& ai_service);
};


// ------------------------------------------------------
// Actual panel classes start here

class FSPanelAIConfiguration : public FSPanelAIInfo
{
public:
    FSPanelAIConfiguration();
    ~FSPanelAIConfiguration() {}

    bool postBuild() override;
    void onSelectAIService();
    void onApplyConfig();

    bool enableUIElement(const std::string& ui_name, const std::string& service_name) const override;

  protected:
    void syncUIWithAISettings();
    void addInLineEditorValue(const std::string& ui_name, LLSD& ai_config) const;
    bool useConfigValue(const std::string& ui_name, const std::string& service_name) const;
};


/////////////////////////////////////////////////////////////////////////////

class FSPanelAIConversation : public FSPanelAIInfo
{
  public:
    FSPanelAIConversation();
    ~FSPanelAIConversation() {}

    bool postBuild() override;
    void resetChat() override;

    void onSendMsgToAgent();  // Send proof-read / edited chat message to avatar
    void onResetChat();     // reset chat button pressed

    // Chat coming from another avatar
    void processIncomingChat(const std::string& name, const std::string& message);

    // Reply from AI
    void displayIncomingAIResponse(const std::string& message);

    void setAIReplyMessagePrompt(const std::string& service_name);

protected:
};


/////////////////////////////////////////////////////////////////////////////

class FSPanelAIDirect2LLM : public FSPanelAIInfo
{
  public:
    FSPanelAIDirect2LLM();
    ~FSPanelAIDirect2LLM() {}

    bool postBuild() override;
    void resetChat() override;

    void onSendMsgToLLM();

    // Reply from AI
    void displayIncomingAIResponse(const std::string& message);

    void setAIServiceNamePrompts(const std::string& service_name);

protected:
};


/////////////////////////////////////////////////////////////////////////////

#endif
