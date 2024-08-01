#include "fchat.h"

void fchat_send_packet(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	guint16 port = (guint16) purple_account_get_int(fchat_conn->gc->account, "port", FCHAT_DEFAULT_PORT);
	GSocketAddress *address = g_inet_socket_address_new(buddy->addr, port);
	GString *str = fchat_make_packet(packet_blocks);
	purple_debug_info("fchat", "Send packet to %s\n", buddy->host);
	fchat_debug_print_packet_blocks(fchat_conn, packet_blocks);
	GError *err = NULL;
	gssize sent = g_socket_send_to(fchat_conn->socket, address, str->str, str->len, NULL, &err);
	if (err) {
		purple_debug_error("fchat", "Error on sending a packet to %s :%s\n", buddy->host, err->message);
	} else if (sent < (gssize)str->len) {
		purple_debug_error("fchat", "Can't send a packet to %s\n", buddy->host);
	}
	g_string_free(str, TRUE);
}

GString *fchat_make_packet(FChatPacketBlocks *packet_blocks) {
	GString *packet = g_string_new("");
	FChatPacketBlocksVector packet_blocks_v = (FChatPacketBlocksVector)packet_blocks;
	for (int block_num = 0; block_num < FCHAT_BLOCKS_COUNT; block_num++) {
		const gchar *block_value = packet_blocks_v[block_num];
		if (block_value) {
			g_string_append_c_inline(packet, (gchar)FCHAT_SEPARATOR_BLOCK);
			const gchar block_key = fchat_blocks_order[block_num];
			g_string_append_c_inline(packet, block_key);
			g_string_append(packet, block_value);
		}
	}
	return packet;
}

static void fchat_reconnect(FChatConnection *fchat_conn, FChatBuddy *buddy) {
	fchat_send_disconnect_cmd(fchat_conn, buddy);
	fchat_send_connect_cmd(fchat_conn, buddy);
}

void fchat_send_connect_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy) {
	//[1][1]278[1][2]Alice[1][8]C
	FChatPacketBlocks *packet_blocks = g_new0(FChatPacketBlocks, 1);
	packet_blocks->protocol_version = g_strdup_printf("%d", fchat_conn->my_buddy->protocol_version);
	packet_blocks->alias = fchat_encode(fchat_conn, fchat_conn->my_buddy->alias, -1);
	gchar str_cmd = FCHAT_CONNECT_CMD;
	packet_blocks->command = g_strndup(&str_cmd, 1);
	fchat_send_packet(fchat_conn, buddy, packet_blocks);
	fchat_delete_packet_blocks(packet_blocks);
}

/**
 * Send ACK for connection
 * @param can_connect Do we allow to connect to us?
 */
void fchat_send_connect_confirm_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, gboolean can_connect) {
	//[1][1]278[1][2]Alice[1][8]F[1][11]000000000000000000000000D2656333[1][12]BFFD4ECCD63C9A8125646077EB91A1D3[1][255]Y
	FChatPacketBlocks *packet_blocks = g_new0(FChatPacketBlocks, 1);
	packet_blocks->protocol_version = g_strdup_printf("%d", fchat_conn->my_buddy->protocol_version);
	packet_blocks->alias = fchat_encode(fchat_conn, fchat_conn->my_buddy->alias, -1);
	gchar str_cmd = FCHAT_CONNECT_CONFIRM_CMD;
	packet_blocks->command = g_strndup(&str_cmd, 1);
	gchar str_msg = can_connect ? 'Y' : 'N';
	packet_blocks->msg = g_strndup(&str_msg, 1);
	fchat_send_packet(fchat_conn, buddy, packet_blocks);
	fchat_delete_packet_blocks(packet_blocks);
}

void fchat_send_disconnect_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy) {
	//[1][1]278[1][2]Alice[1][8]D
	FChatPacketBlocks *packet_blocks = g_new0(FChatPacketBlocks, 1);
	gchar str_cmd = FCHAT_DISCONNECT_CMD;
	packet_blocks->command = g_strndup(&str_cmd, 1);
	fchat_send_packet(fchat_conn, buddy, packet_blocks);
	fchat_delete_packet_blocks(packet_blocks);
}

