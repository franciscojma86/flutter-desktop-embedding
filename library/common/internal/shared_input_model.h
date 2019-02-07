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
  // the |config| JSON doesn't contain the required values.
  explicit TextInputModelShared(const Json::Value &config);
  virtual ~TextInputModelShared();

  // Update the the complete model state.
  bool SetEditingState(const Json::Value &state);
  // Get the model editing state in a JSON.
  Json::Value GetEditingState() const;

  void speak();

  // Replaces a section of the stored string with a given |string|. |location|
  // is the starting point where the new string will be added. |length| is the
  // number of characters to be substituted from the stored string. Erases any
  // previously selected text.
  void ReplaceString(std::string string, int location, int length);

  void AddCharacter(char c);
  // Adds a string at the current cursor location. Erases any previously
  // selected text.
  void AddString(std::string string);

  // Erases the currently selected text. Return true if any deletion ocurred.
  bool EraseSelected();

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
  bool Delete();
  bool MoveCursorToBeginning();
  bool MoveCursorToEnd();
  bool MoveCursorForward();
  bool MoveCursorBack();
  bool InsertNewLine();

  std::string input_action();

 private:
  std::string text_;
  std::string input_type_;
  std::string input_action_;
  int selection_base_ = 0;
  int selection_extent_ = 0;
  int composing_base_ = 0;
  int composing_extent_ = 0;
  std::string text_affinity_;

  bool MoveCursorToLocation(int location);
};

}  // namespace flutter_desktop_embedding

#endif  // LIBRARY_COMMON_INTERNAL_SHARED_INPUT_MODEL_H_
