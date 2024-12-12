#include <../include/FTP.h>


int parseFTPUrl(const char *input, struct URL *url) {
    regex_t regex;
    struct hostent *h;

    // Validate the URL structure
    if (regcomp(&regex, SLASH, 0) != 0 || regexec(&regex, input, 0, NULL, 0) != 0) {
        return -1;
    }

    // Check for user:password in URL
    regcomp(&regex, AT, 0);
    if (regexec(&regex, input, 0, NULL, 0) != 0) {
        // URL without user:password
        sscanf(input, HOST_REGEX, url->host);
        strcpy(url->user, DEFAULT_USER);
        strcpy(url->password, DEFAULT_PASSWORD);
    } else {
        // URL with user:password
        sscanf(input, HOST_AT_REGEX, url->host);
        sscanf(input, USER_REGEX, url->user);
        sscanf(input, PASS_REGEX, url->password);
    }

    // Extract resource path and file
    sscanf(input, RESOURCE_REGEX, url->resource);
    char *fileName = strrchr(input, '/');
    if (fileName) strcpy(url->file, fileName + 1);

    // Resolve host to IP address
    if (strlen(url->host) == 0 || (h = geshostbyname(url->host)) == NULL) {
        fprintf(stderr, "Invalid hostname: '%s'\n", url->host);
        return -1;
    }
    
    strcpy(url->ip, inet_ntoa(*((struct in_addr *)h->h_addr)));

    // Validate URL components
    if (!strlen(url->host) || !strlen(url->user) || !strlen(url->password) || 
        !strlen(url->resource) || !strlen(url->file)) {
        return -1;
    }

    return 0;
}
