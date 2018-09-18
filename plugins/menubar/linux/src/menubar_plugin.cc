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

namespace plugins_menubar {

void showMenuBar() {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *menubar;
    GtkWidget *fileMenu;
    GtkWidget *fileMi;
    GtkWidget *quitMi;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 50);
    gtk_window_set_title(GTK_WINDOW(window), "Simple menu");

    vbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    menubar = gtk_menu_bar_new();
    fileMenu = gtk_menu_new();

    fileMi = gtk_menu_item_new_with_label("File");  
    quitMi = gtk_menu_item_new_with_label("Quit");

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMi), fileMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quitMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileMi);
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(window), "destroy",
                     G_CALLBACK(gtk_main_quit), NULL);

    g_signal_connect(G_OBJECT(quitMi), "activate",
                     G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
}

}