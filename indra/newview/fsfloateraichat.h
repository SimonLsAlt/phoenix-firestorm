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

class LLDispatcher;
class LLLineEditor;
class LLTabContainer;
class LLViewerRegion;
class LLViewerTextEditor;
class LLCheckBoxCtrl;
class LLTextBox;
class LLEventTimer;

class LLPanelAIConfiguration;
class LLPanelAIConversation;
class LLPanelAIInfo;


class LLFloaterAIChat : public LLFloater
{
public:

    LLFloaterAIChat(const LLSD& seed);
    ~LLFloaterAIChat();


    void onOpen(const LLSD& key) override;
    void onClose(bool app_quitting) override;
    bool postBuild() override;

    static LLPanelAIConfiguration * getPanelConfiguration();
    static LLPanelAIConversation  * getPanelConversation();

    // from LLPanel
    void refresh() override;

protected:
    void onTabSelected(const LLSD& param);
    void disableTabCtrls();
    void refreshFromRegion(LLViewerRegion* region);
    void onGodLevelChange(U8 god_level);

    // member data
    LLTabContainer* mTab;
    typedef std::vector<LLPanelAIInfo*> ai_info_panels_t;
    ai_info_panels_t                    mAIInfoPanels;

private:
};


// Base class for all AI information panels.
class LLPanelAIInfo : public LLPanel
{
public:
    LLPanelAIInfo();

    void onBtnSet();
    void onChangeChildCtrl(LLUICtrl* ctrl);
    void onChangeAnything();
    static void onChangeText(LLLineEditor* caller, void* user_data);


    bool postBuild() override;
    virtual void updateChild(LLUICtrl* child_ctrl);

    void enableButton(const std::string& btn_name, bool enable = true);
    void disableButton(const std::string& btn_name);

protected:
    void initCtrl(const std::string& name);
    template<typename CTRL> void initAndSetCtrl(CTRL*& ctrl, const std::string& name);
};

/////////////////////////////////////////////////////////////////////////////
// Actual panels start here
/////////////////////////////////////////////////////////////////////////////


class LLPanelAIConfiguration : public LLPanelAIInfo
{
public:
    static void initDispatch(LLDispatcher& dispatch);

    LLPanelAIConfiguration();
    ~LLPanelAIConfiguration() {}

    void updateControls(LLViewerRegion* region);

    bool postBuild() override;
    void updateChild(LLUICtrl* child_ctrl) override;
    void refresh() override;

protected:
};


/////////////////////////////////////////////////////////////////////////////

class LLPanelAIConversation : public LLPanelAIInfo
{
  public:
    static void initDispatch(LLDispatcher& dispatch);

    LLPanelAIConversation();
    ~LLPanelAIConversation() {}

    void updateControls(LLViewerRegion* region);

    bool postBuild() override;
    void updateChild(LLUICtrl* child_ctrl) override;
    void refresh() override;

  protected:
};


/////////////////////////////////////////////////////////////////////////////

#endif
