#include "rumble.hpp"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <string.h>
#include <gio/gio.h>

// Most of this is copied from dunst/dbus.c
namespace {
  GDBusConnection *dbus_conn;

  static GDBusNodeInfo *introspection_data = 0;

  static const char *introspection_xml =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<node name=\"/org/freedesktop/Notifications\">"
    "    <interface name=\"org.freedesktop.Notifications\">"

    "        <method name=\"GetCapabilities\">"
    "            <arg direction=\"out\" name=\"capabilities\"    type=\"as\"/>"
    "        </method>"

    "        <method name=\"Notify\">"
    "            <arg direction=\"in\"  name=\"app_name\"        type=\"s\"/>"
    "            <arg direction=\"in\"  name=\"replaces_id\"     type=\"u\"/>"
    "            <arg direction=\"in\"  name=\"app_icon\"        type=\"s\"/>"
    "            <arg direction=\"in\"  name=\"summary\"         type=\"s\"/>"
    "            <arg direction=\"in\"  name=\"body\"            type=\"s\"/>"
    "            <arg direction=\"in\"  name=\"actions\"         type=\"as\"/>"
    "            <arg direction=\"in\"  name=\"hints\"           type=\"a{sv}\"/>"
    "            <arg direction=\"in\"  name=\"expire_timeout\"  type=\"i\"/>"
    "            <arg direction=\"out\" name=\"id\"              type=\"u\"/>"
    "        </method>"

    "        <method name=\"CloseNotification\">"
    "            <arg direction=\"in\"  name=\"id\"              type=\"u\"/>"
    "        </method>"

    "        <method name=\"GetServerInformation\">"
    "            <arg direction=\"out\" name=\"name\"            type=\"s\"/>"
    "            <arg direction=\"out\" name=\"vendor\"          type=\"s\"/>"
    "            <arg direction=\"out\" name=\"version\"         type=\"s\"/>"
    "            <arg direction=\"out\" name=\"spec_version\"    type=\"s\"/>"
    "        </method>"

    "        <signal name=\"NotificationClosed\">"
    "            <arg name=\"id\"         type=\"u\"/>"
    "            <arg name=\"reason\"     type=\"u\"/>"
    "        </signal>"

    "        <signal name=\"ActionInvoked\">"
    "            <arg name=\"id\"         type=\"u\"/>"
    "            <arg name=\"action_key\" type=\"s\"/>"
    "        </signal>"
    "   </interface>"
    "</node>";
  void onGetServerInformation(GDBusConnection * connection,
			      const gchar * /*sender*/,
			      const GVariant * /*parameters*/,
				     GDBusMethodInvocation * invocation)
  {
    GVariant *value;

    value = g_variant_new("(ssss)", "rumblenotify", "pmiddend", "1.0", "1.2");
    g_dbus_method_invocation_return_value(invocation, value);

    g_dbus_connection_flush(connection, NULL, NULL, NULL);
  }


  void handle_method_call(GDBusConnection * connection,
			  const gchar * sender,
			  const gchar * /*object_path*/,
			  const gchar * /*interface_name*/,
			  const gchar * method_name,
			  GVariant * parameters,
			  GDBusMethodInvocation * invocation,
			  gpointer /*user_data*/)
  {
    if (g_strcmp0(method_name, "GetCapabilities") == 0) {
    } else if (g_strcmp0(method_name, "Notify") == 0) {
      if(rumblenotify::rumble() != EXIT_SUCCESS) {
	std::cerr << "rumbling failed\n";
      }
      GVariant *reply = g_variant_new("(u)", 0);
      g_dbus_method_invocation_return_value(invocation, reply);
      g_dbus_connection_flush(connection, NULL, NULL, NULL);
    } else if (g_strcmp0(method_name, "CloseNotification") == 0) {
    } else if (g_strcmp0(method_name, "GetServerInformation") == 0) {
      onGetServerInformation(connection, sender, parameters,
			     invocation);
    } else {
      std::cerr << "WARNING: sender " << sender << "; unknown method_name: " << method_name << "\n";
    }
  }

  GDBusInterfaceVTable const interface_vtable = {
    handle_method_call,
    0,
    0,
    0
  };

  void on_bus_acquired(GDBusConnection *connection,
		       gchar const */*name*/,
		       gpointer /*user_data*/)
  {
    guint registration_id;

    registration_id = g_dbus_connection_register_object(connection,
							"/org/freedesktop/Notifications",
							introspection_data->
							interfaces[0],
							&interface_vtable,
							NULL, NULL, NULL);

    if (registration_id <= 0) {
      fprintf(stderr, "Unable to register\n");
      exit(1);
    }
  }

  void on_name_acquired(GDBusConnection *connection,
			gchar const */*name*/,
			gpointer /*user_data*/)
  {
    dbus_conn = connection;
  }

  void on_name_lost(GDBusConnection * /*connection*/,
		    gchar const * /*name*/,
		    gpointer /*user_data*/)
  {
    fprintf(stderr, "Name Lost. Is Another notification daemon running?\n");
    exit(1);
  }

  int initdbus()
  {
    guint owner_id;

#if !GLIB_CHECK_VERSION(2,35,0)
    g_type_init();
#endif

    introspection_data = g_dbus_node_info_new_for_xml(introspection_xml,
						      NULL);

    owner_id = g_bus_own_name(G_BUS_TYPE_SESSION,
			      "org.freedesktop.Notifications",
			      G_BUS_NAME_OWNER_FLAGS_NONE,
			      on_bus_acquired,
			      on_name_acquired, on_name_lost, NULL, NULL);

    return owner_id;
  }
}

int main() {
  // Simple enough, initialize D-BUS (error codes don't matter, it's asynchronous anyway), start main loop.
  int owner_id = initdbus();
  GMainLoop *mainloop = g_main_loop_new(0,FALSE);
  g_main_loop_run(mainloop);
  g_bus_unown_name(owner_id);
}
