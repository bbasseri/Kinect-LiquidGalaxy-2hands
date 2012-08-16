#include "fakenav.h"

//TODO: 
// - Buttons
// - Not all the axes work...

//Axis order:
//x, y, z, pitch, roll, yaw

//axis values
float x, y, z, yaw, pitch, roll = 0.0f;
    
//the named pipe
unsigned int fd = 0;

char buffer[256];

std::vector<int> handles;

//write a single axis input_event
void write_axis(int axid, float amount){
    struct input_event ev;

    ev.code = axid;
    ev.value = amount;
    ev.type = EV_REL;
    
    gettimeofday(&ev.time, NULL);

    write(fd, &ev, sizeof(ev));
}

void update_dev_file(){
    write_axis(0, x);
    write_axis(1, y);
    write_axis(2, z);
    write_axis(3, pitch);
    write_axis(4, roll);
    write_axis(5, yaw);
}

//main fork()'ing socket server
int run_server(){
    int serversock, portno;    
    
    socklen_t clilen;
    serversock = socket(AF_INET, SOCK_STREAM, 0);

    if (serversock < 0){
        printf("ERROR opening socket");
        return -1;
    }
    int optval = 1;
    setsockopt(serversock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    struct sockaddr_in serv_addr, cli_addr;

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = SERVER_PORT;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(serversock, (struct sockaddr *) &serv_addr,
            sizeof(serv_addr)) < 0){
        printf("ERROR on binding");
        return -1;
    }

    listen(serversock, 5);
                
    printf("waiting for clients\n");
    
    clilen = sizeof(cli_addr);
    while (true){
        int newsockfd = accept(serversock, 
                                (struct sockaddr *) &cli_addr, 
                                &clilen);
        if (newsockfd < 0){
            printf("error on accept()\n");
            exit(1);
            
        }
                
        int pid = fork();
                
        if (pid == 0){
            //this is the client process            
            close(serversock);
            
            printf("Client connected\n");
            
            while(true){                                    
                if(read(newsockfd, buffer, 256) <= 0){
                    printf("client disconnected\n");
                    exit(0);
                }
                
                printf("Got: %s\n", buffer);
                                 
                if(sscanf(buffer, "%f, %f, %f, %f, %f, %f\n", 
                        &x, &y, &z, &yaw, &pitch, &roll) == 6){                
                    update_dev_file();
                }else{
                    printf("Couldn't parse: '%s'\n", buffer);
                }
                
            }
           
            
        }else{
        
            //in the server process
            close(newsockfd);
        }
    }
}

//control-c handler
void on_sigint(int sig) {
       
    signal(SIGINT, SIG_DFL);
    
    close(fd);
    
    printf("goodbye...\n");
    
    exit(0);
}

//entry point
int main(int argc, char **argv){

    printf("fakenav!\n");
    printf("waiting for google earth to open %s...\n", DEV_NAME);

    fd = open(DEV_NAME, O_WRONLY);
        
    printf("opened pipe!\n");
    
    signal(SIGINT, on_sigint);
        
    run_server();
}
