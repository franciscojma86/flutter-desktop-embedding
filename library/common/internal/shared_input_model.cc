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

Json::Value TextInputModelShared::GetEditingState() {
  Json::Value editing_state;
  editing_state[kComposingBaseKey] = composing_base_;
  editing_state[kComposingExtentKey] = composing_extent_;
  editing_state[kSelectionAffinityKey] = text_affinity_;
  editing_state[kSelectionBaseKey] = selection_base_;
  editing_state[kSelectionExtentKey] = selection_extent_;
  editing_state[kSelectionIsDirectionalKey] = false;
  editing_state[kTextKey] = text_;

  Json::Value args = Json::arrayValue;
  args.append(editing_state);
  return args;
}

void TextInputModelShared::ReplaceString(std::string string, int location = 0,
                                         int length = 0) {
  EraseSelected();
  text_.replace(location, length, string);
  MoveSelectedLocation(location + string.length());
}

void TextInputModelShared::AddString(std::string string) {
  EraseSelected();
  text_.insert(selection_base_, string);
  MoveSelectedLocation(++selection_base_);
  speak();
}

void TextInputModelShared::MoveSelectedLocation(int location) {
  selection_base_ = location;
  selection_extent_ = location;
}

void TextInputModelShared::EraseSelected() {
  if (selection_base_ == selection_extent_) {
    return;
  }
  text_.erase(selection_base_, selection_extent_);
  MoveSelectedLocation(selection_extent_);
}

void TextInputModelShared::BackSpace() {
  EraseSelected();
  if (selection_base_ == 0) {
    return;
  }
  text_.erase(selection_base_ - 1, selection_base_);
  MoveSelectedLocation(--selection_base_);
  speak();
}

void TextInputModelShared::Delete() {
  EraseSelected();
  text_.erase(selection_base_, 1);
  speak();
}

void TextInputModelShared::speak() {
  std::cout << "Speak" << std::endl;
  std::cout << GetEditingState();
  std::cout << std::endl;
  std::cout << "speaking " << std::endl;
}

}  // namespace flutter_desktop_embedding
