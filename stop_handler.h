#ifndef STOP_HANDLER_H
#define STOP_HANDLER_H

#include "dispatcher.h"
#include "return_code.h"

typedef struct stop_handler stop_handler;

return_code stop_handler_create(stop_handler **result, dispatcher *disp);
void stop_handler_destroy(stop_handler *handler);

#endif


