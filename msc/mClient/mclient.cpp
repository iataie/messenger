#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <map>
#include <vector>
#include <utility>
#include <iostream>
#include <string>
using namespace std;

#define SUCCESSFUL 1
#define UNSUCCESSFUL 2
#define NEWSOCK 1
#define CRTSOCK 2
#define NOSOCK  3

map<string, string> activeFriendsIP;
//map<int, string> activeSock2Friendname;
map<string, int> activeFriendname2Sock;
std::vector<int> activeSocks;
map<string, int> IP2Sock;
char myusername[24] = "";
int logdin = false;

int login_register(int socketfd);
int sendLocationInfo(int sock);
int create_Server_sock();
int processCmd(char * messageP, int sock, fd_set *orig_setP, int *maxfP, int myservSock);
int removeIntVector(vector<int> *v, int element);
int processServerMsg(char * messageP);
void printActiveFriends();
int chatSocket(string friendUserName, int *sockP);
int findFriendNameByIP(string ipAddP, string *FriendName);
void printClientMessage(char * messageP, int sockP);
int findFriendNameBySock(int sockP, string *friendName);
void StrTokensm(const string * str, string seperators, vector<string>* tokens);
void removeUnvalidSockByIp(void);



const unsigned MAXBUFLEN = 512;

int main(int argc, char *argv[]) {
    int sock, rv, flag, maxf, myservsock, rec_sock, len;
    struct addrinfo hints, *res, *ressave;
    struct sockaddr_in server, recaddr;
    char message[1000], server_reply[2000];
    fd_set rset, orig_set;
    vector<int> u2;
    vector <int> ::iterator iInt;

    if (argc < 3) {
        printf("Usage: a.out ip_addr port.\n");
        exit(0);
    }

    /*
        //Create socket
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            printf("Could not create socket");
        }
        puts("Socket created");

        server.sin_addr.s_addr = inet_addr(argv[1]);
        server.sin_family = AF_INET;
        server.sin_port = htons((short) atoi(argv[2]));

        //Connect to remote server
        if (connect(sock, (struct sockaddr *) &server, sizeof (server)) < 0) {
            perror("connect failed. Error");
            return 1;
        }
     */

    while (1) {

        bzero(&hints, sizeof (struct addrinfo));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        if ((rv = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0) {
            cout << "getaddrinfo wrong: " << gai_strerror(rv) << endl;
            exit(1);
        }

        ressave = res;
        flag = 0;
        do {
            sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (sock < 0)
                continue;
            if (connect(sock, res->ai_addr, res->ai_addrlen) == 0) {
                flag = 1;
                break;
            }
            close(sock);
        } while ((res = res->ai_next) != NULL);
        freeaddrinfo(ressave);

        if (flag == 0) {
            fprintf(stderr, "cannot connect\n");
            exit(1);
        }


        puts("Connected\n");

        while (login_register(sock) == UNSUCCESSFUL)
            if (login_register(sock) != UNSUCCESSFUL)
                break;

        logdin = true;



        do {
            if ((myservsock = create_Server_sock()) != -1)
                break;
            printf("try again...");
            char c;
            getc(stdin);
        } while (1);

        sendLocationInfo(sock);

        FD_ZERO(&orig_set);
        FD_SET(STDIN_FILENO, &orig_set);
        FD_SET(sock, &orig_set);
        FD_SET(myservsock, &orig_set);
        if (myservsock > STDIN_FILENO) maxf = myservsock;
        else maxf = STDIN_FILENO;


        while (logdin) {
            rset = orig_set;
            select(maxf + 1, &rset, NULL, NULL, NULL);
            if (FD_ISSET(sock, &rset)) {
                if (read(sock, message, sizeof (message)) == 0) {
                    printf("server crashed.\n");
                    exit(0);
                }
                //printf("msg:%d",msg.msg_typ);
                //printf("friends port:%d",msg.FriendsPort);
                //printf("friends Address:%s",msg.FriendsAdd);
                processServerMsg(message);

            }

            if (FD_ISSET(myservsock, &rset)) {
                /* somebody tries to connect */
                len = sizeof (struct sockaddr_in);
                if ((rec_sock = accept(myservsock, (struct sockaddr
                        *) (&recaddr), (socklen_t *) & len)) < 0) {
                    if (errno == EINTR)
                        continue;
                    else {
                        perror(":accept error");
                        exit(1);
                    }
                }
                /* print the remote socket information */

                printf("remote machine = %s, port = %d.\n",
                        inet_ntoa(recaddr.sin_addr), ntohs(recaddr.sin_port));

                activeSocks.push_back(rec_sock); //add friend socket to the list
                if (rec_sock > maxf)
                    maxf = rec_sock;
                FD_SET(rec_sock, &orig_set); // add friend socket to the set

                //relates ip of sender to socket
                IP2Sock[inet_ntoa(recaddr.sin_addr)] = rec_sock;

                cout << endl << "socket number: " << IP2Sock[inet_ntoa(recaddr.sin_addr)] << endl;
                fflush(0);





            }


            if (FD_ISSET(STDIN_FILENO, &rset)) {

                if (fgets(message, 100, stdin) == NULL) exit(0);


                processCmd(message, sock, &orig_set, &maxf, myservsock);

                //write(sock, message, strlen(message) + 1);

            }

            //printf("number of active sockets:%d",activeSocks.size());
            //fflush(0);
            for (int i = 0; i < activeSocks.size(); i++) {
                if (FD_ISSET(activeSocks[i], &rset)) {
                    int num;

                    //fflush(0);
                    num = read(activeSocks[i], message, 1000);
                    if (num == 0) {
                        /* client exits */
                        //printf("number element in vector: %d", activeSocks.size());
                        close(activeSocks[i]);
                        FD_CLR(activeSocks[i], &orig_set);
                        //activeSocks.erase(std::remove(activeSocks.begin(), activeSocks.end(), activeSocks[i]), activeSocks.end());
                        string fname = "";
                        findFriendNameBySock(activeSocks[i], &fname);
                        int x = removeIntVector(&activeSocks, activeSocks[i]);
                        //puts("closed connection with client.");
                        printf("\n%s closed connection\n", fname.c_str());
                        //printf("number element in vector: %d", activeSocks.size());
                        fflush(0);
                    }
                    if (num > 0) {

                        //printf("Client message: %s", message);

                        printClientMessage(message, activeSocks[i]);


                        fflush(0);
                    }

                }
                //u2 = userFriends["user1"];
                // for (iStr = u2.begin(); iStr != u2.end(); ++iStr)
                //    cout << *iStr << '\t';
                //if (FD_ISSET(client[i], &rset)) 

            }


        }
    }


    //close(sock);
    return 0;
}

int login_register(int socketfd) {
    //string cmd;
    char cmd2[100] = "";

    char password[24] = "";
    // string temp;
    // Packet msg;
    char buf[MAXBUFLEN] = "";
    //write(STDOUT_FILENO, "l(login)/r(register): : ", sizeof("l(login)/r(register): "));
    printf(">l(login)/r(register):");
    scanf("%s", &cmd2);

    if (cmd2[0] == 'l') {
        printf("username: ");
        scanf("%s", myusername);
        printf("password: ");
        scanf("%s", password);

        strcpy(buf, "");
        strcat(buf, "LOGIN|");
        strcat(buf, myusername);
        strcat(buf, "|");
        strcat(buf, password);
        write(socketfd, buf, strlen(buf) + 1);

        if (read(socketfd, buf, MAXBUFLEN) == 0) {
            printf("server crashed.\n");
            exit(0);
        }
        if (strstr(buf, "500") == buf) {
            puts("Username or Password is not correct, Please try again.");
            return UNSUCCESSFUL;
        }

        if (strstr(buf, "200") == buf) {
            puts("login is successful");
            return SUCCESSFUL;
        }
    }
    if (cmd2[0] == 'r') {
        printf("your username: ");
        scanf("%s", myusername);
        printf("your password: ");
        scanf("%s", password);

        strcpy(buf, "");
        strcat(buf, "REGISTER|");
        strcat(buf, myusername);
        strcat(buf, "|");
        strcat(buf, password);
        write(socketfd, buf, strlen(buf) + 1);
        if (read(socketfd, buf, MAXBUFLEN) == 0) {
            printf("server crashed.\n");
            exit(0);
        }
        if (strstr(buf, "500") == buf) {
            puts("Your user name is not available.");
            return UNSUCCESSFUL;
        }

        if (strstr(buf, "200") == buf) {
            puts("You are registered successfully.");
            return SUCCESSFUL;
        }

    }
    if (strcmp(cmd2, "exit") == 0) {
        close(socketfd);
        exit(0);
    }
    return UNSUCCESSFUL;
}

int create_Server_sock() {
    int socket_desc, client_sock, c, *new_sock;
    struct sockaddr_in server, client;
    int tr;

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket for listening to the other clients.");
    }
    puts("Socket for chat with others created");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(5100);

    //Bind
    if (bind(socket_desc, (struct sockaddr *) &server, sizeof (server)) < 0) {
        //print the error message
        if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof (int)) == -1) {
            perror("setsockopt");
            return -1;
        }
        //perror("Bind failed. Error");
        //return -1;
    }
    puts("server socket binding done.");

    //Listen
    listen(socket_desc, 5);

    //Accept and incoming connection
    puts("Waiting for incoming connections to chat...");
    //c = sizeof (struct sockaddr_in);
    return socket_desc;

}

