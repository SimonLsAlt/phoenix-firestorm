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


constexpr S32 AI_LINE_HT  = 35;
constexpr S32 AI_NOT_USED = -1;

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
    LL_DEBUGS("AIChat") << "in postBuild()" << LL_ENDL;

    LLTabContainer* tab = getChild<LLTabContainer>(UI_AI_BOTS_PANELS);

    // contruct the panels
    FSPanelAIConfiguration* config_panel = new FSPanelAIConfiguration;
    mAIInfoPanels.push_back(config_panel);
    config_panel->buildFromFile("panel_ai_configuration.xml");
    tab->addTabPanel(LLTabContainer::TabPanelParams().panel(config_panel).select_tab(true));

    // Adjust UI after addTabPanel() so that cordinate postions are correct
    config_panel->syncUIWithAISettings();

    FSPanelAIConversation* conversation_panel = new FSPanelAIConversation;
    mAIInfoPanels.push_back(conversation_panel);
    conversation_panel->buildFromFile("panel_ai_conversation.xml");
    tab->addTabPanel(conversation_panel);

    FSPanelAIDirect2LLM* direct_panel = new FSPanelAIDirect2LLM;
    mAIInfoPanels.push_back(direct_panel);
    direct_panel->buildFromFile("panel_ai_direct2llm.xml");
    tab->addTabPanel(direct_panel);

    LL_DEBUGS("AIChat") << "leaving postBuild()" << LL_ENDL;
    return true; // Assuming success for placeholder
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


// -----------------------------------------------------------------------------
// class FSPanelAIConfiguration

FSPanelAIConfiguration::FSPanelAIConfiguration()
{
}


bool FSPanelAIConfiguration::postBuild()
{   // Set up the panel UI after it's constructed

    LL_DEBUGS("AIChat") << "in postBuild()" << LL_ENDL;
    LLComboBox* cb = getChild<LLComboBox>(AI_SERVICE);
    if (cb)
    {
        cb->setCommitCallback(boost::bind(&FSPanelAIConfiguration::onSelectAIService, this));
    }
    cb = getChild<LLComboBox>(AI_LLM_MODE);
    if (cb)
    {
        cb->setCommitCallback(boost::bind(&FSPanelAIConfiguration::onSelectAIMode, this));
    }
    cb = getChild<LLComboBox>(AI_TARGET_LANGUAGE);
    if (cb)
    {
        cb->setCommitCallback(boost::bind(&FSPanelAIConfiguration::onSelectAILanguage, this));
    }

    LLButton * button = getChild<LLButton>(AI_APPLY_BTN);
    if (button)
    {
        button->setCommitCallback(boost::bind(&FSPanelAIConfiguration::onApplyConfig, this));
    }

    // Create data for UI positioning and visibility
    createUILayoutData();

    // Read and set saved values
    syncUIWithAISettings();

    return true;
}

void FSPanelAIConfiguration::saveUIRect(const char* ui_ctrl_name)
{
    LLUICtrl* ui_ctrl = getChild<LLUICtrl>(ui_ctrl_name);
    if (ui_ctrl)
    {
        LLRect start_rect = ui_ctrl->getRect();
        mOriginalRects[ui_ctrl_name] = start_rect;
        LL_DEBUGS("AIChat") << "Saving UI rect " << start_rect << " for " << std::string(ui_ctrl_name) << LL_ENDL;
    }
    else
    {
        LL_WARNS("AIChat") << "Unable to find line editor " << std::string(ui_ctrl_name) << LL_ENDL;
    }
}


//void FSPanelAIConfiguration::shiftUIRect(const char* ui_ctrl_name, S32 x_shift, S32 y_shift)
//{
//    if (x_shift || y_shift)
//    {
//        LLRect old_rect = mOriginalRects[ui_ctrl_name];
//        old_rect.translate(x_shift, y_shift);
//        mOriginalRects[ui_ctrl_name] = old_rect;
//    }
//}



