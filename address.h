#ifndef ADDRESS_H
#define ADDRESS_H

enum { ip_buf_size = 41 }; // enough for IPv6

void get_local_ip_address(char buf[ip_buf_size], int fd);
int get_local_port_number(int fd);

void get_remote_ip_address(char buf[ip_buf_size], int fd);
int get_remote_port_number(int fd);

#endif


