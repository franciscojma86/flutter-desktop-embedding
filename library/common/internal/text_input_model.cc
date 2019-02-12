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
#include "library/common/internal/text_input_model.h"

#include <iostream>

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

// InputType keys.
// https://docs.flutter.io/flutter/services/TextInputType-class.html
static constexpr char kMultilineInputType[] = "TextInputType.multiline";

static constexpr char kLineBreakKey = '\n';

namespace flutter_desktop_embedding {

Model TextInputModel::GetModel() const { return model_; }

void TextInputModel::ReplaceModel(Model model) { model_ = model; }

bool LocationIsAtEnd(int location, std::string text) {
  return (location == static_cast<int>(text.length()));
}

bool LocationIsAtBeginning(int location) { return (location == 0); }

TextInputModel::TextInputModel(const Json::Value &config) {
  // Use/inspect more arguments as needed. input_type and input_action should
  // always be present.
  std::string input_action = config[kTextInputAction].asString();
  Json::Value input_type_info = config[kTextInputType];
  std::string input_type = input_type_info[kTextInputTypeName].asString();
  if (input_action.empty() || input_type.empty()) {
    throw std::invalid_argument("Missing arguments input_action or input_type");
  }
  input_type_ = input_type;
  input_action_ = input_action;
}

TextInputModel::TextInputModel(std::string input_type, std::string input_action)
    : input_type_(input_type), input_action_(input_action) {}

TextInputModel::~TextInputModel() {}

Json::Value TextInputModel::GetEditingState() const {
  Json::Value editing_state;
  editing_state[kComposingBaseKey] = model_.composing_base_;
  editing_state[kComposingExtentKey] = model_.composing_extent_;
  editing_state[kSelectionAffinityKey] =
      model_.text_affinity_.compare(kTextAffinityUpstream) == 0
          ? kTextAffinityUpstream
          : kTextAffinityDownstream;
  editing_state[kSelectionBaseKey] = model_.selection_base_;
  editing_state[kSelectionExtentKey] = model_.selection_extent_;
  editing_state[kSelectionIsDirectionalKey] = false;
  editing_state[kTextKey] = model_.text_;

  return editing_state;
}

void TextInputModel::ReplaceString(std::string string, int location = 0,
                                   int length = 0) {
  EraseSelected();
  model_.text_.replace(location, length, string);
  MoveCursorToLocation(location + string.length());
}
void TextInputModel::AddCharacter(char c) {
  std::string s(1, c);
  AddString(s);
}

void TextInputModel::AddString(std::string string) {
  EraseSelected();
  model_.text_.insert(model_.selection_base_, string);
  MoveCursorToLocation(++model_.selection_base_);
}

bool TextInputModel::EraseSelected() {
  if (model_.selection_base_ == model_.selection_extent_) {
    return false;
  }
  int base = std::min(model_.selection_base_, model_.selection_extent_);
  int extent = std::max(model_.selection_base_, model_.selection_extent_);
  model_.text_.erase(base, extent);
  MoveCursorToLocation(base);
  return true;
}

bool TextInputModel::Backspace() {
  // If a selection was deleted, don't delete more characters.
  if (EraseSelected()) {
    return true;
  }
  if (model_.selection_base_ == 0) {
    return false;
  }
  model_.text_.erase(model_.selection_base_ - 1, 1);
  MoveCursorToLocation(--model_.selection_base_);

  return true;
}

bool TextInputModel::Delete() {
  // If a selection was deleted, don't delete more characters.
  if (EraseSelected()) {
    return true;
  }
  if (model_.selection_base_ == static_cast<int>(model_.text_.length())) {
    return false;
  }
  model_.text_.erase(model_.selection_base_, 1);

  return true;
}

bool TextInputModel::MoveCursorToLocation(int location) {
  if (location == model_.selection_base_ &&
      location == model_.selection_extent_) {
    return false;
  }
  if (location > static_cast<int>(model_.text_.length()) || location < 0) {
    return false;
  }
  model_.selection_base_ = location;
  model_.selection_extent_ = location;
  return true;
}

bool TextInputModel::MoveCursorToBeginning() {
  if (LocationIsAtBeginning(model_.selection_base_)) {
    return false;
  }
  MoveCursorToLocation(0);

  return true;
}

bool TextInputModel::MoveCursorToEnd() {
  if (LocationIsAtEnd(model_.selection_base_, model_.text_)) {
    return false;
  }
  MoveCursorToLocation(model_.text_.length());

  return true;
}

bool TextInputModel::MoveCursorForward() {
  if (LocationIsAtEnd(model_.selection_base_, model_.text_)) {
    return false;
  }
  MoveCursorToLocation(++model_.selection_base_);

  return true;
}

bool TextInputModel::MoveCursorBack() {
  if (LocationIsAtBeginning(model_.selection_base_)) {
    return false;
  }
  MoveCursorToLocation(--model_.selection_base_);

  return true;
}

bool TextInputModel::MoveCursorUp() {
  // Only perform for multiline models.
  // Trying to find a line break before position 0 will find the last line
  // break, which is undesired behavior.
  if (input_type_ != kMultilineInputType ||
      LocationIsAtBeginning(model_.selection_base_)) {
    return false;
  }

  // rfind will get the line break before or at the given location. Substract 1
  // to avoid finding the line break the cursor is standing on.
  std::size_t previous_break =
      model_.text_.rfind(kLineBreakKey, model_.selection_base_ - 1);
  if (previous_break == std::string::npos) {
    return false;
  }
  // Attempt to find a line break before the previous one. Finding this break
  // helps in the calculation to adjust the cursor to the previous line break,
  // in case there are more than one.
  std::size_t before_previous =
      model_.text_.rfind(kLineBreakKey, previous_break - 1);

  // |before_previous| will return an unsigned int with the position of the
  // character found. If none is found, std::string::npos is returned,
  // which is a constant to -1. When a line break is found, it includes the line
  // break position. If -1 is returned, it's used to compensate for the missing
  // line break we would otherwise get.
  int new_location = model_.selection_base_ - previous_break +
                     static_cast<int>(before_previous);

  // It is possible that the calculation above results in a new location further
  // from the previous break. e.g. 'aaaaa\na\naaaaa|a' where | is the position
  // of the cursor. The expected behavior would be to move to the end of the
  // previous line.
  if (new_location > static_cast<int>(previous_break)) {
    new_location = previous_break;
  }
  if (new_location < 0) {
    new_location = 0;
  }
  MoveCursorToLocation(new_location);
  return true;
}

bool TextInputModel::MoveCursorDown() {
  // Only perform for multiline models.
  if (input_type_ != kMultilineInputType ||
      LocationIsAtEnd(model_.selection_base_, model_.text_))
    return false;

  std::size_t next_break =
      model_.text_.find(kLineBreakKey, model_.selection_base_);
  if (next_break == std::string::npos) {
    // No need to continue if there's no new line below.
    return false;
  }
  std::size_t previous_break = std::string::npos;
  if (!LocationIsAtBeginning(model_.selection_base_)) {
    // Avoid looking for line break before position -1.
    previous_break =
        model_.text_.rfind(kLineBreakKey, model_.selection_base_ - 1);
  }

  // std::string::npos is a constant to -1. When a line break is found, it
  // includes the line break position. If |previous_break| is npos, it's used to
  // compensate for the missing line break we would otherwise get.
  int new_location =
      model_.selection_base_ - static_cast<int>(previous_break) + next_break;

  // Find the next break to avoid going over more than one line.
  std::size_t further_break = model_.text_.find(kLineBreakKey, next_break + 1);
  if (further_break != std::string::npos &&
      new_location > static_cast<int>(further_break)) {
    new_location = further_break;
  }
  if (LocationIsAtEnd(new_location, model_.text_)) {
    new_location = model_.text_.length();
  }

  MoveCursorToLocation(new_location);
  return true;
}

bool TextInputModel::InsertNewLine() {
  if (input_type_ != kMultilineInputType) {
    return false;
  }
  AddCharacter(kLineBreakKey);
  return true;
}

std::string TextInputModel::input_action() const { return input_action_; }

}  // namespace flutter_desktop_embedding
