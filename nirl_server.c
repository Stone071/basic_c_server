// Following Nir Lichtman's C Webserver Video
// https://www.youtube.com/watch?v=2HrYIl6GpYg
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
//#include <stdio.h> // for perror

#define NUM_BYTES 1024

int main()
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    const struct sockaddr_in addr = {
        AF_INET,
        htons(0x1f90), // port 8080 
        0,
    };
    bind(s, &addr, sizeof(addr));

    listen(s, 10); // 10 is number of waiting connections before declining

    int client_fd = accept(s, 0, 0);

    char buffer[NUM_BYTES] = {0};
    recv(client_fd, buffer, NUM_BYTES, 0);

    // GET /file.html ....
    char* filename = buffer + 5;
    // Set the space after the filename to 0 for null terminated
    *strchr(filename, ' ') = 0;
    int opened_fd = open(filename, O_RDONLY);
    // Find the size of the file
    struct stat* fInfo = malloc(sizeof(struct stat));
    int fSize = fstat(opened_fd, fInfo);
    
    // Send the content of the file to the client socket.
    sendfile(client_fd, opened_fd, 0, fInfo->st_size);

    // Close everything
    close(opened_fd);
    close(client_fd);
    close(s);
}