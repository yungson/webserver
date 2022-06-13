#include "server/webserver.h"
#include "stdlib.h"

int main(int argc, char* argv[]){

    WebServer server(8000, 3, 60000, false,
        3306, "root", getenv("webserver_mysql_passwod"), "webserver",
        12, 56, 1, 1024);
    server.Start();
}