#pragma once
#include "datamanager.hpp"
#include "httplib.h"
#include "user.hpp"

extern cloud::DataManager *_data;
namespace cloud {
    class Server {
    public:
        Server() {
            //获取对应的属性,属性存放在config.conf中
            Config *config = Config::GetInstance();
            _server_port = config->GetServerPort();
            _server_ip = config->GetServerIp();
            _url_prefix = config->GetUrlPrefix();
        }

    private:
        int _server_port;       //服务端口
        std::string _server_ip; //服务ip
        std::string _url_prefix;//下载前缀
        httplib::Server _server;//httplib类

    private:
        //文件上传
        static void Upload(const httplib::Request &req, httplib::Response &rsp) {
            // post /upload 文件数据在正文中(正文并不全是文件数据)
            auto ret = req.has_file("file");//检查是否上传了文件
            if (ret == false) {
                rsp.status = 400;
                return;
            }

            const auto &file = req.get_file_value("file");
            //file.filename//文件名称    file.content//文件数据
            std::string back_dir = Config::GetInstance()->GetBackDir();//备份目录
            std::string realpath = back_dir + FileUtil(file.filename).FileName();
            std::cout << "real path:" << realpath << std::endl;
            FileUtil fu(realpath);
            fu.SetContent(file.content);//将数据写入文件中

            std::string user_id = req.get_header_value("user_id");// 获取用户ID
            std::cout << "user_id:" << user_id << std::endl;

            BackupInfo info;                      //文件信息
            info.NewBackupInfo(realpath, user_id);//组织备份的文件信息，包含用户ID
            _data->Insert(info);                  //向数据管理模块添加备份的文件信息

            rsp.body = "ok";
            rsp.set_header("Content-Type", "text/html; charset=UTF-8");
            return;
        }


        static std::string TimetoStr(time_t t) {
            std::string tmp = std::ctime(&t);
            return tmp;
        }

