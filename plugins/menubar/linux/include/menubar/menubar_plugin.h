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
#ifndef PLUGINS_MENUBAR_LINUX_INCLUDE_MENUBAR_MENUBAR_PLUGIN_H_
#define PLUGINS_MENUBAR_LINUX_INCLUDE_MENUBAR_MENUBAR_PLUGIN_H_

#include <flutter_desktop_embedding/plugin.h>

class MethodCall;
class MethodResult;

namespace plugins_menubar {

class MenuBarPlugin : public flutter_desktop_embedding::Plugin {
    public:
        MenuBarPlugin();
        virtual ~MenuBarPlugin();

        void showMenuBar();
        void HandleMethodCall(const MethodCall &method_call,
                        std::unique_ptr<MethodResult> result) override;

};

}

#endif  // PLUGINS_MENUBAR_LINUX_INCLUDE_MENUBAR_MENUBAR_PLUGIN_H_
