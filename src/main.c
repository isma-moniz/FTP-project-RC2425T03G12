#include "../include/FTP.h"

int parseFTPUrl(const char *input, struct URL *url)
{
    regex_t regex;
    struct hostent *h;

    // Validate the URL structure
    if (regcomp(&regex, SLASH, 0) != 0 || regexec(&regex, input, 0, NULL, 0) != 0)
    {
        exit(-1);
    }

    // Check for user:password in URL
    regcomp(&regex, AT, 0);
    if (regexec(&regex, input, 0, NULL, 0) != 0)
    {
        // URL without user:password
        sscanf(input, HOST_REGEX, url->host);
        strcpy(url->username, DEFAULT_USER);
        strcpy(url->password, DEFAULT_PASSWORD);
    }
    else
    {
        // URL with user:password
        sscanf(input, HOST_AT_REGEX, url->host);
        sscanf(input, USER_REGEX, url->username);
        sscanf(input, PASS_REGEX, url->password);
    }

    // Extract resource path and file
    sscanf(input, RESOURCE_REGEX, url->resource);
    char *fileName = strrchr(input, '/');
    if (fileName)
        strcpy(url->file, fileName + 1);

    // Resolve host to IP address
    if (strlen(url->host) == 0 || (h = gethostbyname(url->host)) == NULL)
    {
        fprintf(stderr, "Invalid hostname: '%s'\n", url->host);
        exit(-1);
    }

    strcpy(url->ip, inet_ntoa(*((struct in_addr *) h->h_addr)));

    // Validate URL components
    if (!strlen(url->host) || !strlen(url->username) || !strlen(url->password) ||
        !strlen(url->resource) || !strlen(url->file))
    {
        exit(-1);
    }

    return 0;
}

int createSocket(char *ip, int port)
{
    int sockfd;
    struct sockaddr_in server_addr;

    // Server address handling
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port); // to network byte order
    server_addr.sin_addr.s_addr = inet_addr(ip);

    // Create the TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error: could not create socket");
        exit(-1);
    }

    // Connect to the server
    if (connect(sockfd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("Error: Could not connect to the server");
        exit(-1);
    }

    return sockfd;
}

int authenticate(const char *user, const char *password, const int socket)
{
    char usr[5 + strlen(user) + 2];
    char pass[5 + strlen(password) + 2];
    sprintf(usr, "user %s\r\n", user);
    sprintf(pass, "pass %s\r\n", password);
    char answer[MAX_LENGTH]; // 512

    write(socket, usr, strlen(usr));
    if (readResponse(socket, answer) != HOST_PASS_READY)
    {
        printf("Unknown user '%s'\n", user);
        exit(-1);
    }

    write(socket, pass, strlen(pass));
    return readResponse(socket, answer);
}

// TODO: temporary solution until response from the teacher
// this function is not mine and i will remove it
// gets the last response code basically
int readResponse(const int socket, char* buffer) {

    char byte;
    int index = 0, responseCode;
    ResponseState state = START;
    memset(buffer, 0, MAX_LENGTH);

    while (state != END) {
        
        read(socket, &byte, 1);
        switch (state) {
            case START:
                if (byte == ' ') state = SINGLE;
                else if (byte == '-') state = MULTIPLE;
                else if (byte == '\n') state = END;
                else buffer[index++] = byte;
                break;
            case SINGLE:
                if (byte == '\n') state = END;
                else buffer[index++] = byte;
                break;
            case MULTIPLE:
                if (byte == '\n') {
                    memset(buffer, 0, MAX_LENGTH);
                    state = START;
                    index = 0;
                }
                else buffer[index++] = byte;
                break;
            case END:
                break;
            default:
                break;
        }
    }

    sscanf(buffer, RESPCODE_REGEX, &responseCode);
    return responseCode;
}

int startPassiveMode(const int socket, char *ip, int *port) {
    char answer[MAX_LENGTH];
    int ip_p1, ip_p2, ip_p3, ip_p4, port1, port2;

    write(socket, "pasv\n", 5);
    if (readResponse(socket, answer) != PASSIVE) return -1;
    sscanf(answer, PASSIVE_REGEX, &ip_p1, &ip_p2, &ip_p3, &ip_p4, &port1, &port2);

    *port = port1 * 256 + port2;
    sprintf(ip, "%d.%d.%d.%d", ip_p1, ip_p2, ip_p3, ip_p4);

    return 0;
}

int requestResource(const int socket, char *resource) {
    char file[5+strlen(resource)+2];
    char answer[MAX_LENGTH];

    sprintf(file, "retr %s\r\n", resource);
    write(socket, file, sizeof(file));
    return readResponse(socket, answer);
}

int downloadResource(const int socket1, const int socket2, char* filename) {
    FILE* fd = fopen(filename, "wb");
    char buffer[MAX_LENGTH];
    int bytes;

    if (fd == NULL) {
        printf("Error: could not open file %s.\n", filename);
        exit(-1);
    }

    while ((bytes = read(socket2, buffer, MAX_LENGTH)) > 0) {
        if (fwrite(buffer, bytes, 1, fd) < 0) 
            return -1;
    }
    fclose(fd);

    return readResponse(socket1, buffer);
}

int endFTP(const int socket1, const int socket2) {
    char answer[MAX_LENGTH];
    write(socket1, "quit\r\n", 6);
    if (readResponse(socket1, answer) != CLOSE_CONNECTION) {
        printf("Error: could not terminate connection.\n");
        exit(-1);
    }
    return close(socket1) || close(socket2);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    } 
}