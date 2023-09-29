#pragma once
#include "util.hpp"
#include <mutex>

#define CONFIG_FILE "./cloud.conf"
namespace cloud {
    class Config {
    public:
        //获得单例对象 当多个线程同时访问GetInstance时，存在多个线程可能同时创建Config类的实例的风险。所以需要加互斥锁
        static Config *GetInstance() {
            _mutex.lock();
            if (_instance == nullptr) {
                _instance = new Config();
            }
            _mutex.unlock();
            return _instance;
        }
        //获取各自的成员变量
        int GetHotTime() {
            return hot_time;
        }

        int GetServerPort() {
            return server_port;
        }

        std::string GetServerIp() {
            return server_ip;
        }

        std::string GetUrlPrefix() {
            return url_prefix;
        }

        std::string GetArc_Suffix() {
            return arc_suffix;
        }

        std::string GetPackDir() {
            return pack_dir;
        }

        std::string GetBackDir() {
            return back_dir;
        }

        std::string GetManagerFile() {
            return manager_file;
        }

    private:
        Config() {
            ReadConfigFile();
        }//单例模式构造函数私有化
    public:
        static Config *_instance;//单例对象
        static std::mutex _mutex;

    private:
        int hot_time;            //热点判断时间
        int server_port;         //服务器监听端口
        std::string server_ip;   //服务器IP地址
        std::string url_prefix;  //下载的url前缀路径
        std::string arc_suffix;  //压缩包后缀名称
        std::string pack_dir;    //压缩包存放目录
        std::string back_dir;    //备份文件存放目录
        std::string manager_file;//数据信息存放文件

        //读取config文件里的数据,并反序列化到root类型中
        bool ReadConfigFile() {
            FileUtil fu(CONFIG_FILE);
            std::string body;
            //将CONFIG_FILE里的文件内容读取到body中,cloud.conf里面是json序列化数据,所以需要反序列化
            if (fu.GetContent(&body) == false) {
                std::cout << "load config file failed" << std::endl;
                return false;
            }

            //将body中的数据写入到root中
            Json::Value root;
            if (JsonUtil::UnSerialize(body, root) == false) {
                std::cout << "parse config file failed" << std::endl;
                return false;
            }
            //取root中的数据
            hot_time = root["hot_time"].asInt();
            server_port = root["server_port"].asInt();
            server_ip = root["server_ip"].asString();
            url_prefix = root["url_prefix"].asString();
            arc_suffix = root["arc_suffix"].asString();
            pack_dir = root["pack_dir"].asString();
            back_dir = root["back_dir"].asString();
            manager_file = root["manager_file"].asString();

            return true;
        }
    };

    //静态成员另外初始化
    Config *Config::_instance = nullptr;
    std::mutex Config::_mutex;
}// namespace cloud