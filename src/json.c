#include "../include/json.h"
#include "../include/axionetd.h"

#include <yyjson.h>

bool _ensure_jsonRoot(AxioRequest *request) {
    if (!request || !request->json)
        return false;

    if (!request->jsonRoot) {
        request->jsonRoot = yyjson_doc_get_root(request->json);
    }

    return request->jsonRoot != NULL;
}

yyjson_val* AxioRequest_JSONGetObj(AxioRequest *request, const char *key) {
    if (!request || !key) return NULL;

    if (!_ensure_jsonRoot(request) || !request->jsonRoot)
        return NULL;

    return yyjson_obj_get(request->jsonRoot, key);
}