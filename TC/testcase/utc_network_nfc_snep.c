/*
 * Copyright (c) 2012, 2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <glib.h>
#include <tet_api.h>
#include <nfc.h>

enum
{
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void nfc_snep_register_server_p(void);
static void nfc_snep_register_server_n(void);
static void nfc_snep_unregister_server_p(void);
static void nfc_snep_unregister_server_n(void);

struct tet_testlist tet_testlist[] =
{
	{ nfc_snep_register_server_p, POSITIVE_TC_IDX },
	{ nfc_snep_register_server_n, NEGATIVE_TC_IDX },
	{ nfc_snep_unregister_server_p, POSITIVE_TC_IDX },
	{ nfc_snep_unregister_server_n, NEGATIVE_TC_IDX },

	{ NULL, 0 },
};

static void _activation_completed_cb(nfc_error_e error, void *user_data)
{
	g_main_loop_quit((GMainLoop *)user_data);
}

//this method is called only once in start
static void startup(void)
{
	/* start of TC */
	nfc_manager_initialize(NULL, NULL);

	if (nfc_manager_is_activated() == false) {
		GMainLoop *loop;

		loop = g_main_loop_new(NULL, false);

		nfc_manager_set_activation(true, _activation_completed_cb, loop);

		g_main_loop_run(loop);
	}
}

static void cleanup(void)
{
	/* end of TC */
	nfc_manager_deinitialize();
}

static const char *san = "abcd:efgh";
static int sap = 0x22;

static void _snep_event_cb(nfc_p2p_snep_h handle,
	nfc_snep_event_e event, nfc_error_e result,
	nfc_ndef_message_h msg, void *user_data)
{

}

static void nfc_snep_register_server_p(void)
{
	int ret;

	ret = nfc_snep_register_server(san, sap, _snep_event_cb, NULL);
	if (ret == NFC_ERROR_NONE) {
		nfc_snep_unregister_server(san, sap);
	}

	dts_check_eq(__func__, ret, NFC_ERROR_NONE, "nfc_snep_register_server is failed");
}

static void nfc_snep_register_server_n(void)
{
	int ret;

	ret = nfc_snep_register_server(NULL, 0, NULL, NULL);

	dts_check_ne(__func__, ret, NFC_ERROR_NONE, "nfc_snep_register_server not allowed");
}

static void nfc_snep_unregister_server_p(void)
{
	int ret;

	ret = nfc_snep_register_server(san, sap, _snep_event_cb, NULL);
	if (ret == NFC_ERROR_NONE) {
		ret = nfc_snep_unregister_server(san, sap);
	}

	dts_check_eq(__func__, ret, NFC_ERROR_NONE, "nfc_snep_register_server is failed");
}

static void nfc_snep_unregister_server_n(void)
{
	int ret;

	ret = nfc_snep_unregister_server(san, sap);

	dts_check_eq(__func__, ret, NFC_ERROR_NONE, "nfc_snep_register_server is failed");
}
