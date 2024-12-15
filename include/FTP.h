#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <termios.h>
#include <regex.h>

#define BUFFER_SIZE 1024
#define FTP_PORT   21

// Default anonymous credentials (might have to be adjusted in feup's lab because no one told us the credentials)
#define DEFAULT_USER        "anonymous"
#define DEFAULT_PASSWORD    "password" // some sites request the user's email as the password but will accept just any value

// Parser regex
#define AT              "@"
#define SLASH           "/"
#define HOST_REGEX      "%*[^/]//%[^/]"
#define HOST_AT_REGEX   "%*[^/]//%*[^@]@%[^/]"
#define PASS_REGEX      "%*[^/]//%*[^:]:%[^@\n$]"
#define USER_REGEX      "%*[^/]//%[^:/]"
#define RESOURCE_REGEX  "%*[^/]//%*[^/]/%s"
#define PASSIVE_REGEX   "%*[^(](%d,%d,%d,%d,%d,%d)%*[^\n$)]"

// Server responses
#define HOST_TRANSFER_READY 150
#define DATA_ALREADY_OPEN   125
#define HOST_AUTH_READY     220
#define CLOSE_CONNECTION    221
#define HOST_TRANSFER_DONE  226
#define PASSIVE             227
#define HOST_LOGGED_IN      230
#define HOST_PASS_READY     331

// Struct for storing url parser output
struct URL {
    char host[BUFFER_SIZE];
    char resource[BUFFER_SIZE];
    char file[BUFFER_SIZE];
    char username[BUFFER_SIZE];
    char password[BUFFER_SIZE];
    char ip[BUFFER_SIZE];
};

int parseFTPUrl(const char* input, struct URL* url);

int createSocket(char* ip, int port);

int authenticate(const char* user, const char* password, const int socket);

int readResponse(const int socket, char* buffer);

int sendCmd(const int socket, char* str);

int startPassiveMode(const int socket, char* ip, int* port);

int requestResource(const int socket, char* resource);

int downloadResource(const int socket1, const int socker2, char* filename);

int endFTP(const int socket1);