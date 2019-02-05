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

static constexpr char kTextInputAction[] = "inpustAction";
static constexpr char kTextInputType[] = "inputType";
static constexpr char kTextInputTypeName[] = "name";

namespace flutter_desktop_embedding {

TextInputModelShared::TextInputModelShared(const Json::Value &config)
    : text_(""),
      selection_base_(text_.begin()),
      selection_extent_(text_.begin()),
      composing_base_(selection_base_),
      composing_extent_(selection_extent_),
      text_affinity_("Affinity") {
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

}  // namespace flutter_desktop_embedding
