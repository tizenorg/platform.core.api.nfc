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

#include <Elementary.h>
#include <Ecore.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <nfc.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

int is_terminate = 0;
int timeout_counter;
int success;


Eina_Bool timeout_handler(void *data){
	if( timeout_counter == 0 ){
		is_terminate = 1;
		printf("Fail, timeout!\n");
		return 0;
	}
	timeout_counter--;

	if( success )
		return 0;

	return 1;
}

int print_result(const char * func, int error )
{
	printf("%s ret = %x\n", func , error);
	if( error != 0 )
		return -1;
	return 0;
}

char *byteToString(unsigned char* buffer, int size){
	static char localbuffer[255];
	memset(localbuffer, 0, 255);
	memcpy(localbuffer, buffer, size);
	return localbuffer;	
}

void print_ndef_message(nfc_ndef_message_h message){
	char *tnf_tbl[] = {
			"NFC_RECORD_TNF_EMPTY", 
			"NFC_RECORD_TNF_WELL_KNOWN",
			"NFC_RECORD_TNF_MIME_MEDIA",
			"NFC_RECORD_TNF_URI",
			"NFC_RECORD_TNF_EXTERNAL_RTD",
			"NFC_RECORD_TNF_UNKNOWN",
			"NFC_RECORD_TNF_UNCHAGNED"
		};
	nfc_record_tnf_e tnf;
	unsigned char *type;
	int type_size;
	unsigned char *payload;
	int payload_size;
	nfc_ndef_record_h record;
	int ret;

	ret = nfc_ndef_message_get_record(message, 0 , &record);
	if( ret != 0 ){
		printf("can't found NDEF Record\n");
		return;
	}

		
	nfc_ndef_record_get_tnf(record, &tnf);
	nfc_ndef_record_get_type(record, &type, &type_size);
	nfc_ndef_record_get_payload(record, &payload, &payload_size);
	
	printf("tnf \t: %s\n", tnf_tbl[tnf]);
	printf("type \t: %s\n", byteToString(type, type_size));
	printf("payload : %s\n", byteToString(payload, payload_size)	);	

	if( tnf == NFC_RECORD_TNF_WELL_KNOWN && type[0] == 'U' ){
		char *uri;
		nfc_ndef_record_get_uri(record, &uri);
		printf("uri \t: %s\n", uri);
		free(uri);
	}

	if( tnf == NFC_RECORD_TNF_WELL_KNOWN && type[0] == 'T' ){
		char *text;
		nfc_ndef_record_get_text(record, &text);
		printf("text \t: %s\n", text);
		free(text);
		nfc_ndef_record_get_langcode(record , &text);
		printf("langcode : %s\n", text);
		free(text);
	}
	
}


