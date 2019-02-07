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

// TODO: Remove the "shared"
#ifndef LIBRARY_COMMON_INTERNAL_SHARED_INPUT_MODEL_H_
#define LIBRARY_COMMON_INTERNAL_SHARED_INPUT_MODEL_H_

#include <string>

#include <json/json.h>

namespace flutter_desktop_embedding {

class TextInputModelShared {
 public:
  // Constructor for TextInputModelShared. An exception is thrown if
  // the |config| JSON contains bad arguments.
  explicit TextInputModelShared(const Json::Value &config);
  virtual ~TextInputModelShared();

  // Attempts to set the text state.
  //
  // Returns false if the state is not valid (base or extent are out of
  // bounds, or base is less than extent).
  bool SetEditingState(const Json::Value &state);

  // Returns the state in the form of a platform message.
  Json::Value GetEditingState() const;

  // Replaces a section of the stored string with a given |string|. |location|
  // is the starting point where the new string will be added. |length| is the
  // number of characters to be substituted from the stored string, starting
  // from |location. Deletes any previously selected text.
  void ReplaceString(std::string string, int location, int length);

  // Adds a character.
  //
  // Either appends after the cursor (when selection base and extent are the
  // same), or deletes the selected characters, replacing the text with the
  // character specified.

  void AddCharacter(char c);

  // Adds a string.
  //
  // Either appends after the cursor (when selection base and extent are the
  // same), or deletes the selected characters, replacing the text with the
  // character specified.
  void AddString(std::string string);

  // Erases the currently selected text. Return true if any deletion ocurred.

  // Deletes either the selection, or one character behind the cursor.
  //
  // Deleting one character behind the cursor occurs when the selection base
  // and extent are the same.
  bool Backspace();

  // Deletes either the selection, or one character ahead of the cursor.
  //
  // Deleting one character ahead of the cursor occurs when the selection base
  // and extent are the same.
  //
  // Returns true if any deletion actually occurred.
  bool Delete();

  // Attempts to move the cursor to the beginning.
  //
  // Returns true if the cursor could be moved.
  bool MoveCursorToBeginning();

  // Attempts to move the cursor to the end.
  //
  // Returns true if the cursor could be moved.
  bool MoveCursorToEnd();

  // Attempts to move the cursor forward.
  //
  // Returns true if the cursor could be moved.
  bool MoveCursorForward();

  // Attempts to move the cursor backward.
  //
  // Returns true if the cursor could be moved. Changes base and extent to be
  // equal to either the extent (if extent is at the end of the string), or
  // for extent to be equal to the base.
  bool MoveCursorBack();

  // Inserts a new line to the text if the |input_type| is multiline.
  bool InsertNewLine();

  // Attempts to move the cursor to a line above, if any.
  //
  // Returns true if the cursor could be moved.
  bool MoveCursorUp();

  // Attempts to move the cursor to a line below, if any.
  //
  // Returns true if the cursor could be moved.
  bool MoveCursorDown();

  // An action requested by the user on the input client. See available options:
  // https://docs.flutter.io/flutter/services/TextInputAction-class.html
  std::string input_action();

 private:
  bool MoveCursorToLocation(int location);
  bool EraseSelected();

  std::string text_;
  std::string input_type_;
  std::string input_action_;
  int selection_base_ = 0;
  int selection_extent_ = 0;
  int composing_base_ = 0;
  int composing_extent_ = 0;
  std::string text_affinity_;
};

}  // namespace flutter_desktop_embedding

#endif  // LIBRARY_COMMON_INTERNAL_SHARED_INPUT_MODEL_H_
