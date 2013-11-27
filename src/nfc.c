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

#include <unistd.h>
#include <glib.h>
#include <gio/gio.h>

#include <vconf.h>
#include <Ecore_X.h>
#include <net_nfc_typedef_internal.h>
#include "net_nfc_util_ndef_record.h"

#include "nfc_internal.h"


#define TIZEN_APP_RECORD_TYPE "tizen.org:app"

#define LOG_TAG "CAPI_NETWORK_NFC"
#include <dlog.h>

/**
 * @brief The default factory key.
 * @details The key is 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
 * @ingroup CAPI_NETWORK_NFC_TAG_MIFARE_MODULE
 */
const unsigned char NFC_TAG_MIFARE_KEY_DEFAULT[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/**
 * @brief The well-known key for tags formatted according to the MIFARE Application Directory (MAD) specification.
 * @details The key is 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5
 * @ingroup CAPI_NETWORK_NFC_TAG_MIFARE_MODULE
 */
const unsigned char NFC_TAG_MIFARE_KEY_APPLICATION_DIRECTORY[6] = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5};

/**
 * @brief The well-known key for tags formatted according to the NDEF on Mifare Classic specification.
 * @details The key is 0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7
 * @ingroup CAPI_NETWORK_NFC_TAG_MIFARE_MODULE
 */
const unsigned char NFC_TAG_MIFARE_KEY_NFC_FORUM[6] = {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7};


/**
 * @brief RTD(Record type definition) Type - Smart Poster type.
 * @ingroup CAPI_NETWORK_NFC_NDEF_RECORD_MODULE
 */
const unsigned char NFC_RECORD_SMART_POSTER_TYPE[2] = { 'S', 'p' };

/**
 * @brief  RTD(Record type definition) Type - Text type.
 * @ingroup CAPI_NETWORK_NFC_NDEF_RECORD_MODULE
 */
const unsigned char NFC_RECORD_TEXT_TYPE[1] = { 'T' };
/**
 * @brief  RTD(Record type definition) Type - URI type.
 * @ingroup CAPI_NETWORK_NFC_NDEF_RECORD_MODULE
 */
const unsigned char NFC_RECORD_URI_TYPE[1] = { 'U' };
/**
 * @brief  RTD(Record type definition) Type - Alternative Carrier type.
 * @ingroup CAPI_NETWORK_NFC_NDEF_RECORD_MODULE
 */
const unsigned char NFC_RECORD_ALTERNATIVE_CARRIER_TYPE[2] = { 'a','c' };
/**
 * @brief  RTD(Record type definition) Type - Handover Carrier type.
 * @ingroup CAPI_NETWORK_NFC_NDEF_RECORD_MODULE
 */
const unsigned char NFC_RECORD_HANDOVER_CARRIER_TYPE[2] = { 'H','c' };
/**
 * @brief  RTD(Record type definition) Type - Handover Request type.
 * @ingroup CAPI_NETWORK_NFC_NDEF_RECORD_MODULE
 */
const unsigned char NFC_RECORD_HANDOVER_REQUEST_TYPE[2] = { 'H','r' };
/**
 * @brief  RTD(Record type definition) Type - Handover Select type.
 * @ingroup CAPI_NETWORK_NFC_NDEF_RECORD_MODULE
 */
const unsigned char NFC_RECORD_HANDOVER_SELECT_TYPE[2] = { 'H','s' };


static _nfc_context_s gdbus_nfc_context;

/********************************Utility functions***********************/

static pid_t _get_focus_app_pid()
{
	Ecore_X_Window focus;
	pid_t pid;

	ecore_x_init(NULL);

	focus = ecore_x_window_focus_get();
	if (ecore_x_netwm_pid_get(focus, &pid))
		return pid;

	return -1;
}

static bool _check_app_permission()
{
	pid_t focus_app_pid, current_app_pid;

	focus_app_pid = _get_focus_app_pid();
	current_app_pid = getpgid(getpid());

	LOGD("[check app permission] focus_app_pid [%d], current_app_pid [%d]",
			focus_app_pid, current_app_pid);

	if (-1 == current_app_pid)
		return false;
	else
		return (focus_app_pid == current_app_pid) ? true : false;
}

static int _return_invalid_param(const char *func)
{
	LOGE("INVALID_PARAMETER : %s", func);

	return NFC_ERROR_INVALID_PARAMETER;
}

static int _convert_error_code(const char *func, int native_error_code)
{
	int error_code = NFC_ERROR_NONE;
	char *errorstr = NULL;

	switch (native_error_code)
	{
	case NET_NFC_OK:
		error_code = NFC_ERROR_NONE;
		errorstr = "ERROR_NONE";
		break;

	case NET_NFC_ALLOC_FAIL:
		error_code = NFC_ERROR_OUT_OF_MEMORY;
		errorstr = "OUT_OF_MEMORY";
		break;

	case NET_NFC_NOT_CONNECTED:
		error_code = NFC_ERROR_NO_DEVICE;
		errorstr = "NO_DEVICE";
		break;

	case NET_NFC_UNKNOWN_ERROR:
	case NET_NFC_THREAD_CREATE_FAIL:
	case NET_NFC_INVALID_STATE:
	case NET_NFC_IPC_FAIL:
	case NET_NFC_BUFFER_TOO_SMALL:
	case NET_NFC_COMMUNICATE_WITH_CONTROLLER_FAILED:
	case NET_NFC_RF_ERROR:
	case NET_NFC_NOT_SUPPORTED:
	case NET_NFC_TAG_READ_FAILED:
	case NET_NFC_TAG_WRITE_FAILED:
	case NET_NFC_OPERATION_FAIL:
	case NET_NFC_SECURITY_FAIL:
	case NET_NFC_INSUFFICIENT_STORAGE:
	case NET_NFC_NOT_INITIALIZED:
	case NET_NFC_NOT_REGISTERED:
		error_code = NFC_ERROR_OPERATION_FAILED;
		errorstr = "OPERATION_FAILED";
		break;

	case NET_NFC_OUT_OF_BOUND:
	case NET_NFC_NULL_PARAMETER:
	case NET_NFC_NOT_ALLOWED_OPERATION:
	case NET_NFC_LLCP_INVALID_SOCKET:
	case NET_NFC_NO_DATA_FOUND:
		error_code = NFC_ERROR_INVALID_PARAMETER;
		errorstr = "INVALID_PARAMETER";
		break;
	case NET_NFC_NDEF_RECORD_IS_NOT_EXPECTED_TYPE:
		error_code = NFC_ERROR_INVALID_RECORD_TYPE;
		errorstr = "INVALID_RECORD_TYPE";
		break;

	case NET_NFC_ALREADY_INITIALIZED:
	case NET_NFC_ALREADY_REGISTERED:
		error_code = NFC_ERROR_NONE;
		errorstr = "ERROR_NONE";
		break;

	case NET_NFC_RF_TIMEOUT:
		error_code = NFC_ERROR_TIMED_OUT;
		errorstr = "TIMED_OUT";
		break;
	case NET_NFC_INVALID_FORMAT:
	case NET_NFC_NDEF_TYPE_LENGTH_IS_NOT_OK:
	case NET_NFC_NDEF_ID_LENGTH_IS_NOT_OK:
	case NET_NFC_NDEF_BUF_END_WITHOUT_ME:
		error_code = NFC_ERROR_INVALID_NDEF_MESSAGE;
		errorstr = "INVALID_NDEF_MESSAGE";
		break;
	case NET_NFC_NO_NDEF_MESSAGE:
		error_code = NFC_ERROR_NO_NDEF_MESSAGE;
		errorstr = "NO_NDEF_MESSAGE";
		break;
	case NET_NFC_BUSY :
		error_code = NFC_ERROR_DEVICE_BUSY;
		errorstr = "DEVICE_BUSY";
		break;
	case NET_NFC_NO_NDEF_SUPPORT:
		error_code = NFC_ERROR_NOT_NDEF_FORMAT;
		errorstr = "NOT_SUPPORTED";
		break;
	default :
		error_code = NFC_ERROR_OPERATION_FAILED;
		errorstr = "OPERATION_FAILED";
	}

	LOGE("NFC %s(0x%08x)", errorstr, error_code);

	return error_code;
}

static bool _is_initialized()
{
	return gdbus_nfc_context.initialized;
}

/**********************Activate/DeActivate*****************/

bool nfc_manager_is_supported(void)
{
	int nfc_supported = 0;

	net_nfc_client_is_nfc_supported(&nfc_supported);

	return (!!nfc_supported);
}

static void _activation_changed(net_nfc_error_e result, void *user_data)
{
	nfc_activation_completed_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		callback(_convert_error_code(__func__, result), user_param);
	}

	g_variant_unref((GVariant *)user_data);
}

int nfc_manager_set_activation(bool activation,
		nfc_activation_completed_cb callback,
		void *user_data)
{
	int ret;
	int nfc_supported = 0;
#if 0
	unsigned int state;
#endif
	ret = net_nfc_client_is_nfc_supported(&nfc_supported);
	if (ret != NET_NFC_OK || nfc_supported == false)
	{
		LOGE("NFC not supported");

		return NFC_ERROR_NOT_SUPPORTED;
	}
#if 0
	ret = net_nfc_client_manager_get_server_state_sync(&state);
	if (ret == NFC_ERROR_NONE && state != 0)
	{
		LOGE("nfc_manager_check_activation BUSY!!!!!");

		return NFC_ERROR_DEVICE_BUSY;
	}
#endif
	if (nfc_manager_is_activated() == activation)
	{
		if (activation)
		{
			ret = NFC_ERROR_ALREADY_ACTIVATED;
		}
		else
		{
			ret = NFC_ERROR_ALREADY_DEACTIVATED;
		}
	}
	else
	{
		GVariant *parameter;

		parameter = g_variant_new("(uu)", callback, user_data);
		if (parameter != NULL)
		{
			ret = net_nfc_client_manager_set_active(activation,
					_activation_changed,
					parameter);
			if (ret != NET_NFC_OK)
			{
				LOGE("net_nfc_client_manager_set_active fail");

				g_variant_unref(parameter);
			}
		}
		else
		{
			ret = NET_NFC_ALLOC_FAIL;
		}

		ret = _convert_error_code(__func__, ret);
	}

	return ret;
}

