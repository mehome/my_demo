
struct resp_header{    
    int status_code;        //HTTP/1.1 '200' OK    
    char content_type[128]; //Content-Type: application/gzip    
    long content_length;    //Content-Length: 11683079
    char *content_buf;
};

int getResponse(int socket_fd, char **text);
int turingBuildRequest(int socketFd,char *host, char *data, int len, char *jsonStr);
int get_socket_fd(char *host);



