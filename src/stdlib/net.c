#include "net.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#define closesocket close
#endif
#include <stdio.h>
#include <string.h>
#include "../interpreter.h"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

static bool ws_initialized = false;

void net_init() {
    if (ws_initialized) return;
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup 失败\n");
        return;
    }
#endif
    ws_initialized = true;
}

Value net_listen(int argc, Value *argv) {
    net_init();
    int port = (argc > 0 && IS_INT(argv[0])) ? (int)argv[0].integer : 8080;
    Value handler = (argc > 1) ? argv[1] : VAL_NIL_V;

    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(listen_sock, SOMAXCONN);

    printf("Pico Web 服务正在监听端口 %d...\n", port);

    while (1) {
        SOCKET client_sock = accept(listen_sock, NULL, NULL);
        if (client_sock == INVALID_SOCKET) break;

        char buf[4096];
        int bytes = recv(client_sock, buf, sizeof(buf) - 1, 0);
        if (bytes > 0) {
            buf[bytes] = '\0';
            // 简单的 HTTP 解析 (仅获取路径)
            char method[16], path[256];
            sscanf(buf, "%15s %255s", method, path);

            if (handler.type == VAL_FN) {
                // 调用 Pico 处理函数
                // 暂时只传路径字符串
                Value arg = VAL_STR_V(str_intern(path, (int)strlen(path)));
                Interpreter *vm = interp_get_current();
                Value response = interp_exec(vm, handler.fn->body, env_new(handler.fn->closure));
                // 暂时假设返回字符串
                const char *res_body = (response.type == VAL_STRING) ? response.string->data : "Hello from Pico";
                
                char res_header[512];
                sprintf(res_header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", (int)strlen(res_body));
                send(client_sock, res_header, (int)strlen(res_header), 0);
                send(client_sock, res_body, (int)strlen(res_body), 0);
            } else {
                const char *res = "HTTP/1.1 200 OK\r\nContent-Length: 11\r\nConnection: close\r\n\r\nHello World";
                send(client_sock, res, (int)strlen(res), 0);
            }
        }
        closesocket(client_sock);
    }

    closesocket(listen_sock);
    return VAL_NIL_V;
}