int removeIntVector(vector<int> *v, int element) {
    std::vector<int>::iterator current = v->begin();
    while (current != v->end()) {
        if (*current == element) {
            current = v-> erase(current);
        } else {
            ++current;
        }
    }

    return true;
}

void StrToken(const string * str, string seperators, vector<string>* tokens) {
    int start = str->find_first_not_of(seperators, 0);
    int pos = str->find_first_of(seperators, start);
    int tokenNum = 1;
    while ((string::npos != pos || string::npos != start) && tokenNum < 3) {
        tokens->push_back(str->substr(start, pos - start));
        tokenNum++;
        start = str->find_first_not_of(seperators, pos);
        pos = str->find_first_of(seperators, start);
    }
    if (string::npos != start)
        tokens->push_back(str->substr(start, str->size() - start + 1));
    return;
}

int processCmd(char * messageP, int sock, fd_set *orig_setP, int *maxfP, int serverSockP) {
    char buf[MAXBUFLEN] = "";
    vector<string> tokens;
    string str(messageP);
    StrToken(&str, " \t", &tokens);

    //for (int i = 0; i < tokens.size(); i++)
    //    printf("\n%s", tokens[i].c_str());

    if (tokens[0].compare("i") == 0) {
        std::string username = tokens[1];
        strcpy(buf, "");
        strcat(buf, "INVITREQ|");
        strcat(buf, username.c_str());
        if (tokens.size() > 2) {
            strcat(buf, "|");
            strcat(buf, tokens[2].c_str());
        }
        //cout << buf;
        write(sock, buf, strlen(buf) + 1);

    }
    if (tokens[0].compare("ia") == 0) {
        std::string username = tokens[1];
        strcpy(buf, "");
        strcat(buf, "INVITACCEPT|");
        strcat(buf, username.c_str());
        if (tokens.size() > 2) {
            strcat(buf, "|");
            strcat(buf, tokens[2].c_str());
        }
        //cout << buf;
        write(sock, buf, strlen(buf) + 1);
    }
    if (tokens[0].compare("m") == 0) {
        std::string username = tokens[1];
        strcpy(buf, "");
        strcat(buf, "MESSAGE|");
        strcat(buf, username.c_str());
        if (tokens.size() > 2) {
            strcat(buf, "|");
            strcat(buf, tokens[2].c_str());
        }
        //cout << buf;
        int friendsock;
        int res = chatSocket(username, &friendsock);
        if (res == NOSOCK)
            return 0;
        if (res == NEWSOCK) {
            activeSocks.push_back(friendsock); //add friend socket to the list
            if (friendsock > *maxfP)
                *maxfP = friendsock;
            FD_SET(friendsock, orig_setP); // add friend socket to the set

        }


        write(friendsock, buf, strlen(buf) + 1);

    }
    if (tokens[0].compare("logout") == 0) {
        //send logout to server
        strcpy(buf, "");
        strcat(buf, "LOGOUT|");
        strcat(buf, myusername);
        write(sock, buf, strlen(buf) + 1);

        //close all socket
        //vector<int>::iterator  itIntV;
        //for(itIntV=activeSocks.begin();itIntV!=activeSocks.end();++itIntV)

        for (int i = 0; i < activeSocks.size(); i++)
            close(activeSocks[i]);

        activeSocks.clear();
        IP2Sock.clear();
        activeFriendname2Sock.clear();
        activeFriendsIP.clear();
        close(serverSockP);
        //close(sock);
        logdin = false;



    }

    return 0;

}

