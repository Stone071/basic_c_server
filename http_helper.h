// This should just be stuff to help with http code
#define GET_METHOD    0
#define POST_METHOD   1

char OK_200[]        = "HTTP/1.0 200 OK\r\n";
char CONTENT_HTML[]  = "Content-Type: text/html\r\n\r\n";
char CONTENT_CSS[]   = "Content-Type: text/css\r\n\r\n";
char NOT_FOUND_404[] = "HTTP/1.0 404 NOT FOUND\r\n";
char FORBIDDEN_403[] = "HTTP/1.0 403 FORBIDDEN\r\n";

