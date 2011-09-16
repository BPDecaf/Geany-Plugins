/*
 *      plugin.c
 *      
 *      Copyright 2010 Alexander Petukhov <devel(at)apetukhov.ru>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

/*
 * 		geany debugger plugin
 */
 
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "geanyplugin.h"
#include "breakpoints.h"
#include "callbacks.h"
#include "debug.h"
#include "tpage.h"
#include "utils.h"
#include "btnpanel.h"
#include "keys.h"
#include "dconfig.h"
#include "dpaned.h"
#include "tabs.h"
#include "envtree.h"

/* These items are set by Geany before plugin_init() is called. */
GeanyPlugin		*geany_plugin;
GeanyData		*geany_data;
GeanyFunctions	*geany_functions;


/* Check that the running Geany supports the plugin API version used below, and check
 * for binary compatibility. */
PLUGIN_VERSION_CHECK(209)
PLUGIN_SET_TRANSLATABLE_INFO(
	LOCALEDIR,
	GETTEXT_PACKAGE,
	_("Debugger"),
	_("Various debuggers integration."),
	VERSION,
	"Alexander Petukhov <devel@apetukhov.ru>")

/* vbox for keeping breaks/stack/watch notebook */
static GtkWidget *hbox = NULL;

PluginCallback plugin_callbacks[] =
{
	/* Set 'after' (third field) to TRUE to run the callback @a after the default handler.
	 * If 'after' is FALSE, the callback is run @a before the default handler, so the plugin
	 * can prevent Geany from processing the notification. Use this with care. */
	{ "editor-notify", (GCallback) &on_editor_notify, FALSE, NULL },
	{ "document_open", (GCallback) &on_document_open, FALSE, NULL },
	{ "document_activate", (GCallback) &on_document_activate, FALSE, NULL },
	{ "document_close", (GCallback) &on_document_close, FALSE, NULL },
	{ "document_save", (GCallback) &on_document_save, FALSE, NULL },
	{ "document_before_save", (GCallback) &on_document_before_save, FALSE, NULL },
	{ "document_new", (GCallback) &on_document_new, FALSE, NULL },
	
	{ NULL, NULL, FALSE, NULL }
};

void on_paned_mode_changed(GtkToggleButton *button, gpointer user_data)
{
	gboolean state = gtk_toggle_button_get_active(button);
	dpaned_set_tabbed(state);
	tpage_pack_widgets(state);
}

/* Called by Geany to initialize the plugin.
 * Note: data is the same as geany_data. */
void plugin_init(GeanyData *data)
{
    main_locale_init(LOCALEDIR, GETTEXT_PACKAGE);

	keys_init();

	/* main box */
	hbox = gtk_hbox_new(FALSE, 0);

	/* add target page */
	tpage_init();
	
	/* init brekpoints */
	breaks_init(editor_open_position);
	
	/* init markers */
	markers_init();

	/* init debug */
	debug_init();

	/* init paned */
	dpaned_init();
	tpage_pack_widgets(dpaned_get_tabbed());

	GtkWidget* vbox = btnpanel_create(on_paned_mode_changed);

	gtk_box_pack_start(GTK_BOX(hbox), dpaned_get_paned(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	gtk_widget_show_all(hbox);
	
	gtk_notebook_append_page(
		GTK_NOTEBOOK(geany->main_widgets->message_window_notebook),
		hbox,
		gtk_label_new(_("Debug")));

	/* init config */
	dconfig_init();
}


/* Called by Geany to show the plugin's configure dialog. This function is always called after
 * plugin_init() was called.
 * You can omit this function if the plugin doesn't need to be configured.
 * Note: parent is the parent window which can be used as the transient window for the created
 *       dialog. */
GtkWidget *plugin_configure(GtkDialog *dialog)
{
	/* configuration dialog */
	GtkWidget *vbox = gtk_vbox_new(FALSE, 6);

	gtk_widget_show_all(vbox);
	return vbox;
}


/* Called by Geany before unloading the plugin.
 * Here any UI changes should be removed, memory freed and any other finalization done.
 * Be sure to leave Geany as it was before plugin_init(). */
void plugin_cleanup(void)
{
	/* stop debugger if running */
	if (DBS_IDLE != debug_get_state())
	{
		debug_stop();
		while (DBS_IDLE != debug_get_state())
			g_main_context_iteration(NULL,FALSE);
	}

	/* destroy debug-related stuff */
	debug_destroy();

	/* destroy breaks */
	breaks_destroy();

	/* clears config */
	dconfig_destroy();

	/* clears debug paned data */
	dpaned_destroy();

	/* clears anv tree data */
	envtree_destroy();
	
	/* release other allocated strings and objects */
	gtk_widget_destroy(hbox);
}
