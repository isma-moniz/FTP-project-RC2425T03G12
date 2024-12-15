#include "../include/FTP.h"

/**
 * @brief Parses FTP url provided by the user to a URL struct
 * 
 * @param input the user URL input in the form of a char array
 * @param url the URL struct to store the user URL input
 * @return int 0 in case of success, int -1 in case of error
 */
int parseFTPUrl(const char *input, struct URL *url)
{
    regex_t regex;
    struct hostent *h;

    // Validate the URL structure
    if (regcomp(&regex, SLASH, 0) != 0 || regexec(&regex, input, 0, NULL, 0) != 0)
    {
        return -1;
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
        return -1;
    }

    strcpy(url->ip, inet_ntoa(*((struct in_addr *) h->h_addr)));

    // Validate URL components
    if (!strlen(url->host) || !strlen(url->username) || !strlen(url->password) ||
        !strlen(url->resource) || !strlen(url->file))
    {
        return -1;
    }

    return 0;
}

/**
 * @brief Creates a socket with the ip and port provided
 * 
 * @param ip the socket ip
 * @param port the socket port
 * @return int sockfd or -1 in case of error
 */
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
        return -1;
    }

    // Connect to the server
    if (connect(sockfd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("Error: Could not connect to the server");
        return -1;
    }

    return sockfd;
}

/**
 * @brief Attempts to authenticate the user with the credentials provided in the url (or the default anonymous ones)
 * 
 * @param user The username to authenticate
 * @param password The user's password
 * @param socket The endpoint to write data to
 * @return The server's response code or -1 in case of error
 */
int authenticate(const char *user, const char *password, const int socket)
{
    char usr[5 + strlen(user) + 2];
    char pass[5 + strlen(password) + 2];
    sprintf(usr, "user %s\r\n", user);
    sprintf(pass, "pass %s\r\n", password);
    char answer[BUFFER_SIZE]; // 512
    memset(answer, 0, sizeof(answer));

    write(socket, usr, strlen(usr));
    if (readResponse(socket, answer) != HOST_PASS_READY)
    {
        printf("Unknown user '%s'\n", user);
        return -1;
    }

    write(socket, pass, strlen(pass));
    return readResponse(socket, answer);
}

/**
 * @brief Reads the response from the server and prints it out to the CLI. Returns the response code.
 * 
 * @param socket The endpoint to read data from
 * @param buffer The buffer to store the line from the server with relevant information such as ip address for data socket
 * @return The response code from the server or -1 in case of error
 */
int readResponse(const int socket, char* buffer) {
    memset(buffer, 0, BUFFER_SIZE);
    char recv_buffer[BUFFER_SIZE] = {0};
    char line_buffer[BUFFER_SIZE * 5] = {0}; // allow for a huge line just in case 
    char response[BUFFER_SIZE * 10] = {0};
    int code = 0;
    int is_multi_line = 0; // Initialize to avoid using uninitialized value

    while (1) {
        // Read BUFFER_SIZE (1024 atm) bytes from the socket and put them in buffer
        ssize_t bytes_received = recv(socket, recv_buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received < 0) {
            printf("Error: Error reading from socket.\n");
            return -1;
        } else if (bytes_received == 0) break;

        recv_buffer[bytes_received] = '\0';

        // Ensure line_buffer doesn't overflow
        if (strlen(line_buffer) + strlen(recv_buffer) >= sizeof(line_buffer)) {
            printf("Error: Line buffer overflow.\n");
            return -1;
        }
        strncat(line_buffer, recv_buffer, sizeof(line_buffer) - strlen(line_buffer) - 1);

        char *line_start = line_buffer;
        char *line_end = NULL;

        while ((line_end = strchr(line_start, '\n')) != NULL) {
            *line_end = '\0';

            if (sscanf(line_start, "%3d", &code) == 1) {
                if (strstr(line_start, "-") == line_start + 3) {
                    is_multi_line = 1;
                } else if (is_multi_line && strstr(line_start, " ") == line_start + 3) {
                    is_multi_line = 0; // Reset multi-line flag
                }
                if (strlen(buffer) + strlen(line_start) + 2 >= BUFFER_SIZE) {
                    printf("Error: Buffer overflow.\n");
                    return -1;
                }
                strncat(buffer, line_start, BUFFER_SIZE - strlen(buffer) - 2);
                strncat(buffer, "\n", BUFFER_SIZE - strlen(buffer) - 1);

                if (!is_multi_line) {
                    strncat(response, line_start, sizeof(response) - strlen(response) - 2);
                    strncat(response, "\n", sizeof(response) - strlen(response) - 1);
                    printf("Server response:\n%s\n", response);
                    return code;
                }
            }

            if (strlen(response) + strlen(line_start) + 2 >= sizeof(response)) {
                printf("Error: Response buffer overflow.\n");
                return -1;
            }
            strncat(response, line_start, sizeof(response) - strlen(response) - 2);
            strncat(response, "\n", sizeof(response) - strlen(response) - 1);

            line_start = line_end + 1;
        }

        memmove(line_buffer, line_start, strlen(line_start) + 1);
    }
    printf("Server response:\n%s\n", response);
    return code;
}

