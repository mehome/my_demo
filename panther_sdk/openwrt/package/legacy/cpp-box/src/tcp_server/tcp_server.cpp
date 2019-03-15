#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <algorithm>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <iostream>
#include <fcntl.h>
extern "C" {
	#include <libcchip/function/log/slog.h>
}
using namespace std;

bool create_socket(int &sock, int port, string interfaceName) {
    int opt = 1;
    int on = 1;
    struct ifreq ifr;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (sock < 0) {
        show_errno(0,"socket");
        return false;
    } else {
        inf("sock:%d",sock);
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(int)) < 0) {
		show_errno(0,"SO_REUSEADDR");
        close(sock);		
        return false;
    }

    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger)) < 0) {
        close(sock);
				cout << "[" << __FUNCTION__ << "] Set SO_REUSEADDR failed: " << strerror(errno) << endl;
        return false;
    }

    int enable = 1;
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable)) < 0) {
		show_errno(0,"TCP_NODELAY");
        close(sock);
        return false;
    }

    if (!interfaceName.empty()) {
        struct ifreq struIR;
        strncpy(struIR.ifr_name, interfaceName.c_str(), IFNAMSIZ);

        if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, (char *)&struIR, sizeof(struIR)) < 0) {
			show_errno(0,"SO_BINDTODEVICE");
            close(sock);
            return false;
        }
    }
#if 0 
	on = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, on|O_NONBLOCK);
#else		
    if (ioctl(sock, FIONBIO, (char *)&on)) {
		show_errno(0,"FIONBIO");
        close(sock);
        return false;
    }
#endif
    int bindFd = -1;
    int bindCount = 3;
    int bindPort = port;
    do {
        bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY); //inet_addr("10.10.10.254");//
        addr.sin_port = htons((u_short) bindPort);
		
        bindFd = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
        if (bindFd < 0) {
			show_errno(0,"bind");
            bindCount --;
            bindPort ++;
        } else {
			inf("bind port:%d succeed!",bindPort);
            break;
        }

    }while(bindCount);

    if (bindFd < 0) {
		err("bindFd fail!");
        close(sock);
        return false;
    }

    listen(sock, 1);

    inf("%s succeed !",__func__);

    return true;
}

bool recv_socket(int fd);

bool accept_socket(int socket_fd) {
    int client_count = 0, client_fd = 0;
    int max_fd, new_fd = -1;
    int desc_ready;
    bool result_flag = false;
    struct timeval timeout;
    fd_set master_set, working_set;

    bool close_server = false;

    inf(__func__);
 
    FD_ZERO(&master_set);
    max_fd = socket_fd;
    FD_SET(socket_fd, &master_set);

    inf(__func__);

    while (close_server == false) {
    		
				
        timeout.tv_sec  = 1;
        timeout.tv_usec = 0;

        if (socket_fd < 0) {
            err(__func__);
            result_flag = true;
            break;
        }

        memcpy(&working_set, &master_set, sizeof(master_set));

        desc_ready = select(max_fd + 1, &working_set, NULL, NULL, &timeout);
		clog(Hblue,"desc_ready:%d",desc_ready);
        if (desc_ready < 0) {
            show_errno(0,"select");
            break;
        } else if (desc_ready == 0) {
			// war("unrecived r socket!");
            if (client_fd > 0) {
                if (client_count == 30) {
                    client_count = 0;
					inf(__func__);
                    close(client_fd);
                    FD_CLR(client_fd, &master_set);
                    client_fd = 0;
                } else {
					inf(__func__);
                    client_count ++;
                }
            }
            // inf(__func__);
            continue;
        }

        for (int i = 0; i <= max_fd && desc_ready > 0; ++i) {
            if (FD_ISSET(i, &working_set)) {
                desc_ready -= 1;

                if (i == socket_fd) {
                    do {
                        inf(__func__);

                        struct sockaddr_in client_addr;
                        socklen_t client_addr_size = sizeof(client_addr);
                        new_fd = accept(socket_fd, (struct sockaddr *) &client_addr, &client_addr_size);
                        if (new_fd < 0) {
                            if (errno != EWOULDBLOCK) {
                                close_server = true;
                            }

                            inf(__func__);
                            break;
                        } else {
                            char ipAddr[64] = {0};
                            struct sockaddr_in peerAddr;
                            socklen_t peerLen = sizeof(peerAddr);

                            if (0 == getpeername(new_fd, (struct sockaddr *)&peerAddr, &peerLen)) {
                                if (inet_ntop(AF_INET, &peerAddr.sin_addr, ipAddr, sizeof(ipAddr)) ) {
                                    inf(__func__);
                                }
                            }
                        }

                        //only one connect
                        if (client_fd > 0) {
                            inf(__func__);

                            close(new_fd);
                            new_fd = -1;
                        } else {
                            FD_SET(new_fd, &master_set);
                            if (new_fd > max_fd) {
                                max_fd = new_fd;
                            }

                            client_fd = new_fd;

                           inf(__func__);

                        }
                    } while (new_fd != -1);
                } else {
                    inf(__func__);

                    //clear wait count
                    client_count = 0;

                    if (recv_socket(i)){
                        int resultValue = 0;
                        char send_buffer[1024] = {0};
                        unsigned short buffer_length = 0;
                    }

                    if (close_server) {
                        close(i);
                        FD_CLR(i, &master_set);
                        client_fd = 0;
                        if (i == max_fd) {
                            while (FD_ISSET(max_fd, &master_set) == false)
                                max_fd -= 1;
                        }
                    }
                }
            }
        }
    }

    for (int i=0; i <= max_fd; ++i) {
        if (FD_ISSET(i, &master_set)) {
            close(i);
            FD_CLR(i, &master_set);
        }
    }

    return result_flag;
}

bool recv_socket(int fd) {
    int len;
    bool close_conn = false;
    char buffer[256] = {0};

    do {
        memset(buffer, 0, sizeof(buffer));
        //need change
        len = recv(fd, buffer, sizeof(buffer) - 1, 0);
        if (len < 0) {
            if (errno != EWOULDBLOCK) {
                inf(__func__);
                close_conn = false;
            }
            break;
        } else if (len == 0) {
//            NetworkLogger::get_instance()->m_duer_link_log_cb("%s::%s Read failed zero.",
//                                                                  __FILENAME__,
//                                                                  __FUNCTION__);
            close_conn = false;
            break;
        } else {
            inf(buffer);
            close_conn = true;

            break;
        }
    } while (1);

    if (len > 0) {
        for (int i = 0; i < len; i ++) {
            inf(__func__);
        }
    }

    return close_conn;
}


    
void close_socket(int socket_fd) {
    close(socket_fd);
    err(__func__);
}

int tcp_server_handle(char *argv[]) {
	log_init("l=11111");
	inf("compile in %s %s",__DATE__,__TIME__);
	int port=atoi(argv[1]);
	string ifr=argv[2];
    int socket_fd = -1;
    if (!create_socket(socket_fd, port, ifr)) {
        err(__func__);
        return -1;
    }

    if (!accept_socket(socket_fd)) {
        err(__func__);
        return -1;
    }
    return 0; 
}
