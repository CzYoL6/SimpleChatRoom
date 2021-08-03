#include "server.h"

int main() {
    Server server;
    server.Start("172.29.249.214", 2333);
    while (server.IsRunning()) {
        server.Update();
        server.HandleData();
    }
    return 0;
}