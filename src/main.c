#include <../include/FTP.h>

int parseFTPUrl(const char *input, struct URL *url) {
    regex_t regex;
    struct hostent *h;

    // Validate the URL structure
    if (regcomp(&regex, SLASH, 0) != 0 || regexec(&regex, input, 0, NULL, 0) != 0) {
        exit(-1);
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
        exit(-1);
    }
    
    strcpy(url->ip, inet_ntoa(*((struct in_addr *)h->h_addr)));

    // Validate URL components
    if (!strlen(url->host) || !strlen(url->user) || !strlen(url->password) || 
        !strlen(url->resource) || !strlen(url->file)) {
        exit(-1);
    }

    return 0;
}

int createSocket(const char *hostname, int port) {
    int sockfd;
    struct hostent *host;
    struct sockaddr_in serverAddr;

    // Resolve hostname to IP address
    if ((host = gethostbyname(hostname)) == NULL) {
        fprintf(stderr, "Error: Could not resolve hostname '%s'\n", hostname);
        exit(-1);
    }

    // Server address handling
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port); // to network byte order
    memcpy(&serverAddr.sin_addr, host->h_addr, host->h_length);

    // Create the TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error: could not create socket");
        exit(-1);
    }

    // Connect to the server
    if (connect(sockfd,
                (struct sockaddr *) &serverAddr,
                sizeof(serverAddr)) < 0) {
        perror("Error: Could not connect to the server");
        
        if (close(sockfd)<0) {
            perror("close()");
            exit(-1);
        }

        exit(-1);
    }

    return sockfd;
}