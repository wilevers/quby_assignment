#include "return_code.h"

const char *return_code_string(return_code code)
{
	switch (code) {
	case ok :
		return "OK";
	case out_of_memory :
		return "out of memory";
	case invalid_map_key :
		return "invalid_map key";
	case cant_resolve_ip :
		return "can't resolve ip address";
	case cant_create_socket :
		return "can't create socket";
	case cant_bind :
		return "can't bind to endpoint";
	case cant_listen :
		return "can't listen";
	case cant_accept :
		return "can't accept";
	case cant_resolve_host :
		return "can't resolve host";
	case cant_connect :
		return "can't connect";
	case would_block :
		return "would block";
	case cant_send :
		return "can't send";
	case cant_receive :
		return "can't receive";
	case unexpected_null_char :
		return "unexpected null character";
	case unexpected_gt :
		return "unexpected '>'";
	case unexpected_whitespace :
		return "unexpected whitespace";
	case gt_expected :
		return "'>' expected";
	case unexpected_slash :
		return "unexpected '/'";
	case unexpected_character_data :
		return "unexpected character data";
	case unexpected_begin_element :
		return "unexpected begin element";
	case unexpected_end_element :
		return "unexpected end element";
	case message_type_mismatch :
		return "message type mismatch";
	case data_key_mismatch :
		return "data key mismatch";
	default :
		return "unknown return code";
	}
}
