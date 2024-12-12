#include <stdio.h>
#include <regex.h>

#define MAX_LENGTH 512
#define FTP_PORT   21

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

// Struct for storing url parser output
struct URL {
    char host[MAX_LENGTH];
    char resource[MAX_LENGTH];
    char file[MAX_LENGTH];
    char username[MAX_LENGTH];
    char passwd[MAX_LENGTH];
    char ip[MAX_LENGTH];
};
