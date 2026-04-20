#ifndef AXIONET_JSON_H
#define AXIONET_JSON_H

#include <yyjson.h>
#include "http.h"

yyjson_val* AxioRequest_JSONGetObj(AxioRequest *request, const char* key);

#endif // AXIONET_JSON_H