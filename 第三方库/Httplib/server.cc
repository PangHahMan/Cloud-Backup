#include "httplib.h"    // 包含 httplib 库
using namespace httplib;// 使用 httplib 命名空间

// 处理 "/hi" 路由的处理函数
void Hello(const Request &req, Response &rsp) {
    rsp.set_content("Hello World!", "text/plain");// 设置响应内容为 "Hello World!"
    rsp.status = 200;                             // 设置响应状态码为 200
}

// 处理 "/numbers/<number>" 路由的处理函数
void Numbers(const Request &req, Response &rsp) {
    auto num = req.matches[1];         // 从路径中提取捕获的数字
    rsp.set_content(num, "text/plain");// 将捕获的数字设置为响应内容
    rsp.status = 200;                  // 设置响应状态码为 200
}

// 处理 "/multipart" 路由的处理函数
void Multipart(const Request &req, Response &rsp) {
    auto ret = req.has_file("file");// 检查是否上传了文件
    if (ret == false) {
        std::cout << "未上传文件\n";// 如果没有上传文件，打印消息
        rsp.status = 404;           // 设置响应状态码为 404
        return;
    }
    const auto &file = req.get_file_value("file");// 获取上传文件的信息
    rsp.body.clear();
    rsp.body = file.filename;// 将上传文件的名称设置为响应体
    rsp.body += "\n";
    rsp.body += file.content;                    // 将文件内容附加到响应体
    rsp.set_header("Content-Type", "text/plain");// 设置响应头
    rsp.status = 200;                            // 设置响应状态码为 200
    return;
}

int main() {
    httplib::Server server;// 实例化 Server 类以创建服务器对象

    // 注册不同 HTTP 方法和路径的路由处理函数
    server.Get("/hi", Hello);              // 为 GET /hi注册Hello处理函数
    server.Get("/numbers/(\\d+)", Numbers);// 为 GET /numbers/<number> 注册Numbers处理函数
    server.Post("/multipart", Multipart);  // 为 POST /multipart 注册Multipart处理函数

    server.listen("0.0.0.0", 9090);// 启动服务器，监听IP为0.0.0.0(任意IP)，端口为 9090
    return 0;                      // 从主函数返回
}
