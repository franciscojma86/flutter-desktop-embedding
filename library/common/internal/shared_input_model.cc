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

namespace flutter_desktop_embedding {

TextInputModelShared::TextInputModelShared(const Json::Value &config)
    : text_("") {
  std::string input_action = config[kTextInputAction].asString();
  Json::Value input_type_info = config[kTextInputType];
  std::string input_type = input_type_info[kTextInputTypeName].asString();
  if (!input_action || !input_type) {
    throw SampleException;
  }
  input_type_ = input_type;
  input_action_ = input_action;
}

}  // namespace flutter_desktop_embedding

}  // namespace flutter_desktop_embedding
