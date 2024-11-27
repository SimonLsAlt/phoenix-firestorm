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
#include "fsaichatstrings.h"

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
    LLTabContainer* tab = getChild<LLTabContainer>(UI_AI_BOTS_PANELS);

    // contruct the panels
    FSPanelAIInfo* panel = new FSPanelAIConfiguration;
    mAIInfoPanels.push_back(panel);
    panel->buildFromFile("panel_ai_configuration.xml");
    tab->addTabPanel(LLTabContainer::TabPanelParams().panel(panel).select_tab(true));

    panel = new FSPanelAIConversation;
    mAIInfoPanels.push_back(panel);
    panel->buildFromFile("panel_ai_conversation.xml");
    tab->addTabPanel(panel);

    panel = new FSPanelAIDirect2LLM;
    mAIInfoPanels.push_back(panel);
    panel->buildFromFile("panel_ai_direct2llm.xml");
    tab->addTabPanel(panel);

    return true;  // Assuming success for placeholder
}

FSPanelAIConfiguration* FSFloaterAIChat::getPanelConfiguration()
{
    if (mAIInfoPanels.size() > 0)
        return dynamic_cast<FSPanelAIConfiguration*>(mAIInfoPanels[0]);
    return nullptr;
}

FSPanelAIConversation* FSFloaterAIChat::getPanelConversation()
{
    if (mAIInfoPanels.size() > 1)
        return dynamic_cast<FSPanelAIConversation*>(mAIInfoPanels[1]);
    return nullptr;
}

FSPanelAIDirect2LLM* FSFloaterAIChat::getPanelDirect2LLM()
{
    if (mAIInfoPanels.size() > 2)
        return dynamic_cast<FSPanelAIDirect2LLM*>(mAIInfoPanels[2]);
    return nullptr;
}

FSFloaterAIChat::FSFloaterAIChat(const LLSD& seed) : LLFloater(seed)
{
    // Placeholder for constructor logic
}

