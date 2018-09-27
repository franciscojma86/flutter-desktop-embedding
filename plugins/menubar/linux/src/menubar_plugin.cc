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

#include <menubar/menubar_plugin.h>

#include <gtk/gtk.h>
#include <stdio.h>
#include <iostream>
#include <typeinfo>

#include "../../common/channel_constants.h"

static constexpr char kWindowTitle[] = "Flutter Menubar";

namespace plugins_menubar {
using flutter_desktop_embedding::JsonMethodCall;
using flutter_desktop_embedding::MethodResult;

MenuBarPlugin::MenuBarPlugin()
    : JsonPlugin(kChannelName, false), menubar_(nullptr) {}

MenuBarPlugin::~MenuBarPlugin() {}

class MenuBarPlugin::Menubar {
 public:
  explicit Menubar(MenuBarPlugin *parent) {
    menubar_window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(menubar_window_), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(menubar_window_), 300, 50);
    gtk_window_set_title(GTK_WINDOW(menubar_window_), kWindowTitle);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(menubar_window_), vbox);

    menubar_ = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(vbox), menubar_, FALSE, FALSE, 0);

    gtk_widget_show_all(menubar_window_);
  }
  virtual ~Menubar() {
    if (menubar_window_) {
      gtk_widget_destroy(menubar_window_);
      menubar_window_ = nullptr;
    }
  }

  GtkWidget *GetRootMenuBar() { return menubar_; }

  static void MenuItemSelected(GtkWidget *menuItem, gpointer *data) {
    auto plugin = reinterpret_cast<MenuBarPlugin *>(data);
    Json::Value result;
    result[kIdKey] = gtk_widget_get_name(menuItem);

    plugin->InvokeMethod(kMenuItemSelectedCallbackMethod, result[kIdKey]);
  }

  void SetMenuItems(const Json::Value &root,
                    flutter_desktop_embedding::Plugin *plugin,
                    GtkWidget *parentWidget) {
    if (root.isArray()) {
      unsigned int counter = 0;
      while (counter < root.size()) {
        if (root[counter].isObject()) {
          SetMenuItems(root[counter], plugin, parentWidget);
          counter++;
        }
      }
      gtk_widget_show_all(menubar_window_);

      return;
    }

    if ((root["label"]).isString()) {
      std::string label = root["label"].asString();

      if (root["children"].isArray()) {
        auto array = root["children"];
        auto menu = gtk_menu_new();
        auto menuItem = gtk_menu_item_new_with_label(label.c_str());
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItem), menu);
        gtk_menu_shell_append(GTK_MENU_SHELL(parentWidget), menuItem);

        SetMenuItems(array, plugin, menu);
      } else {
        std::cerr << "no children" << std::endl;

        auto menuItem = gtk_menu_item_new_with_label(label.c_str());
        if (root["id"].asInt()) {
          std::string idString = std::to_string(root["id"].asInt());
          gtk_widget_set_name(menuItem, idString.c_str());
        }
        g_signal_connect(G_OBJECT(menuItem), "activate",
                         G_CALLBACK(MenuItemSelected), plugin);
        gtk_menu_shell_append(GTK_MENU_SHELL(parentWidget), menuItem);
      }
    }

    if (root.get("isDivider", false).asBool()) {
      auto separator = gtk_separator_menu_item_new();
      gtk_menu_shell_append(GTK_MENU_SHELL(parentWidget), separator);
    }
    gtk_widget_show_all(menubar_window_);
  }

  void ClearMenuItems() {
    GList *children, *iter;

    children = gtk_container_get_children(GTK_CONTAINER(menubar_));
    for (iter = children; iter != NULL; iter = g_list_next(iter))
      gtk_widget_destroy(GTK_WIDGET(iter->data));
    g_list_free(children);
  }

 protected:
  GtkWidget *menubar_window_;
  GtkWidget *menubar_;
};

GtkWidget *menubar_window;

static void IterateMenuContents(const Json::Value &root, GtkWidget *widget,
                                flutter_desktop_embedding::Plugin *plugin);

void MenuBarPlugin::HandleJsonMethodCall(const JsonMethodCall &method_call,
                                         std::unique_ptr<MethodResult> result) {
  if (method_call.method_name().compare(kMenuSetMethod) == 0) {
    result->Success();
    if (menubar_ == nullptr) {
      menubar_ = std::make_unique<MenuBarPlugin::Menubar>(this);
    }
    menubar_->ClearMenuItems();
    menubar_->SetMenuItems(method_call.GetArgumentsAsJson(), this,
                           menubar_->GetRootMenuBar());
  } else {
    result->NotImplemented();
  }
}

static void MenuItemSelected(GtkWidget *menuItem, gpointer *data) {
  auto plugin = reinterpret_cast<MenuBarPlugin *>(data);
  Json::Value result;
  result[kIdKey] = gtk_widget_get_name(menuItem);

  plugin->ChangeColor(result);
  std::cerr << "Clicked " << gtk_widget_get_name(menuItem);
}

void MenuBarPlugin::ChangeColor(Json::Value colorArgs) {
  InvokeMethod(kMenuItemSelectedCallbackMethod, colorArgs[kIdKey]);
}

void MenuBarPlugin::showMenuBar(const Json::Value &args) {
  if (menubar_window != nullptr) return;
  menubar_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(menubar_window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size(GTK_WINDOW(menubar_window), 300, 50);
  gtk_window_set_title(GTK_WINDOW(menubar_window), kWindowTitle);

  GtkWidget *vbox;
  vbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add(GTK_CONTAINER(menubar_window), vbox);

  GtkWidget *menubar = gtk_menu_bar_new();
  IterateMenuContents(args, menubar, this);
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
}

static void IterateMenuContents(const Json::Value &root, GtkWidget *menubar,
                                flutter_desktop_embedding::Plugin *plugin) {
  if (root.isArray()) {
    unsigned int counter = 0;
    while (counter < root.size()) {
      if (root[counter].isObject()) {
        IterateMenuContents(root[counter], menubar, plugin);
        counter++;
      }
    }
    return;
  }

  if ((root["label"]).isString()) {
    std::string label = root["label"].asString();

    if (root["children"].isArray()) {
      auto array = root["children"];
      auto menu = gtk_menu_new();
      auto menuItem = gtk_menu_item_new_with_label(label.c_str());
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItem), menu);
      gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menuItem);

      IterateMenuContents(array, menu, plugin);
    } else {
      std::cerr << "no children" << std::endl;

      auto menuItem = gtk_menu_item_new_with_label(label.c_str());
      if (root["id"].asInt()) {
        std::string idString = std::to_string(root["id"].asInt());
        gtk_widget_set_name(menuItem, idString.c_str());
      }
      g_signal_connect(G_OBJECT(menuItem), "activate",
                       G_CALLBACK(MenuItemSelected), plugin);
      gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menuItem);
    }
  }

  if (root.get("isDivider", false).asBool()) {
    auto separator = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), separator);
  }
}

}  // namespace plugins_menubar