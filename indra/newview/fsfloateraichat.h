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

    virtual S32  getUIElementYShift(const char* ui_name, const char* service_name) { return 0; };
    virtual void resetChat() {};

  protected:
};


// ------------------------------------------------------
// Actual panel classes start here

class FSPanelAIConfiguration : public FSPanelAIInfo
{
public:
    FSPanelAIConfiguration();
    ~FSPanelAIConfiguration() {}

    bool postBuild() override;

    void onSelectAIService();  // Pick the service
    void onSelectAIMode();     // Change the mode (chat or translations)
    void onSelectAILanguage(); // Change the translation target language
    void onApplyConfig(); // Save

    void syncUIWithAISettings();

protected:
    void addInLineEditorValue(const std::string& ui_name, LLSD& ai_config) const;
    bool useConfigValue(const std::string& ui_name, const std::string& service_name) const;

    void saveUIRect(const char * ui_ctrl_name);

    typedef std::map<std::string, LLRect> rect_map_t;
    rect_map_t mOriginalRects; // Map saving original position of UI prompts and text fields

    void createUILayoutData();
    void fillLayoutData(const char* service,
                        S32 epp, S32 sp, S32 akp, S32 ak, S32 cip, S32 ci, S32 mp, S32 m);
    void updatePromptAndTextEditor(const char* line_editor_name, const char* text_box_name, const LLSD& ai_config, const char* ai_service);
    void updateUIWidgets(const LLSD& ai_config);
    S32  getUIElementYShift(const char* ui_name, const char* service_name) override;

    typedef std::map<std::string, S32>                   service_layout_data_t; // UI element name to Y offset mapping
    typedef std::map<std::string, service_layout_data_t> layout_data_t; // Map of AI service name to UI layout positions and visibility
    layout_data_t mUILayoutData;
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