FSFloaterAIChat::~FSFloaterAIChat()
{
    // Placeholder for destructor logic
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

void FSPanelAIInfo::updateUIElement(const std::string& lineEditorName, const std::string& textBoxName,
                                    const LLSD& ai_config, const std::string & ai_service)
{
    LLLineEditor* le = getChild<LLLineEditor>(lineEditorName);
    LLTextBox*    tb = getChild<LLTextBox>(textBoxName);
    if (le && tb)
    {
        le->setValue(ai_config.get(lineEditorName));
        bool enabled = enableUIElement(lineEditorName, ai_service);
        tb->setVisible(enabled);
        le->setVisible(enabled);
    }
}




// -----------------------------------------------------------------------------
// class FSPanelAIConfiguration

FSPanelAIConfiguration::FSPanelAIConfiguration()
{
}


bool FSPanelAIConfiguration::postBuild()
{
    // Placeholder implementation for post-build actions
    // Code here should set up the panel UI after it's constructed

    LLComboBox* cb = getChild<LLComboBox>(AI_SERVICE);
    if (cb)
    {
        cb->setCommitCallback(boost::bind(&FSPanelAIConfiguration::onSelectAIService, this));
    }

    LLButton * button = getChild<LLButton>(AI_APPLY_BTN);
    if (button)
    {
        button->setCommitCallback(boost::bind(&FSPanelAIConfiguration::onApplyConfig, this));
    }

    // Read and set saved values
    syncUIWithAISettings();

    return FSPanelAIInfo::postBuild();
}

void FSPanelAIConfiguration::onSelectAIService()
{
    LLComboBox* cb = getChild<LLComboBox>(AI_SERVICE);
    if (cb)
    {
        std::string ai_service_name = cb->getValue().asString();
        LL_INFOS("AIChat") << "Selected AI service " << ai_service_name << LL_ENDL;

        LLSD ai_config = LLSD::emptyMap();
        ai_config[AI_SERVICE] = ai_service_name;
        FSAIChatMgr::getInstance()->setAIConfig(ai_config);

        updateUIElement(AI_ENDPOINT, AI_ENDPOINT_PROMPT, ai_config, ai_service_name);
        updateUIElement(AI_API_KEY, AI_API_KEY_PROMPT, ai_config, ai_service_name);
        updateUIElement(AI_CHARACTER_ID, AI_CHARACTER_ID_PROMPT, ai_config, ai_service_name);
    }
}

void FSPanelAIConfiguration::syncUIWithAISettings()
{
    const LLSD& ai_config = FSAIChatMgr::getInstance()->getAIConfig();

    LL_INFOS("AIChat") << "current config is " << ai_config << LL_ENDL;

    LLCheckBoxCtrl* cbc = getChild<LLCheckBoxCtrl>(AI_CHAT_ON);
    if (cbc)
    {
        cbc->setValue(ai_config.get(AI_CHAT_ON));
    }

    const std::string& ai_service_name = ai_config.get(AI_SERVICE).asStringRef();
    LLComboBox* cb = getChild<LLComboBox>(AI_SERVICE);
    if (cb)
    {
        cb->setValue(ai_service_name);
    }
    updateUIElement(AI_ENDPOINT, AI_ENDPOINT_PROMPT, ai_config, ai_service_name);
    updateUIElement(AI_API_KEY, AI_API_KEY_PROMPT, ai_config, ai_service_name);
    updateUIElement(AI_CHARACTER_ID, AI_CHARACTER_ID_PROMPT, ai_config, ai_service_name);
}


bool FSPanelAIConfiguration::enableUIElement(const std::string& ui_name, const std::string& service_name) const
{   // This isn't elegant, but works
    if (service_name == LLM_KINDROID)
    {
    }
    else if (service_name == LLM_LMSTUDIO)
    {
    }
    else if (service_name == LLM_OPENAI)
    {
    }
    else if (service_name == LLM_NOMI)
    {   // Character ID is embedded in the service url
        return (ui_name != AI_CHARACTER_ID && ui_name != AI_CHARACTER_ID_PROMPT);
    }
    return true;
}


void FSPanelAIConfiguration::onApplyConfig()
{
    // Callback for pressing "Apply" button
    LL_INFOS("AIChat") << "Config Apply button pressed" << LL_ENDL;

    LLSD        ai_config      = LLSD::emptyMap();

    LLCheckBoxCtrl* cbc = getChild<LLCheckBoxCtrl>(AI_CHAT_ON);
    if (cbc)
    {
        ai_config[AI_CHAT_ON] = cbc->getValue().asBoolean();
    }

    LLComboBox* cb = getChild<LLComboBox>(AI_SERVICE);
    if (cb)
    {
        ai_config[AI_SERVICE] = cb->getValue().asString();
    }

    LLLineEditor* le = getChild<LLLineEditor>(AI_ENDPOINT);
    if (le)
    {
        std::string endpoint  = le->getValue().asString();
        ai_config[AI_ENDPOINT] = endpoint;
    }
    le = getChild<LLLineEditor>(AI_API_KEY);
    if (le)
    {
        ai_config[AI_API_KEY] = le->getValue().asString();
    }
    //le = getChild<LLLineEditor>(AI_API_KEY);
    //if (le)   // character key is embedded in the service url
    //{
    //    ai_config[AI_CHARACTER_ID] = le->getValue().asString();
    //}

    FSAIChatMgr::getInstance()->setAIConfig(ai_config);
}



// -----------------------------------------------------------------------------
// class FSPanelAIConversation

FSPanelAIConversation::FSPanelAIConversation()
{
    // Constructor implementation here
    // Initialize any members or set up necessary configurations
}

bool FSPanelAIConversation::postBuild()
{
    // Placeholder implementation for post-build actions
    // Code here should set up the panel UI after it's constructed
    const ai_chat_history_t& chat_history = FSAIChatMgr::getInstance()->getAIChatHistory();

    LLTextEditor* te = getChild<LLTextEditor>(UI_AI_CHAT_EDITOR);
    LLTextBox*    tb = getChild<LLTextBox>(UI_AI_CHAT_MESSAGES);
    if (te && tb)
    {   // Display any last messages from history
        for (auto it = chat_history.rbegin(); it != chat_history.rend(); ++it)
        {
            if (LLStringUtil::startsWith(*it, "AI:") && te->getValue().asStringRef().empty())
            {
                te->setText((*it).substr(3));
                if (!tb->getValue().asStringRef().empty())
                    break;
            }
            else if (LLStringUtil::startsWith(*it, "SL:") && te->getValue().asStringRef().empty())
            {
                tb->setText((*it).substr(3));
                if (!te->getValue().asStringRef().empty())
                    break;
            }
        }
    }

    const LLSD& ai_config = FSAIChatMgr::getInstance()->getAIConfig();
    setAIReplyMessagePrompt(ai_config.get(AI_SERVICE));

    return FSPanelAIInfo::postBuild();
}

void FSPanelAIConversation::processIncomingChat(const std::string& name, const std::string& message)
{  // Handle chat from other user.  Caller must check "chat on" switch
    std::string                last_from_str;
    LLStringUtil::format_map_t args;
    args["[OTHER_AV]"] = name;
    LLTrans::findString(last_from_str, UI_AI_CHAT_WITH, args);  // "Last message from [OTHER_AV]:" from strings.xml

    LLTextBox* tb = getChild<LLTextBox>(UI_AI_CHAT_WITH);
    if (tb && last_from_str != tb->getValue().asStringRef())
    {
        tb->setText(last_from_str);
    }
    tb = getChild<LLTextBox>(UI_AI_CHAT_MESSAGES);
    if (tb)
    {   // Show the last message from other agent and sent to AI service
        tb->setText(message);
    }
}


// Handle chat response from AI
void FSPanelAIConversation::displayIncomingAIResponse(const std::string& ai_message)
{
    LLTextEditor* te = getChild<LLTextEditor>(UI_AI_CHAT_EDITOR);
    if (te)
    {
        // Set reply from AI
        te->setText(ai_message);
    }
    else
    {
        LL_WARNS("AIChat") << "Unexpected null child reply editor, can't show AI chat reply" << LL_ENDL;
    }
}


void FSPanelAIConversation::setAIReplyMessagePrompt(const std::string& service_name)
{
    // "Last message generated by [AI_SERVICE_NAME]:" text
    LLTextBox* tb = getChild<LLTextBox>(UI_AI_CHAT_FROM);
    if (tb)
    {
        // Set the "chat from..." message
        std::string                tmp_str;
        LLStringUtil::format_map_t args;
        args["[AI_SERVICE_NAME]"] = service_name;
        LLTrans::findString(tmp_str, UI_AI_CHAT_FROM, args);
        tb->setText(tmp_str);
    }
    else
    {
        LL_WARNS("AIChat") << "Unexpected empty AI service, can't process AI chat reply" << LL_ENDL;
    }
}




// -----------------------------------------------------------------------------
// class FSPanelAIDirect2LLM

FSPanelAIDirect2LLM::FSPanelAIDirect2LLM()
{
    // Initialize any members or set up necessary configurations
}

bool FSPanelAIDirect2LLM::postBuild()
{
    // Placeholder implementation for post-build actions
    // Code here should set up the panel UI after it's constructed
    LLButton* button = getChild<LLButton>(UI_AI_SEND_BTN);
    if (button)
    {
        button->setCommitCallback(boost::bind(&FSPanelAIDirect2LLM::onSendMsgToLLM, this));
    }


    LLTextEditor* te = getChild<LLTextEditor>(UI_AI_CHAT_EDITOR);
    LLTextBox*    tb = getChild<LLTextBox>(UI_AI_CHAT_MESSAGES);
    if (te && tb)
    {  // Display any last messages from history
        te->setText(std::string());
        tb->setText(std::string());
    }

    const LLSD& ai_config = FSAIChatMgr::getInstance()->getAIConfig();
    setAIServiceNamePrompts(ai_config.get(AI_SERVICE));

    return FSPanelAIInfo::postBuild();
}


void FSPanelAIDirect2LLM::onSendMsgToLLM()
{   // Callback for pressing "Send Message" button
    LL_INFOS("AIChat") << "Direct2LLM Send Message button pressed" << LL_ENDL;

    // Get message xtext and send it via mgr.
    LLTextEditor* te = getChild<LLTextEditor>(UI_AI_CHAT_EDITOR);
    if (te)
    {
        const std::string & message   = te->getValue().asStringRef();
        FSAIChatMgr::getInstance()->sendChatToAIService(message, true);
    }
}



// Handle chat response from AI
void FSPanelAIDirect2LLM::displayIncomingAIResponse(const std::string& ai_message)
{
    LLTextEditor* te = getChild<LLTextEditor>(UI_AI_CHAT_EDITOR);
    if (te)
    {
        // Set reply from AI
        te->setText(ai_message);
    }
    else
    {
        LL_WARNS("AIChat") << "Unexpected null child reply editor, can't show AI chat reply" << LL_ENDL;
    }
}

void FSPanelAIDirect2LLM::setAIServiceNamePrompts(const std::string& service_name)
{
    std::string tmp_str;
    LLStringUtil::format_map_t args;
    args["[AI_SERVICE_NAME]"] = service_name;

    // "Direct conversation with [AI_SERVICE_NAME]:" text
    LLTextBox* tb = getChild<LLTextBox>(UI_AI_DIRECT_LLM_CHAT_WITH);
    if (tb)
    {
        // Set the "chat from..." message
        LLTrans::findString(tmp_str, UI_AI_DIRECT_LLM_CHAT_WITH, args);
        tb->setText(tmp_str);
    }
    else
    {
        LL_WARNS("AIChat") << "Unexpected empty UI_AI_DIRECT_LLM_CHAT_WITH prompt, can't set display" << LL_ENDL;
    }

    // Set "Send message to [AI_SERVICE_NAME]"
    tb = getChild<LLTextBox>(UI_AI_SEND_DIRECT_TO);
    if (tb)
    {
        // Set the "chat from..." message
        LLTrans::findString(tmp_str, UI_AI_SEND_DIRECT_TO, args);
        tb->setText(tmp_str);
    }
    else
    {
        LL_WARNS("AIChat") << "Unexpected empty UI_AI_SEND_DIRECT_TO prompt, can't set display" << LL_ENDL;
    }
}