/**
 * Send a PING packet to check the connection
 */
void fchat_send_ping_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy) {
	//[1][8]X
	FChatPacketBlocks *packet_blocks = g_new0(FChatPacketBlocks, 1);
	gchar str_cmd = FCHAT_PING_CMD;
	packet_blocks->command = g_strndup(&str_cmd, 1);
	fchat_send_packet(fchat_conn, buddy, packet_blocks);
	fchat_delete_packet_blocks(packet_blocks);
}

/**
 * Send a PONG packet to confirm that we are still online
 */
void fchat_send_pong_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy) {
	//[1][2]Alice[1][8]Y
	FChatPacketBlocks *packet_blocks = g_new0(FChatPacketBlocks, 1);
	gchar str_cmd = FCHAT_PONG_CMD;
	packet_blocks->command = g_strndup(&str_cmd, 1);
	fchat_send_packet(fchat_conn, buddy, packet_blocks);
	fchat_delete_packet_blocks(packet_blocks);
}

/**
 * Send a message
 * @param msg_confirmation Ask to send a confirmation that the message was received
 */
void fchat_send_msg_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, const gchar *msg_text,  FChatMsgType msg_type, gboolean msg_confirmation) {
	//[1][2]Alice[1][8]P[1][9]78[1][10][1][255]Hello  - personal
	//[1][2]Alice[1][8]M[1][9]134[1][255]bye guys     - public
	FChatPacketBlocks *packet_blocks = g_new0(FChatPacketBlocks, 1);
	gchar str_cmd = msg_type == FCHAT_MSG_TYPE_PRIVATE ? FCHAT_PRIVATE_MSG_CMD : FCHAT_MSG_CMD;
	packet_blocks->command = g_strndup(&str_cmd, 1);
	// Each message has own id that is used in reply for delivery.
	// The classic FChat uses just incremental id.
	packet_blocks->msg_id = g_strdup_printf("%d", fchat_conn->next_id++);
	packet_blocks->msg_confirmation = g_strdup(msg_confirmation ? "\0" : NULL); // Если требуется подтверждение о доставке
	packet_blocks->msg = fchat_encode(fchat_conn, msg_text, -1);
	fchat_send_packet(fchat_conn, buddy, packet_blocks);
	fchat_delete_packet_blocks(packet_blocks);
}

void fchat_send_confirm_msg_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, gchar *msg_id) {
	//[1][2]Alice[1][8]O[1][255]36
	FChatPacketBlocks *packet_blocks = g_new0(FChatPacketBlocks, 1);
	gchar str_cmd = FCHAT_CONFIRM_MSG_CMD;
	packet_blocks->command = g_strndup(&str_cmd, 1);
	packet_blocks->msg = g_strdup(msg_id);
	fchat_send_packet(fchat_conn, buddy, packet_blocks);
	fchat_delete_packet_blocks(packet_blocks);
}

void fchat_send_change_alias_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, const gchar *new_nickname) {
	//[1][2]Alice[1][8]e[1][255]Adelaide[255]
	FChatPacketBlocks *packet_blocks = g_new0(FChatPacketBlocks, 1);
	gchar str_cmd = FCHAT_ALIAS_CHANGE_CMD;
	packet_blocks->command = g_strndup(&str_cmd, 1);
	packet_blocks->msg = fchat_encode(fchat_conn, new_nickname, -1);
	fchat_send_packet(fchat_conn, buddy, packet_blocks);
	fchat_delete_packet_blocks(packet_blocks);
}


/**
 * Send my status (auto responder) to the buddy e.g. when the status was changed.
 */
