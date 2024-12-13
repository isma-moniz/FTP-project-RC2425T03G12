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

#define MAX_LENGTH 512
#define FTP_PORT   21

#define DEFAULT_USER        "anonymous"
#define DEFAULT_PASSWORD    "password"

// Parser regex
#define AT              "@"
#define SLASH           "/"
#define HOST_REGEX      "%*[^/]//%[^/]"
#define HOST_AT_REGEX   "%*[^/]//%*[^@]@%[^/]"
#define RESOURCE_REGEX  "%*[^/]//%*[^/]/%s"
#define USER_REGEX      "%*[^/]//%[^:/]"
#define PASS_REGEX      "%*[^/]//%*[^:]:%[^@\n$]"
#define RESPCODE_REGEX  "%d"
#define PASSIVE_REGEX   "%*[^(](%d,%d,%d,%d,%d,%d)%*[^\n$)]"

#define HOST_TRANSFER_READY 150
#define HOST_AUTH_READY     220
#define CLOSE_CONNECTION    221
#define HOST_TRANSFER_DONE  226
#define PASSIVE             227
#define HOST_LOGGED_IN      230
#define HOST_PASS_READY     331

// Struct for storing url parser output
struct URL {
    char host[MAX_LENGTH];
    char resource[MAX_LENGTH];
    char file[MAX_LENGTH];
    char username[MAX_LENGTH];
    char password[MAX_LENGTH];
    char ip[MAX_LENGTH];
};


// TODO: temporary 
typedef enum {
    START,
    SINGLE,
    MULTIPLE,
    END
} ResponseState;

int parseFTPUrl(const char* input, struct URL* url);

int createSocket(char* ip, int port);

int authenticate(const char* user, const char* password, const int socket);

int readResponse(const int socket, char* buffer);

int sendCmd(const int socket, char* str);

int startPassiveMode(const int socket, char* ip, int* port);

int requestResource(const int socket, char* resource);

int downloadResource(const int socket1, const int socker2, char* filename);

int endFTP(const int socket1, const int socket2);