int processServerMsg(char * messageP) {
    vector<string> tokens;
    std::string str(messageP);
    cout << str;
    StrTokensm(&str, "|", &tokens);

    if (tokens[0].compare("NEWFRIENDS") == 0) {
        activeFriendsIP.clear();
        cout << str;
        fflush(0);
    }

    if (tokens[0].compare("FRIEND") == 0) {
        activeFriendsIP.clear();
        for (int j = 1; j < tokens.size(); j += 2) {
            std::string username = tokens[j];
            std::string IpAddress = tokens[j + 1];
            cout << "\nusername:" << username;
            cout << "\nIP Address:" << IpAddress;

            activeFriendsIP[username] = IpAddress;
        }
        printActiveFriends();

        removeUnvalidSockByIp(); //check<--------------------

        cout << messageP;
        fflush(0);
    }
    if (tokens[0].compare("INVITREQ") == 0) {
        std::string username = tokens[1];

        cout << endl << username << " invites you to the chat: ";
        if (tokens.size() > 2)
            cout << tokens[2] << endl;
    }

    if (tokens[0].compare("INVITACCEPT") == 0) {
        std::string username = tokens[1];

        cout << "\nYour invitation accepted by " << username << ": ";
        if (tokens.size() > 2)
            cout << "'" << tokens[2] << "'" << endl;

    }
}