int ndef_record_create_test(){
	int ret=0;
	char *tnf_tbl[] = {
			"NFC_RECORD_TNF_EMPTY", 
			"NFC_RECORD_TNF_WELL_KNOWN",
			"NFC_RECORD_TNF_MIME_MEDIA",
			"NFC_RECORD_TNF_URI",
			"NFC_RECORD_TNF_EXTERNAL_RTD",
			"NFC_RECORD_TNF_UNKNOWN",
			"NFC_RECORD_TNF_UNCHAGNED"
		};
	nfc_ndef_record_h record1;
	nfc_record_tnf_e tnf;
	unsigned char *type;
	int type_size;
	unsigned char *id;
	int id_size;
	unsigned char *payload;
	int payload_size;
	
	char *strp = NULL;
	char *strp2 = NULL;	
	

	printf("---------------------------------------------------\n");
	printf(" NEF Record Create Test\n");	
	
	ret = nfc_ndef_record_create(&record1, NFC_RECORD_TNF_WELL_KNOWN, NFC_RECORD_SMART_POSTER_TYPE, sizeof(NFC_RECORD_SMART_POSTER_TYPE), (unsigned char*)"id", strlen("id"), (unsigned char*)"testpayload", strlen("testpayload"));
	print_result("nfc_ndef_record_create" , ret);
	
	nfc_ndef_record_get_tnf(record1, &tnf);
	nfc_ndef_record_get_type(record1, &type, &type_size);
	nfc_ndef_record_get_id(record1, &id, &id_size);	
	nfc_ndef_record_get_payload(record1, &payload, &payload_size);

	printf("tnf \t: %s\n", tnf_tbl[tnf]);
	printf("type \t: %s\n", byteToString(type, type_size));
	printf("id \t: %s\n" , byteToString(id, id_size) );
	printf("payload : %s\n", byteToString(payload, payload_size)	);		
	
	nfc_ndef_record_destroy(record1);
	
	printf("\n");

	strp = NULL;
	ret = nfc_ndef_record_create_mime(&record1, "text/plain", (unsigned char*)"the text record", sizeof("the text record"));
	print_result("nfc_ndef_record_create_mime" , ret);

	nfc_ndef_record_get_tnf(record1, &tnf);
	nfc_ndef_record_get_mime_type(record1 , &strp);
	nfc_ndef_record_get_type(record1, &type, &type_size);
	nfc_ndef_record_get_payload(record1, &payload, &payload_size);

	printf("tnf \t: %s\n", tnf_tbl[tnf]);
	printf("mimetype: %s\n", strp);
	printf("type \t: %s\n", byteToString(type, type_size));
	printf("payload : %s\n", byteToString(payload, payload_size)	);		


	nfc_ndef_record_destroy(record1);
	free(strp);
	
	printf("\n");	
	
	ret = nfc_ndef_record_create_text(&record1, "the text record", "us-en", NFC_ENCODE_UTF_8);
	print_result("nfc_ndef_record_create_text" , ret);
	
	nfc_ndef_record_get_tnf(record1, &tnf);
	nfc_ndef_record_get_type(record1, &type, &type_size);
	nfc_ndef_record_get_payload(record1, &payload, &payload_size);
	nfc_ndef_record_get_text(record1, &strp);
	nfc_ndef_record_get_langcode(record1, &strp2);

	printf("tnf \t: %s\n", tnf_tbl[tnf]);
	printf("type \t: %s\n", byteToString(type, type_size));
	printf("payload : %s\n", byteToString(payload, payload_size)	);		
	printf("text \t: %s\n", strp);
	printf("langcode: %s\n", strp2);
	nfc_ndef_record_destroy(record1);
	free(strp);
	free(strp2);
	
	printf("\n");	
	
	ret = nfc_ndef_record_create_uri(&record1,"http://samsung.com");
	print_result("nfc_ndef_record_create_uri" , ret);


	nfc_ndef_record_get_tnf(record1, &tnf);
	nfc_ndef_record_get_type(record1, &type, &type_size);
	nfc_ndef_record_get_payload(record1, &payload, &payload_size);
	nfc_ndef_record_get_uri(record1, &strp);

	printf("tnf \t: %s\n", tnf_tbl[tnf]);
	printf("type \t: %s\n", byteToString(type, type_size));
	printf("payload : %s\n", byteToString(payload, payload_size)	);		
	printf("uri \t: %s\n", strp);

	nfc_ndef_record_destroy(record1);
	free(strp);
	
	printf("\n");	

	
	ret = nfc_ndef_record_create_uri(&record1,"http://samsung.com");
	print_result("nfc_ndef_record_create_uri" , ret);

	ret = nfc_ndef_record_get_mime_type(record1, &strp);
	print_result("nfc_ndef_record_get_mime_type" , ret);
	ret = nfc_ndef_record_get_text(record1,&strp);
	print_result("nfc_ndef_record_get_text" , ret);
	ret = nfc_ndef_record_get_langcode(record1,&strp);
	print_result("nfc_ndef_record_get_langcode" , ret);
	ret = nfc_ndef_record_get_uri(record1,&strp);
	print_result("nfc_ndef_record_get_uri" , ret);
	free(strp);
	
	nfc_ndef_record_destroy(record1);

	printf("---------------------------------------------------\n");
	printf("\n");
	is_terminate = 1;
	return 0;
	
}



void _ndef_discovered_cb(nfc_ndef_message_h message, void * user_data){
	printf("Discovered NDEF Message!\n");
	print_ndef_message(message);
	success = 1;
	is_terminate = 1;
}

void read_ndef_from_tag_using_ndef_discovered(){
	int ret;
	
	success = 0;
	timeout_counter = 30;
	
	ret = nfc_manager_set_ndef_discovered_cb(_ndef_discovered_cb , NULL);
	print_result("nfc_manager_set_ndef_discovered_cb", ret);
	printf("Now, Bring the tag closer.\n");
	ecore_timer_add(1, timeout_handler, NULL);
	
}

void _write_completed_cb(nfc_error_e result ,  void * user_data){

	printf("write completed!result %s\n" , result == 0 ? "Success": "Fail");
	success  = 1;	
	is_terminate = 1;
}


