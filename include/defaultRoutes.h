#ifndef AXIONET_DEFAULTROUTES_H
#define AXIONET_DEFAULTROUTES_H

#include "axionetd.h"

void route404(AxioResponse *resp); // Not Found
void route405(AxioResponse *resp); // Not Allowed
void route400(AxioResponse *resp); // Bad Request

#endif // AXIONET_DEFAULTROUTES_H