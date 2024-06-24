#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <map>
#include <vector>
#include <utility>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

using namespace std;

map<string, vector<string> > userFriends;
map<string, string> userPasswords;
map<string, int> activeUserSock;
map<string, string> activeUserIP;

struct arg_struct {
    char arg1[20];
    int arg2;
};

int isFriend(string user, string userf);
int sendFriends(int sockfd, string usernameP);
void StrToken(const string * str, string seperators, vector<string>* tokens);
void StrTokensm(const string * str, string seperators, vector<string>* tokens);
int username2sock(string unameP);
void printhostnamePort();
int readConfigFile(void);
int writeConfigFile(void);
void sendOthersFriends(string usernameP);


void initializeUsers();
int validUserNamePass(string username, string password);
int registerUser(string username, string password);
//the thread function to process connection
void *process_connection(void *);

const unsigned MAXBUFLEN = 512;
int socket_desc;

void intHandler(int sigvalue) {
    writeConfigFile();
    close(socket_desc);

    exit(0);
}

int main(int argc, char *argv[]) {
    int client_sock, c, *new_sock;
    struct sockaddr_in server, client;
    struct arg_struct *t_args;

    signal(SIGINT, intHandler);

    //Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket");
    }
    puts("Socket created");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(9999);

    //Bind
    if (bind(socket_desc, (struct sockaddr *) &server, sizeof (server)) < 0) {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
    printhostnamePort();

    //Listen
    listen(socket_desc, 5);

    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof (struct sockaddr_in);

    //initializeUsers();
    readConfigFile();


    while ((client_sock = accept(socket_desc, (struct sockaddr *) &client, (socklen_t*) & c))) {
        puts("Connection accepted");

        pthread_t connection_thread;
        new_sock = (int *) malloc(1);
        *new_sock = client_sock;

        t_args = (struct arg_struct*) malloc(sizeof (struct arg_struct));
        t_args->arg2 = client_sock;
        t_args->arg1[0] = '\0';
        strcat(t_args->arg1, inet_ntoa(client.sin_addr));


        if (pthread_create(&connection_thread, NULL, process_connection, (void*) t_args) < 0) {
            perror("could not create thread");
            return 1;
        }

        //join the thread , we dont terminate before the connection thread
        //pthread_join( connection_thread , NULL);
        puts("Handler assigned");
    }

    if (client_sock < 0) {
        perror("accept failed");
        return 1;
    }

    return 0;
}

/*
 * This will handle connection for each client
 * */