void FSPanelAIConfiguration::fillLayoutData(const char* service,
                                            S32 epp, S32 ep, S32 akp, S32 ak, S32 cip, S32 ci, S32 mp,S32 m)
{   // Helper routine
    service_layout_data_t layout;

    layout[AI_ENDPOINT_PROMPT]     = epp;
    layout[AI_ENDPOINT]            = ep;
    layout[AI_API_KEY_PROMPT]      = akp;
    layout[AI_API_KEY]             = ak;
    layout[AI_CHARACTER_ID_PROMPT] = cip;
    layout[AI_CHARACTER_ID]        = ci;
    layout[AI_MODEL_PROMPT]        = mp;
    layout[AI_MODEL]               = m;

    mUILayoutData[service] = layout;
    LL_DEBUGS("AIChat") << "Saving layout data " << epp << ", "  << ep << ", " << akp << ", " << ak << ", "
                        << cip << ", " << ci << ", " << mp << ", " << m << " for " << std::string(service) << LL_ENDL;
}

void FSPanelAIConfiguration::createUILayoutData()
{   // Create mUILayoutData
    // Values are a Y offset, or -1 for a unused element
    // parameters for:
    //   AI_ENDPOINT_PROMPT, AI_ENDPOINT, AI_API_KEY_PROMPT, AI_API_KEY,
    //   AI_CHARACTER_ID_PROMPT, AI_CHARACTER_ID, AI_MODEL_PROMPT, AI_MODEL

    fillLayoutData(LLM_CONVAI,   0, 0, 0, 0,
                                 0, 0, AI_NOT_USED, AI_NOT_USED);
    fillLayoutData(LLM_GEMINI,   0, 0, 0, 0,
                                 AI_NOT_USED, AI_NOT_USED, AI_LINE_HT, AI_LINE_HT);
    fillLayoutData(LLM_KINDROID, 0, 0, 0, 0,
                                 0, 0, AI_NOT_USED, AI_NOT_USED);
    fillLayoutData(LLM_LMSTUDIO, 0, 0, AI_NOT_USED, AI_NOT_USED,
                                 AI_LINE_HT, AI_LINE_HT, AI_NOT_USED, AI_NOT_USED);
    fillLayoutData(LLM_NOMI,     0, 0, 0, 0,
                                 AI_NOT_USED, AI_NOT_USED, AI_NOT_USED, AI_NOT_USED);
    fillLayoutData(LLM_OLLAMA,   0, 0, AI_NOT_USED, AI_NOT_USED,
                                 AI_NOT_USED, AI_NOT_USED, AI_LINE_HT * 2, AI_LINE_HT * 2);
    fillLayoutData(LLM_OPENAI,   0, 0, 0, 0,
                                 0, 0, AI_NOT_USED, AI_NOT_USED);
}


void FSPanelAIConfiguration::onSelectAIService()
{
    LLComboBox* cb = getChild<LLComboBox>(AI_SERVICE);
    if (cb)
    {
        std::string ai_service_name = cb->getValue().asString();
        FSAIChatMgr::getInstance()->switchAIService(ai_service_name);

        const LLSD& ai_config = FSAIChatMgr::getInstance()->getAIConfig();
        updateUIWidgets(ai_config);
    }
}

void FSPanelAIConfiguration::onSelectAIMode()
{   // Change the mode
    LLComboBox* cb = getChild<LLComboBox>(AI_LLM_MODE);
    if (cb)
    {
        std::string ai_mode = cb->getValue().asString();
        FSAIChatMgr::getInstance()->switchAIMode(ai_mode);

        const LLSD& ai_config = FSAIChatMgr::getInstance()->getAIConfig();
        updateUIWidgets(ai_config);
    }
}

void FSPanelAIConfiguration::onSelectAILanguage()
{   // Change the translation target langauge
    LLComboBox* cb = getChild<LLComboBox>(AI_TARGET_LANGUAGE);
    if (cb)
    {
        std::string langauge = cb->getValue().asString();
        FSAIChatMgr::getInstance()->switchAILanguage(langauge);

        const LLSD& ai_config = FSAIChatMgr::getInstance()->getAIConfig();
        updateUIWidgets(ai_config);
    }
}