void _tag_discovered_cb(nfc_discovered_type_e type, nfc_tag_h tag, void * user_data){
	if( type == NFC_DISCOVERED_TYPE_ATTACHED ){
		
		printf("Discovered Tag!\n");
		//write NDEF Message
		nfc_ndef_record_h record;
		nfc_ndef_message_h message;
		nfc_ndef_record_create_uri(&record, "http://samsung.com");
		nfc_ndef_message_create(&message);
		nfc_ndef_message_append_record(message, record);
		printf("Write request!\n");
		timeout_counter = 30;
		nfc_tag_write_ndef(tag, message , _write_completed_cb , NULL);
		nfc_ndef_message_destroy(message);		
	}
}


void write_ndef_to_tag(){
	int ret;

	success = 0;
	timeout_counter = 30;
	
	ret = nfc_manager_set_tag_discovered_cb( _tag_discovered_cb , NULL);
	print_result("nfc_manager_set_tag_discovered_cb", ret);

	printf("Now, Bring the tag closer. Will be writen a new NDEF Message\n");
	ecore_timer_add(1, timeout_handler, NULL);
		
}


void _send_completed_cb(nfc_error_e result , void *user_data){
	printf("send completed!result %s\n" , result == 0 ? "Success": "Fail");
	success  = 1;	
	is_terminate = 1;
}


Eina_Bool send_test(void *data){

	nfc_p2p_target_h target = (nfc_p2p_target_h)data;
	nfc_ndef_message_h message;
	nfc_ndef_record_h record;
	
	nfc_ndef_record_create_uri(&record, "http://samsung.com");
	nfc_ndef_message_create(&message);
	nfc_ndef_message_append_record(message, record);
	printf("Send Request!\n");
	timeout_counter =30;
	nfc_p2p_send(target, message , _send_completed_cb, NULL);
	nfc_ndef_message_destroy(message);
	return 0;
}

void _target_discovered_cb(nfc_discovered_type_e type, nfc_p2p_target_h target, void * user_data){
	if( type == NFC_DISCOVERED_TYPE_ATTACHED){
		printf("Discovered new target!\n");
		ecore_idler_add(send_test, target);
	}	
}

void send_ndef_to_peer(){
	int ret ;
	success = 0;
	timeout_counter =30;
	ret = nfc_manager_set_p2p_target_discovered_cb(_target_discovered_cb , NULL);
	printf("Now, Bring the target closer. Will be sent a new NDEF Message\n");
	ecore_timer_add(1, timeout_handler, NULL);
}


void _p2p_recv_cb(nfc_p2p_target_h target , nfc_ndef_message_h message, void *user_data){

	printf("recevie a new message!\n");
	print_ndef_message(message);

	success  = 1;	
	is_terminate = 1;
}


void _target_discovered_cb2(nfc_discovered_type_e type, nfc_p2p_target_h target, void * user_data){
	if( type == NFC_DISCOVERED_TYPE_ATTACHED){
		printf("Discovered new target!\n");
		printf("I wait a new Message.......\n");
		timeout_counter = 30;
		nfc_p2p_set_data_received_cb(target, _p2p_recv_cb,NULL);
	}else{
		printf("Peer target is detached!!!!\n");
	}
}


void recv_ndef_from_peer(){
	int ret ;
	success = 0;
	timeout_counter = 30;
	ret = nfc_manager_set_p2p_target_discovered_cb(_target_discovered_cb2 , NULL);
	printf("Now, Bring the target closer. and send a new NDEF Message to the this target\n");
	ecore_timer_add(1, timeout_handler, NULL);
	
}



Eina_Bool check_terminate(void *data){
	if( is_terminate ){
		nfc_manager_deinitialize();
		elm_exit();
		return 0;
	}
 	return 1;
}


char *menu = 
	"------------------\n"
	" a. ndef record test\n"
	" b. ndef discoverd cb test\n"
	" c. write ndef to tag \n"
	" d. send ndef to peer \n"
	" e. recv ndef from peer\n"
	"------------------\n"
	">";
	


int main(int argc, char ** argv)
{	
	elm_init(argc, argv);
	is_terminate = 0;
	char select[255];
	int ret;
	
	ret = nfc_manager_initialize();
	print_result("nfc_manager_initialize", ret);

	
	printf( menu );
	gets(select);

	switch(select[0]){
		case 'a':
			ndef_record_create_test();			
			break;
		case 'b':
			read_ndef_from_tag_using_ndef_discovered();
			break;		
		case 'c':
			write_ndef_to_tag();
			break;
		case 'd':
			send_ndef_to_peer();
			break;
		case 'e':
			recv_ndef_from_peer();
			break;			
		default:
			printf("wrong selection!\n");
			is_terminate = 1;
	}

	
		
	ecore_timer_add(1, check_terminate, NULL);

	elm_run();
	elm_shutdown();
	

	return 0;
}
