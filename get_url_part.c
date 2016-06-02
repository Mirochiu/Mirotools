#include <stdio.h>
#include <string.h>

#define URL_PARTS_PORT_UNDEFINED -1

typedef struct {
    char protocol[16];
    char host[64];
    int  port;
    char path[128];
} UrlParts;

// input:  url
// output: url_parts 
int getUrlParts(char* url, UrlParts* url_parts) {
    static const char sepr_cand[] = {'/', '?', '#', ';'};
    char *prefix = NULL;
    char *slash = NULL;
    char *semi = NULL;
    int idx, len;
    if (!url || !url_parts) return -1;

    // init 
    memset(url_parts, 0, sizeof(UrlParts));
    url_parts->port = URL_PARTS_PORT_UNDEFINED;

    // copy url prefix as protocol name
    prefix = strstr(url, "://");
    if (prefix) {
        len = prefix-url;
        if (len > sizeof(url_parts->protocol)-1)
            len = sizeof(url_parts->protocol)-1;
        strncpy(url_parts->protocol, url, len);
        prefix += 3;
    }
    else {
        prefix = url;
    }

    // find separator
    for (idx=sizeof(sepr_cand)-1 ; idx>=0 ; --idx) {
        char *sepr = strchr(prefix, sepr_cand[idx]);
        if (sepr) {
            if (!slash || sepr<slash)
                slash = sepr;
        }
    }

    // setup host and port 
    semi = strchr(prefix, ':');
    if (semi) {
        if (!slash) {
            len = semi-prefix;
            if (len > sizeof(url_parts->host)-1)
                len = sizeof(url_parts->host)-1;
            strncpy(url_parts->host, prefix, len);
            url_parts->port = atoi(semi+1) & 0xffff;
        }
        else {
            if (semi < slash) {
                len = semi-prefix;
                if (len > sizeof(url_parts->host)-1)
                    len = sizeof(url_parts->host)-1;
                strncpy(url_parts->host, prefix, len);
                url_parts->port = atoi(semi+1) & 0xffff;
            } else {
                len = slash-prefix;
                if (len > sizeof(url_parts->host)-1)
                    len = sizeof(url_parts->host)-1;
                strncpy(url_parts->host, prefix, len);
            }
        }
    }
    else { // no port number assigned
        if (slash) {
            len = slash-prefix;
            if (len > sizeof(url_parts->host)-1)
                len = sizeof(url_parts->host)-1;
            strncpy(url_parts->host, prefix, len);
        } else {
            len = sizeof(url_parts->host)-1;
            strncpy(url_parts->host, prefix, len);
        }
    }

    if (slash) {
        strncpy(url_parts->path, slash, sizeof(url_parts->path)-1);
    }
    return 0;
}

#define LOGD printf

int main(int argc, char **argv)
{
    int                         Ret = 0;
    UrlParts                    url_parts;

    strncpy(url_parts.protocol, "udp://", sizeof(url_parts.protocol)-1);
    strncpy(url_parts.host, "224.1.6.183", sizeof(url_parts.host)-1);
    url_parts.port = 11111;

    if (argc > 1) {
        getUrlParts(argv[1], &url_parts);
        LOGD("protocol '%s'\n", url_parts.protocol);
        LOGD("host '%s' port %d\n", url_parts.host, url_parts.port);
        LOGD("path '%s'\n", url_parts.path);
    }

    if (argc == 1) {
        LOGD("Usage: %s [url=%s%s:%d]\n", argv[0],
            url_parts.protocol, url_parts.host, url_parts.port);
        LOGD("Example: %s udp://224.1.6.183:11111\n", argv[0]);
    }

    return 0;
}