bool nfc_manager_is_activated(void)
{
	int activated = 0;

	net_nfc_client_get_nfc_state(&activated);

	return (!!activated);
}

int nfc_manager_set_activation_changed_cb(nfc_activation_changed_cb callback,
		void *user_data)
{
	if (callback == NULL)
		return _return_invalid_param(__func__);

	net_nfc_client_manager_set_activated(callback, user_data);

	return NFC_ERROR_NONE;
}

void nfc_manager_unset_activation_changed_cb(void)
{
	net_nfc_client_manager_unset_activated();
}

#if 0
void _net_nfc_manager_get_current_target_handle_(
		net_nfc_error_e result,
		net_nfc_target_handle_s *handle,
		void *user_data)
{
	gdbus_nfc_context.current_target =(net_nfc_target_handle_s*) handle;

	nfc_initialize_completed_cb cb =
		gdbus_nfc_context.on_initialize_completed_cb;
	gdbus_nfc_context.on_initialize_completed_cb = NULL;
	if( cb )
	{
		cb( result, user_data );
	}

}

void _net_nfc_manager_get_current_tag_info_(
		net_nfc_error_e result,
		net_nfc_target_info_s *info,
		void *user_data)
{
	if (info != NULL)
	{
		if (gdbus_nfc_context.current_tag != NULL)
		{
			net_nfc_release_tag_info(gdbus_nfc_context.current_tag);
			gdbus_nfc_context.current_tag = NULL;
		}

		net_nfc_duplicate_target_info(info, &gdbus_nfc_context.current_tag);
	}

	nfc_initialize_completed_cb cb =
		gdbus_nfc_context.on_initialize_completed_cb;
	gdbus_nfc_context.on_initialize_completed_cb = NULL;
	if (cb)
	{
		cb(result, user_data);
	}
}


void _net_nfc_manager_tag_connected_cb(
		net_nfc_error_e result,
		net_nfc_target_type_e dev_type,
		void *user_data)
{
	net_nfc_target_type_e  devType = dev_type;
	int capi_result;

	if( (devType == NET_NFC_NFCIP1_TARGET )||
			(devType == NET_NFC_NFCIP1_INITIATOR ))
	{
		net_nfc_client_tag_get_current_target_handle(
				_net_nfc_manager_get_current_target_handle_,
				user_data);
	}
	else if( (devType > NET_NFC_UNKNOWN_TARGET )&&
			(devType < NET_NFC_NFCIP1_TARGET ))
	{
		net_nfc_client_tag_get_current_tag_info(
				_net_nfc_manager_get_current_tag_info_,
				user_data);
	}
	else
	{
		if (result == NET_NFC_NOT_CONNECTED)
		{
			capi_result = NFC_ERROR_NONE;
		}

		if( gdbus_nfc_context.on_initialize_completed_cb )
		{
			nfc_initialize_completed_cb cb =
				gdbus_nfc_context.on_initialize_completed_cb;
			gdbus_nfc_context.on_initialize_completed_cb = NULL;
			cb( capi_result,user_data );
		}
	}
}
#endif

static void _nfc_manager_tag_discovered_cb(net_nfc_target_info_s *info,
		void *user_data)
{
	gdbus_nfc_context.current_tag = info;

	if (gdbus_nfc_context.on_tag_discovered_cb != NULL) {
		gdbus_nfc_context.on_tag_discovered_cb(
				NFC_DISCOVERED_TYPE_ATTACHED,
				gdbus_nfc_context.current_tag,
				gdbus_nfc_context.on_tag_discovered_user_data);
	}

	/* ndef discovered cb */
	if (gdbus_nfc_context.on_ndef_discovered_cb &&
			info->raw_data.buffer != NULL)
	{
		ndef_message_s *ndef_message;

		net_nfc_create_ndef_message_from_rawdata(&ndef_message, &(info->raw_data));

		gdbus_nfc_context.on_ndef_discovered_cb(ndef_message,
				gdbus_nfc_context.on_ndef_discovered_user_data);

		net_nfc_free_ndef_message(ndef_message);
	}
}

static void _nfc_manager_tag_detached_cb(void *user_data)
{
	if (gdbus_nfc_context.on_tag_discovered_cb != NULL) {
		gdbus_nfc_context.on_tag_discovered_cb(
				NFC_DISCOVERED_TYPE_DETACHED,
				gdbus_nfc_context.current_tag,
				gdbus_nfc_context.on_tag_discovered_user_data);
	}

	gdbus_nfc_context.current_tag = NULL;
}
static void _p2p_target_discovered_cb(
		net_nfc_target_handle_s *handle_info,
		void *user_data)
{
	gdbus_nfc_context.current_target = handle_info;

	if (gdbus_nfc_context.on_p2p_target_discovered_cb != NULL) {
		gdbus_nfc_context.on_p2p_target_discovered_cb(
				NFC_DISCOVERED_TYPE_ATTACHED,
				(nfc_p2p_target_h)gdbus_nfc_context.current_target,
				gdbus_nfc_context.on_p2p_target_discovered_user_data);
	}
}

static void _p2p_device_detached(void *user_data)
{
	nfc_p2p_target_h handle = (nfc_p2p_target_h)gdbus_nfc_context.current_target;

	if (gdbus_nfc_context.on_p2p_target_discovered_cb != NULL) {
		gdbus_nfc_context.on_p2p_target_discovered_cb(
				NFC_DISCOVERED_TYPE_DETACHED,
				handle,
				gdbus_nfc_context.on_p2p_target_discovered_user_data);
	}

	/* unset data_received callback */
	nfc_p2p_unset_data_received_cb(handle);

	gdbus_nfc_context.current_target = NULL;
}

static void _se_event_cb(net_nfc_message_e message, void *user_data)
{
	if (gdbus_nfc_context.on_se_event_cb != NULL) {
		if(message == NET_NFC_MESSAGE_SE_CARD_EMULATION_CHANGED)
			gdbus_nfc_context.on_se_event_cb(
				NFC_SE_EVENT_CARD_EMULATION_CHANGED,
				gdbus_nfc_context.on_se_event_user_data);

		else if(message == NET_NFC_MESSAGE_SE_TYPE_CHANGED)
			gdbus_nfc_context.on_se_event_cb(
				NFC_SE_EVENT_SE_TYPE_CHANGED,
				gdbus_nfc_context.on_se_event_user_data);
	}
}

/* deprecated */
static gboolean _initialize_cb(gpointer user_data)
{
	nfc_initialize_completed_cb callback;
	void *user_param;

	if (user_data == NULL)
		return false;

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		callback(NFC_ERROR_NONE, user_param);
	}

	g_variant_unref((GVariant *)user_data);

	return false;
}

int nfc_manager_initialize(nfc_initialize_completed_cb callback, void *user_data)
{
	int ret;
	int nfc_supported = 0;

	net_nfc_client_is_nfc_supported(&nfc_supported);
	if (!nfc_supported)
	{
		LOGE("NFC_ERROR_NOT_SUPPORTED");

		return NFC_ERROR_NOT_SUPPORTED;
	}

	if (!_is_initialized())
	{
		ret = net_nfc_client_initialize();
		if (ret != NET_NFC_OK)
			return _convert_error_code(__func__, ret);

		memset(&gdbus_nfc_context, 0, sizeof(gdbus_nfc_context));

		gdbus_nfc_context.initialized = true;

		net_nfc_client_tag_set_tag_detached(_nfc_manager_tag_detached_cb, NULL);
		net_nfc_client_tag_set_tag_discovered(_nfc_manager_tag_discovered_cb, NULL);

		net_nfc_client_p2p_set_device_discovered(_p2p_target_discovered_cb, NULL);

		net_nfc_client_p2p_set_device_detached(_p2p_device_detached, NULL);
		net_nfc_client_se_set_event_cb(_se_event_cb, NULL);

		/* invoke callback if it needs */
		if (callback != NULL) {
			g_main_context_invoke(NULL,
					_initialize_cb,
					g_variant_new("(uu)", callback, user_data));
		}
#if 0
		gdbus_nfc_context.on_initialize_completed_cb = callback;

		ret = net_nfc_client_tag_is_tag_connected(
				_net_nfc_manager_tag_connected_cb,
				user_data);

		if( ret != NET_NFC_OK )
			return _convert_error_code(__func__, ret);
#endif
	}

	return NFC_ERROR_NONE;
}

