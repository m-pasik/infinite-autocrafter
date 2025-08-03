#include "net_utils.h"

#include <stdlib.h>
#include <string.h>

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t totalSize = size * nmemb;
    char **response_ptr = (char **)userp;

    *response_ptr = realloc(*response_ptr, strlen(*response_ptr) + totalSize + 1);

    strncat(*response_ptr, contents, totalSize);
    return totalSize;
}
