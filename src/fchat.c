#include "fchat.h"

static PurplePlugin *_fchat_protocol = NULL;

void fchat_debug_print_packet_blocks(FChatConnection *fchat_conn, FChatPacketBlocks *packet_blocks) {
	gchar *debug_msg = g_strdup_printf("packet blocks:\n"
			"protocol_version=%s\n"
			"nick_name=%s\n"
			"command=%s\n"
			"msg_id=%s\n"
			"msg_confirmation=%s\n"
			"msg=%s\n",
			packet_blocks->protocol_version, packet_blocks->alias, packet_blocks->command,
			packet_blocks->msg_id, packet_blocks->msg_confirmation, packet_blocks->msg);
	gchar *debug_msg_decoded = fchat_decode(fchat_conn, debug_msg, -1);
	g_free(debug_msg);
	purple_debug_info("fchat", "%s", debug_msg_decoded);
	g_free(debug_msg_decoded);
}


/*
 * prpl functions
 */
static const gchar *fchat_prpl_list_icon(PurpleAccount *account, PurpleBuddy *buddy) {
	return "fchat";
}

static gint fchat_prpl_send_im(PurpleConnection *gc, const gchar *who, const gchar *message, PurpleMessageFlags flags) {
	FChatConnection *fchat_conn = (FChatConnection *)gc->proto_data;
	FChatBuddy *buddy = fchat_find_buddy(fchat_conn, who, NULL, TRUE);
	gboolean msg_confirmation = purple_account_get_bool(gc->account, "confirm_private_msg", TRUE);
	fchat_send_msg_cmd(fchat_conn, buddy, message, FCHAT_MSG_TYPE_PRIVATE, msg_confirmation);
	return 1;
}


/*
 * prpl stuff. see prpl.h for more information.
 */

static PurplePluginProtocolInfo prpl_info = {
	OPT_PROTO_NO_PASSWORD | OPT_PROTO_REGISTER_NOSCREENNAME,
	NULL,
	NULL,
	NO_BUDDY_ICONS,
	fchat_prpl_list_icon,
	NULL,
	fchat_prpl_status_text,
	NULL,
	fchat_prpl_status_types,
	fchat_prpl_blist_node_menu,
	fchat_prpl_chat_info,
	fchat_prpl_chat_info_defaults,
	fchat_prpl_login,
	fchat_prpl_close,
	fchat_prpl_send_im,
	NULL,
	NULL,
	fchat_prpl_get_info,
	fchat_prpl_set_status,
	NULL,
	NULL,
	fchat_prpl_add_buddy,
	NULL,
	fchat_prpl_remove_buddy,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	fchat_prpl_chat_join,
	NULL,
	fchat_prpl_chat_get_name,
	NULL,
	NULL,
	NULL,
	fchat_prpl_chat_send,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	fchat_prpl_chat_get_roomlist,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	fchat_prpl_send_attention,
	fchat_prpl_get_attention_types,
	sizeof(PurplePluginProtocolInfo),
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

static void fchat_init(PurplePlugin *plugin) {
	// see accountopt.h for information about user splits and protocol options
	PurpleAccountOption *option;

	// Get charset from the system locale
	const gchar *charset;
	g_get_charset (&charset);
	option = purple_account_option_string_new(_("Codeset"), "codeset", charset);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_int_new(_("Port"), "port", FCHAT_DEFAULT_PORT);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_bool_new(_("Use broadcast"), "broadcast", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_bool_new(_("Accept unknown protocol version"), "accept_unknown_protocol_version", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_int_new(_("Keepalive interval"), "keepalive_timeout", 1);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_int_new(_("Keepalive periods"), "keepalive_periods", 3);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_bool_new(_("Require authorization for new contacts"), "require_authorization", FALSE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_bool_new(_("Confirm private messages"), "confirm_private_msg", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_bool_new(_("Confirm public messages"), "confirm_public_msg", FALSE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_bool_new(_("Allow beep"), "accept_beep", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	GList *buddy_list_privacy = NULL;
	PurpleKeyValuePair *kvp;
	kvp = g_new0(PurpleKeyValuePair, 1);
	kvp->key = g_strdup(_("Request"));
	kvp->value = g_strdup_printf("%d", FCHAT_BUDDIES_LIST_PRIVACY_REQUEST);
	buddy_list_privacy = g_list_append(buddy_list_privacy, kvp);
	kvp = g_new0(PurpleKeyValuePair, 1);
	kvp->key = g_strdup(_("Allow"));
	kvp->value = g_strdup_printf("%d", FCHAT_BUDDIES_LIST_PRIVACY_ALLOW);
	buddy_list_privacy = g_list_append(buddy_list_privacy, kvp);
	kvp = g_new0(PurpleKeyValuePair, 1);
	kvp->key = g_strdup(_("Deny"));
	kvp->value = g_strdup_printf("%d", FCHAT_BUDDIES_LIST_PRIVACY_DENY);
	buddy_list_privacy = g_list_append(buddy_list_privacy, kvp);

	option = purple_account_option_list_new(_("Buddy list privacy"), "buddy_list_privacy", buddy_list_privacy);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	_fchat_protocol = plugin;
}

gboolean fchat_prpl_load(PurplePlugin *plugin) {
	return TRUE;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_PROTOCOL,
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,
	FCHATPRPL_ID,
	"Friendly Chat",
	"1.2.0",
	N_("FChat protocol plugin"),
	N_("Friendly Chat works over local network via a lightweight UDP protocol. It's feature reach and don't need for a central server."),
	N_("Sergey Ponomarev <stokito@gmail.com>"),
	"https://github.com/stokito/fchat-pidgin",
	fchat_prpl_load,
	NULL,
	NULL,
	NULL,
	&prpl_info,
	NULL,
	fchat_prpl_actions,
	NULL,
	NULL,
	NULL,
	NULL,
};

PURPLE_INIT_PLUGIN(fchat, fchat_init, info);
