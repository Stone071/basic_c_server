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
#include "http_helper.h"

#define DEBUG_MSGS 1

#define NUM_BYTES         1024
#define TCP_CORK_ENA         1
#define TCP_CORK_DIS         0
#define PROTOCOL_TCP         6

#define UNKNOWN_FILE       255
#define HTML_FILE            0
#define CSS_FILE             1
#define ICO_FILE             2

#define SOME_ERR            -1


// Append will memcpy n bytes from src into dest and then return the next
// open address in dest.
char* append(char* dest, char* src, size_t n)
{
    memcpy(dest, src, n);
    return (dest+n);
}

// Returns the request method received in the http header. 
// GET_METHOD, POST_METHOD, or SOME_ERR;
int identify_request(char* pReq)
{
    int retVal = SOME_ERR;
    // Apparently GET, POST, PUT, HEAD, DELETE, PATCH, OPTIONS, CONNECT, TRACE are all valid.
    // Without modifying the original received content, identify what we are looking at.
    char* pEnd = strchr(pReq, ' ');
    if (pReq != pEnd)
    {
        //Copy the method into a new buffer and null terminate the string.
        size_t numChars = pEnd - pReq;
        if (numChars <= 7)
        {
            // None of the valid methods are more than 7 chars
            char* tempBuf = malloc(numChars + 1);
            char* pNext = append(tempBuf, pReq, numChars);
            // Null terminate the string now in tempBuf;
            *pNext = '\0';
    
    
            if (strncmp(tempBuf, "GET", numChars+1) == 0)
            {
                retVal = GET_METHOD;
            }
            else if (strncmp(tempBuf, "POST", numChars+1) == 0)
            {
                retVal = POST_METHOD;
            }

            free(tempBuf);
        }
        else
        {
            // Something went wrong...
            // We are already returning SOME_ERR;
            printf("\nHTTP REQUEST WAS %li CHARS in SIZE\n", numChars);
        }
    }
    return retVal;
}

// Identify the filetype they are requesting
int identify_filetype(const char* pPath)
{
    // Assume failure
    int retVal = SOME_ERR;

    char* pDot = strchr(pPath, '.');
    // By the time this function is called, there has been a null terminator inserted
    // after the filename in the buffer.
    char* pEnd = strchr(pPath, '\0');
    size_t numChars = pEnd - pDot;
    // I can't imagine a filetype we would use greater than 8 characters.
    if (numChars <= 8)
    {
        char* tempBuf = malloc(numChars+1);
        char* pNext = append(tempBuf, pDot, numChars);
        *pNext = '\0';
    
        if (strncmp(tempBuf, ".html", numChars+1) == 0)
        {
            retVal = HTML_FILE;
        }
        else if (strncmp(tempBuf, ".css", numChars+1) == 0)
        {
            retVal = CSS_FILE;
        }
        else if (strncmp(tempBuf, ".ico", numChars+1) == 0)
        {
            retVal = ICO_FILE;
        }
        else
        {
            retVal = UNKNOWN_FILE;
        }
        free(tempBuf);
    }
    else
    {
        // Something went wrong...
        // We are already returning SOME_ERR;
        printf("\nFILETYPE WAS %li CHARS in SIZE\n", numChars);
    }

    return retVal;
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

// Must return pointer to next index in buffer.
char* populate_file_metadata(char* rspBuf, int filetype)
{
    char* retVal = rspBuf;
    char* pNext = NULL;
    // Make sure to exclude the \0 when appending.
    if (filetype == HTML_FILE)
    {
        pNext = append(rspBuf, CONTENT_HTML, sizeof(CONTENT_HTML)-1);
        retVal = pNext;
    }
    else if (filetype == CSS_FILE)
    {
        pNext = append(rspBuf, CONTENT_CSS, sizeof(CONTENT_CSS)-1);
        retVal = pNext;
    }

    return retVal;
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
    int sListener = socket(AF_INET, SOCK_STREAM, 0);
    if (sListener < 0)
    {
        perror("SOCKET ERR");
    }

    // Bind the port to the socket
    if (bind(sListener, &port_desc, sizeof(port_desc)) < 0)
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
            if (listen(sListener, 10) < 0) // 10 is number of waiting connections before declining
            {
                perror("LISTEN ERR");
            }
            else
            {
                printf("Listening for connection...\n");
            }

            int client_fd = accept(sListener, 0, 0);

            char inBuf[NUM_BYTES] = {0};
            if (recv(client_fd, inBuf, NUM_BYTES, 0) < 0)
            {
                perror("RECV ERR");
            }
            #ifdef DEBUG_MSGS
            // Print the request because I am curious.
            printf("\nHTTP REQUEST:\n%s", inBuf);
            #endif
            
            // Identify what kind of HTTP request this is.
            if (identify_request(inBuf) == GET_METHOD)
            {
                // GET /file.html ....
                // GET / HTTP/1.1
                char* pFilename = inBuf + 5;
                char* pFirstSpace = strchr(pFilename, ' ');
                // Handle specific file cases
                if (pFirstSpace == pFilename)
                {
                    // If pointer to the first space in the string is the same as filename,
                    // then user has not requested a specific page, just the base address.
                    pFilename = "index.html";
                }
                else if(isFavicon(pFilename))
                {
                    // Our favicon for now is a web address in the .html which will be
                    // retrieved separately. We don't need to send it over.
                    GetAndSend = false;
                    // send back 200 ok?
                    send(client_fd, OK_200, sizeof(OK_200), 0);
                }
                else
                {
                    // Set the space after the filename to 0 for null terminated
                    *pFirstSpace = '\0';
                }

                // Should perform a check here to see if the file they have requested
                // is allowed, or should not be accessible to them.
                if (GetAndSend)
                {
                    int opened_fd = open(pFilename, O_RDONLY);
                    if (opened_fd == SOME_ERR)
                    {
                        // send back 404 not found
                        send(client_fd, NOT_FOUND_404, sizeof(NOT_FOUND_404), 0);
                        perror("OPEN FILE ERR");
                    }
                    else
                    {
                        // Identify the filetype
                        int filetype = identify_filetype(pFilename);
                        // Find the size of the file
                        struct stat* fInfo = malloc(sizeof(struct stat));
                        int fSize = fstat(opened_fd, fInfo);
                        // Allocate a response buffer
                        size_t bufSize = sizeof(OK_200)-1 + sizeof(CONTENT_HTML)-1 + fInfo->st_size;
                        char* rspBuf = malloc(bufSize);
                        // Put the HTTP response in the buffer
                        // Okay so one critical component seems to be that the HTTP response
                        // must have two \n\n before the content comes, and standard is \r\n\r\n.
                        // Also need to exclude the null terminator.
                        char* pNext = append(rspBuf, OK_200, sizeof(OK_200)-1);
                        if (filetype != UNKNOWN_FILE)
                        {
                            pNext = populate_file_metadata(pNext, filetype);
                        }
                        
                        // Put the file content after the HTTP response in the buffer
                        read(opened_fd, pNext, fInfo->st_size);

                        // Send it off!
                        send(client_fd, rspBuf, bufSize, 0);
                        
                        #ifdef DEBUG_MSGS
                        printf("\nRESPONSE BUFFER: %s\n", rspBuf); // debug purpose.
                        #endif
                        
                        free(rspBuf);
                        free(fInfo);
                    }
                    // Close file
                    close(opened_fd);
                }
            }        
            else
            {
                // We currently only support GET requests. Do nothing.
            }

            // Close client connection
            close(client_fd);   
        }
    }

    // Close the socket before exiting.
    close(sListener);
}