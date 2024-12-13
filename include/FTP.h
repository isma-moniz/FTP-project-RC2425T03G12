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

#define DEFAULT_USER        "rcom"
#define DEFAULT_PASSWORD    "rcom"

// Parser regex
#define AT              "@"
#define HOST_REGEX      "%*[^/]//%[^/]"
#define HOST_AT_REGEX   "%*[^/]//%*[^@]@%[^/]"
#define SLASH           "/"
#define RESOURCE_REGEX  "%*[^/]//%*[^/]/%s"
#define USER_REGEX      "%*[^/]//%[^:/]"
#define PASS_REGEX      "%*[^/]//%*[^:]:%[^@\n$]"
#define RESPCODE_REGEX  "%d"
#define PASSIVE_REGEX   "%*[^(](%d,%d,%d,%d,%d,%d)%*[^\n$)]"

#define HOST_PASS_READY     331
#define HOST_AUTH_READY     220
#define PASSIVE             227
#define CLOSE_CONNECTION    221

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