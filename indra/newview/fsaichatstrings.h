/**
 * @file fsaichatstrings.h
 * @author simonlsalt
 * @brief Strings for AI chat
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

/*
* This is experimental code to integrate external chat systems to the SL / FS 
* chat experience.
* 
* This module contains the main chat processing, while fsfloateraichat.h/.cpp
* create the tabbed window that handles the UI for configuration and interacting
* with the chat process.
*/

#ifndef FS_FSAICHATSTRINGS_H
#define FS_FSAICHATSTRINGS_H


#include "stdtypes.h"

// Elements in configuration tab.  Some values are also used in settings keys
inline constexpr char AI_CHAT_ON[]   = "chat_on";
inline constexpr char AI_APPLY_BTN[] = "apply_btn";
inline constexpr char AI_SERVICE[]   = "service";

// Names of supported AI back-ends, must match up with panel_ai_configuration.xml combo box values
inline constexpr char LLM_CONVAI[]   = "Convai";
inline constexpr char LLM_GEMINI[]   = "Gemini";
inline constexpr char LLM_KINDROID[] = "Kindroid.ai";
inline constexpr char LLM_LMSTUDIO[] = "LMStudio";
inline constexpr char LLM_OPENAI[]   = "OpenAI";
inline constexpr char LLM_NOMI[]     = "Nomi.ai";

inline constexpr char AI_ENDPOINT_PROMPT[] = "endpoint_prompt";
inline constexpr char AI_ENDPOINT[] = "endpoint";
inline constexpr char AI_API_KEY_PROMPT[] = "api_key_prompt";
inline constexpr char AI_API_KEY[] = "api_key";
inline constexpr char AI_CHARACTER_ID_PROMPT[] = "character_id_prompt";
inline constexpr char AI_CHARACTER_ID[] = "character_id";

// Elements in the conversation tab
inline constexpr char UI_AI_CHAT_WITH[]      = "ai_chat_with";
inline constexpr char UI_AI_CHAT_FROM[]      = "ai_chat_from";
inline constexpr char UI_AI_CHAT_MESSAGES[]  = "ai_chat_messages";
inline constexpr char UI_AI_CHAT_EDITOR[]    = "ai_chat_editor";
inline constexpr char UI_AI_RESET_CHAT_BTN[] = "ai_reset_chat_btn";

// Elements in the direct2LLM tab
inline constexpr char UI_AI_DIRECT_LLM_CHAT_WITH[] = "ai_direct_llm_chat_with";
inline constexpr char UI_AI_SEND_DIRECT_TO[]       = "ai_send_direct_to";
inline constexpr char UI_AI_SEND_BTN[]             = "ai_send_btn";
// also uses UI_AI_CHAT_EDITOR and UI_AI_CHAT_MESSAGES

inline constexpr char UI_AI_BOTS_PANELS[] = "ai_bots_panels";

#endif