int nfc_manager_initialize_sync()
{
	int ret;
	int nfc_supported = 0;

	net_nfc_client_is_nfc_supported(&nfc_supported);
	if (!nfc_supported)
	{
		LOGE("NFC_ERROR_NOT_SUPPORTED");

		return NFC_ERROR_NOT_SUPPORTED;
	}

	if (!_is_initialized())
	{
#if 0
		net_nfc_target_type_e devType;
#endif
		memset(&gdbus_nfc_context, 0, sizeof(gdbus_nfc_context));

		ret = net_nfc_client_initialize();
		if (ret != NET_NFC_OK)
			return _convert_error_code(__func__, ret);

		gdbus_nfc_context.initialized = true;

		net_nfc_client_tag_set_tag_detached(_nfc_manager_tag_detached_cb, NULL);
		net_nfc_client_tag_set_tag_discovered(_nfc_manager_tag_discovered_cb, NULL);

		net_nfc_client_p2p_set_device_discovered(_p2p_target_discovered_cb, NULL);

		net_nfc_client_p2p_set_device_detached(_p2p_device_detached, NULL);
		net_nfc_client_se_set_event_cb(_se_event_cb, NULL);
#if 0
		ret = net_nfc_client_tag_is_tag_connected_sync(&devType);
		if (ret == NET_NFC_OK)
		{
			if ((devType == NET_NFC_NFCIP1_TARGET) ||
					(devType == NET_NFC_NFCIP1_INITIATOR))
			{
				net_nfc_client_tag_get_current_target_handle_sync(
						&gdbus_nfc_context.current_target);
			}
			else if ((devType > NET_NFC_UNKNOWN_TARGET) &&
					(devType < NET_NFC_NFCIP1_TARGET))
			{
				if (gdbus_nfc_context.current_tag != NULL)
				{
					net_nfc_release_tag_info(gdbus_nfc_context.current_tag);
					gdbus_nfc_context.current_tag = NULL;
				}

				net_nfc_client_tag_get_current_tag_info_sync(
						&gdbus_nfc_context.current_tag);
			}
		}
		else
		{
			if (ret == NET_NFC_NOT_CONNECTED)
			{
				ret = NFC_ERROR_NONE;
			}
		}

		if (ret != NET_NFC_OK)
			return _convert_error_code(__func__, ret);
#endif
	}

	return NFC_ERROR_NONE;
}

int nfc_manager_deinitialize (void)
{
	int ret = NET_NFC_OK;

	if (_is_initialized())
	{
		net_nfc_client_p2p_unset_device_discovered();
		net_nfc_client_p2p_unset_device_detached();

		net_nfc_client_tag_unset_tag_discovered();
		net_nfc_client_tag_unset_tag_detached();

		ret = net_nfc_client_deinitialize();

		gdbus_nfc_context.initialized = false;
	}

	return _convert_error_code(__func__, ret);
}

/*********************** NDEF Functions*********************/

int nfc_manager_set_ndef_discovered_cb(nfc_ndef_discovered_cb callback,
		void *user_data)
{
	if (callback == NULL)
		return _return_invalid_param(__func__);

	gdbus_nfc_context.on_ndef_discovered_cb = callback;
	gdbus_nfc_context.on_ndef_discovered_user_data = user_data;

	return NFC_ERROR_NONE;
}

void nfc_manager_unset_ndef_discovered_cb(void)
{
	gdbus_nfc_context.on_ndef_discovered_cb = NULL;
	gdbus_nfc_context.on_ndef_discovered_user_data = NULL;
}

void nfc_manager_set_tag_filter(int filter)
{
	net_nfc_client_tag_set_filter(filter);
}

int nfc_manager_get_tag_filter(void)
{
	return net_nfc_client_tag_get_filter();
}

static void _net_nfc_manager_tag_format_ndef_cb(net_nfc_error_e result,
		void *user_data)
{
	nfc_tag_format_completed_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		callback(_convert_error_code(__func__, result),
				user_param);
	}

	g_variant_unref((GVariant *)user_data);
}

int nfc_tag_format_ndef(nfc_tag_h tag, unsigned char *key, int key_size,
		nfc_tag_format_completed_cb callback, void *user_data)
{
	int ret;
	data_s key_data = { key, key_size };
	GVariant *parameter;

	if (tag == NULL)
		return _return_invalid_param(__func__);

	if (!nfc_manager_is_activated())
	{
		return NFC_ERROR_NOT_ACTIVATED;
	}

	if (_check_app_permission() == false)
	{
		LOGE("permission check fail");

		return NFC_ERROR_SECURITY_RESTRICTED;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		ret = net_nfc_client_ndef_format(
				tag->handle,
				&key_data,
				_net_nfc_manager_tag_format_ndef_cb,
				parameter);
		if (ret != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		ret = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, ret);
}

static void _net_nfc_manager_tag_read_ndef_cb(net_nfc_error_e result,
		ndef_message_s *message, void *user_data)
{
	nfc_tag_read_completed_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		callback(_convert_error_code(__func__, result), message, user_param);
	}

	g_variant_unref((GVariant *)user_data);
}