void FSPanelAIConfiguration::syncUIWithAISettings()
{
    const LLSD& ai_config = FSAIChatMgr::getInstance()->getAIConfig();

    LL_INFOS("AIChat") << "current config is " << ai_config << LL_ENDL;

    LLCheckBoxCtrl* cbc = getChild<LLCheckBoxCtrl>(AI_FEATURES_ON);
    if (cbc)
    {
        bool ai_on = ai_config.get(AI_FEATURES_ON).asBoolean();
        cbc->setValue(ai_on);
    }

    LLComboBox* cb = getChild<LLComboBox>(AI_SERVICE);
    if (cb)
    {
        const std::string& ai_service_name = ai_config.get(AI_SERVICE).asStringRef();
        cb->setValue(ai_service_name);
    }

    cb = getChild<LLComboBox>(AI_LLM_MODE);
    if (cb)
    {
        const std::string& ai_mode = ai_config.get(AI_LLM_MODE).asStringRef();
        cb->setValue(ai_mode);
    }
    cb = getChild<LLComboBox>(AI_TARGET_LANGUAGE);
    if (cb)
    {
        const std::string& ai_language = ai_config.get(AI_TARGET_LANGUAGE).asStringRef();
        cb->setValue(ai_language);
    }

    updateUIWidgets(ai_config);
}

void FSPanelAIConfiguration::updateUIWidgets(const LLSD& ai_config)
{
    const std::string& ai_service = ai_config.get(AI_SERVICE).asStringRef();
    LL_DEBUGS("AIChat") << "Updating UI for " << ai_service << LL_ENDL;

    const std::string& ai_mode = ai_config.get(AI_LLM_MODE).asStringRef();
    LLComboBox*        cb      = getChild<LLComboBox>(AI_LLM_MODE);
    if (cb)
    {
        cb->setValue(ai_mode);
    }

    cb = getChild<LLComboBox>(AI_TARGET_LANGUAGE);
    if (cb)
    {
        cb->setValue(ai_config.get(AI_TARGET_LANGUAGE).asStringRef());
        if (ai_mode.find(AI_MODE_XLATE) != std::string::npos)
        { // Have a translation mode with "xlate" in the value
            cb->setVisible(true);
        }
        else
        { // Not translation
            cb->setVisible(false);
        }
    }

    updatePromptAndTextEditor(AI_ENDPOINT, AI_ENDPOINT_PROMPT, ai_config, ai_service.c_str());
    updatePromptAndTextEditor(AI_API_KEY, AI_API_KEY_PROMPT, ai_config, ai_service.c_str());
    updatePromptAndTextEditor(AI_CHARACTER_ID, AI_CHARACTER_ID_PROMPT, ai_config, ai_service.c_str());
    updatePromptAndTextEditor(AI_MODEL, AI_MODEL_PROMPT, ai_config, ai_service.c_str());
}

void FSPanelAIConfiguration::updatePromptAndTextEditor(const char* line_editor_name, const char* text_box_name, const LLSD& ai_config,
                                              const char* ai_service)
{   // Hide, show and move UI elements for AI configuration around
    LLLineEditor* le = getChild<LLLineEditor>(line_editor_name);
    LLTextBox*    tb = getChild<LLTextBox>(text_box_name);
    if (le && tb)
    {
        // Save starting rects so positions can be adjusted based on the service
        if (mOriginalRects.empty())
        {
            saveUIRect(AI_ENDPOINT_PROMPT);
            saveUIRect(AI_ENDPOINT);
            saveUIRect(AI_API_KEY_PROMPT);
            saveUIRect(AI_API_KEY);
            saveUIRect(AI_CHARACTER_ID_PROMPT);
            saveUIRect(AI_CHARACTER_ID);
            saveUIRect(AI_MODEL_PROMPT);
            saveUIRect(AI_MODEL);
        }

        S32 y_shift = getUIElementYShift(line_editor_name, ai_service);
        LL_DEBUGS("AIChat") << "UI y shift for " << std::string(ai_service) << ": " << y_shift << LL_ENDL;

        // Reset / adjust Y postion to fill gaps for unused configuration
        bool enabled = (y_shift != AI_NOT_USED);
        if (enabled)
        {   // Ensure prompt and text editor are in the correct positions
            LLRect rex = mOriginalRects[std::string(line_editor_name)];
            rex.translate(0, y_shift);
            le->setRect(rex);

            rex = mOriginalRects[std::string(text_box_name)];
            rex.translate(0, y_shift);
            tb->setRect(rex);
        }

        tb->setVisible(enabled);
        le->setVisible(enabled);
        le->setValue(ai_config.get(line_editor_name));
    }
}


S32 FSPanelAIConfiguration::getUIElementYShift(const char* ui_name, const char* service_name)
{   // return Y shift for the given UI element.  -1 means it should be hidden

    service_layout_data_t& layout = mUILayoutData[std::string(service_name)];
    return layout[std::string(ui_name)];
}