/**
 * @brief Attempts to start passive mode communication with the server
 * 
 * @param socket The control socket
 * @param ip Char array to store the calculated ip for the data socket
 * @param port Integer to store the port for the data socket
 * @return PASSIVE response code in case of success, -1 otherwise
 */
int startPassiveMode(const int socket, char *ip, int *port) {
    char answer[BUFFER_SIZE];
    int ip_p1, ip_p2, ip_p3, ip_p4, port1, port2;

    write(socket, "pasv\r\n", 6);
    if (readResponse(socket, answer) != PASSIVE) return -1;
    sscanf(answer, PASSIVE_REGEX, &ip_p1, &ip_p2, &ip_p3, &ip_p4, &port1, &port2);

    *port = port1 * 256 + port2;
    sprintf(ip, "%d.%d.%d.%d", ip_p1, ip_p2, ip_p3, ip_p4);

    return PASSIVE;
}

/**
 * @brief Sends request for resource specified to control socket
 * 
 * @param socket Control socket
 * @param resource Char array with the resource name
 * @return Server response code
 */
int requestResource(const int socket, char *resource) {
    char file[5+strlen(resource)+2];
    char answer[BUFFER_SIZE];

    sprintf(file, "retr %s\r\n", resource);
    write(socket, file, sizeof(file));
    return readResponse(socket, answer);
}

/**
 * @brief Downloads requested resource to specified filename and then closes data socket
 * 
 * @param socket1 Control socket
 * @param socket2 Data socket
 * @param filename The filename to store the resource under
 * @return Response code from the server or -1 in case of error
 */
int downloadResource(const int socket1, const int socket2, char* filename) {
    FILE* fd = fopen(filename, "wb");
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    int bytes;

    if (fd == NULL) {
        printf("Error: could not open file %s.\n", filename);
        return -1;
    }
    
    while ((bytes = recv(socket2, buffer, BUFFER_SIZE, 0)) > 0) {
        if (fwrite(buffer, bytes, 1, fd) < 0) 
            return -1;
    }
    fclose(fd);
    close(socket2);
    memset(buffer, 0, sizeof(buffer));

    return readResponse(socket1, buffer);
}

/**
 * @brief Closes FTP connection with server gracefully
 * 
 * @param socket1 Control socket
 *
 * @return close(socket1), -1 in case of error
 */
int endFTP(const int socket1) {
    char answer[BUFFER_SIZE];
    write(socket1, "quit\r\n", 6);
    if (readResponse(socket1, answer) != CLOSE_CONNECTION) {
        printf("Error: could not terminate connection.\n");
        return -1;
    }
    return close(socket1);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    struct URL host_url;
    memset(&host_url, 0, sizeof(host_url));
    if (parseFTPUrl(argv[1], &host_url) != 0) {
        printf("Error: parse error. Format should be ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    printf("Connecting to host: %s\nResource: %s\nFile: %s\nUser: %s\nPassword: %s\nIP Address: %s\n", 
    host_url.host, host_url.resource, host_url.file, host_url.username, host_url.password, host_url.ip);

    char answer[BUFFER_SIZE];
    int socketControl = createSocket(host_url.ip, FTP_PORT);
    if (readResponse(socketControl, answer) != HOST_AUTH_READY || socketControl < 0) {
        printf("Error: could not create socket to port %d with ip %s\n", FTP_PORT, host_url.ip);
        exit(-1);
    }
    printf("Connected to host. Proceeding to authentication.\n");

    if (authenticate(host_url.username, host_url.password, socketControl) != HOST_LOGGED_IN) {
        printf("Auth failed. Username: %s, Password: %s\n", host_url.username, host_url.password);
        exit(-1);
    }

    printf("Authenticated. Requesting passive mode.\n");

    int port;
    char ip[BUFFER_SIZE];
    if (startPassiveMode(socketControl, ip, &port) != PASSIVE) {
        printf("Error: Could not start passive mode.\n");
        exit(-1);
    }

    printf("Passive mode initiated. Creating socket for data transmission.\n");
    
    int socketData = createSocket(ip, port);
    if (socketData < 0) {
        printf("Error: could not create socket to port %d with ip %s\n", port, ip);
        exit(-1);
    }

    printf("Requesting resource at %s\n", host_url.resource);
    int response = requestResource(socketControl, host_url.resource);
    if ((response != HOST_TRANSFER_READY) && (response != DATA_ALREADY_OPEN)) {
        printf("Error: Resource request failed.\n");
        exit(-1);
    }

    printf("Downloading resource.\n");

    if (downloadResource(socketControl, socketData, host_url.file) != HOST_TRANSFER_DONE) {
        printf("Download failed.\n");
        exit(-1);
    }

    printf("Resource downloaded. Ending connection.\n");

    if (endFTP(socketControl) < 0) {
        printf("Error: Error when closing connection.\n");
        exit(-1);
    }

    printf("Goodbye.\n");
    return 0;
}