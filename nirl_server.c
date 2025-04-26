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
#include <stdio.h>

#define NUM_BYTES 1024

char NOT_FOUND_404[] = "HTTP/1.0 404 NOT FOUND";

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
        perror("BIND ERR\n");
    }
    else
    {
        // Let's just sequentially serve all connections.
        while(1)
        {  
            // Listen for a connection
            if (listen(s, 10) < 0) // 10 is number of waiting connections before declining
            {
                perror("LISTEN ERR");
            }

            int client_fd = accept(s, 0, 0);

            char buffer[NUM_BYTES] = {0};
            if (recv(client_fd, buffer, NUM_BYTES, 0) < 0)
            {
                perror("RECV ERR");
            }
            // Print the request because I am curious.
            printf("\nHTTP REQUEST:\n%s", buffer);

            // GET /file.html ....
            char* filename = buffer + 5;
            // Set the space after the filename to 0 for null terminated
            *strchr(filename, ' ') = 0;
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
                
                // Send the content of the file to the client socket.
                if (sendfile(client_fd, opened_fd, 0, fInfo->st_size) < 0)
                {
                    perror("SENDFILE ERR");
                }
            }

            // Close everything
            close(opened_fd);
            close(client_fd);   
        }
    }

    // Close the socket before exiting.
    close(s);
}