void FSPanelAIConfiguration::onApplyConfig()
{
    // Callback for pressing "Apply" button
    LL_INFOS("AIChat") << "Config Apply button pressed" << LL_ENDL;

    LLSD        ai_config      = LLSD::emptyMap();

    // First save on/off switch
    LLCheckBoxCtrl* cbc = getChild<LLCheckBoxCtrl>(AI_FEATURES_ON);
    if (cbc)
    {
        ai_config[AI_FEATURES_ON] = cbc->getValue().asBoolean();
    }

    LLComboBox* cb = getChild<LLComboBox>(AI_SERVICE);
    if (cb)
    {
        ai_config[AI_SERVICE] = cb->getValue().asString();
    }

    // Save values that current service cares about
    addInLineEditorValue(AI_ENDPOINT, ai_config);
    addInLineEditorValue(AI_API_KEY, ai_config);
    addInLineEditorValue(AI_CHARACTER_ID, ai_config);
    addInLineEditorValue(AI_MODEL, ai_config);

    FSAIChatMgr::getInstance()->setAIConfigValues(ai_config);
}


void FSPanelAIConfiguration::addInLineEditorValue(const std::string& ui_name, LLSD& ai_config) const
{  // If needed, get a string from a line editor and add to the config
    LLLineEditor* le = nullptr;
    if (useConfigValue(ui_name, ai_config[AI_SERVICE].asStringRef()))
    {
        le = getChild<LLLineEditor>(ui_name);
        if (le)
        {
            ai_config[ui_name] = le->getValue().asString();
        }
    }
}

bool FSPanelAIConfiguration::useConfigValue(const std::string& config_name, const std::string& service_name) const
{   // return true if the give configuration name is used for the service   To do - just put this into a static map or something
    if (service_name == LLM_CONVAI)
    {
        return (config_name == AI_ENDPOINT || config_name == AI_API_KEY || config_name == AI_CHARACTER_ID);
    }
    if (service_name == LLM_GEMINI)
    {
        return (config_name == AI_ENDPOINT || config_name == AI_API_KEY || config_name == AI_MODEL);
    }
    if (service_name == LLM_KINDROID)
    {
        return (config_name == AI_ENDPOINT || config_name == AI_API_KEY || config_name == AI_CHARACTER_ID);
    }
    if (service_name == LLM_LMSTUDIO)
    {
        return (config_name == AI_ENDPOINT || config_name == AI_CHARACTER_ID);
    }
    if (service_name == LLM_NOMI)
    { // Character ID is embedded in the service url so doesn't need AI_CHARACTER_ID
        return (config_name == AI_ENDPOINT || config_name == AI_API_KEY);
    }
    if (service_name == LLM_OLLAMA)
    {
        return (config_name == AI_ENDPOINT || config_name == AI_MODEL);
    }
    if (service_name == LLM_OPENAI)
    {   // Needs all 3 basics
        return (config_name == AI_ENDPOINT || config_name == AI_API_KEY || config_name == AI_CHARACTER_ID);
    }

    LL_WARNS("AIChat") << "Unknown LLM " << service_name << " in useConfigValue()" << LL_ENDL;
    return true;
}




// -----------------------------------------------------------------------------
// class FSPanelAIConversation

FSPanelAIConversation::FSPanelAIConversation()
{
    // Constructor implementation here
    // Initialize any members or set up necessary configurations
}

