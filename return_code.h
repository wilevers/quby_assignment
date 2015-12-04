#ifndef RETURN_CODE_H
#define RETURN_CODE_H

typedef enum {

	ok,
	out_of_memory,
	invalid_map_key,
	cant_resolve_ip,
	cant_create_socket,
	cant_bind,
	cant_listen,
	cant_accept,
	cant_resolve_host,
	cant_connect,
	would_block,
	cant_send,
	cant_receive,
	cant_await_events,
	unexpected_null_char,
	unexpected_gt,
	unexpected_whitespace,
	gt_expected,
	unexpected_slash,
	unexpected_character_data,
	unexpected_begin_element,
	unexpected_end_element,
	message_type_mismatch,
	data_key_mismatch,
	invalid_message_type,
	key_expected,
	
	n_return_codes

} return_code;

const char *return_code_string(return_code code);

#endif