void *process_connection(void *arguments) {
    //Get the socket descriptor
    //int sock = *(int*) socket_desc;
    struct arg_struct *args = (struct arg_struct *) arguments;
    char clientIpAdd[20] = "";
    strcat(clientIpAdd, args->arg1);
    int sock = args->arg2;
    puts(clientIpAdd);
    int read_size;
    char *message, client_message[2000];
    string regUsername;


    //Receive a message from client
    while ((read_size = read(sock, client_message, 2000)) > 0) {
        //while ((read_size = recv(sock, client_message, 2000, 0)) > 0) {
        if (strstr(client_message, "LOGIN") == client_message) {
            string u, p;
            //u=msg.username;
            //p=msg.password;
            std::string str(client_message);
            cout << str;
            std::size_t found1 = str.find_first_of('|');
            std::size_t found2 = str.find_first_of('|', found1 + 1);
            std::string username = str.substr(found1 + 1, found2 - found1 - 1);
            std::string password = str.substr(found2 + 1);
            cout << "\nusername:" << username;
            cout << "\npassword:" << password;

            if (validUserNamePass(username, password)) {

                regUsername = string(username);
                client_message[0] = '\0';
                strcat(client_message, "200");
                client_message[5] = '\0';
                write(sock, client_message, strlen(client_message) + 1);
                //strncpy(msg.FriendsAdd,activeUserIP["user2"].c_str(),sizeof(msg.FriendsAdd));
                //strncpy(msg.FriendsAdd, "192.168.100.2",sizeof("192.168.100.2"));
                //write(sockfd, buf, strlen(buf));}
                activeUserSock[username] = sock;

            } else {
                client_message[0] = '\0';
                strcat(client_message, "500");
                client_message[7] = '\0';
                write(sock, client_message, strlen(client_message) + 1);
            }


            printf("\n writeback to client");
            fflush(0);
        }


        if (strstr(client_message, "REGISTER") == client_message) {
            string u, p;
            //u=msg.username;
            //p=msg.password;
            std::string str(client_message);
            cout << str;
            std::size_t found1 = str.find_first_of('|');
            std::size_t found2 = str.find_first_of('|', found1 + 1);
            std::string username = str.substr(found1 + 1, found2 - found1 - 1);
            std::string password = str.substr(found2 + 1);
            cout << "\nusername:" << username;
            cout << "\npassword:" << password;

            if (registerUser(username, password)) {
                regUsername = string(username);
                client_message[0] = '\0';
                strcat(client_message, "200");
                client_message[5] = '\0';
                write(sock, client_message, strlen(client_message) + 1);
                activeUserSock[username] = sock;

            } else {
                client_message[0] = '\0';
                strcat(client_message, "500");
                client_message[5] = '\0';
                write(sock, client_message, strlen(client_message) + 1);
            }

        }

        if (strstr(client_message, "LOCINFO") == client_message) {
            std::string str(client_message);
            std::size_t found1 = str.find_first_of('|');
            std::string username = str.substr(found1 + 1);
            activeUserIP[username] = clientIpAdd;
            cout << str << ": " << clientIpAdd << endl;
            sendFriends(sock, username);
            sendOthersFriends(username);


        }

        if (strstr(client_message, "INVITREQ") == client_message) {

            vector<string> tokens;
            std::string str(client_message);
            StrToken(&str, "|", &tokens);

            strcpy(client_message, "");
            strcat(client_message, "INVITREQ|");
            strcat(client_message, regUsername.c_str());
            if (tokens.size() > 2) {
                strcat(client_message, "|");
                strcat(client_message, tokens[2].c_str());
            }
            int clientsockid = username2sock(tokens[1]);
            if (clientsockid != -1) {
                write(clientsockid, client_message, strlen(client_message) + 1);
            }

            cout << str;

        }

        if (strstr(client_message, "INVITACCEPT") == client_message) {

            vector<string> tokens;
            std::string str(client_message);
            StrToken(&str, "|", &tokens);

            strcpy(client_message, "");
            strcat(client_message, "INVITACCEPT|");
            strcat(client_message, regUsername.c_str());
            if (tokens.size() > 2) {
                strcat(client_message, "|");
                strcat(client_message, tokens[2].c_str());
            }
            int clientsockid = username2sock(tokens[1]);
            if (clientsockid != -1) {
                write(clientsockid, client_message, strlen(client_message) + 1);

                userFriends[regUsername].push_back(tokens[1]);
                userFriends[tokens[1]].push_back(regUsername);
                sendFriends(clientsockid, tokens[1]);
                sendFriends(sock, regUsername);

            }
            cout << str;
        }

        if (strstr(client_message, "LOGOUT") == client_message) {

            vector<string> tokens;
            std::string str(client_message);
            StrToken(&str, "|", &tokens);

            map<string, string>::iterator it;
            it = activeUserIP.find(tokens[1]);
            activeUserIP.erase(it);

            map<string, int>::iterator itascock;
            itascock = activeUserSock.find(tokens[1]);
            activeUserSock.erase(itascock);

            sendOthersFriends(tokens[1]);
            //sendFriends(clientsockid, tokens[1]);


            cout << str;
            break;
        }

        //Send the message back to client
        //write(sock, client_message, read_size);
    }

    if (read_size == 0) {
        puts("Client disconnected");
        fflush(stdout);
    } else if (read_size == -1) {
        perror("something wrong");
    }

    //Free the arguments pointer
    free(arguments);
    close(sock);
    return 0;
}

void initializeUsers() {
    vector<string> * users, u2;
    users = new vector<string>();
    vector <string> ::iterator iStr;
    users->push_back("user2");
    users->push_back("user3");
    users->push_back("user5");
    userPasswords["user1"] = "123";
    userPasswords["user2"] = "234";
    //userFriends["user1"]="user2";
    //userFriends["user1"]="user3";
    activeUserIP["user2"] = "192.168.100.22";
    activeUserIP["user5"] = "192.168.100.55";
    //userFriends["user1"] = *users;



    //userFriends["user2"] = *users;
    for (iStr = userFriends["user1"].begin(); iStr != userFriends["user1"].end(); ++iStr)
        cout << *iStr << '\t';
    if (isFriend("user1", "user3"))
        cout << "++++++++Founded";
}

int validUserNamePass(string username, string password) {
    if (userPasswords.find(username) != userPasswords.end())
        if (userPasswords[username].compare(password) == 0)
            return true;
    return false;
}

int registerUser(string username, string password) {
    if (userPasswords.find(username) != userPasswords.end())
        return false;
    userPasswords[username] = password;
    return true;
}

int isFriend(string user, string userf) {
    vector <string> ::iterator iStr;

    for (iStr = userFriends[user].begin(); iStr != userFriends[user].end(); ++iStr)
        if (iStr->compare(userf) == 0)
            return true;

    return false;
}

