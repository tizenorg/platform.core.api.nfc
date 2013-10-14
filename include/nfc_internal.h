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

#ifndef __NFC_INTERNAL_H__
#define __NFC_INTERNAL_H__

#include <net_nfc.h>

#include "nfc.h"

typedef struct
{
	bool initialized;
	bool on_activation_doing;

	net_nfc_target_info_s *current_tag;
	net_nfc_target_handle_s *current_target;

	nfc_tag_discovered_cb on_tag_discovered_cb;
	void *on_tag_discovered_user_data;

	nfc_ndef_discovered_cb on_ndef_discovered_cb;
	void *on_ndef_discovered_user_data;

	nfc_p2p_target_discovered_cb on_p2p_target_discovered_cb;
	void *on_p2p_target_discovered_user_data;

	nfc_p2p_data_recived_cb on_p2p_recv_cb;
	void *on_p2p_recv_user_data;

	nfc_se_transaction_event_cb on_se_transaction_event_cb;
	void *on_se_transaction_event_user_data;
}_nfc_context_s;

#endif //__NFC_INTERNAL_H__