/*
* Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <net_nfc.h>
#include <net_nfc_typedef_private.h>
#include <dlog.h>
#include <nfc.h>
#include <nfc_private.h>
#include <net_nfc_exchanger.h>


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


#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "TIZEN_N_NFC"


static int _return_invalid_param(const char *func){
	LOGE( "[%s] INVALID_PARAMETER (0x%08x)",func, NFC_ERROR_INVALID_PARAMETER);
	return NFC_ERROR_INVALID_PARAMETER;
}

static int _convert_error_code(const char *func, int native_error_code)
{
	int error_code = NFC_ERROR_NONE;
	char * errorstr = NULL;
	switch(native_error_code){
		case 0 :
			error_code = NFC_ERROR_NONE;
			errorstr  = "ERROR_NONE";
			break;

		case NET_NFC_ALLOC_FAIL:
			error_code = NFC_ERROR_OUT_OF_MEMORY;
			errorstr  = "OUT_OF_MEMORY";
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
		case NET_NFC_NO_NDEF_SUPPORT:
		case NET_NFC_INSUFFICIENT_STORAGE:
		case NET_NFC_NOT_CONNECTED:
		case NET_NFC_NOT_INITIALIZED:
		case NET_NFC_NOT_REGISTERED:
			error_code = NFC_ERROR_OPERATION_FAILED;
			errorstr  = "OPERATION_FAILED";
			break;

		case NET_NFC_OUT_OF_BOUND:
		case NET_NFC_NULL_PARAMETER:
		case NET_NFC_NOT_ALLOWED_OPERATION:
		case NET_NFC_LLCP_INVALID_SOCKET:
		case NET_NFC_NO_DATA_FOUND:
			error_code = NFC_ERROR_INVALID_PARAMETER;
			errorstr  = "INVALID_PARAMETER";
			break;
		case NET_NFC_NDEF_RECORD_IS_NOT_EXPECTED_TYPE:
			error_code = NFC_ERROR_INVALID_RECORD_TYPE;
			errorstr  = "INVALID_RECORD_TYPE";
			break;

		case NET_NFC_ALREADY_INITIALIZED:
		case NET_NFC_ALREADY_REGISTERED:
			error_code = NFC_ERROR_NONE;
			errorstr  = "ERROR_NONE";
			break;

		case NET_NFC_RF_TIMEOUT:
			error_code = NFC_ERROR_TIMED_OUT;
			errorstr  = "TIMED_OUT";
			break;
		case NET_NFC_INVALID_FORMAT:
		case NET_NFC_NDEF_TYPE_LENGTH_IS_NOT_OK:
		case NET_NFC_NDEF_ID_LENGTH_IS_NOT_OK:
		case NET_NFC_NDEF_BUF_END_WITHOUT_ME:
		case NET_NFC_NO_NDEF_MESSAGE:
			error_code = NFC_ERROR_INVALID_NDEF_MESSAGE;
			errorstr  = "INVALID_NDEF_MESSAGE";
			break;
		case NET_NFC_BUSY :
			error_code = NFC_ERROR_DEVICE_BUSY;
			errorstr  = "DEVICE_BUSY";
			break;
		default :
			error_code = NFC_ERROR_OPERATION_FAILED;
			errorstr  = "OPERATION_FAILED";
	}

	LOGE( "[%s] %s(0x%08x)",func, errorstr, error_code);

	return error_code;

}

_nfc_context_s g_nfc_context;
static nfc_set_activation_completed_cb g_nfc_set_activation_completed_cb = NULL;


void _nfc_response_handler(net_nfc_message_e message, net_nfc_error_e result, void* data, void* user_param, void * trans_data)
{
	LOGI("[%s] message %d - start", __func__, message);

	int capi_result = _convert_error_code("EVENT", result);

	switch ( message ){
		case NET_NFC_MESSAGE_TRANSCEIVE:
		{
			if( trans_data != NULL ){
				_async_callback_data *user_cb = (_async_callback_data*)trans_data;

				if( user_cb->callback_type == _NFC_CALLBACK_TYPE_DATA ){
					unsigned char * buffer = NULL;
					int buffer_size = 0;
					if( result == 0 && data != NULL){
						data_s *arg = (data_s*) data;
						buffer = arg->buffer;
						buffer_size = arg->length;
					}
					void (* data_type_callback)(int result , unsigned char * buffer, int buffer_size,  void * user_data);
					data_type_callback = user_cb->callback;
					data_type_callback(capi_result, buffer, buffer_size, user_cb->user_data);
				}else if ( user_cb->callback_type == _NFC_CALLBACK_TYPE_RESULT){
					void (* result_type_callback)(int result , void * user_data);
					result_type_callback = user_cb->callback;
					result_type_callback(capi_result, user_cb->user_data);
				}
				free(user_cb);
			}
			break;
		}
		case NET_NFC_MESSAGE_READ_NDEF:
		{
			if( trans_data != NULL ) {
				ndef_message_h ndef_message = (ndef_message_h)data;
				_async_callback_data *user_cb = (_async_callback_data*)trans_data;
				((nfc_tag_read_completed_cb)user_cb->callback)(capi_result, ndef_message, user_cb->user_data);
				free(user_cb);
			}
			break;
		}
		case NET_NFC_MESSAGE_WRITE_NDEF:
		{
			if( trans_data != NULL ){
				_async_callback_data *user_cb = (_async_callback_data*)trans_data;
				((nfc_tag_write_completed_cb)user_cb->callback)(capi_result, user_cb->user_data);
				free(user_cb);
			}
			break;
		}
		case NET_NFC_MESSAGE_TAG_DISCOVERED:
		{
			int i;
			net_nfc_target_info_s *target_info = (net_nfc_target_info_s*)data;

			memset(&g_nfc_context.current_tag , 0 , sizeof( g_nfc_context.current_tag ));
			g_nfc_context.current_tag = * target_info;
			net_nfc_tag_info_s *list = g_nfc_context.current_tag.tag_info_list;
			net_nfc_tag_info_s *newlist ;
			newlist = (net_nfc_tag_info_s *)calloc(g_nfc_context.current_tag.number_of_keys, sizeof(net_nfc_tag_info_s));


			//copy info list
			for(i = 0; i < g_nfc_context.current_tag.number_of_keys ; i++){
				if( list[i].key ){
					newlist[i].key = strdup(list[i].key);
				}
				if ( list[i].value ){
					net_nfc_create_data(&newlist[i].value , ((data_s*)list[i].value)->buffer, ((data_s*)list[i].value)->length);
				}
			}
			g_nfc_context.current_tag.tag_info_list = newlist;

			if( g_nfc_context.on_tag_discovered_cb ){

				g_nfc_context.on_tag_discovered_cb( NFC_DISCOVERED_TYPE_ATTACHED, (nfc_tag_h)&g_nfc_context.current_tag , g_nfc_context.on_tag_discovered_user_data );
			}

			//ndef discovered cb
			if( g_nfc_context.on_ndef_discovered_cb && target_info->raw_data.buffer != NULL ){
				ndef_message_h ndef_message ;
				net_nfc_create_ndef_message_from_rawdata (&ndef_message, (data_h)&(target_info->raw_data) );
				g_nfc_context.on_ndef_discovered_cb(ndef_message , g_nfc_context.on_ndef_discovered_user_data);
				net_nfc_free_ndef_message(ndef_message);
			}
			break;
		}
		case NET_NFC_MESSAGE_NOTIFY:
		{
			break;
		}
		case NET_NFC_MESSAGE_TAG_DETACHED:
		{
			if( g_nfc_context.on_tag_discovered_cb ){
				g_nfc_context.on_tag_discovered_cb( NFC_DISCOVERED_TYPE_DETACHED,  (nfc_tag_h)&g_nfc_context.current_tag , g_nfc_context.on_tag_discovered_user_data );
			}

			net_nfc_tag_info_s* list  = g_nfc_context.current_tag.tag_info_list;

			//delete key list
			if(list != NULL)
			{
				int i = 0;
				for(i=0 ; i < g_nfc_context.current_tag.number_of_keys ; i++)	{
					if(list[i].key != NULL)
						free(list[i].key );
					if(list[i].value != NULL)
						net_nfc_free_data(list[i].value);
				}
				free(list);
			}
			if(  g_nfc_context.current_tag.keylist != NULL )
				free( g_nfc_context.current_tag.keylist);

			memset(&g_nfc_context.current_tag , 0 , sizeof( g_nfc_context.current_tag ));
			break;
		}
		case NET_NFC_MESSAGE_P2P_DISCOVERED:
		{
			g_nfc_context.current_target = (net_nfc_target_handle_h)data;
			g_nfc_context.on_p2p_recv_cb = NULL;
			g_nfc_context.on_p2p_recv_user_data = NULL;
			g_nfc_context.on_p2p_send_completed_cb = NULL;
			g_nfc_context.on_p2p_send_completed_user_data = NULL;

			if( g_nfc_context.on_p2p_discovered_cb ){
				g_nfc_context.on_p2p_discovered_cb(NFC_DISCOVERED_TYPE_ATTACHED , (nfc_p2p_target_h)g_nfc_context.current_target, g_nfc_context.on_p2p_discovered_user_data );
			}

			break;
		}
		case NET_NFC_MESSAGE_P2P_DETACHED:
		{
			if( g_nfc_context.on_p2p_discovered_cb ){
				g_nfc_context.on_p2p_discovered_cb( NFC_DISCOVERED_TYPE_DETACHED,  (nfc_p2p_target_h)(g_nfc_context.current_target) , g_nfc_context.on_p2p_discovered_user_data );
			}
			memset(&g_nfc_context.current_target , 0 , sizeof( g_nfc_context.current_target ));
			g_nfc_context.on_p2p_recv_cb = NULL;
			g_nfc_context.on_p2p_recv_user_data = NULL;
			g_nfc_context.on_p2p_send_completed_cb = NULL;
			g_nfc_context.on_p2p_send_completed_user_data = NULL;

			break;
		}
		case NET_NFC_MESSAGE_P2P_SEND :
		{
			if( g_nfc_context.on_p2p_send_completed_cb != NULL ){

				nfc_p2p_send_completed_cb 	cb = g_nfc_context.on_p2p_send_completed_cb;
				void *						user_data = g_nfc_context.on_p2p_send_completed_user_data;
				g_nfc_context.on_p2p_send_completed_cb = NULL;
				g_nfc_context.on_p2p_send_completed_user_data = NULL;
				cb(result , user_data );
			}
			break;
		}
		case NET_NFC_MESSAGE_P2P_RECEIVE :
		{
			if( g_nfc_context.on_p2p_recv_cb != NULL ){
				ndef_message_h ndef_message ;
				net_nfc_create_ndef_message_from_rawdata (&ndef_message, (data_h)(data) );
				g_nfc_context.on_p2p_recv_cb( (nfc_p2p_target_h)(g_nfc_context.current_target) , ndef_message ,g_nfc_context.on_p2p_recv_user_data );
				net_nfc_free_ndef_message(ndef_message);
			}

			break;
		}
		case NET_NFC_MESSAGE_FORMAT_NDEF:
		{
			if( trans_data != NULL) {
				_async_callback_data *user_cb = (_async_callback_data*)trans_data;
				((nfc_tag_format_completed_cb)user_cb->callback)(capi_result, user_cb->user_data);
				free(user_cb);
			}
			break;
		}

		case NET_NFC_MESSAGE_CONNECTION_HANDOVER :
		{
			if( g_nfc_context.on_p2p_connection_handover_completed_cb != NULL ){

				net_nfc_conn_handover_carrier_type_e type = NET_NFC_CONN_HANDOVER_CARRIER_UNKNOWN;
				nfc_ac_type_e carrior_type = NFC_AC_TYPE_UNKNOWN;
				char * ac_data = NULL;
				int ac_data_size = 0;
				char * temp = NULL;
				char buffer[50] = {0,};
				data_h ac_info = NULL;


				net_nfc_exchanger_get_alternative_carrier_type((net_nfc_connection_handover_info_h)data, &type);
				if (type == NET_NFC_CONN_HANDOVER_CARRIER_BT)
				{
					carrior_type = NFC_AC_TYPE_BT;
					if(net_nfc_exchanger_get_alternative_carrier_data((net_nfc_connection_handover_info_h)data, &ac_info)== 0)
					{
						temp = (char *)net_nfc_get_data_buffer(ac_info);
						if( temp != NULL)
						{

							snprintf(buffer, 50, "%02x:%02x:%02x:%02x:%02x:%02x",temp[0], temp[1], temp[2], temp[3], temp[4], temp[5]);

							 ac_data = (strdup(buffer));
							 ac_data_size = strlen(ac_data ) +1;
						}
						net_nfc_free_data(ac_info);
					}
				}

				nfc_p2p_connection_handover_completed_cb	cb = g_nfc_context.on_p2p_connection_handover_completed_cb;
				void *										user_data = g_nfc_context.on_p2p_connection_handover_completed_user_data;
				g_nfc_context.on_p2p_connection_handover_completed_cb = NULL;
				g_nfc_context.on_p2p_connection_handover_completed_user_data = NULL;
				cb(result , carrior_type, (void *)ac_data, ac_data_size, user_data );

				net_nfc_exchanger_free_alternative_carrier_data((net_nfc_connection_handover_info_h)data);
				free(ac_data);
			}
			break;
		}


		case NET_NFC_MESSAGE_IS_TAG_CONNECTED :
		{
			net_nfc_target_type_e  devType = *(net_nfc_target_type_e *)data;

			if( (devType == NET_NFC_NFCIP1_TARGET )||(devType == NET_NFC_NFCIP1_INITIATOR ))
			{
				net_nfc_get_current_target_handle(trans_data);
			}
			else if( (devType > NET_NFC_UNKNOWN_TARGET )&&(devType < NET_NFC_NFCIP1_TARGET ))
			{
				net_nfc_get_current_tag_info(trans_data);
			}
			else
			{
				if( g_nfc_context.on_initialize_completed_cb ){
					nfc_initialize_completed_cb	 	cb = g_nfc_context.on_initialize_completed_cb;
					g_nfc_context.on_initialize_completed_cb = NULL;
					cb( result,trans_data );
				}
			}
			break;
		}

		case NET_NFC_MESSAGE_GET_CURRENT_TAG_INFO :
		{
			int i;
			net_nfc_target_info_s *target_info = (net_nfc_target_info_s*)data;

			memset(&g_nfc_context.current_tag , 0 , sizeof( g_nfc_context.current_tag ));

			if(result == NET_NFC_OK)
			{
				g_nfc_context.current_tag = * target_info;
				net_nfc_tag_info_s *list = g_nfc_context.current_tag.tag_info_list;
				net_nfc_tag_info_s *newlist ;
				newlist = (net_nfc_tag_info_s *)calloc(g_nfc_context.current_tag.number_of_keys, sizeof(net_nfc_tag_info_s));

				//copy info list
				for(i = 0; i < g_nfc_context.current_tag.number_of_keys ; i++){
					if( list[i].key ){
						newlist[i].key = strdup(list[i].key);
					}
					if ( list[i].value ){
						net_nfc_create_data(&newlist[i].value , ((data_s*)list[i].value)->buffer, ((data_s*)list[i].value)->length);
					}
				}
				g_nfc_context.current_tag.tag_info_list = newlist;

			}

			nfc_initialize_completed_cb	 	cb = g_nfc_context.on_initialize_completed_cb;
			g_nfc_context.on_initialize_completed_cb = NULL;
			if( cb ){
				cb( result, trans_data );
			}
			break;
		}

		case NET_NFC_MESSAGE_GET_CURRENT_TARGET_HANDLE :
		{
			memset(&g_nfc_context.current_target , 0 , sizeof( g_nfc_context.current_target ));
			g_nfc_context.on_p2p_recv_cb = NULL;
			g_nfc_context.on_p2p_recv_user_data = NULL;
			g_nfc_context.on_p2p_send_completed_cb = NULL;
			g_nfc_context.on_p2p_send_completed_user_data = NULL;

			if(result == NET_NFC_OK)
			{
				g_nfc_context.current_target = (net_nfc_target_handle_h)data;
			}

			nfc_initialize_completed_cb	 	cb = g_nfc_context.on_initialize_completed_cb;
			g_nfc_context.on_initialize_completed_cb = NULL;
			if( cb ){
				cb( result, trans_data );
			}

			break;
		}

		case NET_NFC_MESSAGE_SE_START_TRANSACTION :
		case NET_NFC_MESSAGE_SE_END_TRANSACTION :
		case NET_NFC_MESSAGE_SE_TYPE_TRANSACTION:
		case NET_NFC_MESSAGE_SE_CONNECTIVITY :
		case NET_NFC_MESSAGE_SE_FIELD_ON :
		case NET_NFC_MESSAGE_SE_FIELD_OFF :
		{
			nfc_se_event_e event = NFC_SE_EVENT_START_TRANSACTION;
			switch( message ){
				case NET_NFC_MESSAGE_SE_START_TRANSACTION:
					event = NFC_SE_EVENT_START_TRANSACTION;
					break;
				case NET_NFC_MESSAGE_SE_END_TRANSACTION:
					event = NFC_SE_EVENT_END_TRANSACTION;
					break;
				case NET_NFC_MESSAGE_SE_CONNECTIVITY:
					event = NFC_SE_EVENT_CONNECTIVITY;
					break;
				case NET_NFC_MESSAGE_SE_FIELD_ON :
					event = NFC_SE_EVENT_FIELD_ON;
					break;
				case NET_NFC_MESSAGE_SE_FIELD_OFF :
					event = NFC_SE_EVENT_FIELD_OFF;
					break;
				case NET_NFC_MESSAGE_SE_TYPE_TRANSACTION:
					event = NFC_SE_EVENT_TRANSACTION;
					break;
				default:
					break;
			}

			if( g_nfc_context.on_se_event_cb ){
				g_nfc_context.on_se_event_cb(event, g_nfc_context.on_se_event_user_data);
			}
			if( message == NET_NFC_MESSAGE_SE_TYPE_TRANSACTION){
				net_nfc_se_event_info_s* transaction_data = (net_nfc_se_event_info_s*)data;
				if( g_nfc_context.on_se_transaction_event_cb && transaction_data != NULL){
					g_nfc_context.on_se_transaction_event_cb(transaction_data->aid.buffer,transaction_data->aid.length, transaction_data->param.buffer,transaction_data->param.length  , g_nfc_context.on_se_transaction_event_user_data);
				}
			}
		}

		default :
			break;
	}

}

static void __nfc_set_activation_completed(net_nfc_error_e error, void *user_data)
{
	int capi_result = _convert_error_code("nfc_set_activation_completed", error);

	nfc_set_activation_completed_cb cb = g_nfc_set_activation_completed_cb;
	g_nfc_set_activation_completed_cb = NULL;
	if(cb)
	{
		cb(capi_result, user_data);
	}
	return ;

}


int nfc_manager_set_activation(bool activation, nfc_set_activation_completed_cb callback, void *user_data)
{
	int ret = 0;
	int nfc_supported = 0;

	net_nfc_is_supported(&nfc_supported);

	if(!nfc_supported)
	{
		ret =   NFC_ERROR_NOT_SUPPORTED;
	}
	else
	{

		if( nfc_manager_is_activated() == activation)
		{
			if(activation)
			{
				ret =   NFC_ERROR_ALREADY_ACTIVATED;
			}
			else
			{
				ret =   NFC_ERROR_ALREADY_DEACTIVATED;
			}
		}
		else
		{
			g_nfc_set_activation_completed_cb = callback;
			ret = net_nfc_set_state(activation, __nfc_set_activation_completed);
			if (ret)
			{
				ret =   NFC_ERROR_OPERATION_FAILED;
			}
			else
			{
				ret =   NFC_ERROR_NONE;
			}
		}
	}
	return ret;

}


bool nfc_manager_is_activated(void)
{
	int activated = 0;

	net_nfc_get_state(&activated);

	if(activated)
	{
		return true;
	}
	else
	{
		return false;
	}

}

int nfc_manager_initialize (nfc_initialize_completed_cb callback, void *user_data)
{
	int ret;

	if(!nfc_manager_is_activated())
	{
		return NFC_ERROR_NOT_ACTIVATED;
	}
	ret = net_nfc_initialize();
	if( ret != NET_NFC_OK )
		return _convert_error_code(__func__, ret);

	memset( &g_nfc_context , 0 , sizeof( g_nfc_context));
	net_nfc_set_response_callback( _nfc_response_handler , &g_nfc_context);
	net_nfc_state_activate (1);
	g_nfc_context.on_initialize_completed_cb = callback;
	ret = net_nfc_is_tag_connected(user_data);
	if( ret != NET_NFC_OK )
		return _convert_error_code(__func__, ret);

	return NFC_ERROR_NONE;
}
int nfc_manager_deinitialize (void)
{
	int ret;
	ret = net_nfc_deinitialize();


	if( ret == 0)
		net_nfc_unset_response_callback();

	net_nfc_state_deactivate();

	return _convert_error_code(__func__, ret);
}

int nfc_manager_set_tag_discovered_cb( nfc_tag_discovered_cb callback , void * user_data)
{
	if( callback == NULL)
		return _return_invalid_param(__func__);
	g_nfc_context.on_tag_discovered_cb = callback;
	g_nfc_context.on_tag_discovered_user_data = user_data;
	return 0;
}
void nfc_manager_unset_tag_discovered_cb( void )
{
	g_nfc_context.on_tag_discovered_cb = NULL;
	g_nfc_context.on_tag_discovered_user_data = NULL;
}

int nfc_manager_set_ndef_discovered_cb( nfc_ndef_discovered_cb callback , void *user_data)
{
	if( callback == NULL)
		return _return_invalid_param(__func__);
	g_nfc_context.on_ndef_discovered_cb= callback;
	g_nfc_context.on_ndef_discovered_user_data= user_data;
	return 0;
}

void nfc_manager_unset_ndef_discovered_cb( void )
{

	g_nfc_context.on_ndef_discovered_cb = NULL;
	g_nfc_context.on_ndef_discovered_user_data = NULL;
}


void nfc_manager_set_tag_filter(int filter )
{

	int ret ;
	ret = net_nfc_set_tag_filter(filter);
}

int nfc_manager_get_tag_filter(void)
{

	return net_nfc_get_tag_filter();
}


int nfc_manager_get_connected_tag(nfc_tag_h * tag)
{
	int ret;
	if( tag == NULL )
		return _return_invalid_param(__func__);

	if(g_nfc_context.current_tag.handle == NULL)
	{
		ret = NFC_ERROR_NO_DEVICE;
	}
	else
	{
		*tag = (nfc_tag_h)&g_nfc_context.current_tag;
		ret = NFC_ERROR_NONE;
	}
	return ret;
}

int nfc_manager_get_connected_target(nfc_p2p_target_h *target)
{
	int ret;
	if( target == NULL )
		return _return_invalid_param(__func__);

	if(g_nfc_context.current_target == NULL)
	{
		ret = NFC_ERROR_NO_DEVICE;
	}
	else
	{
		*target = (nfc_p2p_target_h)g_nfc_context.current_target;
		ret = NFC_ERROR_NONE;
	}

	return ret;
}


int nfc_manager_set_system_handler_enable(bool enable)
{
	int ret = NFC_ERROR_NONE;

	ret = net_nfc_set_launch_popup_state(enable);

	if( ret != NET_NFC_OK )
		return _convert_error_code(__func__, ret);
	else
		return ret;


}

bool nfc_manager_is_system_handler_enabled(void)
{
	return net_nfc_get_launch_popup_state();

}




int nfc_manager_get_cached_message(nfc_ndef_message_h *ndef_message){
	int ret;
	if( ndef_message == NULL )
		return _return_invalid_param(__func__);
	ret = net_nfc_retrieve_current_ndef_message(ndef_message);
	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_create(nfc_ndef_record_h* record, nfc_record_tnf_e tnf, const unsigned char* type, int type_size , const unsigned char * id , int id_size, const unsigned char * payload, int payload_size)
{

	if(record == NULL  )
		return _return_invalid_param(__func__);

	data_s type_data = { (unsigned char*)type, type_size };
	data_s id_data = { (unsigned char*)id , id_size };
	data_s payload_data = {(unsigned char*)payload , payload_size };
	int ret;
	ret = net_nfc_create_record((ndef_record_h*)record , tnf , (data_h)&type_data , (data_h)&id_data, (data_h)&payload_data);

	return _convert_error_code(__func__, ret);
}
int nfc_ndef_record_create_text(nfc_ndef_record_h* record, const char * text, const char * lang_code, nfc_encode_type_e encode )
{

	if(record == NULL ||  text == NULL || lang_code == NULL )
		return _return_invalid_param(__func__);
	int ret;
	ret = net_nfc_create_text_type_record((ndef_record_h*)record, text, lang_code, encode);
	return _convert_error_code(__func__, ret);
}
int nfc_ndef_record_create_uri(nfc_ndef_record_h* record, const char* uri)
{

	if(record == NULL ||  uri == NULL)
		return _return_invalid_param(__func__);
	int ret;
	ret = net_nfc_create_uri_type_record((ndef_record_h*)record , uri , NET_NFC_SCHEMA_FULL_URI);
	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_create_mime(nfc_ndef_record_h* record, const char * mime_type , const unsigned char * data , int data_size)
{
	if(record == NULL ||  mime_type == NULL || data == NULL)
		return _return_invalid_param(__func__);
	return nfc_ndef_record_create(record , NFC_RECORD_TNF_MIME_MEDIA , (unsigned char *) mime_type, strlen(mime_type), NULL, 0, data, data_size );
}

int nfc_ndef_record_get_mime_type(nfc_ndef_record_h record , char **mime_type){

	int ret;
	unsigned char *typename;
	int length;

	if(record == NULL ||  mime_type == NULL)
		return _return_invalid_param(__func__);
	nfc_record_tnf_e tnf;
	if( nfc_ndef_record_get_tnf(record, &tnf ) != 0 || tnf != NFC_RECORD_TNF_MIME_MEDIA ){
		return NFC_ERROR_INVALID_RECORD_TYPE;
	}

	ret = nfc_ndef_record_get_type(record,&typename, &length);
	if( ret != 0 )
		return _convert_error_code(__func__, ret);

	*mime_type = malloc(length+1);
	if( *mime_type == NULL ){
		LOGE( "[%s] OUT_OF_MEMORY (0x%08x)",__func__ , NFC_ERROR_OUT_OF_MEMORY);
		return NFC_ERROR_OUT_OF_MEMORY;
	}

	memset(*mime_type, 0 , length+1);
	memcpy(*mime_type, typename , length );
	return 0;
}


int nfc_ndef_record_destroy(nfc_ndef_record_h record)
{

	if(record == NULL  )
		return _return_invalid_param(__func__);
	int ret;
	ret = net_nfc_free_record(record);
	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_set_id (nfc_ndef_record_h record, unsigned char *id , int id_size)
{

	if(record == NULL ||  id == NULL )
		return _return_invalid_param(__func__);
	int ret;
	data_s id_data = {id, id_size};
	ret = net_nfc_set_record_id(record, (data_h)&id_data);
	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_get_payload (nfc_ndef_record_h record, unsigned char ** payload, int *size)
{

	if(record == NULL ||  payload == NULL || size == NULL )
		return _return_invalid_param(__func__);
	int ret ;
	data_s *payload_data;
	ret = net_nfc_get_record_payload(record, (data_h*)&payload_data);
	if( ret == 0){
		*payload = payload_data->buffer;
		*size = payload_data->length;
	}
	return _convert_error_code(__func__, ret);
}


int nfc_ndef_record_get_type (nfc_ndef_record_h record, unsigned char ** type, int *size)
{

	int ret ;
	data_s *type_data;
	ret = net_nfc_get_record_type(record, (data_h*)&type_data);
	if( ret == 0){
		*type = type_data->buffer;
		*size = type_data->length;
	}
	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_get_id (nfc_ndef_record_h record, unsigned char **id , int *size)
{

	if(record == NULL ||  id == NULL || size == NULL )
		return _return_invalid_param(__func__);

	int ret ;
	data_s *id_data;
	ret = net_nfc_get_record_id(record, (data_h*)&id_data);
	if( ret == 0){
		*id = id_data->buffer;
		*size = id_data->length;
	}
	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_get_tnf(nfc_ndef_record_h record, nfc_record_tnf_e * tnf)
{

	if(record == NULL ||  tnf == NULL )
		return _return_invalid_param(__func__);
	int ret;
	ret = net_nfc_get_record_tnf(record, (net_nfc_record_tnf_e*)tnf);
	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_get_text(nfc_ndef_record_h record, char** buffer)
{

	if(record == NULL ||  buffer == NULL)
		return _return_invalid_param(__func__);
	int ret;
	ret = net_nfc_create_text_string_from_text_record(record, buffer);
	return _convert_error_code(__func__, ret);
}

int nfc_ndef_record_get_langcode(nfc_ndef_record_h record, char **lang_code)
{

	if(record == NULL ||  lang_code == NULL)
		return _return_invalid_param(__func__);
	int ret;
	ret = net_nfc_get_languange_code_string_from_text_record(record, lang_code);
	return _convert_error_code(__func__, ret);
}
int nfc_ndef_record_get_encode_type(nfc_ndef_record_h record, nfc_encode_type_e *encode)
{

	if(record == NULL ||  encode == NULL  )
		return _return_invalid_param(__func__);
	int ret=0;
	ret = net_nfc_get_encoding_type_from_text_record(record, (net_nfc_encode_type_e*)encode);
//	if( ret == NFC_NDEF_RECORD_IS_NOT_EXPECTED_TYPE)
//		LOGE("%s reord type is not text type");
	return _convert_error_code(__func__, ret);
}
int nfc_ndef_record_get_uri(nfc_ndef_record_h record , char **uri)
{

	if(record == NULL ||  uri == NULL  )
		return _return_invalid_param(__func__);
	int ret=0;
	ret = net_nfc_create_uri_string_from_uri_record(record, uri);
	return _convert_error_code(__func__, ret);
}


int nfc_ndef_message_create(nfc_ndef_message_h* ndef_message)
{

	if( ndef_message == NULL )
		return _return_invalid_param(__func__);
	int ret=0;
	ret = net_nfc_create_ndef_message(ndef_message);
	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_create_from_rawdata(nfc_ndef_message_h* ndef_message, const unsigned char* rawdata, int rawdata_size)
{

	if( ndef_message == NULL || rawdata == NULL)
		return _return_invalid_param(__func__);
	int ret=0;
	data_s rawdata_data = {(unsigned char *)rawdata, rawdata_size};
	ret = net_nfc_create_ndef_message_from_rawdata((ndef_message_h*)ndef_message , (data_h)&rawdata_data);
	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_destroy(nfc_ndef_message_h ndef_message)
{

	if( ndef_message == NULL )
		return _return_invalid_param(__func__);
	int ret=0;
	ret = net_nfc_free_ndef_message(ndef_message);
	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_get_record_count(nfc_ndef_message_h ndef_message , int *count)
{

	if( ndef_message == NULL || count == NULL)
		return _return_invalid_param(__func__);
	int ret=0;
	ret = net_nfc_get_ndef_message_record_count(ndef_message , count);
	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_get_rawdata(nfc_ndef_message_h ndef_message , unsigned char ** rawdata , int *rawdata_size)
{

	if( ndef_message == NULL || rawdata == NULL || rawdata_size == NULL)
		return _return_invalid_param(__func__);
	int ret=0;
	data_s *rawdata_data;
	ret = net_nfc_create_rawdata_from_ndef_message(ndef_message , (data_h*)&rawdata_data);
	if( ret == 0 )
	{
		*rawdata = rawdata_data->buffer;
		*rawdata_size = rawdata_data->length;
	}
	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_get_rawdata_size(nfc_ndef_message_h ndef_message , int *byte_size)
{

	if( ndef_message == NULL || byte_size == NULL)
		return _return_invalid_param(__func__);
	int ret=0;
	ret = net_nfc_get_ndef_message_byte_length(ndef_message , byte_size);
	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_append_record(nfc_ndef_message_h ndef_message , nfc_ndef_record_h record)
{

	if( ndef_message == NULL || record == NULL)
		return _return_invalid_param(__func__);
	int ret=0;
	ret = net_nfc_append_record_to_ndef_message(ndef_message , record );
	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_insert_record(nfc_ndef_message_h ndef_message , int index, nfc_ndef_record_h record)
{

	if( ndef_message == NULL || record == NULL )
		return _return_invalid_param(__func__);
	int ret=0;
	ret = net_nfc_append_record_by_index(ndef_message , index , record);
	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_remove_record(nfc_ndef_message_h ndef_message , int index)
{

	if( ndef_message == NULL )
		return _return_invalid_param(__func__);
	int ret=0;
	ret = net_nfc_remove_record_by_index(ndef_message , index);
	return _convert_error_code(__func__, ret);
}

int nfc_ndef_message_get_record(nfc_ndef_message_h ndef_message , int index , nfc_ndef_record_h *record)
{

	if( ndef_message == NULL || record == NULL)
		return _return_invalid_param(__func__);
	int ret=0;
	ret = net_nfc_get_record_by_index(ndef_message , index , (ndef_record_h*)record);
	return _convert_error_code(__func__, ret);
}

int nfc_tag_get_type( nfc_tag_h tag , nfc_tag_type_e *type)
{

	if( tag == NULL || type == NULL)
		return _return_invalid_param(__func__);
	int ret=0;
	ret = net_nfc_get_tag_type( tag, (net_nfc_target_type_e*) type);
	return _convert_error_code(__func__, ret);
}

int nfc_tag_is_support_ndef(nfc_tag_h tag, bool * is_support)
{

	if( tag == NULL || is_support== NULL)
		return _return_invalid_param(__func__);
	int ret=0;
	ret = net_nfc_get_tag_ndef_support(tag , is_support);
	return _convert_error_code(__func__, ret);
}

int nfc_tag_get_maximum_ndef_size(nfc_tag_h tag , unsigned int * max_size)
{

	if( tag == NULL || max_size == NULL)
		return _return_invalid_param(__func__);
	int ret=0;
	ret = net_nfc_get_tag_max_data_size(tag , max_size);
	return _convert_error_code(__func__, ret);
}

int nfc_tag_get_ndef_size(nfc_tag_h tag, unsigned int * actual_data_size)
{

	if( tag == NULL || actual_data_size == NULL)
		return _return_invalid_param(__func__);
	int ret=0;
	ret = net_nfc_get_tag_actual_data_size(tag , actual_data_size);
	return _convert_error_code(__func__, ret);
}

int nfc_tag_foreach_information(nfc_tag_h tag , nfc_tag_information_cb callback , void * user_data ){
	char **keys;
	int number_of_keys;
	int i;
	int ret ;

	if( tag == NULL || callback == NULL )
		return _return_invalid_param(__func__);

	ret = net_nfc_get_tag_info_keys(tag, &keys , &number_of_keys);
	if( ret != 0 )
		return _convert_error_code(__func__, ret);

	for( i = 0 ; i < number_of_keys ; i++){
		data_s *tmpdata;
		ret = net_nfc_get_tag_info_value( tag, keys[i], (data_h*)&tmpdata);
		if( ret == 0 ){
			bool cont;
			cont = callback(keys[i], tmpdata->buffer, tmpdata->length, user_data);
			if( !cont )
				break;
		}
	}

	//free(keys);

	return 0;

}


int nfc_tag_transceive( nfc_tag_h tag, unsigned char * buffer, int buffer_size,  nfc_tag_transceive_completed_cb callback , void * user_data )
{

	if( tag == NULL )
		return _return_invalid_param(__func__);
	int ret=0;
	data_s rawdata = { buffer, buffer_size };
	net_nfc_target_info_s *tag_info = (net_nfc_target_info_s*)tag;


	_async_callback_data * trans_data = NULL;
	if( callback != NULL ){
	 	trans_data = (_async_callback_data*)malloc( sizeof(_async_callback_data));
		if(trans_data == NULL )
			return NFC_ERROR_OUT_OF_MEMORY;
		memset(trans_data , 0 , sizeof(_async_callback_data));
		trans_data->callback = callback;
		trans_data->user_data = user_data;
		trans_data->callback_type = _NFC_CALLBACK_TYPE_DATA;
	}
	ret = net_nfc_transceive((net_nfc_target_handle_h)tag_info->handle , (data_h) &rawdata, trans_data );


	return _convert_error_code(__func__, ret);
}

int nfc_tag_read_ndef( nfc_tag_h tag, nfc_tag_read_completed_cb callback , void * user_data)
{

	if( tag == NULL )
		return _return_invalid_param(__func__);
	int ret=0;
	_async_callback_data * trans_data = NULL;
	net_nfc_target_info_s *tag_info = (net_nfc_target_info_s*)tag;


	if( callback != NULL ){
	 	trans_data = (_async_callback_data*)malloc( sizeof(_async_callback_data));
		if(trans_data == NULL )
			return NFC_ERROR_OUT_OF_MEMORY;
		memset(trans_data , 0 , sizeof(_async_callback_data));
		trans_data->callback = callback;
		trans_data->user_data = user_data;
	}
	ret = net_nfc_read_tag((net_nfc_target_handle_h)tag_info->handle , trans_data );
	return _convert_error_code(__func__, ret);
}
int nfc_tag_write_ndef(nfc_tag_h tag, nfc_ndef_message_h msg , nfc_tag_write_completed_cb callback ,  void *user_data)
{

	if( tag == NULL )
		return _return_invalid_param(__func__);
	int ret=0;
	_async_callback_data * trans_data = NULL;
	net_nfc_target_info_s *tag_info = (net_nfc_target_info_s*)tag;

	if (tag_info->ndefCardState == NET_NFC_NDEF_CARD_READ_ONLY )
	{
		return NFC_ERROR_READ_ONLY_NDEF;

	}

	 int byte_size = 0;
	nfc_ndef_message_get_rawdata_size(msg , &byte_size);

	if(tag_info->maxDataSize < byte_size)
	{
		return NFC_ERROR_NO_SPACE_ON_NDEF;
	}



	if( callback != NULL ){
	 	trans_data = (_async_callback_data*)malloc( sizeof(_async_callback_data));
		if(trans_data == NULL )
			return NFC_ERROR_OUT_OF_MEMORY;
		memset(trans_data , 0 , sizeof(_async_callback_data));
		trans_data->callback = callback;
		trans_data->user_data = user_data;
	}
	ret = net_nfc_write_ndef( (net_nfc_target_handle_h)tag_info->handle , msg , trans_data );
	return _convert_error_code(__func__, ret);
}

int nfc_tag_format_ndef(nfc_tag_h tag , unsigned char * key, int key_size , nfc_tag_format_completed_cb callback, void * user_data )
{

	if( tag == NULL )
		return _return_invalid_param(__func__);
	data_s key_data = { key, key_size };
	int ret=0;
	net_nfc_target_info_s *tag_info = (net_nfc_target_info_s*)tag;

	_async_callback_data * trans_data = NULL;
	if( callback != NULL ){
	 	trans_data = (_async_callback_data*)malloc( sizeof(_async_callback_data));
		if(trans_data == NULL )
			return NFC_ERROR_OUT_OF_MEMORY;
		memset(trans_data , 0 , sizeof(_async_callback_data));
		trans_data->callback = callback;
		trans_data->user_data = user_data;
	}

	ret = net_nfc_format_ndef( (net_nfc_target_handle_h)tag_info->handle, (data_h)&key_data, trans_data );
	return _convert_error_code(__func__, ret);
}


int nfc_mifare_authenticate_with_keyA(nfc_tag_h tag,  int sector_index, unsigned char * auth_key, nfc_mifare_authenticate_with_keyA_completed_cb callback, void *user_data)
{

	if( tag == NULL )
		return _return_invalid_param(__func__);
	data_s auth_key_data = { auth_key , 6};
	int ret = 0;
	net_nfc_target_info_s *tag_info = (net_nfc_target_info_s*)tag;

	_async_callback_data * trans_data = NULL;
	if( callback != NULL ){
	 	trans_data = (_async_callback_data*)malloc( sizeof(_async_callback_data));
		if(trans_data == NULL )
			return NFC_ERROR_OUT_OF_MEMORY;
		memset(trans_data , 0 , sizeof(_async_callback_data));
		trans_data->callback = callback;
		trans_data->user_data = user_data;
		trans_data->callback_type = _NFC_CALLBACK_TYPE_RESULT;
	}

	ret = net_nfc_mifare_authenticate_with_keyA( (net_nfc_target_handle_h)tag_info->handle, sector_index, (data_h)&auth_key_data, trans_data);
	return _convert_error_code(__func__, ret);
}

int nfc_mifare_authenticate_with_keyB(nfc_tag_h tag,  int sector_index, unsigned char * auth_key, nfc_mifare_authenticate_with_keyB_completed_cb callback, void *user_data)
{

	if( tag == NULL )
		return _return_invalid_param(__func__);
	data_s auth_key_data = { auth_key , 6};
	int ret = 0;
	_async_callback_data * trans_data = NULL;
	net_nfc_target_info_s *tag_info = (net_nfc_target_info_s*)tag;

	if( callback != NULL ){
	 	trans_data = (_async_callback_data*)malloc( sizeof(_async_callback_data));
		if(trans_data == NULL )
			return NFC_ERROR_OUT_OF_MEMORY;
		memset(trans_data , 0 , sizeof(_async_callback_data));
		trans_data->callback = callback;
		trans_data->user_data = user_data;
		trans_data->callback_type = _NFC_CALLBACK_TYPE_RESULT;
	}

	ret = net_nfc_mifare_authenticate_with_keyB( (net_nfc_target_handle_h)tag_info->handle, sector_index, (data_h)&auth_key_data, trans_data);
	return _convert_error_code(__func__, ret);
}

int nfc_mifare_read_block(nfc_tag_h tag, int block_index, nfc_mifare_read_block_completed_cb callback, void *user_data)
{

	if( tag == NULL )
		return _return_invalid_param(__func__);
	int ret = 0;
	_async_callback_data * trans_data = NULL;
	net_nfc_target_info_s *tag_info = (net_nfc_target_info_s*)tag;

	if( callback != NULL ){
	 	trans_data = (_async_callback_data*)malloc( sizeof(_async_callback_data));
		if(trans_data == NULL )
			return NFC_ERROR_OUT_OF_MEMORY;
		memset(trans_data , 0 , sizeof(_async_callback_data));
		trans_data->callback = callback;
		trans_data->user_data = user_data;
		trans_data->callback_type = _NFC_CALLBACK_TYPE_DATA;
	}

	ret = net_nfc_mifare_read( (net_nfc_target_handle_h)tag_info->handle, block_index, trans_data);
	return _convert_error_code(__func__, ret);
}

int nfc_mifare_read_page(nfc_tag_h tag, int page_index, nfc_mifare_read_block_completed_cb callback, void *user_data)
{

	return nfc_mifare_read_block(tag, page_index, callback, user_data);
}

int nfc_mifare_write_block (nfc_tag_h tag, int block_index, unsigned char* buffer, int buffer_size, nfc_mifare_write_block_completed_cb callback, void* user_data)
{

	if( tag == NULL )
		return _return_invalid_param(__func__);
	int ret = 0;
	data_s block_data = { buffer , buffer_size};
	_async_callback_data * trans_data = NULL;
	net_nfc_target_info_s *tag_info = (net_nfc_target_info_s*)tag;

	if( callback != NULL ){
	 	trans_data = (_async_callback_data*)malloc( sizeof(_async_callback_data));
		if(trans_data == NULL )
			return NFC_ERROR_OUT_OF_MEMORY;
		memset(trans_data , 0 , sizeof(_async_callback_data));
		trans_data->callback = callback;
		trans_data->user_data = user_data;
		trans_data->callback_type = _NFC_CALLBACK_TYPE_RESULT;
	}

	ret = net_nfc_mifare_write_block( (net_nfc_target_handle_h)tag_info->handle, block_index, (data_h)&block_data, trans_data);
	return _convert_error_code(__func__, ret);
}

int nfc_mifare_write_page(nfc_tag_h tag, int page_index, unsigned char* buffer, int buffer_size, nfc_mifare_write_block_completed_cb callback, void* user_data)
{

	if( tag == NULL )
		return _return_invalid_param(__func__);
	int ret = 0;
	data_s block_data = { buffer , buffer_size};
	_async_callback_data * trans_data = NULL;

	net_nfc_target_info_s *tag_info = (net_nfc_target_info_s*)tag;


	if( callback != NULL ){
	 	trans_data = (_async_callback_data*)malloc( sizeof(_async_callback_data));
		if(trans_data == NULL )
			return NFC_ERROR_OUT_OF_MEMORY;
		memset(trans_data , 0 , sizeof(_async_callback_data));
		trans_data->callback = callback;
		trans_data->user_data = user_data;
		trans_data->callback_type = _NFC_CALLBACK_TYPE_RESULT;
	}

	ret = net_nfc_mifare_write_page( (net_nfc_target_handle_h)tag_info->handle, page_index, (data_h)&block_data, trans_data);
	return _convert_error_code(__func__, ret);
}

int nfc_mifare_increment(nfc_tag_h tag, int block_index, int value, nfc_mifare_increment_completed_cb callback, void *user_data)
{

	if( tag == NULL )
		return _return_invalid_param(__func__);
	int ret = 0;
	_async_callback_data * trans_data = NULL;
	net_nfc_target_info_s *tag_info = (net_nfc_target_info_s*)tag;

	if( callback != NULL ){
	 	trans_data = (_async_callback_data*)malloc( sizeof(_async_callback_data));
		if(trans_data == NULL )
			return NFC_ERROR_OUT_OF_MEMORY;
		memset(trans_data , 0 , sizeof(_async_callback_data));
		trans_data->callback = callback;
		trans_data->user_data = user_data;
		trans_data->callback_type = _NFC_CALLBACK_TYPE_RESULT;
	}

	ret = net_nfc_mifare_increment( (net_nfc_target_handle_h)tag_info->handle, block_index,value, trans_data);
	return _convert_error_code(__func__, ret);

}

int nfc_mifare_decrement(nfc_tag_h tag, int block_index, int value, nfc_mifare_decrement_completed_cb callback, void *user_data)
{

	if( tag == NULL )
		return _return_invalid_param(__func__);
	int ret = 0;
	_async_callback_data * trans_data = NULL;
	net_nfc_target_info_s *tag_info = (net_nfc_target_info_s*)tag;


	if( callback != NULL ){
	 	trans_data = (_async_callback_data*)malloc( sizeof(_async_callback_data));
		if(trans_data == NULL )
			return NFC_ERROR_OUT_OF_MEMORY;
		memset(trans_data , 0 , sizeof(_async_callback_data));
		trans_data->callback = callback;
		trans_data->user_data = user_data;
		trans_data->callback_type = _NFC_CALLBACK_TYPE_RESULT;
	}

	ret = net_nfc_mifare_decrement( (net_nfc_target_handle_h)tag_info->handle, block_index,value, trans_data);
	return _convert_error_code(__func__, ret);
}

int nfc_mifare_transfer(nfc_tag_h tag, int block_index, nfc_mifare_transfer_completed_cb callback, void *user_data)
{

	if( tag == NULL )
		return _return_invalid_param(__func__);
	int ret = 0;
	_async_callback_data * trans_data = NULL;
	net_nfc_target_info_s *tag_info = (net_nfc_target_info_s*)tag;


	if( callback != NULL ){
	 	trans_data = (_async_callback_data*)malloc( sizeof(_async_callback_data));
		if(trans_data == NULL )
			return NFC_ERROR_OUT_OF_MEMORY;
		memset(trans_data , 0 , sizeof(_async_callback_data));
		trans_data->callback = callback;
		trans_data->user_data = user_data;
		trans_data->callback_type = _NFC_CALLBACK_TYPE_RESULT;
	}

	ret = net_nfc_mifare_transfer( (net_nfc_target_handle_h)tag_info->handle, block_index, trans_data);
	return _convert_error_code(__func__, ret);
}

int nfc_mifare_restore(nfc_tag_h tag, int block_index, nfc_mifare_restore_completed_cb callback, void *user_data)
{

	if( tag == NULL )
		return _return_invalid_param(__func__);
	int ret = 0;
	_async_callback_data * trans_data = NULL;
	net_nfc_target_info_s *tag_info = (net_nfc_target_info_s*)tag;
	if( tag_info == NULL )
		return _return_invalid_param(__func__);

	if( callback != NULL ){
	 	trans_data = (_async_callback_data*)malloc( sizeof(_async_callback_data));
		if(trans_data == NULL )
			return NFC_ERROR_OUT_OF_MEMORY;
		memset(trans_data , 0 , sizeof(_async_callback_data));
		trans_data->callback = callback;
		trans_data->user_data = user_data;
		trans_data->callback_type = _NFC_CALLBACK_TYPE_RESULT;
	}

	ret = net_nfc_mifare_restore( (net_nfc_target_handle_h)tag_info->handle, block_index, trans_data);
	return _convert_error_code(__func__, ret);
}




int nfc_p2p_send(nfc_p2p_target_h target , nfc_ndef_message_h message , nfc_p2p_send_completed_cb callback, void *user_data){
	int ret;
	if( target == NULL || message == NULL  )
		return _return_invalid_param(__func__);

	net_nfc_exchanger_data_h data_handle;
	data_h rawdata;
	net_nfc_create_rawdata_from_ndef_message(message, &rawdata);
	ret = net_nfc_create_exchanger_data(&data_handle,  rawdata);
	net_nfc_free_data(rawdata);


	if( ret != 0)
		return _convert_error_code(__func__, ret);

	ret = net_nfc_send_exchanger_data(data_handle ,(net_nfc_target_handle_h)target);

	if( ret != 0 ){
		net_nfc_free_exchanger_data(data_handle);
		return _convert_error_code(__func__, ret);
	}

	g_nfc_context.on_p2p_send_completed_cb = callback;
	g_nfc_context.on_p2p_send_completed_user_data = user_data;

	return 0;
}


int nfc_p2p_connection_handover(nfc_p2p_target_h target , nfc_ac_type_e type, nfc_p2p_connection_handover_completed_cb callback, void *user_data){
	int ret;
	net_nfc_conn_handover_carrier_type_e net_ac_type = NET_NFC_CONN_HANDOVER_CARRIER_UNKNOWN;
	if( target == NULL  )
		return _return_invalid_param(__func__);

	if( type > NFC_AC_TYPE_UNKNOWN  )
		return _return_invalid_param(__func__);

	switch( type ){
		case NFC_AC_TYPE_BT:
			net_ac_type = NET_NFC_CONN_HANDOVER_CARRIER_BT;
			break;
		case NFC_AC_TYPE_WIFI:
			net_ac_type = NET_NFC_CONN_HANDOVER_CARRIER_WIFI_BSS;
			break;
		case NFC_AC_TYPE_WIFI_DIRECT:
			net_ac_type = NET_NFC_CONN_HANDOVER_CARRIER_WIFI_IBSS;
			break;
		case NFC_AC_TYPE_UNKNOWN :
			net_ac_type = NET_NFC_CONN_HANDOVER_CARRIER_UNKNOWN;
			break;

		default:
			break;
		}
	ret = net_nfc_exchanger_request_connection_handover((net_nfc_target_handle_h)target, net_ac_type);

	if( ret != 0 ){
		return _convert_error_code(__func__, ret);
	}

	g_nfc_context.on_p2p_connection_handover_completed_cb = callback;
	g_nfc_context.on_p2p_connection_handover_completed_user_data = user_data;

	return 0;
}



bool nfc_p2p_is_supported_ac_type( nfc_ac_type_e carrior){

	if( carrior > NFC_AC_TYPE_UNKNOWN  )
		return false;


	if( carrior == NFC_AC_TYPE_BT){
		return true;
	}
	else{
		return false;
	}
}

int nfc_p2p_set_data_received_cb(nfc_p2p_target_h target, nfc_p2p_data_recived_cb callback, void *user_data){
	if( target == NULL || callback == NULL )
		return _return_invalid_param(__func__);

//	if( g_nfc_context.current_target.connection_id != ((net_nfc_target_handle_s*)target)->connection_id )
	if(g_nfc_context.current_target != target )
		return _return_invalid_param(__func__);

	g_nfc_context.on_p2p_recv_cb = callback;
	g_nfc_context.on_p2p_recv_user_data = user_data;
	return 0;
}

int nfc_p2p_unset_data_received_cb(nfc_p2p_target_h target){
	if( target == NULL )
		return _return_invalid_param(__func__);

//	if( g_nfc_context.current_target.connection_id != ((net_nfc_target_handle_s*)target)->connection_id )
	if(g_nfc_context.current_target != target )
		return _return_invalid_param(__func__);

	g_nfc_context.on_p2p_recv_cb = NULL;
	g_nfc_context.on_p2p_recv_user_data = NULL;
	return 0;
}




int nfc_manager_set_p2p_target_discovered_cb( nfc_p2p_target_discovered_cb callback , void *user_data){
	if( callback == NULL )
		return _return_invalid_param(__func__);
	g_nfc_context.on_p2p_discovered_cb = callback;
	g_nfc_context.on_p2p_discovered_user_data = user_data;
	return 0;
}

void nfc_manager_unset_p2p_target_discovered_cb( void ){
	g_nfc_context.on_p2p_discovered_cb = NULL;
	g_nfc_context.on_p2p_discovered_user_data = NULL;
}



int nfc_manager_set_se_event_cb(nfc_se_event_cb callback, void *user_data){
	if( callback == NULL )
		return _return_invalid_param(__func__);
	g_nfc_context.on_se_event_cb = callback;
	g_nfc_context.on_se_event_user_data = user_data;
	return 0;
}

void nfc_manager_unset_se_event_cb(void){
	g_nfc_context.on_se_event_cb = NULL;
	g_nfc_context.on_se_event_user_data = NULL;
}

int nfc_manager_set_se_transaction_event_cb(nfc_se_transaction_event_cb callback, void *user_data){
	if( callback == NULL )
		return _return_invalid_param(__func__);
	g_nfc_context.on_se_transaction_event_cb = callback;
	g_nfc_context.on_se_transaction_event_user_data = user_data;
	return 0;
}

void nfc_manager_unset_se_transaction_event_cb(void){
	g_nfc_context.on_se_transaction_event_cb = NULL;
	g_nfc_context.on_se_transaction_event_user_data = NULL;
}
