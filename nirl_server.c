// Following Nir Lichtman's C Webserver Video
// https://www.youtube.com/watch?v=2HrYIl6GpYg
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define NUM_BYTES         1024
#define TCP_CORK_ENA         1
#define TCP_CORK_DIS         0
#define PROTOCOL_TCP         6

char OK_200[]        = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n";
char CONTENT_HTML[]  = "Content-Type: text/html\r\n";
char NOT_FOUND_404[] = "HTTP/1.0 404 NOT FOUND\r\n";
char FORBIDDEN_403[] = "HTTP/1.0 403 FORBIDDEN\r\n";

// Append will memcpy n bytes from src into dest and then return the next
// open address in dest.
char* append(char* dest, char* src, size_t n)
{
    memcpy(dest, src, n);
    return (dest+n);
}

bool isFavicon(char* filename)
{
    char refName[] = "favicon.ico";
    bool isMatch = true;

    // "favicon.ico" is a has 12 chars because of the null terminator.
    // We do not want to compare against the null terminator, so cut
    // the comparison one index short.
    for (int i=0; i<sizeof(refName)-1; i++)
    {
        if (filename[i] != refName[i])
        {
            isMatch = false;
            break;
        }
    }
    return isMatch;
}

int main()
{
    // Initialize struct to feed to bind which describes the port.
    const struct sockaddr_in port_desc = {
        AF_INET,
        htons(0x1f90), // port 8080 
        0,
    };

    // Open the socket
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
    {
        perror("SOCKET ERR");
    }

    // Bind the port to the socket
    if (bind(s, &port_desc, sizeof(port_desc)) < 0)
    {
        perror("BIND ERR");
    }
    else
    {
        // Let's just sequentially serve all connections.
        while(1)
        {  
            bool GetAndSend = true;
            // Listen for a connection
            if (listen(s, 10) < 0) // 10 is number of waiting connections before declining
            {
                perror("LISTEN ERR");
            }
            else
            {
                printf("Listening for connection...\n");
            }

            int client_fd = accept(s, 0, 0);

            char buffer[NUM_BYTES] = {0};
            if (recv(client_fd, buffer, NUM_BYTES, 0) < 0)
            {
                perror("RECV ERR");
            }
            // Print the request because I am curious.
            //printf("\nHTTP REQUEST:\n%s", buffer);

            // GET /file.html ....
            char* filename = buffer + 5;
            char* firstSpace = strchr(filename, ' ');
            // Handle specific file cases
            if (firstSpace == filename)
            {
                // If pointer to the first space in the string is the same as filename,
                // then user has not requested a specific page, just the base address.
                filename = "public/index.html";
            }
            else if(isFavicon(filename))
            {
                // It is requesting favicon. If you say 404 not found the page won't load.
                GetAndSend = false;
                // send back 200 ok?
                send(client_fd, OK_200, sizeof(OK_200), 0);
            }
            else
            {
                // Set the space after the filename to 0 for null terminated
                *firstSpace = 0;
            }

            // Should perform a check here to see if the file they have requested
            // is allowed, or should not be accessible to them.
            if (GetAndSend)
            {
                int opened_fd = open(filename, O_RDONLY);
                if (opened_fd == -1)
                {
                    // send back 404 not found
                    send(client_fd, NOT_FOUND_404, sizeof(NOT_FOUND_404), 0);
                    perror("OPEN FILE ERR");
                }
                else
                {
                    // Find the size of the file
                    struct stat* fInfo = malloc(sizeof(struct stat));
                    int fSize = fstat(opened_fd, fInfo);
                    // Allocate a response buffer
                    size_t bufSize = sizeof(OK_200) + fInfo->st_size;
                    char* rspBuf = malloc(bufSize);
                    // Put the HTTP response in the buffer
                    // Okay so one critical component seems to be that the HTTP response
                    // must have two \n\n before the content comes, and standard is \r\n\r\n.
                    // Whether or not the null terminator is included in the response strings
                    // does not seem to affect anything.
                    char* pNext = append(rspBuf, OK_200, sizeof(OK_200));
                    
                    // Put the file content in the buffer
                    read(opened_fd, pNext, fInfo->st_size);

                    // Send it off!
                    send(client_fd, rspBuf, bufSize, 0);

                    printf("%s", rspBuf);
                    
                    free(rspBuf);
                    free(fInfo);
                }
                // Close file
                close(opened_fd);
            }

            // Close client connection
            close(client_fd);   
        }
    }

    // Close the socket before exiting.
    close(s);
}