libquby_objs = \
	acceptor.o \
	connection.o \
	data_session.o \
	data_source.o \
	data_store.o \
	dispatcher.o \
	lprintf.o \
	map.o \
	message_buffer.o \
	push_lexer.o \
	push_parser.o \
	return_code.o \
	socket_utils.o \
	stop_handler.o

tests = \
	alarm_slot_test \
	connection_test \
	map_test \
	return_code_test

executables = \
	server

gcc_flags = -Wall -Werror

.DELETE_ON_ERROR :

.PHONY : all
all : $(addsuffix .ok, $(tests)) $(executables)

.PHONY: clean
clean :
	rm -f $(executables)
	rm -f *.ok
	rm -f $(tests)
	rm -f libquby.a
	rm -f *.o
	rm -f *.dep

libquby.a : $(libquby_objs)
	rm -f $@
	ar cru $@ $+

# $(call define_executable, name, <linked file> ...)
define define_executable
$1: $1.o $2 ; \
	gcc -o $1 $(gcc_flags) $1.o $2
endef

$(call define_executable, alarm_slot_test, libquby.a)
$(call define_executable, connection_test, libquby.a)
$(call define_executable, map_test, libquby.a)
$(call define_executable, server, libquby.a)
$(call define_executable, return_code_test, libquby.a)

%.ok : %
	./$<
	echo timestamp >$@

%.o : %.c 
	gcc -MMD -MF $@.dep $(gcc_flags) -c $<

-include *.dep
