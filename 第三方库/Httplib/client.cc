#include "httplib.h"
using namespace httplib;

// 定义服务器的IP地址和端口
#define SERVER_IP "192.168.70.128"
#define SERVER_PORT 9090

int main() {
    // 创建一个客户端实例，指定服务器的IP和端口
    Client client(SERVER_IP, SERVER_PORT);

    // 创建一个MultipartFormData对象，用于存储数据
    MultipartFormData item;

    // 填写要发送的数据的详细信息
    item.name = "file";              // 表单字段的名称
    item.filename = "hello.txt";     // 要发送的文件的名称
    item.content = "Hello World!";   // 文件的内容
    item.content_type = "text/plain";// 内容的MIME类型

    // 创建一个MultipartFormDataItems向量，用于存储数据项
    MultipartFormDataItems items;
    items.push_back(item);

    // 发送一个带有多部分表单数据的POST请求到服务器的"/multipart"端点
    auto res = client.Post("/multipart", items);

    // 打印响应的状态和内容
    std::cout << "响应状态: " << res->status << std::endl;
    std::cout << "响应内容: " << res->body << std::endl;

    return 0;
}