int nfc_tag_read_ndef(nfc_tag_h tag, nfc_tag_read_completed_cb callback,
		void *user_data)
{
	int ret;
	GVariant *parameter;

	if (tag == NULL)
		return _return_invalid_param(__func__);

	if (!nfc_manager_is_activated())
	{
		return NFC_ERROR_NOT_ACTIVATED;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		ret = net_nfc_client_ndef_read(
				tag->handle,
				_net_nfc_manager_tag_read_ndef_cb,
				parameter);
		if (ret != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		ret = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_get_rawdata_size(nfc_ndef_message_h ndef_message,
		unsigned int *byte_size)
{
	int ret;

	if (ndef_message == NULL || byte_size == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_get_ndef_message_byte_length(ndef_message,
			(unsigned int *)byte_size);

	return _convert_error_code(__func__, ret);
}

static void _net_nfc_manager_tag_write_ndef_cb(net_nfc_error_e result,
		void *user_data)
{
	nfc_tag_write_completed_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		callback(_convert_error_code(__func__, result), user_param);
	}

	g_variant_unref((GVariant *)user_data);
}

int nfc_tag_write_ndef(nfc_tag_h tag, nfc_ndef_message_h msg,
		nfc_tag_write_completed_cb callback, void *user_data)
{
	int ret;
	unsigned int byte_size = 0;
	GVariant *parameter;

	if (tag == NULL)
		return _return_invalid_param(__func__);

	if (!nfc_manager_is_activated())
	{
		return NFC_ERROR_NOT_ACTIVATED;
	}

	if (_check_app_permission() == false)
	{
		LOGE("permission check fail");
		return NFC_ERROR_SECURITY_RESTRICTED;
	}

	if (tag->ndefCardState == NET_NFC_NDEF_CARD_READ_ONLY) {
		return NFC_ERROR_READ_ONLY_NDEF;
	}

	nfc_ndef_message_get_rawdata_size(msg, &byte_size);
	if (tag->maxDataSize < byte_size) {
		return NFC_ERROR_NO_SPACE_ON_NDEF;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		ret = net_nfc_client_ndef_write(
				tag->handle,
				msg,
				_net_nfc_manager_tag_write_ndef_cb,
				parameter);
		if (ret != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		ret = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, ret);
}

int nfc_manager_get_cached_message(nfc_ndef_message_h *ndef_message)
{
	int ret;

	if (ndef_message == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_retrieve_current_ndef_message(ndef_message);

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_create(nfc_ndef_record_h *record,
		nfc_record_tnf_e tnf,
		const unsigned char *type,
		int type_size,
		const unsigned char *id,
		int id_size,
		const unsigned char *payload,
		int payload_size)
{
	int ret;
	data_s type_data = { (unsigned char *)type, type_size };
	data_s id_data = { (unsigned char *)id, id_size };
	data_s payload_data = { (unsigned char *)payload, payload_size };

	if (record == NULL || type == NULL || type_size <= 0)
		return _return_invalid_param(__func__);

	ret = net_nfc_create_record(record, tnf, &type_data, &id_data, &payload_data);

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_create_text(nfc_ndef_record_h *record, const char *text,
		const char *lang_code, nfc_encode_type_e encode)
{
	int ret;

	if (record == NULL || text == NULL || lang_code == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_create_text_type_record(record, text, lang_code, encode);

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_create_uri(nfc_ndef_record_h *record,
		const char *uri)
{
	int ret;

	if (record == NULL || uri == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_create_uri_type_record(record, uri, NET_NFC_SCHEMA_FULL_URI);

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_create_mime(nfc_ndef_record_h *record, const char *mime_type,
		const unsigned char *data, int data_size)
{
	int ret;

	if (record == NULL || mime_type == NULL ||
			data == NULL || data_size <= 0)
		return _return_invalid_param(__func__);

	ret = nfc_ndef_record_create(record,
			NFC_RECORD_TNF_MIME_MEDIA,
			(unsigned char *)mime_type,
			strlen(mime_type),
			NULL,
			0,
			data,
			data_size);

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_get_mime_type(nfc_ndef_record_h record, char **mime_type)
{
	int ret;
	nfc_record_tnf_e tnf;
	unsigned char *typename;
	int length;

	if (record == NULL || mime_type == NULL)
		return _return_invalid_param(__func__);

	if (nfc_ndef_record_get_tnf(record, &tnf) != NET_NFC_OK ||
			tnf != NFC_RECORD_TNF_MIME_MEDIA)
	{
		return NFC_ERROR_INVALID_RECORD_TYPE;
	}

	ret = nfc_ndef_record_get_type(record, &typename, &length);
	if (ret == NET_NFC_OK) {
		*mime_type = calloc(1, length + 1);
		if (*mime_type != NULL) {
			memcpy(*mime_type, typename, length);
		} else {
			ret = NET_NFC_ALLOC_FAIL;
		}
	}

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_destroy(nfc_ndef_record_h record)
{
	int ret;

	if (record == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_free_record(record);

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_set_id(nfc_ndef_record_h record, unsigned char *id, int id_size)
{
	int ret;
	data_s id_data = { id, id_size };

	if (record == NULL || id == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_set_record_id(record, &id_data);

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_get_payload(nfc_ndef_record_h record,
	unsigned char **payload,
	unsigned int *size)
{
	int ret;
	data_s *payload_data;

	if (record == NULL || payload == NULL || size == NULL)
		return _return_invalid_param(__func__);

	*payload = NULL;
	*size = 0;

	ret = net_nfc_get_record_payload(record, &payload_data);
	if (ret == NET_NFC_OK) {
		*payload = payload_data->buffer;
		*size = payload_data->length;
	} else {
		*payload = NULL;
		*size = 0;
	}

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_get_type(nfc_ndef_record_h record, unsigned char **type,
		int *size)
{
	int ret;
	data_s *type_data;

	if (record == NULL || type == NULL || size == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_get_record_type(record, &type_data);
	if (ret == NET_NFC_OK) {
		*type = type_data->buffer;
		*size = type_data->length;
	} else {
		*type = NULL;
		*size = 0;
	}

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_get_id(nfc_ndef_record_h record, unsigned char **id, int *size)
{
	int ret;
	data_s *id_data;

	if (record == NULL || id == NULL || size == NULL)
		return _return_invalid_param(__func__);

	*id = NULL;
	*size = 0;

	ret = net_nfc_get_record_id(record, &id_data);
	if (ret == NET_NFC_OK) {
		*id = id_data->buffer;
		*size = id_data->length;
	} else {
		*id = NULL;
		*size = 0;
	}

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_get_tnf(nfc_ndef_record_h record, nfc_record_tnf_e *tnf)
{
	int ret;

	if (record == NULL || tnf == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_get_record_tnf(record, (net_nfc_record_tnf_e *)tnf);

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_get_text(nfc_ndef_record_h record, char **buffer)
{
	int ret;

	if (record == NULL || buffer == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_create_text_string_from_text_record(record, buffer);

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_get_langcode(nfc_ndef_record_h record, char **lang_code)
{
	int ret;

	if (record == NULL || lang_code == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_get_languange_code_string_from_text_record(record,
			lang_code);

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_get_encode_type(nfc_ndef_record_h record,
		nfc_encode_type_e *encode)
{
	int ret;

	if (record == NULL || encode == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_get_encoding_type_from_text_record(
			record,
			(net_nfc_encode_type_e *)encode);

	/*	if( ret == NFC_NDEF_RECORD_IS_NOT_EXPECTED_TYPE)
		LOGE("%s reord type is not text type");*/

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_get_uri(nfc_ndef_record_h record, char **uri)
{
	int ret;

	if (record == NULL || uri == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_create_uri_string_from_uri_record(record, uri);

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_create(nfc_ndef_message_h *ndef_message)
{
	int ret;

	if (ndef_message == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_create_ndef_message(ndef_message);

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_create_from_rawdata(
	nfc_ndef_message_h *ndef_message,
	const unsigned char *rawdata,
	unsigned int rawdata_size)
{
	int ret;
	data_s rawdata_data = { (unsigned char *)rawdata, rawdata_size };

	if (ndef_message == NULL || rawdata == NULL || rawdata_size <= 0)
		return _return_invalid_param(__func__);

	ret = net_nfc_create_ndef_message_from_rawdata(ndef_message, &rawdata_data);

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_destroy(nfc_ndef_message_h ndef_message)
{
	int ret;

	if (ndef_message == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_free_ndef_message(ndef_message);

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_get_record_count(nfc_ndef_message_h ndef_message,
		int *count)
{
	int ret;

	if (ndef_message == NULL || count == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_get_ndef_message_record_count(ndef_message, count);

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_get_rawdata(nfc_ndef_message_h ndef_message,
	unsigned char **rawdata,
	unsigned int *rawdata_size)
{
	int ret;
	data_s *rawdata_data;

	if (ndef_message == NULL || rawdata == NULL || rawdata_size == NULL)
		return _return_invalid_param(__func__);

	*rawdata = NULL;
	*rawdata_size = 0;

	ret = net_nfc_create_rawdata_from_ndef_message(ndef_message, &rawdata_data);
	if (ret == NET_NFC_OK) {
		*rawdata = calloc(1, rawdata_data->length);
		memcpy(*rawdata, rawdata_data->buffer, rawdata_data->length);
		*rawdata_size = rawdata_data->length;

		net_nfc_free_data(rawdata_data);
	}

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_append_record(nfc_ndef_message_h ndef_message,
		nfc_ndef_record_h record)
{
	int ret;

	if (ndef_message == NULL || record == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_append_record_to_ndef_message(ndef_message, record);

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_insert_record(nfc_ndef_message_h ndef_message,
		int index, nfc_ndef_record_h record)
{
	int ret;

	if (ndef_message == NULL || record == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_append_record_by_index(ndef_message, index, record);

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_remove_record(nfc_ndef_message_h ndef_message,
		int index)
{
	int ret;

	if (ndef_message == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_remove_record_by_index(ndef_message, index);

	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_get_record(nfc_ndef_message_h ndef_message,
		int index, nfc_ndef_record_h *record)
{
	int ret;

	if (ndef_message == NULL || record == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_get_record_by_index(ndef_message, index, record);

	return _convert_error_code(__func__, ret);
}

int nfc_tag_get_type(nfc_tag_h tag, nfc_tag_type_e *type)
{
	int ret;

	if (tag == NULL || type == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_get_tag_type(tag, (net_nfc_target_type_e *)type);

	return _convert_error_code(__func__, ret);
}

int nfc_tag_is_support_ndef(nfc_tag_h tag, bool *is_support)
{
	int ret;

	if (tag == NULL || is_support == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_get_tag_ndef_support(tag, is_support);

	return _convert_error_code(__func__, ret);
}

int nfc_tag_get_maximum_ndef_size(nfc_tag_h tag, unsigned int *max_size)
{
	int ret;

	if (tag == NULL || max_size == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_get_tag_max_data_size(tag, max_size);

	return _convert_error_code(__func__, ret);
}

int nfc_tag_get_ndef_size(nfc_tag_h tag, unsigned int *actual_data_size)
{
	int ret;

	if (tag == NULL || actual_data_size == NULL)
		return _return_invalid_param(__func__);

	ret = net_nfc_get_tag_actual_data_size(tag, actual_data_size);

	return _convert_error_code(__func__, ret);
}

int nfc_tag_foreach_information(nfc_tag_h tag, nfc_tag_information_cb callback,
		void *user_data)
{
	net_nfc_tag_info_s *taglist = NULL;
	int i;
	bool cont;

	if (tag == NULL || callback == NULL)
		return _return_invalid_param(__func__);

	taglist = tag->tag_info_list;

	if (taglist == NULL) {
		return NFC_ERROR_NOT_SUPPORTED;
	}

	for (i = 0; i < tag->number_of_keys; i++) {
		cont = callback(taglist[i].key,
				net_nfc_get_data_buffer(taglist[i].value),
				net_nfc_get_data_length(taglist[i].value),
				user_data);

		if (!cont)
			break;
	}

	return NFC_ERROR_NONE;
}

/********************************Tag Functions***********************************/

int nfc_manager_get_connected_tag(nfc_tag_h *tag)
{
	int ret;
	net_nfc_target_info_s *result = NULL;

	if (tag == NULL)
		return _return_invalid_param(__func__);

	*tag = NULL;

	if (gdbus_nfc_context.current_tag == NULL) {
		ret = net_nfc_client_tag_get_current_tag_info_sync(&result);
		if (ret == NET_NFC_OK) {
			*tag = result;
		}
	} else {
		/* FIXME ??? */
		*tag = gdbus_nfc_context.current_tag;

		ret = NET_NFC_OK;
	}

	return _convert_error_code(__func__, ret);
}

int nfc_manager_get_connected_target(nfc_p2p_target_h *target)
{
	int ret;
	net_nfc_target_handle_s *result = NULL;

	if (target == NULL)
		return _return_invalid_param(__func__);

	*target = NULL;

	if (gdbus_nfc_context.current_target == NULL) {
		ret = net_nfc_client_tag_get_current_target_handle_sync(&result);
		if (ret == NET_NFC_OK) {
			gdbus_nfc_context.current_target = result;

			*target = gdbus_nfc_context.current_target;
		}
	} else {
		/* FIXME ??? */
		*target = gdbus_nfc_context.current_target;

		ret = NET_NFC_OK;
	}

	return _convert_error_code(__func__, ret);
}

int nfc_manager_set_tag_discovered_cb(nfc_tag_discovered_cb callback,
		void *user_data)
{
	if (callback == NULL)
		return _return_invalid_param(__func__);

	gdbus_nfc_context.on_tag_discovered_cb = callback;
	gdbus_nfc_context.on_tag_discovered_user_data = user_data;

	return NFC_ERROR_NONE;
}

void nfc_manager_unset_tag_discovered_cb(void)
{
	gdbus_nfc_context.on_tag_discovered_cb = NULL;
	gdbus_nfc_context.on_tag_discovered_user_data = NULL;
}
#if 0
void _nfc_manager_tag_detached_cb(void *user_data)
{
	if (gdbus_nfc_context.on_tag_discovered_cb)
	{
		gdbus_nfc_context.on_tag_discovered_cb(
				NFC_DISCOVERED_TYPE_DETACHED,
				(nfc_tag_h)gdbus_nfc_context.current_tag,
				gdbus_nfc_context.on_tag_discovered_user_data);
	}

	if (gdbus_nfc_context.current_tag != NULL )
	{
		net_nfc_release_tag_info(gdbus_nfc_context.current_tag);
		gdbus_nfc_context.current_tag = NULL;
	}
}

int nfc_manager_set_tag_detached_cb(nfc_tag_discovered_cb callback,
		void *user_data)
{
	if (callback == NULL)
		return _return_invalid_param(__func__);

	gdbus_nfc_context.on_tag_discovered_cb = callback;
	gdbus_nfc_context.on_tag_discovered_user_data = user_data;

	net_nfc_client_tag_set_tag_detached(
			_nfc_manager_tag_detached_cb,
			user_data);

	return NFC_ERROR_NONE;
}

void nfc_manager_unset_tag_detached_cb(void)
{
	net_nfc_client_tag_unset_tag_detached();
}
#endif
/******************************Popup Notification*******************************/

int nfc_manager_set_system_handler_enable(bool enable)
{
	int ret;
	int state;

	if (enable == true) {
		state = NET_NFC_LAUNCH_APP_SELECT;
	} else {
		state = NET_NFC_NO_LAUNCH_APP_SELECT;
	}

	ret = net_nfc_client_sys_handler_set_launch_popup_state(state);

	return _convert_error_code(__func__, ret);
}

int nfc_manager_set_system_handler_enable_force(bool enable)
{
	int ret;
	int state;

	if (enable == true) {
		state = NET_NFC_LAUNCH_APP_SELECT;
	} else {
		state = NET_NFC_NO_LAUNCH_APP_SELECT;
	}

	ret = net_nfc_client_sys_handler_set_launch_popup_state_force(state);

	return _convert_error_code(__func__, ret);
}

bool nfc_manager_is_system_handler_enabled(void)
{
	int state = 0;

	net_nfc_client_sys_handler_get_launch_popup_state(&state);

	if (NET_NFC_LAUNCH_APP_SELECT == state)
		return true;
	else
		return false;
}

/******************************Handover*******************************/

static void _net_nfc_manager_p2p_handover_cb(net_nfc_error_e result,
		net_nfc_conn_handover_carrier_type_e carrier, data_s *ac_data, void *user_data)
{
	nfc_p2p_connection_handover_completed_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		nfc_ac_type_e carrior_type = NFC_AC_TYPE_UNKNOWN;
		char buffer[50] = { 0, };

		if (carrier == NET_NFC_CONN_HANDOVER_CARRIER_BT) {
			carrior_type = NFC_AC_TYPE_BT;
			if (ac_data != NULL && ac_data->buffer != NULL)
			{
				snprintf(buffer, sizeof(buffer),
						"%02x:%02x:%02x:%02x:%02x:%02x",
						ac_data->buffer[0], ac_data->buffer[1],
						ac_data->buffer[2], ac_data->buffer[3],
						ac_data->buffer[4], ac_data->buffer[5]);
			}
		}

		callback(_convert_error_code(__func__, result),
				carrior_type, (void *)buffer,
				strlen(buffer), user_param);
	}

	g_variant_unref((GVariant *)user_data);
}

int nfc_p2p_connection_handover(nfc_p2p_target_h target, nfc_ac_type_e type,
		nfc_p2p_connection_handover_completed_cb callback, void *user_data)
{
	int ret;
	net_nfc_conn_handover_carrier_type_e net_ac_type =
		NET_NFC_CONN_HANDOVER_CARRIER_UNKNOWN;
	GVariant *parameter;

	if (target == NULL)
		return _return_invalid_param(__func__);

	if (type > NFC_AC_TYPE_UNKNOWN)
		return _return_invalid_param(__func__);

	if (!nfc_manager_is_activated()) {
		return NFC_ERROR_NOT_ACTIVATED;
	}

	switch (type)
	{
	case NFC_AC_TYPE_BT :
		net_ac_type = NET_NFC_CONN_HANDOVER_CARRIER_BT;
		break;

	case NFC_AC_TYPE_WIFI :
		net_ac_type = NET_NFC_CONN_HANDOVER_CARRIER_WIFI_BSS;
		break;

	case NFC_AC_TYPE_WIFI_DIRECT :
		net_ac_type = NET_NFC_CONN_HANDOVER_CARRIER_WIFI_IBSS;
		break;

	case NFC_AC_TYPE_UNKNOWN :
		net_ac_type = NET_NFC_CONN_HANDOVER_CARRIER_UNKNOWN;
		break;

	default:
		break;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		ret = net_nfc_client_p2p_connection_handover(
				(net_nfc_target_handle_s*)target,
				net_ac_type,
				_net_nfc_manager_p2p_handover_cb,
				parameter);
		if (ret != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		ret = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, ret);
}

bool nfc_p2p_is_supported_ac_type(nfc_ac_type_e carrier)
{
	bool result = false;

	if (carrier == NFC_AC_TYPE_BT)
		result = true;

	return result;
}

/**********************P2P functions************************************/

static void _p2p_send_cb(net_nfc_error_e result, void *user_data)
{
	nfc_p2p_send_completed_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		callback(_convert_error_code(__func__, result), user_param);
	}

	g_variant_unref((GVariant *)user_data);
}

int nfc_p2p_send(nfc_p2p_target_h target, nfc_ndef_message_h message,
		nfc_p2p_send_completed_cb callback, void *user_data)
{
	int ret;
	data_s *rawdata;
	GVariant *parameter;

	if (target == NULL || message == NULL)
		return _return_invalid_param(__func__);

	if (!nfc_manager_is_activated())
		return NFC_ERROR_NOT_ACTIVATED;

	if (_check_app_permission() == false)
	{
		LOGE("permission check fail");

		return NFC_ERROR_SECURITY_RESTRICTED;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		net_nfc_create_rawdata_from_ndef_message(message, &rawdata);
		ret = net_nfc_client_p2p_send((net_nfc_target_handle_s*)target,
				rawdata,
				_p2p_send_cb,
				parameter);
		if (ret != NET_NFC_OK) {
			g_variant_unref(parameter);
		}

		net_nfc_free_data(rawdata);
	} else {
		ret = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, ret);
}

int nfc_p2p_send_no_permission(nfc_p2p_target_h target,
		nfc_ndef_message_h message, nfc_p2p_send_completed_cb callback, void *user_data)
{
	int ret;
	data_s *rawdata;
	GVariant *parameter;

	if (target == NULL || message == NULL)
		return _return_invalid_param(__func__);

	if (!nfc_manager_is_activated())
		return NFC_ERROR_NOT_ACTIVATED;

	/* skip check app permission */

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		net_nfc_create_rawdata_from_ndef_message(message, &rawdata);
		ret = net_nfc_client_p2p_send((net_nfc_target_handle_s*)target,
				rawdata,
				_p2p_send_cb,
				parameter);
		if (ret != NET_NFC_OK) {
			g_variant_unref(parameter);
		}

		net_nfc_free_data(rawdata);
	} else {
		ret = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, ret);
}

static void _net_nfc_manager_p2p_set_data_received_cb(data_s *data,
		void *user_data)
{
	if (gdbus_nfc_context.on_p2p_recv_cb != NULL)
	{
		ndef_message_s *ndef_message;

		net_nfc_create_ndef_message_from_rawdata(&ndef_message, data);

		gdbus_nfc_context.on_p2p_recv_cb(
				(nfc_p2p_target_h)(gdbus_nfc_context.current_target),
				ndef_message,
				gdbus_nfc_context.on_p2p_recv_user_data);

		net_nfc_free_ndef_message(ndef_message);
	}
}

int nfc_p2p_set_data_received_cb(nfc_p2p_target_h target,
		nfc_p2p_data_recived_cb callback, void *user_data)
{
	if (target == NULL || callback == NULL)
		return _return_invalid_param(__func__);

	if (gdbus_nfc_context.current_target != target)
		return _return_invalid_param(__func__);

	gdbus_nfc_context.on_p2p_recv_cb = callback;
	gdbus_nfc_context.on_p2p_recv_user_data = user_data;

	net_nfc_client_p2p_set_data_received(
			_net_nfc_manager_p2p_set_data_received_cb,
			NULL);

	return NFC_ERROR_NONE;
}

int nfc_p2p_unset_data_received_cb(nfc_p2p_target_h target)
{
	if (target == NULL)
		return _return_invalid_param(__func__);

	if (gdbus_nfc_context.current_target != target)
		return _return_invalid_param(__func__);

	net_nfc_client_p2p_unset_data_received();

	gdbus_nfc_context.on_p2p_recv_cb = NULL;
	gdbus_nfc_context.on_p2p_recv_user_data = NULL;

	return NFC_ERROR_NONE;
}

int nfc_manager_set_p2p_target_discovered_cb(
		nfc_p2p_target_discovered_cb callback, void *user_data)
{
	if (callback == NULL)
		return _return_invalid_param(__func__);

	gdbus_nfc_context.on_p2p_target_discovered_cb = callback;
	gdbus_nfc_context.on_p2p_target_discovered_user_data = user_data;

	return NFC_ERROR_NONE;
}

void nfc_manager_unset_p2p_target_discovered_cb(void)
{
	gdbus_nfc_context.on_p2p_target_discovered_cb = NULL;
	gdbus_nfc_context.on_p2p_target_discovered_user_data = NULL;
}

/******************************SNEP***********************************/

static void _net_nfc_manager_snep_start_server(
		net_nfc_snep_handle_h arg_handle,
		net_nfc_snep_type_t event,
		net_nfc_error_e result,
		nfc_ndef_message_h msg,
		void *user_data)
{
	nfc_snep_event_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		nfc_snep_event_e snep_event;

		switch ((int)event) {
		case NET_NFC_SNEP_GET :
			snep_event = NFC_SNEP_EVENT_GET;
			break;

		case NET_NFC_SNEP_PUT :
			snep_event = NFC_SNEP_EVENT_PUT;
			break;

		case NET_NFC_LLCP_REGISTERED :
			snep_event = NFC_SNEP_EVENT_REGISTER;
			break;

		case NET_NFC_LLCP_UNREGISTERED :
			snep_event = NFC_SNEP_EVENT_UNREGISTER;
			break;

		case NET_NFC_LLCP_START :
			snep_event = NFC_SNEP_EVENT_START;
			break;

		case NET_NFC_LLCP_STOP :
		default :
			snep_event = NFC_SNEP_EVENT_STOP;
			break;
		}

		callback(arg_handle, snep_event,
				_convert_error_code(__func__, result)
				, msg, user_param);

		if (snep_event == NFC_SNEP_EVENT_STOP) {
			g_variant_unref(user_data);
		}
	}
}

int nfc_snep_start_server(nfc_p2p_target_h target, const char *san, int sap,
		nfc_snep_event_cb callback, void *user_data)
{
	net_nfc_error_e result;
	GVariant *parameter;

	if (target == NULL) {
		return NFC_ERROR_INVALID_PARAMETER;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		result = net_nfc_client_snep_start_server(
				(net_nfc_target_handle_s*)target,
				san,
				(sap_t)sap,
				_net_nfc_manager_snep_start_server,
				parameter);
		if (result != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		result = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, result);
}

static void _net_nfc_manager_snep_start_client(
		net_nfc_snep_handle_h arg_handle,
		net_nfc_snep_type_t event,
		net_nfc_error_e result,
		nfc_ndef_message_h msg,
		void *user_data)
{
	nfc_snep_event_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		nfc_snep_event_e snep_event;

		switch ((int)event) {
		case NET_NFC_SNEP_GET :
			snep_event = NFC_SNEP_EVENT_GET;
			break;

		case NET_NFC_SNEP_PUT :
			snep_event = NFC_SNEP_EVENT_PUT;
			break;

		case NET_NFC_LLCP_REGISTERED :
			snep_event = NFC_SNEP_EVENT_REGISTER;
			break;

		case NET_NFC_LLCP_UNREGISTERED :
			snep_event = NFC_SNEP_EVENT_UNREGISTER;
			break;

		case NET_NFC_LLCP_START :
			snep_event = NFC_SNEP_EVENT_START;
			break;

		case NET_NFC_LLCP_STOP :
		default :
			snep_event = NFC_SNEP_EVENT_STOP;
			break;
		}

		callback(arg_handle, snep_event, _convert_error_code(__func__, result),
				msg, user_param);

		if (snep_event == NFC_SNEP_EVENT_STOP) {
			g_variant_unref(user_data);
		}
	}
}

int nfc_snep_start_client(nfc_p2p_target_h target, const char *san, int sap,
		nfc_snep_event_cb callback, void *user_data)
{
	net_nfc_error_e result;
	GVariant *parameter;

	if (target == NULL) {
		return NFC_ERROR_INVALID_PARAMETER;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		result = net_nfc_client_snep_start_client(
				(net_nfc_target_handle_s*)target,
				san,
				(sap_t)sap,
				_net_nfc_manager_snep_start_client,
				parameter);
		if (result != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		result = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, result);
}

static void _net_nfc_manager_snep_send_request(
		net_nfc_snep_handle_h target,
		net_nfc_snep_type_t event,
		net_nfc_error_e result,
		ndef_message_s *msg,
		void *user_data)
{
	nfc_snep_event_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		callback((nfc_p2p_snep_h)target,
				event,
				_convert_error_code(__func__, result),
				(nfc_ndef_message_h)msg,
				user_param);
	}

	g_variant_unref((GVariant *)user_data);
}

int nfc_snep_send_client_request(nfc_p2p_snep_h handle, nfc_snep_type_e type,
		nfc_ndef_message_h msg, nfc_snep_event_cb callback, void *user_data)
{
	net_nfc_error_e result;
	GVariant *parameter;

	if (handle == NULL || msg == NULL) {
		return NFC_ERROR_INVALID_PARAMETER;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		result = net_nfc_client_snep_send_client_request(
				(net_nfc_target_handle_s*)handle,
				type,
				msg,
				_net_nfc_manager_snep_send_request,
				parameter);
		if (result != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		result = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, result);
}

static void _snep_event_cb(net_nfc_snep_handle_h target, net_nfc_snep_type_t event,
		net_nfc_error_e result, ndef_message_s *msg, void *user_data)
{
	nfc_snep_event_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		switch ((int)event) {
		case NET_NFC_SNEP_GET :
			event = NFC_SNEP_EVENT_GET;
			break;

		case NET_NFC_SNEP_PUT :
			event = NFC_SNEP_EVENT_PUT;
			break;

		case NET_NFC_LLCP_REGISTERED :
			event = NFC_SNEP_EVENT_REGISTER;
			break;

		case NET_NFC_LLCP_UNREGISTERED :
			event = NFC_SNEP_EVENT_UNREGISTER;
			break;

		case NET_NFC_LLCP_START :
			event = NFC_SNEP_EVENT_START;
			break;

		case NET_NFC_LLCP_STOP :
			event = NFC_SNEP_EVENT_STOP;
			break;
		}

		callback(target, event, _convert_error_code(__func__, result), msg, user_param);
	}

	if ((int)event == NFC_SNEP_EVENT_UNREGISTER) {
		g_variant_unref((GVariant *)user_data);
	}
}

int nfc_snep_register_server(const char *san, int sap,
		nfc_snep_event_cb callback, void *user_data)
{
	net_nfc_error_e result;
	GVariant *parameter;

	if (san == NULL) {
		return _return_invalid_param(__func__);
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		result = net_nfc_client_snep_register_server(san, (sap_t)sap,
				_snep_event_cb, parameter);
		if (result != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		result = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, result);
}

int nfc_snep_unregister_server(const char *san, int sap)
{
	net_nfc_error_e result;

	result = net_nfc_client_snep_unregister_server(san, (sap_t)sap);

	return _convert_error_code(__func__, result);
}

int nfc_snep_stop_service(nfc_p2p_target_h target, nfc_p2p_snep_h clients)
{
	int ret;

	ret = net_nfc_client_snep_stop_service_sync(target, clients);

	return _convert_error_code(__func__, ret);
}

/******************************Secure Element***********************************/

static void _net_nfc_manager_set_card_emulation_se_type_cb(
		net_nfc_error_e result, void *user_data)
{
	nfc_set_card_emulation_completed_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		callback(_convert_error_code(__func__, result),
				user_param);
	}

	g_variant_unref((GVariant *)user_data);
}

int nfc_manager_set_card_emulation_se_type(nfc_se_type_e type,
		nfc_set_card_emulation_completed_cb callback, void *user_data)
{
	int ret;
	net_nfc_se_type_e se_type;
	GVariant *parameter;

	if ((type < NFC_SE_TYPE_DISABLE) || (type > NFC_SE_TYPE_UICC))
	{
		return _return_invalid_param(__func__);
	}

	if (!nfc_manager_is_activated())
	{
		return NFC_ERROR_NOT_ACTIVATED;
	}

	switch (type)
	{
	case NFC_SE_TYPE_DISABLE :
		se_type = NET_NFC_SE_TYPE_NONE;
		break;
	case NFC_SE_TYPE_ESE :
		se_type = NET_NFC_SE_TYPE_ESE;
		break;
	case NFC_SE_TYPE_UICC :
		se_type = NET_NFC_SE_TYPE_UICC;
		break;
	default:
		break;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		ret = net_nfc_client_se_set_secure_element_type(
				se_type,
				_net_nfc_manager_set_card_emulation_se_type_cb,
				parameter);
		if (ret != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		ret = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, ret);
}

int nfc_manager_set_se_type(nfc_se_type_e type)
{
	int ret;
	net_nfc_se_type_e se_type = NET_NFC_SE_TYPE_NONE;

	if ((type < NFC_SE_TYPE_DISABLE) || (type > NFC_SE_TYPE_UICC))
	{
		return _return_invalid_param(__func__);
	}

	if (!nfc_manager_is_activated())
	{
		return NFC_ERROR_NOT_ACTIVATED;
	}

	switch (type)
	{
	case NFC_SE_TYPE_DISABLE :
		se_type = NET_NFC_SE_TYPE_NONE;
		break;
	case NFC_SE_TYPE_ESE :
		se_type = NET_NFC_SE_TYPE_ESE;
		break;
	case NFC_SE_TYPE_UICC :
		se_type = NET_NFC_SE_TYPE_UICC;
		break;
	}

	ret = net_nfc_client_se_set_secure_element_type_sync(se_type);

	return _convert_error_code(__func__, ret);
}

int nfc_manager_get_card_emulation_se_type(nfc_se_type_e *type)
{
	int ret;
	int se_type;

	ret = vconf_get_int(VCONFKEY_NFC_SE_TYPE, &se_type);
	if (ret == 0)
	{
		if ((se_type >= NFC_SE_TYPE_DISABLE)
				&& (se_type <= NFC_SE_TYPE_UICC))
		{
			*type = se_type;

			ret = NFC_ERROR_NONE;
		}
		else
		{
			ret = NFC_ERROR_OPERATION_FAILED;
		}
	}
	else
	{
		ret = NFC_ERROR_OPERATION_FAILED;
	}

	return ret;
}

int nfc_se_open_secure_element(nfc_se_type_e se_type, nfc_se_h *handle)
{
	net_nfc_error_e result;
	net_nfc_se_type_e type;
	net_nfc_target_handle_s *temp = NULL;

	switch (se_type)
	{
	case NFC_SE_TYPE_ESE :
		type = NET_NFC_SE_TYPE_ESE;
		break;

	case NFC_SE_TYPE_UICC :
		type = NET_NFC_SE_TYPE_UICC;
		break;

	default:
		return NFC_ERROR_INVALID_PARAMETER;
		break;
	}

	result = net_nfc_client_se_open_internal_secure_element_sync(type, &temp);
	if (result == NET_NFC_OK) {
		*handle = (nfc_se_h)temp;
	}

	return _convert_error_code(__func__, result);
}

int nfc_se_send_apdu(nfc_se_h handle, unsigned char *cmd, unsigned int cmd_len,
		unsigned char **resp, unsigned int *resp_len)
{
	net_nfc_error_e result;
	data_s command, *response = NULL;

	if (handle == NULL || cmd == NULL || cmd_len == 0 ||
			resp == NULL || resp_len == NULL ) {
		return _return_invalid_param(__func__);
	}

	*resp = NULL;
	*resp_len = 0;

	command.buffer = cmd;
	command.length = cmd_len;

	result = net_nfc_client_se_send_apdu_sync(
			(net_nfc_target_handle_s*)handle,
			&command,
			&response);

	if (result == NET_NFC_OK && response != NULL
			&& response->buffer != NULL) {
		if (response->length >= 2) {
			*resp = calloc(1, response->length);
			if (*resp != NULL) {
				memcpy(*resp, response->buffer, response->length);
				*resp_len = response->length;
			} else {
				result = NET_NFC_ALLOC_FAIL;
			}
		} else {
			result = NET_NFC_OPERATION_FAIL;
		}
	}

	if (response != NULL) {
		net_nfc_free_data(response);
	}

	return _convert_error_code(__func__, result);
}

int nfc_se_get_atr(nfc_se_h handle, unsigned char **atr, unsigned int *atr_len)
{
	net_nfc_error_e result;
	data_s *response = NULL;

	if (handle == NULL || atr == NULL || atr_len == NULL) {
		return _return_invalid_param(__func__);
	}

	*atr = NULL;
	*atr_len = 0;

	result = net_nfc_client_se_get_atr_sync(
			(net_nfc_target_handle_s*)handle,
			&response);

	if (result == NET_NFC_OK && response != NULL
			&& response->buffer != NULL && response->length > 0) {
		*atr = calloc(1, response->length);
		if (*atr != NULL) {
			memcpy(*atr, response->buffer, response->length);
			*atr_len = response->length;
		} else {
			result = NET_NFC_ALLOC_FAIL;
		}
	}

	if (response != NULL) {
		net_nfc_free_data(response);
	}

	return _convert_error_code(__func__, result);
}

int nfc_se_close_secure_element(nfc_se_h handle)
{
	net_nfc_error_e result;

	result = net_nfc_client_se_close_internal_secure_element_sync(
			(net_nfc_target_handle_s*)handle);

	return _convert_error_code(__func__, result);
}

int nfc_se_enable_card_emulation()
{
	net_nfc_error_e result;

	result = net_nfc_set_card_emulation_mode_sync(NET_NFC_CARD_EMELATION_ENABLE);

	return _convert_error_code(__func__, result);
}

int nfc_se_disable_card_emulation()
{
	net_nfc_error_e result;

	result = net_nfc_set_card_emulation_mode_sync(NET_NFC_CARD_EMULATION_DISABLE);

	return _convert_error_code(__func__, result);
}

int nfc_se_get_card_emulation_mode(nfc_se_card_emulation_mode_type_e *type)
{
	net_nfc_error_e result;
	net_nfc_se_type_e se_type;

	result = net_nfc_get_card_emulation_mode(&se_type);

	if(result == NET_NFC_OK)
	{
		if(se_type == NET_NFC_SE_TYPE_NONE)
		{
			*type = NFC_SE_CARD_EMULATION_MODE_OFF;
		}
		else if(se_type == NET_NFC_SE_TYPE_ESE || se_type == NET_NFC_SE_TYPE_UICC)
		{
			*type = NFC_SE_CARD_EMULATION_MODE_ON;
		}
	}
	else
	{
		result = NET_NFC_OPERATION_FAIL;
	}

	return _convert_error_code(__func__, result);;
}

int nfc_manager_set_se_event_cb(nfc_se_event_cb callback, void *user_data)
{
	if (callback == NULL)
		return _return_invalid_param(__func__);

	net_nfc_client_se_set_event_cb(
			(net_nfc_client_se_event)callback,
			user_data);

	return NFC_ERROR_NONE;
}

void nfc_manager_unset_se_event_cb(void)
{
	net_nfc_client_se_unset_event_cb();

	gdbus_nfc_context.on_se_transaction_event_cb = NULL;
	gdbus_nfc_context.on_se_transaction_event_user_data = NULL;
}

static void _se_transaction_event_cb(
						net_nfc_se_type_e se_type,
						data_s* aid,
						data_s* param,
						void *user_data)
{
	if (gdbus_nfc_context.on_se_transaction_event_cb != NULL) {
		gdbus_nfc_context.on_se_transaction_event_cb(
			se_type,
			net_nfc_get_data_buffer(aid),
			net_nfc_get_data_length(aid),
			net_nfc_get_data_buffer(param),
			net_nfc_get_data_length(param),
			gdbus_nfc_context.on_se_transaction_event_user_data);
	}
}

int nfc_manager_set_se_transaction_event_cb(
	nfc_se_type_e se_type,
	nfc_se_transaction_event_cb callback,
	void *user_data)
{
	net_nfc_se_type_e type = NET_NFC_SE_TYPE_NONE;

	if (callback == NULL)
		return _return_invalid_param(__func__);

	gdbus_nfc_context.on_se_transaction_event_cb = callback;
	gdbus_nfc_context.on_se_transaction_event_user_data = user_data;

	switch(se_type)
	{
		case NFC_SE_TYPE_ESE:
			type = NET_NFC_SE_TYPE_ESE;
			break;

		case NFC_SE_TYPE_UICC:
			type = NET_NFC_SE_TYPE_UICC;
			break;
		default:
			return _return_invalid_param(__func__);
			break;

	}

	net_nfc_client_se_set_transaction_event_cb(type, _se_transaction_event_cb, user_data);

	return NFC_ERROR_NONE;
}

void nfc_manager_unset_se_transaction_event_cb(nfc_se_type_e se_type)
{
	net_nfc_se_type_e type = NET_NFC_SE_TYPE_NONE;

	switch(se_type)
	{
		case NFC_SE_TYPE_ESE:
			type = NET_NFC_SE_TYPE_ESE;
			break;

		case NFC_SE_TYPE_UICC:
			type = NET_NFC_SE_TYPE_UICC;
			break;
		default:
			type = NET_NFC_SE_TYPE_NONE;
			break;
	}
	net_nfc_client_se_unset_transaction_event_cb(type);
}

/************************************Raw Data- Transceive*********************/

static void _net_nfc_manager_tag_transceive_data(net_nfc_error_e result,
		data_s *arg_data, void *user_data)
{
	nfc_tag_transceive_completed_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		unsigned char *buffer = NULL;
		int buffer_size = 0;

		if (result == NET_NFC_OK && arg_data != NULL) {
			buffer = arg_data->buffer;
			buffer_size = arg_data->length;
		}

		callback(_convert_error_code(__func__, result),
				buffer,
				buffer_size,
				user_param);
	}

	g_variant_unref((GVariant *)user_data);
}

int nfc_tag_transceive(nfc_tag_h tag, unsigned char *buffer, int buffer_size,
		nfc_tag_transceive_completed_cb callback, void *user_data)
{
	int ret;
	data_s rawdata = { buffer, buffer_size };
	GVariant *parameter;

	if (tag == NULL || buffer == NULL || buffer_size <= 0)
		return _return_invalid_param(__func__);

	if (!nfc_manager_is_activated())
	{
		return NFC_ERROR_NOT_ACTIVATED;
	}

	if (_check_app_permission() == false)
	{
		LOGE("permission check fail");

		return NFC_ERROR_SECURITY_RESTRICTED;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		ret = net_nfc_client_transceive_data(
				tag->handle,
				&rawdata,
				_net_nfc_manager_tag_transceive_data,
				parameter);
		if (ret != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		ret = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, ret);
}

/*********************************Mifare*********************************/
/* FIXME */
static void _net_nfc_manager_mifare_authenticate_with_keyA(
		net_nfc_error_e result, void *user_data)
{
	nfc_mifare_authenticate_with_keyA_completed_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)",(guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		callback(_convert_error_code(__func__, result), user_param);
	}

	g_variant_unref((GVariant *)user_data);
}

int nfc_mifare_authenticate_with_keyA(nfc_tag_h tag,
		int sector_index,
		unsigned char *auth_key,
		nfc_mifare_authenticate_with_keyA_completed_cb callback,
		void *user_data)
{
	int ret;
	data_s auth_key_data = { auth_key, 6 };
	GVariant *parameter;

	if (tag == NULL || auth_key == NULL)
		return _return_invalid_param(__func__);

	if (!nfc_manager_is_activated())
	{
		return NFC_ERROR_NOT_ACTIVATED;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		ret = net_nfc_client_mifare_authenticate_with_keyA(
				tag->handle,
				sector_index,
				&auth_key_data,
				_net_nfc_manager_mifare_authenticate_with_keyA,
				parameter);
		if (ret != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		ret = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, ret);
}

static void _net_nfc_manager_mifare_authenticate_with_keyB(
		net_nfc_error_e result, void *user_data)
{
	nfc_mifare_authenticate_with_keyB_completed_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		callback(_convert_error_code(__func__, result), user_param);
	}

	g_variant_unref((GVariant *)user_data);
}

int nfc_mifare_authenticate_with_keyB(nfc_tag_h tag,
		int sector_index,
		unsigned char *auth_key,
		nfc_mifare_authenticate_with_keyB_completed_cb callback,
		void *user_data)
{
	int ret;
	data_s auth_key_data = { auth_key, 6 };
	GVariant *parameter;

	if (tag == NULL || auth_key == NULL)
		return _return_invalid_param(__func__);

	if (!nfc_manager_is_activated())
	{
		return NFC_ERROR_NOT_ACTIVATED;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		ret = net_nfc_client_mifare_authenticate_with_keyB(
				tag->handle,
				sector_index,
				&auth_key_data,
				_net_nfc_manager_mifare_authenticate_with_keyB,
				parameter);
		if (ret != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		ret = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, ret);
}

/* FIXME */
static void _net_nfc_manager_mifare_read_block(net_nfc_error_e result,
		data_s *data, void *user_data)
{
	nfc_mifare_read_block_completed_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		uint8_t *buffer = NULL;
		int length = 0;

		if (result == NET_NFC_OK && data != NULL) {
			buffer = data->buffer;
			length = data->length;
		}

		callback(_convert_error_code(__func__, result),
				buffer,
				length,
				user_param);
	}

	g_variant_unref((GVariant *)user_data);
}

int nfc_mifare_read_block(nfc_tag_h tag, int block_index,
		nfc_mifare_read_block_completed_cb callback, void *user_data)
{
	int ret;
	GVariant *parameter;

	if (tag == NULL)
		return _return_invalid_param(__func__);

	if (!nfc_manager_is_activated())
	{
		return NFC_ERROR_NOT_ACTIVATED;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		ret = net_nfc_client_mifare_read(
				tag->handle,
				block_index,
				_net_nfc_manager_mifare_read_block,
				parameter);
		if (ret != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		ret = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, ret);
}
#if 0
/* FIXME : ... */
static void _mifare_read_page(net_nfc_error_e result,
		data_s *data,
		void *user_data)
{
	nfc_mifare_read_page_completed_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		uint8_t *buffer = NULL;
		int length = 0;

		if (result == NET_NFC_OK && data != NULL) {
			buffer = data->buffer;
			length = data->length;
		}

		callback(_convert_error_code(__func__, result),
				buffer,
				length,
				user_param);
	}

	g_variant_unref((GVariant *)user_data);
}
#endif
int nfc_mifare_read_page(nfc_tag_h tag, int page_index,
		nfc_mifare_read_page_completed_cb callback, void *user_data)
{
	int ret;
	GVariant *parameter;

	if (!nfc_manager_is_activated())
	{
		return NFC_ERROR_NOT_ACTIVATED;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		/* FIXME : not implemented */
		//		ret = net_nfc_client_mifare_read_block(tag,
		//			page_index,
		//			_mifare_read_page,
		//			parameter);
		//		if (ret != NET_NFC_OK) {
		g_variant_unref(parameter);
		//		}
	} else {
		ret = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, ret);
}

static void _net_nfc_manager_mifare_write_block(net_nfc_error_e result,
		void *user_data)
{
	nfc_mifare_write_block_completed_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		callback(_convert_error_code(__func__, result), user_param);
	}

	g_variant_unref((GVariant *)user_data);
}

int nfc_mifare_write_block(nfc_tag_h tag, int block_index, unsigned char *buffer,
		int buffer_size, nfc_mifare_write_block_completed_cb callback, void *user_data)
{
	int ret;
	data_s block_data = { buffer, buffer_size };
	GVariant *parameter;

	if (tag == NULL || buffer == NULL || buffer_size <= 0)
		return _return_invalid_param(__func__);

	if (!nfc_manager_is_activated())
	{
		return NFC_ERROR_NOT_ACTIVATED;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		ret = net_nfc_client_mifare_write_block(
				tag->handle,
				block_index,
				&block_data,
				_net_nfc_manager_mifare_write_block,
				parameter);
		if (ret != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		ret = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, ret);
}

static void _mifare_write_page(net_nfc_error_e result, void *user_data)
{
	nfc_mifare_write_page_completed_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data,"(uu)", (guint *)&callback, (guint*)&user_param);

	if (callback != NULL) {
		callback(_convert_error_code(__func__, result), user_param);
	}

	g_variant_unref((GVariant *)user_data);
}

int nfc_mifare_write_page(nfc_tag_h tag, int page_index, unsigned char *buffer,
		int buffer_size, nfc_mifare_write_page_completed_cb callback, void *user_data)
{
	int ret;
	data_s block_data = { buffer, buffer_size };
	GVariant *parameter;

	if (tag == NULL || buffer == NULL || buffer_size <= 0)
		return _return_invalid_param(__func__);

	if (!nfc_manager_is_activated())
	{
		return NFC_ERROR_NOT_ACTIVATED;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		ret = net_nfc_client_mifare_write_page(
				tag->handle,
				page_index,
				&block_data,
				_mifare_write_page,
				parameter);
		if (ret != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		ret = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, ret);
}

static void _net_nfc_manager_mifare_increment(net_nfc_error_e result,
		void *user_data)
{
	nfc_mifare_increment_completed_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		callback(_convert_error_code(__func__, result), user_param);
	}

	g_variant_unref((GVariant *)user_data);
}

int nfc_mifare_increment(nfc_tag_h tag, int block_index, int value,
		nfc_mifare_increment_completed_cb callback, void *user_data)
{
	int ret;
	GVariant *parameter;

	if (tag == NULL )
		return _return_invalid_param(__func__);

	if (!nfc_manager_is_activated())
	{
		return NFC_ERROR_NOT_ACTIVATED;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		ret = net_nfc_client_mifare_increment(
				tag->handle,
				block_index,
				value,
				_net_nfc_manager_mifare_increment,
				parameter);
		if (ret != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		ret = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, ret);
}

static void _net_nfc_manager_mifare_decrement(net_nfc_error_e result,
		void *user_data)
{
	nfc_mifare_decrement_completed_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		callback(_convert_error_code(__func__, result), user_param);
	}

	g_variant_unref((GVariant *)user_data);
}

int nfc_mifare_decrement(nfc_tag_h tag, int block_index, int value,
		nfc_mifare_decrement_completed_cb callback, void *user_data)
{
	int ret;
	GVariant *parameter;

	if (tag == NULL)
		return _return_invalid_param(__func__);

	if (!nfc_manager_is_activated())
	{
		return NFC_ERROR_NOT_ACTIVATED;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		ret = net_nfc_client_mifare_decrement(
				tag->handle,
				block_index,
				value,
				_net_nfc_manager_mifare_decrement,
				parameter);
		if (ret != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		ret = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, ret);
}

static void _net_nfc_manager_mifare_transfer(net_nfc_error_e result,
		void *user_data)
{
	nfc_mifare_transfer_completed_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data, "(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		callback(_convert_error_code(__func__, result), user_param);
	}

	g_variant_unref((GVariant *)user_data);
}

int nfc_mifare_transfer(nfc_tag_h tag, int block_index,
		nfc_mifare_transfer_completed_cb callback, void *user_data)
{
	int ret;
	GVariant *parameter;

	if (tag == NULL)
		return _return_invalid_param(__func__);

	if (!nfc_manager_is_activated())
	{
		return NFC_ERROR_NOT_ACTIVATED;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		ret = net_nfc_client_mifare_transfer(
				tag->handle,
				block_index,
				_net_nfc_manager_mifare_transfer,
				parameter);
		if (ret != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		ret = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, ret);
}

static void _net_nfc_manager_mifare_restore(net_nfc_error_e result,
		void *user_data)
{
	nfc_mifare_restore_completed_cb callback;
	void *user_param;

	if (user_data == NULL) {
		return;
	}

	g_variant_get((GVariant*)user_data,"(uu)", (guint*)&callback, (guint*)&user_param);

	if (callback != NULL) {
		callback(_convert_error_code(__func__, result), user_param);
	}

	g_variant_unref((GVariant *)user_data);
}

int nfc_mifare_restore(nfc_tag_h tag, int block_index,
		nfc_mifare_restore_completed_cb callback, void *user_data)
{
	int ret;
	GVariant *parameter;

	if (tag == NULL)
		return _return_invalid_param(__func__);

	if (!nfc_manager_is_activated())
	{
		return NFC_ERROR_NOT_ACTIVATED;
	}

	parameter = g_variant_new("(uu)",
			GPOINTER_TO_UINT(callback), GPOINTER_TO_UINT(user_data));
	if (parameter != NULL) {
		ret = net_nfc_client_mifare_restore(
				tag->handle,
				block_index,
				_net_nfc_manager_mifare_restore,
				parameter);
		if (ret != NET_NFC_OK) {
			g_variant_unref(parameter);
		}
	} else {
		ret = NET_NFC_ALLOC_FAIL;
	}

	return _convert_error_code(__func__, ret);
}

int net_nfc_create_tizen_app_launch_record(data_s* app_id,nfc_ndef_record_h* ndef_record)
{

	ndef_record_s* tizenAppRec = NULL;
	data_s type_data ;
	data_s payload_data;

	type_data.buffer = (uint8_t*)TIZEN_APP_RECORD_TYPE;
	type_data.length = strlen(TIZEN_APP_RECORD_TYPE);

	payload_data.length = app_id->length;

	payload_data.buffer = (uint8_t*)calloc(1,payload_data.length);

	if (payload_data.buffer == NULL)
	{
		return NET_NFC_ALLOC_FAIL;
	}

	memcpy(payload_data.buffer,app_id->buffer,payload_data.length);

	net_nfc_util_create_record(NET_NFC_RECORD_EXTERNAL_RTD,
				&type_data,
				NULL,
				&payload_data,
				&tizenAppRec);

	ndef_record = &tizenAppRec;

	return NET_NFC_OK;
}
