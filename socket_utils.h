#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

enum { ip_buf_size = 41 }; // enough for IPv6

void set_nonblocking(int fd);
void set_keepalive(int fd);
void disable_nagle(int fd);
void set_reuseaddress(int fd);

void await_input(int fd);
void await_output(int fd);

void get_local_ip_address(char buf[ip_buf_size], int fd);
int get_local_port_number(int fd);

void get_remote_ip_address(char buf[ip_buf_size], int fd);
int get_remote_port_number(int fd);

#endif
