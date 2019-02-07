// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "library/common/internal/shared_input_model.h"

#include <iostream>
#include <sstream>

// Client config  keys.
static constexpr char kTextInputAction[] = "inputAction";
static constexpr char kTextInputType[] = "inputType";
static constexpr char kTextInputTypeName[] = "name";

// Text affinity options keys.
static constexpr char kTextAffinityDownstream[] = "TextAffinity.downstream";
static constexpr char kTextAffinityUpstream[] = "TextAffinity.upstream";

// State keys.
static constexpr char kComposingBaseKey[] = "composingBase";
static constexpr char kComposingExtentKey[] = "composingExtent";
static constexpr char kSelectionBaseKey[] = "selectionBase";
static constexpr char kSelectionExtentKey[] = "selectionExtent";
static constexpr char kSelectionAffinityKey[] = "selectionAffinity";
static constexpr char kSelectionIsDirectionalKey[] = "selectionIsDirectional";
static constexpr char kTextKey[] = "text";

static constexpr char kMultilineInputType[] = "TextInputType.multiline";

namespace flutter_desktop_embedding {

TextInputModelShared::TextInputModelShared(const Json::Value &config)
    : text_(""), text_affinity_(kTextAffinityUpstream) {
  // Inspect the config arguments. There are a number of arguments receiveed
  // here. Add as needed. If not configured properly, the class should throw.
  std::string input_action = config[kTextInputAction].asString();
  Json::Value input_type_info = config[kTextInputType];
  std::string input_type = input_type_info[kTextInputTypeName].asString();
  if (input_action.empty() || input_type.empty()) {
    throw std::invalid_argument("Missing arguments input_action or input_type");
  }
  input_type_ = input_type;
  input_action_ = input_action;
}
TextInputModelShared::~TextInputModelShared() {}

bool TextInputModelShared::SetEditingState(const Json::Value &state) {
  std::cout << state << std::endl;
  Json::Value text = state[kTextKey];
  if (text.isNull()) {
    std::cerr << "Set editing state has been invoked, but without text."
              << std::endl;
    return false;
  }
  text_ = text.asString();

  Json::Value selection_base = state[kSelectionBaseKey];
  Json::Value selection_extent = state[kSelectionExtentKey];
  if (selection_base.isNull() || selection_extent.isNull()) {
    std::cerr << "Selection base/extent values invalid." << std::endl;
    return false;
  }
  selection_base_ = selection_base.asInt();
  selection_extent_ = selection_extent.asInt();

  Json::Value composing_base = state[kComposingBaseKey];
  Json::Value composing_extent = state[kComposingExtentKey];
  composing_base_ =
      composing_base.isNull() ? selection_base_ : composing_base.asInt();
  composing_extent_ =
      composing_extent.isNull() ? selection_extent_ : composing_extent.asInt();

  Json::Value text_affinity = state[kSelectionAffinityKey];
  text_affinity_ = text_affinity.isNull() ? kTextAffinityDownstream
                                          : text_affinity.asString();

  return true;
}

Json::Value TextInputModelShared::GetEditingState() const {
  Json::Value editing_state;
  editing_state[kComposingBaseKey] = composing_base_;
  editing_state[kComposingExtentKey] = composing_extent_;
  editing_state[kSelectionAffinityKey] = text_affinity_;
  editing_state[kSelectionBaseKey] = selection_base_;
  editing_state[kSelectionExtentKey] = selection_extent_;
  editing_state[kSelectionIsDirectionalKey] = false;
  editing_state[kTextKey] = text_;

  return editing_state;
}

void TextInputModelShared::ReplaceString(std::string string, int location = 0,
                                         int length = 0) {
  EraseSelected();
  text_.replace(location, length, string);
  MoveCursorToLocation(location + string.length());
}
void TextInputModelShared::AddCharacter(char c) {
  std::string s(1, c);
  AddString(s);
}

void TextInputModelShared::AddString(std::string string) {
  EraseSelected();
  text_.insert(selection_base_, string);
  MoveCursorToLocation(++selection_base_);
  speak();
}

bool TextInputModelShared::MoveCursorToLocation(int location) {
  if (location == selection_base_ && location == selection_extent_) {
    return false;
  }
  selection_base_ = location;
  selection_extent_ = location;
  return true;
}

bool TextInputModelShared::EraseSelected() {
  if (selection_base_ == selection_extent_) {
    return false;
  }
  text_.erase(selection_base_, selection_extent_);
  MoveCursorToLocation(selection_extent_);
  return true;
}

bool TextInputModelShared::Backspace() {
  if (EraseSelected()) {
    return true;
  }
  if (selection_base_ == 0) {
    return false;
  }
  text_.erase(selection_base_ - 1, 1);
  MoveCursorToLocation(--selection_base_);
  speak();

  return true;
}

bool TextInputModelShared::Delete() {
  if (EraseSelected()) {
    return true;
  }
  if (selection_base_ == static_cast<int>(text_.length())) {
    return false;
  }
  text_.erase(selection_base_, 1);
  speak();
  return true;
}

bool TextInputModelShared::MoveCursorToBeginning() {
  if (selection_base_ == 0) {
    return false;
  }
  MoveCursorToLocation(0);
  speak();
  return true;
}

bool TextInputModelShared::MoveCursorToEnd() {
  if (selection_base_ == static_cast<int>(text_.length())) {
    return false;
  }
  MoveCursorToLocation(text_.length());
  speak();
  return true;
}

bool TextInputModelShared::InsertNewLine() {
  if (input_type_ != kMultilineInputType) {
    return false;
  }
  AddCharacter('\n');
  return true;
}

bool TextInputModelShared::MoveCursorForward() {
  if (selection_base_ == static_cast<int>(text_.length())) {
    return false;;
  }
  MoveCursorToLocation(++selection_base_);
  speak();
  return true;
}

bool TextInputModelShared::MoveCursorBack() {
  if (selection_base_ == 0) {
    return false;
  }
  MoveCursorToLocation(--selection_base_);
  speak();
  return true;
}

std::string TextInputModelShared::input_action() {
  return input_action_;
}
void TextInputModelShared::speak() {
  std::cout << "Speak" << std::endl;
  std::cout << GetEditingState();
  std::cout << std::endl;
  std::cout << "speaking " << std::endl;
}

}  // namespace flutter_desktop_embedding
