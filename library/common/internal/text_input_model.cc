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
#include "../common/internal/text_input_model.h"

#include <iostream>

// InputType keys.
// https://docs.flutter.io/flutter/services/TextInputType-class.html
static constexpr char kMultilineInputType[] = "TextInputType.multiline";

static constexpr char kLineBreakKey = '\n';

namespace flutter_desktop_embedding {

TextInputModel::TextInputModel(std::string input_type, std::string input_action)
    : input_type_(input_type), input_action_(input_action) {}

TextInputModel::~TextInputModel() {}

State TextInputModel::GetState() const { return state_; }

void TextInputModel::UpdateState(State state) { state_ = state; }

void TextInputModel::ReplaceString(std::string string, int location = 0,
                                   int length = 0) {
  EraseSelected();
  state_.text.replace(location, length, string);
  MoveCursorToLocation(location + static_cast<int>(string.length()));
}

void TextInputModel::ReplaceString(std::string string, Range range) {
  EraseSelected();
  state_.text.replace(range.location, range.length, string);
  MoveCursorToLocation(range.location + string.length());
}

void TextInputModel::AddCharacter(char c) {
  std::string s(1, c);
  AddString(s);
}

void TextInputModel::AddString(std::string string) {
  EraseSelected();
  state_.text.insert(state_.selection_base, string);
  MoveCursorToLocation(++state_.selection_base);
}

bool TextInputModel::EraseSelected() {
  if (state_.selection_base == state_.selection_extent) {
    return false;
  }
  int base = std::min(state_.selection_base, state_.selection_extent);
  int extent = std::max(state_.selection_base, state_.selection_extent);
  state_.text.erase(base, extent);
  MoveCursorToLocation(base);
  return true;
}

bool TextInputModel::Backspace() {
  // If a selection was deleted, don't delete more characters.
  if (EraseSelected()) {
    return true;
  }
  if (state_.selection_base == 0) {
    return false;
  }
  state_.text.erase(state_.selection_base - 1, 1);
  MoveCursorToLocation(--state_.selection_base);

  return true;
}

bool TextInputModel::Delete() {
  // If a selection was deleted, don't delete more characters.
  if (EraseSelected()) {
    return true;
  }
  if (state_.selection_base == static_cast<int>(state_.text.length())) {
    return false;
  }
  state_.text.erase(state_.selection_base, 1);

  return true;
}

bool TextInputModel::LocationIsAtEnd(int location) {
  return (location == static_cast<int>(state_.text.length()));
}

bool TextInputModel::LocationIsAtBeginning(int location) {
  return (location == 0);
}

bool TextInputModel::MoveCursorToLocation(int location) {
  if (location == state_.selection_base &&
      location == state_.selection_extent) {
    return false;
  }
  if (location > static_cast<int>(state_.text.length()) || location < 0) {
    return false;
  }
  state_.selection_base = location;
  state_.selection_extent = location;
  return true;
}

bool TextInputModel::MoveCursorToBeginning() {
  if (LocationIsAtBeginning(state_.selection_base)) {
    return false;
  }
  MoveCursorToLocation(0);

  return true;
}

bool TextInputModel::MoveCursorToEnd() {
  if (LocationIsAtEnd(state_.selection_base)) {
    return false;
  }
  MoveCursorToLocation(static_cast<int>(state_.text.length()));

  return true;
}

bool TextInputModel::MoveCursorForward() {
  std::cout << state_.selection_base;
  if (LocationIsAtEnd(state_.selection_base)) {
    return false;
  }
  MoveCursorToLocation(++state_.selection_base);
  return true;
}

bool TextInputModel::MoveCursorBack() {
  std::cout << state_.selection_base;
  if (LocationIsAtBeginning(state_.selection_base)) {
    return false;
  }
  MoveCursorToLocation(--state_.selection_base);
  return true;
}

bool TextInputModel::MoveCursorUp() {
  // Only perform for multiline models.
  // Trying to find a line break before position 0 will find the last line
  // break, which is undesired behavior.
  if (input_type_ != kMultilineInputType ||
      LocationIsAtBeginning(state_.selection_base)) {
    return false;
  }

  // rfind will get the line break before or at the given location. Substract 1
  // to avoid finding the line break the cursor is standing on.
  std::size_t previous_break =
      state_.text.rfind(kLineBreakKey, state_.selection_base - 1);
  if (previous_break == std::string::npos) {
    return false;
  }
  // Attempt to find a line break before the previous one. Finding this break
  // helps in the calculation to adjust the cursor to the previous line break,
  // in case there are more than one.
  std::size_t before_previous =
      state_.text.rfind(kLineBreakKey, previous_break - 1);

  // |before_previous| will return an unsigned int with the position of the
  // character found. If none is found, std::string::npos is returned,
  // which is a constant to -1. When a line break is found, it includes the line
  // break position. If -1 is returned, it's used to compensate for the missing
  // line break we would otherwise get.
  int previous_break_int = static_cast<int>(previous_break);
  int new_location = state_.selection_base - previous_break_int +
                     static_cast<int>(before_previous);

  // It is possible that the calculation above results in a new location further
  // from the previous break. e.g. 'aaaaa\na\naaaaa|a' where | is the position
  // of the cursor. The expected behavior would be to move to the end of the
  // previous line.
  if (new_location > previous_break_int) {
    new_location = previous_break_int;
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
      LocationIsAtEnd(state_.selection_base))
    return false;

  std::size_t next_break =
      state_.text.find(kLineBreakKey, state_.selection_base);
  if (next_break == std::string::npos) {
    // No need to continue if there's no new line below.
    return false;
  }
  std::size_t previous_break = std::string::npos;
  if (!LocationIsAtBeginning(state_.selection_base)) {
    // Avoid looking for line break before position -1.
    previous_break =
        state_.text.rfind(kLineBreakKey, state_.selection_base - 1);
  }

  // std::string::npos is a constant to -1. When a line break is found, it
  // includes the line break position. If |previous_break| is npos, it's used to
  // compensate for the missing line break we would otherwise get.
  int new_location = state_.selection_base - static_cast<int>(previous_break) +
                     static_cast<int>(next_break);

  // Find the next break to avoid going over more than one line.
  std::size_t further_break = state_.text.find(kLineBreakKey, next_break + 1);
  if (further_break != std::string::npos &&
      new_location > static_cast<int>(further_break)) {
    new_location = static_cast<int>(further_break);
  }
  if (LocationIsAtEnd(new_location)) {
    new_location = static_cast<int>(state_.text.length());
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

void TextInputModel::MarkText(int location, int length) {
  state_.composing_base = location;
  state_.composing_extent = location + length;
}

void TextInputModel::MarkText(Range range) {
  state_.composing = range;
}

void TextInputModel::SelectText(Range range) {
  state_.selection = range;
}

void TextInputModel::SelectText(int location, int length) {
  state_.selection_base = location;
  state_.selection_extent = location + length;
}

std::string TextInputModel::input_action() const { return input_action_; }

}  // namespace flutter_desktop_embedding
