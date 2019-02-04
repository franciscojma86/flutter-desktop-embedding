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
#ifndef LIBRARY_COMMON_INTERNAL_TEXT_INPUT_MODEL_H_
#define LIBRARY_COMMON_INTERNAL_TEXT_INPUT_MODEL_H_

#include <string>

#include <json/json.h>

namespace flutter_desktop_embedding {

class TextInputModelShared {
 public:
  explicit TextInputModelShared(const Json::Value &config);
  virtual ~TextInputModelShared();
 
 private:
  std::string::iterator selection_base_ = -1;
  std::string::iterator selection_extent_ = -1;
  std::string::iterator composing_base_ = -1;
  std::string::iterator composing_extent_ = -1;
  std::string text_; 
  std::string input_type_;
  std::string input_action_;
  std::string text_affinity_;
};

}  // namespace flutter_desktop_embedding

#endif  // LIBRARY_COMMON_INTERNAL_TEXT_INPUT_MODEL_H_