int sendFriends(int sockfdP, string usernameP) {
    char buf[MAXBUFLEN] = "";
    vector <string> ::iterator iStr;

    strcpy(buf, "");
    strcat(buf, "FRIEND");
    for (iStr = userFriends[usernameP].begin(); iStr != userFriends[usernameP].end(); ++iStr) {

        if (activeUserIP.find(*iStr) != activeUserIP.end()) {
            string uname = *iStr;
            strcat(buf, "|");
            strcat(buf, iStr->c_str());
            strcat(buf, "|");
            strcat(buf, activeUserIP[uname].c_str());
        }
    }
    cout << endl << buf << endl;
    cout << usernameP << "---->socket:" << sockfdP;
    fflush(0);
    string str1(buf);
    if(str1.compare("FRIEND")>0)
    write(sockfdP, buf, strlen(buf) + 1);
    return 0;
}

int username2sock(string unameP) {
    if (activeUserSock.find(unameP) != activeUserSock.end())
        return activeUserSock[unameP];
    return -1;
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

void printhostnamePort() {
    struct addrinfo hints, *info, *p;
    int result;

    char hostname[512] = "";
    //hostname[511] = '\0';
    gethostname(hostname, 511);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    if ((result = getaddrinfo(hostname, "http", &hints, &info)) != 0) {
        fprintf(stderr, "get add info: %s\n", gai_strerror(result));
        exit(1);
    }

    for (p = info; p != NULL; p = p->ai_next) {
        printf("hostname: %s\n", p->ai_canonname);
    }

    printf("port:%d\n", 9999);
    freeaddrinfo(info);

    return;
}

int readConfigFile(void) {
    FILE * fp;
    char *line;
    size_t len = 100;

    ssize_t read;
    vector<string> * users;
    vector <string> ::iterator iStr;

    line = (char *) malloc(len * sizeof (char));
    //char cwd[1000];
    //if (getcwd(cwd, sizeof (cwd)) != NULL) {
    //  printf("Current working dir: %s\n", cwd);
    //}

    fp = fopen("./user_info_file", "r");
    if (fp == NULL)
        exit(0);

    while ((read = getline(&line, &len, fp)) != -1) {

        // while ( fgets (line , 100 , fp) != NULL ){
        printf("%s", line);
        //line[read]='0';
        //puts(line);
        fflush(0);
        vector<string> tokens;
        std::string str(line);
        StrTokensm(&str, "|;\n\r", &tokens);
        string uname = tokens[0];
        string pswd = tokens[1];

        userPasswords[uname] = pswd;
        users = new vector<string>();

        for (int i = 2; i < tokens.size(); i++) {
            users->push_back(tokens[i]);
        }
        userFriends[uname] = *users;
    }

    fclose(fp);
    //if (line)
    //  free(line);
    return 0;
}

int writeConfigFile(void) {
    // map<string, vector<string> > userFriends;
    //map<string, string> userPasswords;

    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    vector<string> users;
    vector <string> ::iterator iStr;
    map<string, vector<string> > ::iterator itUsrFrnds;

    fp = fopen("./user_info_file", "w");
    if (fp == NULL)
        exit(0);

    for (itUsrFrnds = userFriends.begin(); itUsrFrnds != userFriends.end(); itUsrFrnds++) {
        string uname = itUsrFrnds->first;
        users = itUsrFrnds->second;
        char line[1024];
        line[0] = 0;
        strcat(line, uname.c_str());
        strcat(line, "|");
        string passwd = userPasswords.find(uname)->second;
        strcat(line, passwd.c_str());
        // strcat(line,"|");       
        for (int i = 0; i < users.size(); ++i) {
            if (i == 0)
                strcat(line, "|");
            if (i > 0)
                strcat(line, ";");
            strcat(line, users[i].c_str());
        }
        //strcat(line,"\n");
        //if (users.size() > 0)
        //  fprintf(fp, "%s", line);
        //else
        fprintf(fp, "%s\n", line);

        //write(fp,line,strlen(line));
        //fwrite(line, strlen(line)+1, 1, fp);

    }
    fclose(fp);


}

int isActiveUser(string usernameP) {
    if (activeUserIP.find(usernameP) != activeUserIP.end())
        return true;
    return false;
}

void sendOthersFriends(string usernameP) {

    cout << "OTHER FRIENDS----";
    if (userFriends.find(usernameP) != userFriends.end())
        for (int i = 0; i < userFriends[usernameP].size(); i++) {

            string uname = userFriends[usernameP][i];
            cout << "-------" << uname;
            if (isActiveUser(uname)) {
                int socku = username2sock(uname);
                cout << uname << "---->socket:" << socku;
                sendFriends(socku, uname);

            }

        }

}