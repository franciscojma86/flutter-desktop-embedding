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
#include "library/common/glfw/text_input_plugin.h"

#include <cstdint>
#include <iostream>

#include "library/include/flutter_desktop_embedding/json_method_codec.h"

static constexpr char kSetEditingStateMethod[] = "TextInput.setEditingState";
static constexpr char kClearClientMethod[] = "TextInput.clearClient";
static constexpr char kSetClientMethod[] = "TextInput.setClient";
static constexpr char kShowMethod[] = "TextInput.show";
static constexpr char kHideMethod[] = "TextInput.hide";

static constexpr char kMultilineInputType[] = "TextInputType.multiline";

static constexpr char kUpdateEditingStateMethod[] =
    "TextInputClient.updateEditingState";
static constexpr char kPerformActionMethod[] = "TextInputClient.performAction";

// static constexpr char kSelectionBaseKey[] = "selectionBase";
// static constexpr char kSelectionExtentKey[] = "selectionExtent";

// static constexpr char kTextKey[] = "text";

static constexpr char kChannelName[] = "flutter/textinput";

static constexpr char kBadArgumentError[] = "Bad Arguments";
static constexpr char kInternalConsistencyError[] =
    "Internal Consistency Error";

static constexpr char kTextAffinityDownstream[] = "TextAffinity.downstream";
static constexpr char kTextAffinityUpstream[] = "TextAffinity.upstream";

static constexpr char kComposingBaseKey[] = "composingBase";
static constexpr char kComposingExtentKey[] = "composingExtent";
static constexpr char kSelectionBaseKey[] = "selectionBase";
static constexpr char kSelectionExtentKey[] = "selectionExtent";
static constexpr char kSelectionAffinityKey[] = "selectionAffinity";
static constexpr char kSelectionIsDirectionalKey[] = "selectionIsDirectional";
static constexpr char kTextKey[] = "text";

static constexpr uint32_t kInputModelLimit = 256;

namespace flutter_desktop_embedding {

void TextInputPlugin::CharHook(GLFWwindow *window, unsigned int code_point) {
  if (active_model_ == nullptr) {
    return;
  }
  // TODO(awdavies): Actually handle potential unicode characters. Probably
  // requires some ICU data or something.
  active_model_->AddCharacter(static_cast<char>(code_point));
  SendStateUpdate(*active_model_);
}

void TextInputPlugin::KeyboardHook(GLFWwindow *window, int key, int scancode,
                                   int action, int mods) {
  if (active_model_ == nullptr) {
    return;
  }
  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    switch (key) {
      case GLFW_KEY_DOWN:
        if (active_model_->MoveCursorDown()) {
          SendStateUpdate(*active_model_);
        }
        break;
      case GLFW_KEY_UP:
        if (active_model_->MoveCursorUp()) {
          SendStateUpdate(*active_model_);
        }
        break;
      case GLFW_KEY_LEFT:
        if (active_model_->MoveCursorBack()) {
          SendStateUpdate(*active_model_);
        }
        break;
      case GLFW_KEY_RIGHT:
        if (active_model_->MoveCursorForward()) {
          SendStateUpdate(*active_model_);
        }
        break;
      case GLFW_KEY_END:
        if (active_model_->MoveCursorToEnd()) {
          SendStateUpdate(*active_model_);
        }
        break;
      case GLFW_KEY_HOME:
        if (active_model_->MoveCursorToBeginning()) {
          SendStateUpdate(*active_model_);
        }
        break;
      case GLFW_KEY_BACKSPACE:
        if (active_model_->Backspace()) {
          SendStateUpdate(*active_model_);
        }
        break;
      case GLFW_KEY_DELETE:
        if (active_model_->Delete()) {
          SendStateUpdate(*active_model_);
        }
        break;
      case GLFW_KEY_ENTER:
        EnterPressed(active_model_);
      default:
        break;
    }
  }
}

TextInputPlugin::TextInputPlugin(PluginRegistrar *registrar)
    : channel_(std::make_unique<MethodChannel<Json::Value>>(
          registrar->messenger(), kChannelName,
          &JsonMethodCodec::GetInstance())),
      active_model_(nullptr) {
  channel_->SetMethodCallHandler(
      [this](const MethodCall<Json::Value> &call,
             std::unique_ptr<MethodResult<Json::Value>> result) {
        HandleMethodCall(call, std::move(result));
      });
}

TextInputPlugin::~TextInputPlugin() {}

