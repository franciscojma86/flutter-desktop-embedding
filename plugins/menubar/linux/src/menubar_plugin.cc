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

MenuBarPlugin::MenuBarPlugin() : JsonPlugin(kChannelName, false) {}

MenuBarPlugin::~MenuBarPlugin() {}

GtkWidget *menubar_window;

static void IterateMenuContents(const Json::Value &root, GtkWidget *widget);

void MenuBarPlugin::HandleJsonMethodCall(const JsonMethodCall &method_call,
                                         std::unique_ptr<MethodResult> result) {
  if (method_call.method_name().compare(kMenuSetMethod) == 0) {
    result->Success();
    showMenuBar(method_call.GetArgumentsAsJson());
  } else {
    result->NotImplemented();
  }
}

// static Json::Value GdkColorToArgs(const GdkRGBA *color) {
//   Json::Value result;
//   result["red"] = color->red * color->alpha;
//   result["green"] = color->green * color->alpha;
//   result["blue"] = color->blue * color->alpha;
//   return result;
// }

// static void RedColorSelected(GtkWidget *menuItem, gpointer *data) {
//   auto plugin = reinterpret_cast<MenuBarPlugin *>(data);
//   GdkRGBA color;
//   color.red = 1.0;
//   color.blue = 0.0;
//   color.green = 0.0;
//   color.alpha = 1.0;
//   std::cerr << "Happening";

//   const char *name = gtk_widget_get_name(menuItem);
//   std::cerr << name;
//   plugin->ChangeColor(GdkColorToArgs(&color));
// }

// void MenuBarPlugin::ChangeColor(Json::Value colorArgs) {
//   std::cerr << "Change color " << colorArgs;
//   // InvokeMethod(kColorSelectedCallbackMethod, colorArgs);
// }

void MenuBarPlugin::showMenuBar(const Json::Value &args) {
  std::cerr << args;
  menubar_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(menubar_window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size(GTK_WINDOW(menubar_window), 300, 50);
  gtk_window_set_title(GTK_WINDOW(menubar_window), kWindowTitle);

  GtkWidget *vbox;
  vbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add(GTK_CONTAINER(menubar_window), vbox);

  GtkWidget *menubar = gtk_menu_bar_new();
  IterateMenuContents(args, menubar);
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

  // fileMenu = gtk_menu_new();
  // colorMenu = gtk_menu_new();

  // fileMi = gtk_menu_item_new_with_label("File");
  // quitMi = gtk_menu_item_new_with_label("Quit");
  // colorMi = gtk_menu_item_new_with_label("Colors");
  // redMi = gtk_menu_item_new_with_label("Red");
  // gtk_widget_set_name(redMi, "Hello");

  // gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMi), fileMenu);
  // gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quitMi);
  // gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileMi);

  // g_signal_connect(G_OBJECT(menubar_window), "destroy",
  //                  G_CALLBACK(gtk_main_quit), NULL);

  // g_signal_connect(G_OBJECT(quitMi), "activate", G_CALLBACK(gtk_main_quit),
  //                  NULL);

  // gtk_menu_item_set_submenu(GTK_MENU_ITEM(colorMi), colorMenu);
  // gtk_menu_shell_append(GTK_MENU_SHELL(colorMenu), redMi);
  // gtk_menu_shell_append(GTK_MENU_SHELL(menubar), colorMi);

  // g_signal_connect(G_OBJECT(redMi), "activate", G_CALLBACK(RedColorSelected),
  //                  this);

  gtk_widget_show_all(menubar_window);
}

static void IterateMenuContents(const Json::Value &root, GtkWidget *menubar) {
  std::cerr << "------Iteratingasdfasfasdfasd \n";

  if (root.isArray()) {
    for (Json::Value::const_iterator itr = root.begin(); itr != root.end();
         itr++) {
      if ((*itr).isObject()) {
        Json::Value object = *itr;
        // if ((object["label"]).isString()) {
        //   std::string label = object["label"].asString();
        // }
        if ((object["label"]).isString()) {
          std::string label = object["label"].asString();

          // Json::Value children = object["children"];
          auto menu = gtk_menu_new();
          GtkWidget *menuItem = gtk_menu_item_new_with_label(label.c_str());
          gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItem), menu);
          // gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu), menubar);
          gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menuItem);
          std::cerr << "Writing\n";
        } else {
                    std::cerr << "not\n";

        }
        // std::cerr << label << std::endl;
        // GtkWidget *menuItem = gtk_menu_item_new_with_label(label);
      }
    }
  }

  // for (Json::Value::const_iterator itr = root.begin(); itr != root.end();
  //      itr++) {
  //   std::cerr << itr.key() << " " << *itr << std::endl;
  //   std::cerr << (*itr)["label"] << std::endl;

  //   auto label = (*itr)["children"].isString();
  //   // if (label != nullptr) {
  //   std::cerr << label << std::endl;
  //   // }

  //   std::cerr << "------in\n";

  //   std::cerr << (*itr)["children"] << std::endl;
  // }
}

}  // namespace plugins_menubar