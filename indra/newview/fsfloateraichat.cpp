/**
 * @file fsfloateraichat.cpp
 * @author simonlsalt
 * @brief Floating window for AI bot control
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

#include "fsfloateraichat.h"
#include "fsaichatmgr.h"

#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lldispatcher.h"
#include "llfloater.h"
#include "lllineeditor.h"
#include "llpanel.h"
#include "lltabcontainer.h"
#include "lltexteditor.h"
#include "lltrans.h"
#include "llviewerregion.h"
#include "lluictrl.h"


// -----------------------------------------------------------------------------
// class FSFloaterAIChat

static FSFloaterAIChat* sAIChatFloater = nullptr;

// static
FSFloaterAIChat* FSFloaterAIChat::getAIChatFloater()
{
    return sAIChatFloater;
}


void FSFloaterAIChat::onOpen(const LLSD& key)
{
    // Placeholder for onOpen logic
    sAIChatFloater = this;
}

void FSFloaterAIChat::onClose(bool app_quitting)
{
    // Placeholder for onClose logic
    sAIChatFloater = nullptr;
}

bool FSFloaterAIChat::postBuild()
{
    mTab = getChild<LLTabContainer>("ai_bots_panels");
    mTab->setCommitCallback(boost::bind(&FSFloaterAIChat::onTabSelected, this, _2));

    // contruct the panels
    FSPanelAIInfo* panel = new FSPanelAIConfiguration;
    mAIInfoPanels.push_back(panel);
    panel->buildFromFile("panel_ai_configuration.xml");
    mTab->addTabPanel(LLTabContainer::TabPanelParams().panel(panel).select_tab(true));

    panel = new FSPanelAIConversation;
    mAIInfoPanels.push_back(panel);
    panel->buildFromFile("panel_ai_conversation.xml");
    mTab->addTabPanel(panel);

    return true;  // Assuming success for placeholder
}

FSPanelAIConfiguration* FSFloaterAIChat::getPanelConfiguration()
{
    // Placeholder for getPanelConfiguration logic
    if (mAIInfoPanels.size() > 0)
        return dynamic_cast<FSPanelAIConfiguration*>(mAIInfoPanels[0]);
    return nullptr;  // Return nullptr as a placeholder
}

FSPanelAIConversation* FSFloaterAIChat::getPanelConversation()
{
    // Placeholder for getPanelConversation logic
    if (mAIInfoPanels.size() > 1)
        return dynamic_cast<FSPanelAIConversation*>(mAIInfoPanels[1]);
    return nullptr;  // Return nullptr as a placeholder
}

void FSFloaterAIChat::refresh()
{
    // Placeholder for refresh logic
}

FSFloaterAIChat::FSFloaterAIChat(const LLSD& seed) : LLFloater(seed)
{
    // Placeholder for constructor logic
}

FSFloaterAIChat::~FSFloaterAIChat()
{
    // Placeholder for destructor logic
}

void FSFloaterAIChat::onTabSelected(const LLSD& param)
{
    // Placeholder for onTabSelected logic
}

void FSFloaterAIChat::disableTabCtrls()
{
    // Placeholder for disableTabCtrls logic
}

void FSFloaterAIChat::refreshFromRegion(LLViewerRegion* region)
{
    // Placeholder for refreshFromRegion logic
}

void FSFloaterAIChat::onGodLevelChange(U8 god_level)
{
    // Placeholder for onGodLevelChange logic
}



// -----------------------------------------------------------------------------
// class FSPanelAIInfo - base class

// Constructor
FSPanelAIInfo::FSPanelAIInfo()
{
    // placeholder
}

// Post build handler
bool FSPanelAIInfo::postBuild()
{
    LL_INFOS("AIChat") << "in FSPanelAIInfo postBuild()" << LL_ENDL;
    return true;  // Assuming success for placeholder
}

// Update child control
void FSPanelAIInfo::updateChild(LLUICtrl* child_ctrl)
{
    // Placeholder for updateChild logic
}

// Enable button
void FSPanelAIInfo::enableButton(const std::string& btn_name, bool enable)
{
    // Placeholder for enableButton logic
}

// Disable button
void FSPanelAIInfo::disableButton(const std::string& btn_name)
{
    // Placeholder for disableButton logic
}

// Initialize control
void FSPanelAIInfo::initCtrl(const std::string& name)
{
    // Placeholder for initCtrl logic
}

// Template method for initializing and setting control
template <typename CTRL> void FSPanelAIInfo::initAndSetCtrl(CTRL*& ctrl, const std::string& name)
{
    // Placeholder for initAndSetCtrl logic
}


// -----------------------------------------------------------------------------
// class FSPanelAIConfiguration

FSPanelAIConfiguration::FSPanelAIConfiguration() : mAIChatEnabled(nullptr),
    mAIServiceCombo(nullptr),
    mAIEndpoint(nullptr),
    mAIAccountKey(nullptr),
    mAICharacterKey(nullptr),
    mAIConfigApplyBtn(nullptr)
{
    // Constructor implementation here
    // Initialize any members or set up necessary configurations
}


bool FSPanelAIConfiguration::postBuild()
{
    // Placeholder implementation for post-build actions
    // Code here should set up the panel UI after it's constructed

    mAIServiceCombo = getChild<LLComboBox>("ai service");
    if (mAIServiceCombo)
    {
        mAIServiceCombo->setCommitCallback(boost::bind(&FSPanelAIConfiguration::onSelectAIService, this));
    }

    mAIChatEnabled  = getChild<LLCheckBoxCtrl>("enable_ai_chat");
    mAIEndpoint     = getChild<LLLineEditor>("ai_endpoint_value");
    mAIAccountKey   = getChild<LLLineEditor>("ai_api_key_value");
    mAICharacterKey = getChild<LLLineEditor>("ai_character_id_value");

    mAIConfigApplyBtn = getChild<LLButton>("apply_btn");
    if (mAIConfigApplyBtn)
    {
        mAIConfigApplyBtn->setCommitCallback(boost::bind(&FSPanelAIConfiguration::onApplyConfig, this));
    }

    // Read and set saved values
    syncUIWithAISettings();

    return FSPanelAIInfo::postBuild();
}

void FSPanelAIConfiguration::onSelectAIService()
{
    if (mAIServiceCombo)
    {
        std::string ai_service_name = mAIServiceCombo->getValue().asString();
        LL_INFOS("AIChat") << "Selected AI service " << ai_service_name << LL_ENDL;

        LLSD ai_config = LLSD::emptyMap();
        ai_config["service"] = ai_service_name;
        FSAIChatMgr::getInstance()->setAIConfig(ai_config);
    }
}

void FSPanelAIConfiguration::syncUIWithAISettings()
{
    const LLSD& ai_config = FSAIChatMgr::getInstance()->getAIConfig();

    LL_INFOS("AIChat") << "current config is " << ai_config << LL_ENDL;

    if (mAIChatEnabled)
    {
        mAIChatEnabled->setValue(ai_config.get("chat_enabled"));
    }
    if (mAIServiceCombo)
    {
        mAIServiceCombo->setValue(ai_config.get("service"));
    }
    if (mAIEndpoint)
    {
        mAIEndpoint->setValue(ai_config.get("endpoint"));
    }
    if (mAIAccountKey)
    {
        mAIAccountKey->setValue(ai_config.get("account_key"));
    }
    if (mAICharacterKey)
    {
        mAICharacterKey->setValue(ai_config.get("character_key"));
    }
}





void FSPanelAIConfiguration::onApplyConfig()
{
    // Callback for pressing "Apply" button
    LL_INFOS("AIChat") << "Config Apply button pressed" << LL_ENDL;

    LLSD        ai_config      = LLSD::emptyMap();

    if (mAIChatEnabled)
    {
        ai_config["chat_enabled"] = mAIChatEnabled->getValue().asBoolean();
    }
    if (mAIServiceCombo)
    {
        ai_config["service"] = mAIServiceCombo->getValue().asString();
    }
    if (mAIEndpoint)
    {
        std::string endpoint  = mAIEndpoint->getValue().asString();
        ai_config["endpoint"] = endpoint;
    }
    if (mAIAccountKey)
    {
        ai_config["account_key"] = mAIAccountKey->getValue().asString();
    }
    if (mAICharacterKey)
    {
        ai_config["character_key"] = mAICharacterKey->getValue().asString();
    }

    FSAIChatMgr::getInstance()->setAIConfig(ai_config);
}


// -----------------------------------------------------------------------------
// class FSPanelAIConversation

FSPanelAIConversation::FSPanelAIConversation() : mChatWith(NULL),
                                                 mLastAVChat(NULL),
                                                 mAIService(NULL),
                                                 mAIReply(NULL)
{
    // Constructor implementation here
    // Initialize any members or set up necessary configurations
}

void FSPanelAIConversation::initDispatch(LLDispatcher& dispatch)
{
    // Placeholder implementation for setting up dispatch routines
    // This function should register the necessary callbacks or handlers in dispatch
}

void FSPanelAIConversation::updateControls(LLViewerRegion* region)
{
    // Placeholder implementation for updating controls based on the given region
    // Code here should modify UI elements or settings based on region-specific data
}

bool FSPanelAIConversation::postBuild()
{
    // Placeholder implementation for post-build actions
    // Code here should set up the panel UI after it's constructed
    mChatWith = getChild<LLTextBox>("ai_chat_with");
    mLastAVChat = getChild<LLTextBox>("last_av_message");
    mAIService  = getChild<LLTextBox>("ai_chat_from");
    mAIReply    = getChild<LLTextEditor>("ai_chat_editor");

    return FSPanelAIInfo::postBuild();
}

void FSPanelAIConversation::updateChild(LLUICtrl* child_ctrl)
{
    // Placeholder implementation for updating a specific child control
    // This could involve setting properties or refreshing the child UI element
}

void FSPanelAIConversation::refresh()
{
    // Placeholder implementation for refreshing the entire panel
    // Code here should ensure all elements are updated to reflect current data
}

void FSPanelAIConversation::processIncomingChat(const std::string& name, const std::string& message)
{   // Handle chat from other user
    if (!mChatWith || !mLastAVChat)
    {
        LL_WARNS("AIChat") << "Unexpected null child item, can't process incoming chat" << LL_ENDL;
        return;
    }

    std::string tmp_str;
    LLStringUtil::format_map_t args;
    args["[OTHER_AV]"] = name;
    LLTrans::findString(tmp_str, "ai_chat_with", args);
    mChatWith->setText(tmp_str);
    mLastAVChat->setText(message);
}

// Handle chat response from AI
void FSPanelAIConversation::processIncomingAIResponse(const std::string& service_name, const std::string& message)
{
    if (!mAIService || !mAIReply)
    {
        LL_WARNS("AIChat") << "Unexpected null child item, can't process AI chat reply" << LL_ENDL;
        return;
    }

    std::string            tmp_str;
    LLStringUtil::format_map_t args;
    args["[AI_SERVICE_NAME]"] = service_name;
    LLTrans::findString(tmp_str, "ai_chat_from", args);
    mAIService->setText(tmp_str);
    mAIReply->setText(message);
}
