#ifndef CALLBACK_FUNCTION
#define CALLBACK_FUNCTION
#include <stddef.h>
#include <stdio.h>


struct Buffer;

void acceptor(int lfd, int revents, void* data);

void reader(int cfd, int revents, void* data);

void writer(int cfd, int revents, void* data);


#endif