int sendLocationInfo(int sock) {
    char buf[MAXBUFLEN] = "";
    strcpy(buf, "");
    strcat(buf, "LOCINFO|");
    strcat(buf, myusername);
    write(sock, buf, strlen(buf) + 1);
    return 0;
}

void printActiveFriends() {
    map<string, string>::iterator pos;
    cout << endl << "Your online friends: " << endl;
    for (pos = activeFriendsIP.begin(); pos != activeFriendsIP.end(); ++pos) {
        cout << pos->first << endl;
    }
    return;
}

int chatSocket(string friendUserName, int *sockP) {
    int sock;
    struct sockaddr_in server, recaddr;

    if (activeFriendname2Sock.find(friendUserName) != activeFriendname2Sock.end()) {
        *sockP = activeFriendname2Sock[friendUserName];
        return CRTSOCK;
    } else {
        //find from ip of of friend the socket
        if (activeFriendsIP.find(friendUserName) != activeFriendsIP.end()) {
            string friendIP = activeFriendsIP[friendUserName];
            if (IP2Sock.find(friendIP) != IP2Sock.end()) {
                *sockP = IP2Sock[friendIP];
                return CRTSOCK;
            }

            //connect to client's server socket and create connection 
            // and socket
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock == -1) {
                printf("Could not create socket");
            }
            puts("Socket created");

            server.sin_addr.s_addr = inet_addr(friendIP.c_str());
            server.sin_family = AF_INET;
            server.sin_port = htons(5100);

            //Connect to remote server
            if (connect(sock, (struct sockaddr *) &server, sizeof (server)) < 0) {
                perror("connect failed. Error");
                return 1;
            }

            puts("Connected\n");
            IP2Sock[friendIP] = sock;

            *sockP = sock;
            return NEWSOCK;
        }
        return NOSOCK;

    }

}

int findFriendNameByIP(string ipAddP, string *FriendName) {
    std::map<string, string>::const_iterator itS;

    for (itS = activeFriendsIP.begin(); itS != activeFriendsIP.end(); ++itS) {
        if (itS->second == ipAddP) {
            *FriendName = itS->first;
            return true;
        }
    }
    return false;

}

int Sock2IP(int sockP, string * IPP) {
    std::map<string, int>::const_iterator itS;
    for (itS = IP2Sock.begin(); itS != IP2Sock.end(); ++itS) {
        if (itS->second == sockP) {
            *IPP = itS->first;
            return true;
        }
    }
    return false;
}

int findFriendNameBySock(int sockP, string *friendName) {
    string ipAdd = "";
    Sock2IP(sockP, &ipAdd);
    string frdName = "";
    findFriendNameByIP(ipAdd, &frdName);
    *friendName = frdName;
    return true;
}

void printClientMessage(char * messageP, int sockP) {
    vector<string> tokens;
    std::string str(messageP);
    //cout << str;
    StrToken(&str, "|", &tokens);
    if (tokens[0].compare("MESSAGE") == 0) {
        std::string username = tokens[1];

        findFriendNameBySock(sockP, &username);
        std::string fmessage = tokens[2];
        cout << username << ">>" << fmessage << endl;
    }
    return;
}

void StrTokensm(const string * str, string seperators, vector<string>* tokens) {
    int start = str->find_first_not_of(seperators, 0);
    int pos = str->find_first_of(seperators, start);
    int tokenNum = 1;
    while ((string::npos != pos && string::npos != start)) {
        tokens->push_back(str->substr(start, pos - start));
        tokenNum++;
        start = str->find_first_not_of(seperators, pos);
        pos = str->find_first_of(seperators, start);
    }
    if (string::npos != start)
        tokens->push_back(str->substr(start, str->size() - start + 1));
    return;
}

void removeUnvalidSockByIp() {
    map<string, int>::iterator it;
    for (it = IP2Sock.begin(); it != IP2Sock.end(); it++)
        if (activeFriendsIP.find(it->first) == activeFriendsIP.end())
            IP2Sock.erase(it);

    return;


}

void cleanupSock(int sockP, string *usernameP) {

}