bool setEditingState(TextInputModel &input_model, Json::Value state) {
  Model model;
  Json::Value text = state[kTextKey];
  if (text.isNull()) {
    std::cerr << "Set editing state has been invoked, but without text."
              << std::endl;
    return false;
  }
  model.text_ = text.asString();

  Json::Value selection_base = state[kSelectionBaseKey];
  Json::Value selection_extent = state[kSelectionExtentKey];
  if (selection_base.isNull() || selection_extent.isNull()) {
    std::cerr << "Selection base/extent values invalid." << std::endl;
    return false;
  }
  model.selection_base_ = selection_base.asInt();
  model.selection_extent_ = selection_extent.asInt();

  Json::Value composing_base = state[kComposingBaseKey];
  Json::Value composing_extent = state[kComposingExtentKey];
  model.composing_base_ =
      composing_base.isNull() ? model.selection_base_ : composing_base.asInt();
  model.composing_extent_ = composing_extent.isNull()
                                ? model.selection_extent_
                                : composing_extent.asInt();

  Json::Value text_affinity = state[kSelectionAffinityKey];
  model.text_affinity_ = text_affinity.isNull() ? kTextAffinityDownstream
                                                : text_affinity.asString();
  input_model.ReplaceModel(model);
  return true;
}

void TextInputPlugin::HandleMethodCall(
    const MethodCall<Json::Value> &method_call,
    std::unique_ptr<MethodResult<Json::Value>> result) {
  const std::string &method = method_call.method_name();

  if (method.compare(kShowMethod) == 0 || method.compare(kHideMethod) == 0) {
    // These methods are no-ops.
  } else if (method.compare(kClearClientMethod) == 0) {
    active_model_ = nullptr;
  } else {
    // Every following method requires args.
    if (!method_call.arguments() || method_call.arguments()->isNull()) {
      result->Error(kBadArgumentError, "Method invoked without args");
      return;
    }
    const Json::Value &args = *method_call.arguments();

    if (method.compare(kSetClientMethod) == 0) {
      // TODO(awdavies): There's quite a wealth of arguments supplied with this
      // method, and they should be inspected/used.
      Json::Value client_id_json = args[0];
      Json::Value client_config = args[1];
      if (client_id_json.isNull()) {
        result->Error(kBadArgumentError, "Could not set client, ID is null.");
        return;
      }
      if (client_config.isNull()) {
        result->Error(kBadArgumentError,
                      "Could not set client, missing arguments.");
      }
      active_client_id = client_id_json.asInt();
      if (input_models_.find(active_client_id) == input_models_.end()) {
        // Skips out on adding a new input model once over the limit.
        if (input_models_.size() > kInputModelLimit) {
          result->Error(
              kInternalConsistencyError,
              "Input models over limit. Aborting creation of new text model.");
          return;
        }
        try {
          auto model = std::make_unique<TextInputModel>(client_config);
          input_models_.insert(
              std::make_pair(active_client_id, std::move(model)));

        } catch (const std::exception &e) {
          result->Error(kBadArgumentError, e.what());
          return;
        }
      }
      active_model_ = input_models_[active_client_id].get();
    } else if (method.compare(kSetEditingStateMethod) == 0) {
      if (active_model_ == nullptr) {
        result->Error(
            kInternalConsistencyError,
            "Set editing state has been invoked, but no client is set.");
        return;
      }
      if (!setEditingState(*active_model_, args)) {
        result->Error(
            kBadArgumentError,
            "Failed to set state. Arguments might be missing or misconfigured");
      }
    } else {
      // Unhandled method.
      result->NotImplemented();
      return;
    }
  }
  // All error conditions return early, so if nothing has gone wrong indicate
  // success.
  result->Success();
}

Json::Value convertState(const Model &model) {
  Json::Value editing_state;
  editing_state[kComposingBaseKey] = model.composing_base_;
  editing_state[kComposingExtentKey] = model.composing_extent_;
  editing_state[kSelectionAffinityKey] =
      model.text_affinity_.compare(kTextAffinityUpstream) == 0
          ? kTextAffinityUpstream
          : kTextAffinityDownstream;
  editing_state[kSelectionBaseKey] = model.selection_base_;
  editing_state[kSelectionExtentKey] = model.selection_extent_;
  editing_state[kSelectionIsDirectionalKey] = false;
  editing_state[kTextKey] = model.text_;
  return editing_state;
}

void TextInputPlugin::SendStateUpdate(const TextInputModel &model) {
  auto state = std::make_unique<Json::Value>();
  state.get()->append(active_client_id);
  Model m = model.GetModel();
  state.get()->append(convertState(m));
  channel_->InvokeMethod(kUpdateEditingStateMethod, std::move(state));
}

void TextInputPlugin::EnterPressed(TextInputModel *model) {
  if (model->InsertNewLine()) {
    SendStateUpdate(*model);
  }
  auto args = std::make_unique<Json::Value>(Json::arrayValue);
  args->append(active_client_id);
  args->append(model->input_action());

  channel_->InvokeMethod(kPerformActionMethod, std::move(args));
}

}  // namespace flutter_desktop_embedding