void fchat_send_status_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, PurpleStatus *status) {
	//[1][2]Alice[1][8]a[1][255]State:WarCraft;Y[1899-12-30 01:42:15][13][10]WarCraft
	/*	State:Busy;Y[1899-12-30 00:01:59]
		State:Busy;N
		State:;N */

	const gchar *status_id = purple_status_get_id(status);
	const gchar *fchat_status;
	if (g_strcmp0(status_id, FCHAT_STATUS_ONLINE) == 0) {
		fchat_status = "State:;N";
	} else if (g_strcmp0(status_id, FCHAT_STATUS_BUSY) == 0) {
		fchat_status = "State:Busy;Y";
	} else if (g_strcmp0(status_id, FCHAT_STATUS_PHONE) == 0) {
		fchat_status = "State:Phone;Y";
	} else if (g_strcmp0(status_id, FCHAT_STATUS_MUSIC) == 0) {
		fchat_status = "State:Music;Y";
	} else if (g_strcmp0(status_id, FCHAT_STATUS_AWAY) == 0) {
		fchat_status = "State:Away;Y";
	} else {
		purple_debug_warning("fchat", "Status %s can't be sent\n", status_id);
		return;
	}
	gchar *msg_block;
	const gchar *status_msg_utf8 = purple_status_get_attr_string(status, "message");
	if (status_msg_utf8) {
		gchar *fchat_status_msg = fchat_encode(fchat_conn, status_msg_utf8, -1);
		msg_block = g_strdup_printf("%s\n\r%s", fchat_status, fchat_status_msg);
		g_free(fchat_status_msg);
	} else {
		msg_block = g_strdup(fchat_status);
	}

	FChatPacketBlocks *packet_blocks = g_new0(FChatPacketBlocks, 1);
	gchar str_cmd = FCHAT_STATUS_CMD;
	packet_blocks->command = g_strndup(&str_cmd, 1);
	packet_blocks->msg = msg_block;
	fchat_send_packet(fchat_conn, buddy, packet_blocks);
	fchat_delete_packet_blocks(packet_blocks);
}


void fchat_send_beep_reply_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy,  FChatBeepReply reply) {
	//[1][2]Alice[1][8]N[1][255]1
	// '0' - Signal are not accepted from us
	// '1' - Signal are not accepted from anybody
	// Empty means that the signal was accepted

	gchar *msg_beep_reply;
	if (reply == FCHAT_BEEP_ACCEPTED) {
		msg_beep_reply = NULL;
	} else if (reply == FCHAT_BEEP_DENIED_FROM_YOU) {
		msg_beep_reply = "0";
	} else  { // reply = BEEP_DENIED_FROM_ALL
		msg_beep_reply = "1";
	}

	FChatPacketBlocks *packet_blocks = g_new0(FChatPacketBlocks, 1);
	gchar str_cmd = FCHAT_BEEP_REPLY_CMD;
	packet_blocks->command = g_strndup(&str_cmd, 1);
	packet_blocks->msg = g_strndup(msg_beep_reply, 1);
	fchat_send_packet(fchat_conn, buddy, packet_blocks);
	fchat_delete_packet_blocks(packet_blocks);
}

void fchat_send_beep_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy) {
	//[1][2]Alice[1][8]B
	FChatPacketBlocks *packet_blocks = g_new0(FChatPacketBlocks, 1);
	gchar str_cmd = FCHAT_BEEP_CMD;
	packet_blocks->command = g_strndup(&str_cmd, 1);
	fchat_send_packet(fchat_conn, buddy, packet_blocks);
	fchat_delete_packet_blocks(packet_blocks);
}

void fchat_send_get_buddy_info_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy) {
	//[1][2]Alice[1][8]u
	FChatPacketBlocks *packet_blocks = g_new0(FChatPacketBlocks, 1);
	gchar str_cmd = FCHAT_GET_BUDDY_INFO_CMD;
	packet_blocks->command = g_strndup(&str_cmd, 1);
	fchat_send_packet(fchat_conn, buddy, packet_blocks);
	fchat_delete_packet_blocks(packet_blocks);
}