        static void Listshow(const httplib::Request &req, httplib::Response &rsp) {
            std::string search_query = req.get_param_value("query");// Get the search query parameter from the request
            std::string user_id = req.get_header_value("user_id");
            // 1. Get all backup info
            std::vector<BackupInfo> arry;
            _data->GetAll(&arry);

            // 2. Organize the HTML response
            std::stringstream ss;
            ss << "<html><head><title>Download</title></head>";
            ss << "<body><h1>Download</h1>";

            // Upload form
            std::ifstream file("index/upload.html");
            if (!file) {
                rsp.status = 500;// Internal Server Error
                return;
            }

            // Read the contents of the file
            std::string html((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

            ss << html;
            // Search form
            ss << "<h2>Search Files</h2>";
            ss << "<form action='/search' method='get'>";
            ss << "<input type='text' name='query' placeholder='Search for files' value='" << search_query << "'>";
            ss << "<input type='submit' value='Search'>";
            ss << "</form>";

            ss << "<table>";
            for (auto &a: arry) {
                std::string filename = FileUtil(a.real_path).FileName();

                // Check if the search query is empty or the filename contains the query
                if (search_query.empty() || filename.find(search_query) != std::string::npos) {
                    ss << "<tr>";
                    ss << "<td><a href='" << a.url << "'>" << filename << "</a></td>";
                    ss << "<td align='right'>" << TimetoStr(a.mtime) << "</td>";
                    ss << "<td align='right'>" << a.fsize / 1024 << "k</td>";

                    // Add the Delete link
                    ss << "<td><a href='/delete?url=" << a.url << "'>Delete</a></td>";

                    ss << "</tr>";
                }
            }
            ss << "</table></body></html>";

            // Send the response
            rsp.body = ss.str();
            rsp.set_header("Content-Type", "text/html; charset=UTF-8");

            rsp.status = 200;
        }

        static std::string GetETag(const BackupInfo &info) {
            // etg :  filename-fsize-mtime
            FileUtil fu(info.real_path);
            std::string etag = fu.FileName();
            etag += "-";
            etag += std::to_string(info.fsize);
            etag += "-";
            etag += std::to_string(info.mtime);
            return etag;
        }

        static void Download(const httplib::Request &req, httplib::Response &rsp) {
            //1. 获取客户端请求的资源路径path   req.path
            //2. 根据资源路径，获取文件备份信息
            BackupInfo info;
            //std::string decodedFilename = url_decode(req.path);
            std::string decodedFilename = httplib::detail::decode_url(req.path, true);
            _data->GetOneByURL(decodedFilename, &info);

            //3. 判断文件是否被压缩，如果被压缩，要先解压缩,
            if (info.pack_flag == true) {
                FileUtil fu(info.pack_path);
                fu.Uncompress(info.real_path);//将文件解压到备份目录下
                //4. 删除压缩包，修改备份信息（已经没有被压缩）
                fu.Remove();
                info.pack_flag = false;
                _data->Update(info);
            }

            bool retrans = false;
            std::string old_etag;
            if (req.has_header("If-Range")) {
                old_etag = req.get_header_value("If-Range");
                //有If-Range字段且，这个字段的值与请求文件的最新etag一致则符合断点续传
                if (old_etag == GetETag(info)) {
                    retrans = true;
                }
            }

            //5. 读取文件数据，放入rsp.body中
            FileUtil fu(info.real_path);
            if (retrans == false) {
                fu.GetContent(&rsp.body);
                //6. 设置响应头部字段： ETag， Accept-Ranges: bytes
                rsp.set_header("Accept-Ranges", "bytes");
                rsp.set_header("ETag", GetETag(info));
                rsp.set_header("Content-Type", "application/octet-stream; charset=UTF-8");
                rsp.status = 200;
            } else {
                //httplib内部实现了对于区间请求也就是断点续传请求的处理
                //只需要我们用户将文件所有数据读取到rsp.body中，它内部会自动根据请求区间，从body中取出指定区间数据进行响应
                // std::string  range = req.get_header_val("Range"); bytes=start-end
                fu.GetContent(&rsp.body);
                rsp.set_header("Accept-Ranges", "bytes");
                rsp.set_header("ETag", GetETag(info));
                rsp.set_header("Content-Type", "application/octet-stream; charset=UTF-8");
                //rsp.set_header("Content-Range", "bytes start-end/fsize");
                rsp.status = 206;//区间请求响应的是206*****
            }
        }

        static void Search(const httplib::Request &req, httplib::Response &rsp) {
            // 从请求中获取搜索查询参数
            std::string search_query = req.get_param_value("query");

            // 如果没有提供搜索查询参数，可以返回一个错误响应
            if (search_query.empty()) {
                rsp.status = 400;// Bad Request
                rsp.body = "Missing search query";
                return;
            }

            // 获取所有备份信息
            std::vector<BackupInfo> all_backup_info;
            _data->GetAll(&all_backup_info);

            // 根据搜索查询参数过滤备份信息
            std::vector<BackupInfo> filtered_backup_info;
            for (const auto &info: all_backup_info) {
                std::string filename = FileUtil(info.real_path).FileName();

                // 如果文件名包含搜索查询参数，将其添加到过滤后的列表中
                if (filename.find(search_query) != std::string::npos) {
                    filtered_backup_info.push_back(info);
                }
            }

            // 生成 HTML 响应，显示过滤后的文件列表
            std::stringstream ss;
            ss << "<html><head><title>Search Results</title></head>";
            ss << "<body><h1>Search Results</h1>";

            // 显示搜索查询参数
            ss << "<p>Search query: " << search_query << "</p>";

            // 显示过滤后的文件列表
            ss << "<table>";
            for (const auto &info: filtered_backup_info) {
                std::string filename = FileUtil(info.real_path).FileName();
                ss << "<tr>";
                ss << "<td><a href='" << info.url << "'>" << filename << "</a></td>";
                ss << "<td align='right'>" << TimetoStr(info.mtime) << "</td>";
                ss << "<td align='right'>" << info.fsize / 1024 << "k</td>";
                ss << "</tr>";
            }
            ss << "</table></body></html>";

            // 发送响应
            rsp.body = ss.str();
            rsp.set_header("Content-Type", "text/html;charset=UTF-8");
            rsp.status = 200;// OK
        }

        static void DeleteFile(const httplib::Request &req, httplib::Response &rsp) {
            // 从请求中获取要删除的文件的URL或其他唯一标识符
            std::string file_url = req.target;// 假设通过请求参数传递文件的URL
            file_url = httplib::detail::decode_url(file_url, true);
            size_t pos = file_url.find("/delete?url=");
            if (pos != std::string::npos) {
                // Remove "/delete?url=" and keep the remaining part
                file_url = file_url.substr(pos + strlen("/delete?url="));
            }

            // 检查是否提供了有效的文件URL
            if (file_url.empty()) {
                rsp.status = 400;// Bad Request
                rsp.body = "Missing file URL";
                return;
            }

            // 根据文件URL查找备份信息
            BackupInfo info;
            if (!_data->GetOneByURL(file_url, &info)) {
                rsp.status = 404;// Not Found
                rsp.body = "File not found";
                return;
            }

            // 删除文件  备份和压缩一起删除
            FileUtil fu(info.real_path);
            FileUtil fu2(info.pack_path);
            if (fu.Remove() && fu2.Remove()) {
                // 如果文件删除成功，还需要从数据管理模块中删除备份信息
                _data->Delete(file_url);
                rsp.status = 200;// OK
                rsp.body = "File deleted successfully";
            } else {
                rsp.status = 500;// Internal Server Error
                rsp.body = "Failed to delete file";
            }
        }

        // 负责处理用户注册请求
        static void UserRegister(const httplib::Request &req, httplib::Response &rsp) {
            std::cout << "注册请求" << std::endl;
            std::string username = req.get_param_value("username");
            std::string password = req.get_param_value("password");


            if (username.size() < 3) {
                rsp.body = "用户名必须为3位";
                rsp.set_header("Content-Type", "text/html;charset=UTF-8");
                return;
            }

            if (password.size() < 8) {
                rsp.body = "密码必须为8位";
                rsp.set_header("Content-Type", "text/html;charset=UTF-8");
                return;
            }

            // 假设我们有一个UserManager类来处理用户相关操作
            cloud::UserManager userManager;
            if (userManager.addUser(username, password)) {
                rsp.status = 200;// OK
                rsp.body = "注册成功";
            } else {
                rsp.status = 500;// Internal Server Error
                rsp.body = "用户已存在";
            }

            rsp.set_header("Content-Type", "text/html;charset=UTF-8");
        }

        // 负责处理用户登录请求
        static void UserLogin(const httplib::Request &req, httplib::Response &rsp) {
            std::cout << "登录请求" << std::endl;
            std::string username = req.get_param_value("username");
            std::string password = req.get_param_value("password");

            // 假设我们有一个UserManager类来处理用户相关操作
            cloud::UserManager userManager;
            std::cout << "用户名:" << username << "密码:" << password << std::endl;
            if (userManager.verifyUser(username, password)) {
                // 设置状态码为302和设置"Location"头部为备份页面的URL，以实现重定向
                rsp.status = 302;// Found
                rsp.set_header("Location", "/listshow");
            } else {
                rsp.status = 401;// Unauthorized
                rsp.body = "Invalid username or password";
            }

            rsp.set_header("Content-Type", "text/html;charset=UTF-8");
        }

        static void HomePage(const httplib::Request &req, httplib::Response &rsp) {
            std::ifstream file("index/login.html");// 读取html文件

            if (file) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                rsp.body = buffer.str();// 将文件内容赋值给response的body

                rsp.set_header("Content-Type", "text/html;charset=UTF-8");
                rsp.status = 200;// OK
            } else {
                rsp.status = 404;// Not Found
            }
        }

    public:
        bool RunModule() {
            //注册不同 HTTP 方法和路径的路由处理函数
            _server.Get("/", HomePage);// 当用户访问根路径时，显示主页
            _server.Post("/register", UserRegister);
            _server.Get("/login", UserLogin);
            _server.Post("/upload", Upload);
            _server.Get("/listshow", Listshow);
            std::string down_load_url = _url_prefix + "(.*)";//获取完整的下载路径
            _server.Get(down_load_url, Download);
            _server.Get("/search", Search);
            _server.Get("/delete", DeleteFile);
            _server.listen(_server_ip, _server_port);//启动服务器
            return true;
        }
    };
}// namespace cloud