#include <../include/FTP.h>

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
        strcpy(url->user, DEFAULT_USER);
        strcpy(url->password, DEFAULT_PASSWORD);
    }
    else
    {
        // URL with user:password
        sscanf(input, HOST_AT_REGEX, url->host);
        sscanf(input, USER_REGEX, url->user);
        sscanf(input, PASS_REGEX, url->password);
    }

    // Extract resource path and file
    sscanf(input, RESOURCE_REGEX, url->resource);
    char *fileName = strrchr(input, '/');
    if (fileName)
        strcpy(url->file, fileName + 1);

    // Resolve host to IP address
    if (strlen(url->host) == 0 || (h = geshostbyname(url->host)) == NULL)
    {
        fprintf(stderr, "Invalid hostname: '%s'\n", url->host);
        exit(-1);
    }

    strcpy(url->ip, inet_ntoa(*((struct in_addr *)h->h_addr)));

    // Validate URL components
    if (!strlen(url->host) || !strlen(url->user) || !strlen(url->password) ||
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
    char usr[5 + strlen(user) + 1];
    char pass[5 + strlen(pass) + 1];
    sprintf(usr, "user %s\n", user);
    sprintf(pass, "pass %s\n", password);
    char answer[MAX_LENGTH]; // 512

    write(socket, usr, strlen(usr));
    if (readResp(socket, answer) != SV_READYFORPASSWORD)
    {
        printf("Unknown user '%s'\n", user);
        exit(-1);
    }

    write(socket, pass, strlen(pass));
    return readResp(socket, answer);
}

// Only handles single line responses
int readResp(const int socket, char *buffer)
{
    memset(buffer, 0, MAX_LENGTH);
    int bytesRead = read(socket, buffer, MAX_LENGTH - 1);
    if (bytesRead <= 0)
    {
        perror("Error reading from socket");
        exit(-1);
    }
    buffer[bytesRead] = '\0';
    int respondeCode;
    sscanf(buffer, "%d", &responseCode);
    return responseCode;
}

int readResponse(const int socket, char *buffer)
{
    char tempBuffer[MAX_LENGTH]; // Temporary buffer for each read
    memset(buffer, 0, MAX_LENGTH);

    int bytesRead, totalRead = 0;
    int responseCode = 0;
    int isMultiLine = 0;

    while ((bytesRead = read(socket, tempBuffer, sizeof(tempBuffer) - 1)) > 0)
    {
        // Prevent buffer overflow by checking before adding more data
        if (totalRead + bytesRead >= MAX_LENGTH)
        {
            bytesRead = MAX_LENGTH - totalRead;
            fprintf(stderr, "Response exceeds MAX_LENGTH. Truncated.\n");
        }

        // Append the new data to the buffer
        memcpy(buffer + totalRead, tempBuffer, bytesRead);
        totalRead += bytesRead;
        buffer[totalRead] = '\0'; // Null-terminate the buffer

        // Process the buffer line by line
        char *start = buffer;
        while (start != NULL && *start != '\0')
        {
            char *end = strstr(start, "\r\n"); // Look for the line terminator
            if (end != NULL)
            {
                int lineLength = end - start;
                char line[MAX_LENGTH];
                strncpy(line, start, lineLength); // Extract the line
                line[lineLength] = '\0';

                // Process the line
                if (responseCode == 0)
                {
                    sscanf(line, "%d", &responseCode);
                    if (line[3] == '-')
                    {
                        isMultiLine = 1; // Multi-line response detected
                    }
                }

                // Append the line to the buffer for readability
                if (totalRead < MAX_LENGTH)
                {
                    strcat(buffer, line);
                    strcat(buffer, "\n");
                }

                // Check if we've reached the last line of the multi-line response
                if (isMultiLine && line[3] == ' ' && strncmp(line, buffer, 3) == 0)
                {
                    return responseCode;
                }

                start = end + 2; // Move past "\r\n"
            }
            else
            {
                // If no "\r\n" found, break out to avoid infinite loop
                break;
            }
        }

        // If the buffer is full, break the loop to prevent further reading
        if (totalRead >= MAX_LENGTH)
        {
            break;
        }
    }

    if (bytesRead < 0)
    {
        perror("Error reading from socket");
        return -1;
    }

    return responseCode;
}