void fchat_send_my_buddy_info_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy) {
	//[1][2]Alice[1][8]U[1][255]FChatVersion[2]4.6.1[2]FullName[2]Alice Doe[2]Female[2]1[2]Day[2]23[2]Month[2]12[2]Year[2]2000
	const gchar *codeset = fchat_get_connection_codeset(fchat_conn);
	FChatPacketBlocks *packet_blocks = g_new0(FChatPacketBlocks, 1);
	gchar str_cmd = FCHAT_BUDDY_INFO_CMD;
	packet_blocks->command = g_strndup(&str_cmd, 1);
	packet_blocks->msg = fchat_buddy_info_serialize(fchat_conn->my_buddy->info, codeset);
	fchat_send_packet(fchat_conn, buddy, packet_blocks);
	fchat_delete_packet_blocks(packet_blocks);
}

void fchat_send_get_msg_board_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy) {
	FChatPacketBlocks *packet_blocks = g_new0(FChatPacketBlocks, 1);
	gchar str_cmd = FCHAT_GET_MSG_BOARD_CMD;
	packet_blocks->command = g_strndup(&str_cmd, 1);
	fchat_send_packet(fchat_conn, buddy, packet_blocks);
	fchat_delete_packet_blocks(packet_blocks);
}

void fchat_send_msg_board_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy) {
	if (!fchat_conn->my_buddy->msg_board) {
		return;
	}
	FChatPacketBlocks *packet_blocks = g_new0(FChatPacketBlocks, 1);
	gchar str_cmd = FCHAT_MSG_BOARD_CMD;
	packet_blocks->command = g_strndup(&str_cmd, 1);
	packet_blocks->msg = fchat_encode(fchat_conn, fchat_conn->my_buddy->msg_board, -1);
	fchat_send_packet(fchat_conn, buddy, packet_blocks);
	fchat_delete_packet_blocks(packet_blocks);
}


void fchat_send_get_buddies(FChatConnection *fchat_conn, FChatBuddy *buddy) {
	FChatPacketBlocks *packet_blocks = g_new0(FChatPacketBlocks, 1);
	gchar str_cmd = FCHAT_GET_BUDDY_LIST_CMD;
	packet_blocks->command = g_strndup(&str_cmd, 1);
	fchat_send_packet(fchat_conn, buddy, packet_blocks);
	fchat_delete_packet_blocks(packet_blocks);
}

/**
 * Send buddies list for requester buddy.
 * @buddies: NULL terminated array of buddies.
 */
void fchat_send_buddies(FChatConnection *fchat_conn, FChatBuddy *buddy, GHashTable *buddies) {
	/* Format: msg = 192.168.56.1192.168.56.1host10.0.2.2хост 10.111.28.16010.111.28.160
	 * msg = "1" - means that buddy list is denied  */
	gchar *msg;
	if (buddies) {
		guint size = g_hash_table_size(buddies);
		/**
		 * buddies_v is NULL terminated string vector
		 * "2 * size" because on one alias (separator) host
		 * "size + 1" because one last element is for NULL
		 */
		gchar **buddies_v = g_new0(gchar *, 2 * size + 1);
		gint i = 0;
		GHashTableIter iter;
		gchar *key;
		FChatBuddy *value;
		g_hash_table_iter_init(&iter, buddies);
		while (g_hash_table_iter_next(&iter, (gpointer)&key, (gpointer)&value)) {
			purple_debug_info("fchat", "buddy alias=%s buddy host =%s\n", value->alias, value->host);
			buddies_v[i] = value->alias ? value->alias : value->host;
			i++;
			buddies_v[i] = value->host;
			i++;
		}
		buddies_v[i] = NULL;
		msg = g_strjoinv(FCHAT_BUDDY_LIST_SEPARATOR, buddies_v);
		g_free(buddies_v);
		purple_debug_info("fchat", "buddies_v=%s\n", msg);
	} else {
		msg = g_strdup("1");
	}
	FChatPacketBlocks *packet_blocks = g_new0(FChatPacketBlocks, 1);
	gchar str_cmd = FCHAT_BUDDY_LIST_CMD;
	packet_blocks->command = g_strndup(&str_cmd, 1);
	packet_blocks->msg = msg;
	fchat_send_packet(fchat_conn, buddy, packet_blocks);
	fchat_delete_packet_blocks(packet_blocks);
}
