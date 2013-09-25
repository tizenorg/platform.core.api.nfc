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

static void nfc_se_open_secure_element_p(void);
static void nfc_se_open_secure_element_n(void);
static void nfc_se_get_atr_p(void);
static void nfc_se_get_atr_n(void);
static void nfc_se_send_apdu_p(void);
static void nfc_se_send_apdu_n(void);
static void nfc_se_close_secure_element_p(void);
static void nfc_se_close_secure_element_n(void);

struct tet_testlist tet_testlist[] =
{
	{ nfc_se_open_secure_element_p, POSITIVE_TC_IDX },
	{ nfc_se_open_secure_element_n, NEGATIVE_TC_IDX },
	{ nfc_se_get_atr_p, POSITIVE_TC_IDX },
	{ nfc_se_get_atr_n, NEGATIVE_TC_IDX },
	{ nfc_se_send_apdu_p, POSITIVE_TC_IDX },
	{ nfc_se_send_apdu_n, NEGATIVE_TC_IDX },
	{ nfc_se_close_secure_element_p, POSITIVE_TC_IDX },
	{ nfc_se_close_secure_element_n, NEGATIVE_TC_IDX },

	{ NULL, 0 },
};

//this method is called only once in start
static void startup(void)
{
	/* start of TC */
	nfc_manager_initialize(NULL, NULL);
}

static void cleanup(void)
{
	/* end of TC */
	nfc_manager_deinitialize();
}

static nfc_se_h se_handle;

static void nfc_se_open_secure_element_p(void)
{
	int ret;

	ret = nfc_se_open_secure_element(NFC_SE_TYPE_ESE, &se_handle);

	dts_check_eq(__func__, ret, NFC_ERROR_NONE, "nfc_se_open_secure_element is failed");
}

static void nfc_se_open_secure_element_n(void)
{
	int ret;

	ret = nfc_se_open_secure_element((nfc_se_type_e)0xff, NULL);

	dts_check_ne(__func__, ret, NFC_ERROR_NONE, "nfc_se_open_secure_element not allow null");
}

static void nfc_se_get_atr_p(void)
{
	int ret;
	unsigned char *atr;
	unsigned int atr_len;

	ret = nfc_se_get_atr(se_handle, &atr, &atr_len);
	if (ret == NFC_ERROR_NONE) {
		free(atr);
	}

	dts_check_eq(__func__, ret, NFC_ERROR_NONE, "nfc_se_get_atr is failed");
}

static void nfc_se_get_atr_n(void)
{
	int ret;

	ret = nfc_se_get_atr(NULL, NULL, NULL);

	dts_check_ne(__func__, ret, NFC_ERROR_NONE, "nfc_se_get_atr not allow null");
}

static void nfc_se_send_apdu_p(void)
{
	int ret;
	unsigned char cmd[] = { 0x00, 0x70, 0x00, 0x00, 0x00 };
	unsigned char *resp;
	unsigned int resp_len;

	ret = nfc_se_send_apdu(se_handle, cmd, sizeof(cmd), &resp, &resp_len);
	if (ret == NFC_ERROR_NONE) {
		free(resp);
	}

	dts_check_eq(__func__, ret, NFC_ERROR_NONE, "nfc_se_send_apdu is failed");
}

static void nfc_se_send_apdu_n(void)
{
	int ret;

	ret = nfc_se_send_apdu(NULL, NULL, 0, NULL, NULL);

	dts_check_ne(__func__, ret, NFC_ERROR_NONE, "nfc_se_send_apdu not allow null");
}

static void nfc_se_close_secure_element_p(void)
{
	int ret;

	ret = nfc_se_close_secure_element(se_handle);

	dts_check_eq(__func__, ret, NFC_ERROR_NONE, "nfc_se_close_secure_element is failed");
}

static void nfc_se_close_secure_element_n(void)
{
	int ret;

	ret = nfc_se_close_secure_element(NULL);

	dts_check_ne(__func__, ret, NFC_ERROR_NONE, "nfc_se_close_secure_element not allow null");
}