bool FSPanelAIConversation::postBuild()
{   // Set up the panel UI after it's constructed
    LL_DEBUGS("AIChat") << "in postBuild()" << LL_ENDL;

    const ai_chat_history_t& chat_history = FSAIChatMgr::getInstance()->getAIChatHistory();

    LLButton* button = getChild<LLButton>(UI_AI_SEND_BTN);
    if (button)
    {
        button->setCommitCallback(boost::bind(&FSPanelAIConversation::onSendMsgToAgent, this));
    }
    button = getChild<LLButton>(UI_AI_RESET_CHAT_BTN);
    if (button)
    {
        button->setCommitCallback(boost::bind(&FSPanelAIConversation::onResetChat, this));
    }

    LLTextEditor* te = getChild<LLTextEditor>(UI_AI_CHAT_EDITOR);
    LLTextBox*    tb = getChild<LLTextBox>(UI_AI_CHAT_MESSAGES);
    if (te && tb)
    {   // Display any last messages from history
        std::string user_chat(AI_HISTORY_USER);
        std::string ai_chat(AI_HISTORY_ASSISTANT);
        for (auto it = chat_history.rbegin(); it != chat_history.rend(); ++it)
        {
            if (it->second == ai_chat)
            {
                te->appendText(it->first, true /* newline */);
            }
            else if (it->second == user_chat)
            {
                tb->appendText(it->first, true /* newline */);
            }
        }
    }

    const LLSD& ai_config = FSAIChatMgr::getInstance()->getAIConfig();
    setAIReplyMessagePrompt(ai_config.get(AI_SERVICE));

    return true;
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

void FSPanelAIConversation::onSendMsgToAgent()
{  // Callback for pressing "Send Message" button
    LL_INFOS("AIChat") << "Conversation Send Message button pressed" << LL_ENDL;

    // tbd - disable button if edit/approve checkbox is off?
    //     - disable / prevent sending duplicates?

    // Get message text and send it via mgr.
    LLTextEditor* te = getChild<LLTextEditor>(UI_AI_CHAT_EDITOR);
    if (te)
    {
        const std::string& message = te->getValue().asStringRef();
        // to do - send IM
    }
}


void FSPanelAIConversation::onResetChat()
{  // Callback for pressing "Reset Chat" button
    LL_INFOS("AIChat") << "Conversation Reset Chat button pressed" << LL_ENDL;
    FSAIChatMgr::getInstance()->resetChat();
}

void FSPanelAIConversation::resetChat()  // override;
{
    LLTextEditor* te = getChild<LLTextEditor>(UI_AI_CHAT_EDITOR);
    if (te)
    {
        te->setText(std::string());
    }

    LLTextBox* tb = getChild<LLTextBox>(UI_AI_CHAT_MESSAGES);
    if (tb)
    {
        tb->setText(std::string());
    }
}



// -----------------------------------------------------------------------------
// class FSPanelAIDirect2LLM

FSPanelAIDirect2LLM::FSPanelAIDirect2LLM()
{
    // Initialize any members or set up necessary configurations
}

bool FSPanelAIDirect2LLM::postBuild()
{   // Set up the panel UI after it's constructed
    LL_DEBUGS("AIChat") << "in postBuild()" << LL_ENDL;

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

    return true;
}


void FSPanelAIDirect2LLM::onSendMsgToLLM()
{   // Callback for pressing "Send Message" button
    LL_INFOS("AIChat") << "Direct2LLM Send Message button pressed" << LL_ENDL;

    // Get message text and send it via mgr.
    LLTextEditor* te = getChild<LLTextEditor>(UI_AI_CHAT_EDITOR);
    if (te)
    {
        const std::string & message   = te->getValue().asStringRef();
        FSAIChatMgr::getInstance()->sendChatToAIService(message, true);
    }
}



static void removeFirstLine(std::string & messages)
{   // thanks chatGPT!
    // Find the position of the first newline character
    size_t newlinePos = messages.find('\n');

    // If a newline is found, erase up to and including it
    if (newlinePos != std::string::npos)
    {
        messages.erase(0, newlinePos + 1);
    }
    else
    {   // If no newline is found, clear the string (there's only one line)
        messages.clear();
    }
}


static constexpr S32 MAX_AI_RESPONSE_BUFFER_SIZE = 5000;
    // Handle chat response from AI
void FSPanelAIDirect2LLM::displayIncomingAIResponse(const std::string& ai_message)
{
    LLTextBox* tb = getChild<LLTextBox>(UI_AI_CHAT_MESSAGES);
    if (tb)
    {
        // Add and display reply from AI
        std::string messages = tb->getValue().asString();
        while (messages.size() > MAX_AI_RESPONSE_BUFFER_SIZE)
        {   // Don't let it grow forever
            removeFirstLine(messages);
        }
        messages += LL_NEWLINE;
        messages.append(ai_message);
        tb->setText(messages);
    }
    else
    {
        LL_WARNS("AIChat") << "Unexpected null ai reply textbox, can't show AI chat reply" << LL_ENDL;
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

void FSPanelAIDirect2LLM::resetChat()  // override;
{
    LLTextEditor* te = getChild<LLTextEditor>(UI_AI_CHAT_EDITOR);
    if (te)
    {
        te->setText(std::string());
    }

    LLTextBox* tb = getChild<LLTextBox>(UI_AI_CHAT_MESSAGES);
    if (tb)
    {
        tb->setText(std::string());
    }
}
