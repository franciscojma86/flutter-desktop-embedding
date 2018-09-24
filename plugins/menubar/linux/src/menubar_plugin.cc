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
#include <iostream>
#include <stdio.h>

#include "../../common/channel_constants.h"

namespace plugins_menubar {
using flutter_desktop_embedding::JsonMethodCall;
using flutter_desktop_embedding::MethodResult;

MenuBarPlugin::MenuBarPlugin() : JsonPlugin(kChannelName, false) {}

MenuBarPlugin::~MenuBarPlugin() {}

GtkWidget *menubar_window;

void MenuBarPlugin::HandleJsonMethodCall(
    const JsonMethodCall &method_call, std::unique_ptr<MethodResult> result) {
    if (method_call.method_name().compare("showBar") == 0) {
      showMenuBar();
    }
}

  static Json::Value GdkColorToArgs(const GdkRGBA *color) {
    Json::Value result;
    result["red"] = color->red * color->alpha;
    result["green"] = color->green * color->alpha;
    result["blue"] = color->blue * color->alpha;
    return result;
  }


static void RedColorSelected(GtkWidget* menuItem, gpointer* data) {
  auto plugin = reinterpret_cast<MenuBarPlugin *>(data);
    GdkRGBA color;
    color.red = 1.0;
    color.blue = 0.0;
    color.green = 0.0;
    color.alpha = 1.0;
      std::cerr << "Happening";

    plugin->ChangeColor(GdkColorToArgs(&color));
}

void MenuBarPlugin::ChangeColor(Json::Value colorArgs) {
  std::cerr << "Change color " << colorArgs;
  InvokeMethod(kColorSelectedCallbackMethod, colorArgs);
}

void MenuBarPlugin::showMenuBar() {
    if (menubar_window != nullptr) return;
    GtkWidget *vbox;
    GtkWidget *menubar;
    GtkWidget *fileMenu;
    GtkWidget *fileMi;
    GtkWidget *quitMi;
    GtkWidget *colorMenu;
    GtkWidget *colorMi;
    GtkWidget *redMi;

    menubar_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(menubar_window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(menubar_window), 300, 50);
    gtk_window_set_title(GTK_WINDOW(menubar_window), "Simple menu");

    vbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(menubar_window), vbox);

    menubar = gtk_menu_bar_new();
    fileMenu = gtk_menu_new();
    colorMenu = gtk_menu_new();

    fileMi = gtk_menu_item_new_with_label("File");  
    quitMi = gtk_menu_item_new_with_label("Quit");
    colorMi = gtk_menu_item_new_with_label("Colors");
    redMi = gtk_menu_item_new_with_label("Red");

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMi), fileMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quitMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileMi);
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(menubar_window), "destroy",
                     G_CALLBACK(gtk_main_quit), NULL);

    g_signal_connect(G_OBJECT(quitMi), "activate",
                     G_CALLBACK(gtk_main_quit), NULL);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(colorMi), colorMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(colorMenu), redMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), colorMi);
    
    g_signal_connect(G_OBJECT(redMi), "activate",
                     G_CALLBACK(RedColorSelected), this);

    gtk_widget_show_all(menubar_window);
}

} // namespace plugins_menubar