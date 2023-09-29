#pragma once
#include "bundle.h"
#include <experimental/filesystem>//C++17
#include <fstream>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
namespace cloud {
    namespace fs = std::experimental::filesystem;//C++17文件系统的命名空间
    class FileUtil {
    public:
        //默认构造函数,传入的文件名,也可能是一个带文件夹的名字
        FileUtil(const std::string &filename)
            : _filename(filename) {
        }

        //删除文件
        bool Remove() {
            if (this->Exists() == false) {
                return true;
            }
            //删除filename文件
            remove(_filename.c_str());
            return true;
        }

        //返回文件大小
        int64_t FileSize() {
            struct stat st;//用于存储文件信息的结构体
            //stat函数用于获取文件信息,一个参数是文件名,第二个是输出型参数,也就是存储文件信息的结构体
            if (stat(_filename.c_str(), &st) < 0) {
                std::cout << "get file size failed!\n";
                return -1;
            }

            return st.st_size;
        }

        //返回最后修改时间
        time_t LastMtime() {
            struct stat st;
            if (stat(_filename.c_str(), &st) < 0) {
                std::cout << "get file Modify time failed!\n";
                return -1;
            }

            return st.st_mtim.tv_sec;//st_mtim是一个结构体,里面存的时间戳
        }

        //返回最后访问时间
        time_t LastAtime() {
            struct stat st;
            if (stat(_filename.c_str(), &st) < 0) {
                std::cout << "get file Modify time failed!\n";
                return -1;
            }

            return st.st_atim.tv_sec;
        }

        //返回文件名称
        std::string FileName() {
            // ./abc/text.text   ->  找最后一个/
            size_t pos = _filename.find_last_of("/");
            if (pos == std::string::npos) {
                return _filename;
            }
            //   /位置后面一个位置就是文件名称
            return _filename.substr(pos + 1);
        }

        //获取文件指定位置，指定长度的数据
        bool GetPosLen(std::string *body, size_t pos, size_t len) {
            std::ifstream ifs;
            ifs.open(_filename, std::ios::binary);//打开文件
            if (ifs.is_open() == false) {
                std::cout << "read open file failed!\n";
                return false;
            }

            size_t fsize = this->FileSize();//记录文件大小
            //pos+len超过文件大小则失败
            if (pos + len > fsize) {
                std::cout << "get file len is error\n";
                return false;
            }
            //文件跳转到pos位置
            ifs.seekg(pos, std::ios::beg);
            body->resize(len);         //body扩容
            ifs.read(&(*body)[0], len);//将文件中数据读到body中
            if (ifs.good() == false) {
                std::cout << "get file content failed\n";
                ifs.close();
                return false;
            }

            ifs.close();
            return true;
        }

        bool GetContent(std::string *body) {
            size_t fsize = this->FileSize();
            //复用代码
            return GetPosLen(body, 0, fsize);
        }

        //将body中的数据写入到文件中
        bool SetContent(const std::string &body) {
            std::ofstream ofs;
            ofs.open(_filename, std::ios::binary);//打开文件
            if (ofs.is_open() == false) {
                std::cout << "write open file failed" << std::endl;
            }
            //body中的数据写入到文件中
            ofs.write(&body[0], body.size());
            if (ofs.good() == false) {
                std::cout << "write file content failed" << std::endl;
                ofs.close();
                return false;
            }
            ofs.close();
            return true;
        }

        //文件压缩,packname是压缩文件名
        bool Compress(const std::string &packname) {
            //1.获取源文件数据
            std::string body;
            if (this->GetContent(&body) == false) {
                std::cout << "compress get file content failed" << std::endl;
                return false;
            }
            //2.对数据进行压缩 -> body的数据进行压缩
            std::string packed = bundle::pack(bundle::LZIP, body);
            //3.将压缩的数据存储到压缩包文件中
            FileUtil fu(packname);//传入的是压缩文件名
            if (fu.SetContent(packed) == false) {
                std::cout << "compress write packed data failed" << std::endl;
                return false;
            }
            return true;
        }

        //文件解压缩
        bool Uncompress(const std::string &filename) {
            //1.将当前压缩包数据读取出来存放在body中
            std::string body;
            if (this->GetContent(&body) == false) {
                std::cout << "uncompress get file content failed" << std::endl;
                return false;
            }
            //2.对压缩的数据进行解压缩 -> 将压缩包的数据body解压缩后存在unpacked中
            std::string unpacked = bundle::unpack(body);
            //3.将解压缩的数据写入到新文件
            FileUtil fu(filename);
            if (fu.SetContent(unpacked) == false) {
                std::cout << "uncompress write packed data failed" << std::endl;
                return false;
            }

            return true;
        }

        //判断文件夹是否存在
        bool Exists() {
            return fs::exists(_filename);
        }

        //创建目录
        bool CreateDirectory() {
            //判断目录是否存在
            if (this->Exists()) {
                return true;
            }
            //没有则创建目录
            std::cout << "创建目录" << std::endl;
            return fs::create_directories(_filename);
        }

        //扫描目录文件
        bool ScanDirectory(std::vector<std::string> *array) {
            //directory_iterator目录迭代器
            for (auto &p: fs::directory_iterator(_filename)) {
                if (fs::is_directory(p) == true) {
                    //如果是目录则找下一个文件
                    continue;
                }
                //relative_path 带有路径的文件名
                //将文件尾插到array向量中
                array->push_back(fs::path(p).relative_path().string());
            }
        }

    private:
        std::string _filename;//文件名称(可以带名称)
    };

    class JsonUtil {
    public:
        //Json序列化
        static bool Serialize(const Json::Value &root, std::string *str) {
            Json::StreamWriterBuilder swb;//Json对象
            //swb.newStreamWriter返回类型是StreamWriter*
            std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
            std::stringstream ss;
            //root数据写入到ss中
            if (sw->write(root, &ss) != 0) {
                std::cout << "Json write failed" << std::endl;
                return false;
            }

            *str = ss.str();
            return true;
        }

        //反序列化
        static bool UnSerialize(const std::string &str, Json::Value &root) {
            Json::CharReaderBuilder crb;
            std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
            std::string err;
            //解析str字符串到root中
            bool ret = cr->parse(str.c_str(), str.c_str() + str.size(), &root, &err);
            if (ret == false) {
                std::cout << "parse error:" << err << std::endl;
                return false;
            }
            return true;
        }
    };
}// namespace cloud