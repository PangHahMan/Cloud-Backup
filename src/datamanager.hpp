#pragma once
#include "config.hpp"
#include "util.hpp"
#include <pthread.h>
#include <unordered_map>

namespace cloud {
    typedef struct BackupInfo {
        std::string user_id;  // 新增的用户ID字段
        bool pack_flag;       //是否压缩标志
        size_t fsize;         //文件大小
        time_t mtime;         //最后一次修改时间
        time_t atime;         //最后一次访问时间
        std::string real_path;//文件实际存储路径名称
        std::string pack_path;//压缩包存储路径名称
        std::string url;      //url

        //填充文件信息
        bool NewBackupInfo(const std::string &realpath, const std::string &userId) {
            FileUtil fu(realpath);
            //文件可能不存在需要判断
            if (fu.Exists() == false) {
                std::cout << "new backupinfo file not exists!" << std::endl;
                return false;
            }
            Config *config = Config::GetInstance();
            //填充文件信息
            std::string packdir = config->GetPackDir();      //压缩包存放目录
            std::string arc_suffix = config->GetArc_Suffix();//压缩包后缀名称
            std::string url_prefix = config->GetUrlPrefix(); //下载URL名称

            this->user_id = userId;// 为用户ID字段赋值
            this->pack_flag = false;
            this->fsize = fu.FileSize();
            this->mtime = fu.LastMtime();
            this->atime = fu.LastAtime();
            this->real_path = realpath;
            //./backdir/a.txt -> ./packdir/a.txt.lz
            this->pack_path = packdir + fu.FileName() + arc_suffix;
            this->url = url_prefix + fu.FileName();

            return true;
        }
    } BackupInfo;

    //数据管理类
    class DataManager {
    public:
        DataManager() {
            //获取数据存储持久化文件
            _manage_file = Config::GetInstance()->GetManagerFile();
            pthread_rwlock_init(&_rwlock, nullptr);//初始化读写锁
            InitLoad();                            //读取back.dat文件中的内容到map _table中
        }

        ~DataManager() {
            pthread_rwlock_destroy(&_rwlock);//销毁读写锁
        }

        //插入数据信息
        bool Insert(const BackupInfo &info) {
            pthread_rwlock_wrlock(&_rwlock);
            //插入一个新的信息,以url为key值
            _table[info.url] = info;
            pthread_rwlock_unlock(&_rwlock);

            Storage();//更新文件信息
            return true;
        }

        //插入和更新逻辑暂时相同
        bool Update(const BackupInfo &info) {
            pthread_rwlock_wrlock(&_rwlock);
            _table[info.url] = info;
            pthread_rwlock_unlock(&_rwlock);

            Storage();
            return true;
        }

        //根据URL获取文件信息
        bool GetOneByURL(const std::string &url, BackupInfo *info) {
            pthread_rwlock_wrlock(&_rwlock);
            //因为url是key值,所以直接通过find进行查找
            auto it = _table.find(url);
            //没找到
            if (it == _table.end()) {
                pthread_rwlock_unlock(&_rwlock);
                return false;
            }
            *info = it->second;
            pthread_rwlock_unlock(&_rwlock);
            return true;
        }

        //根据文件路径获取文件信息
        bool GetOneByRealPath(const std::string &realpath, BackupInfo *info) {
            pthread_rwlock_wrlock(&_rwlock);
            auto it = _table.begin();
            //map中进行查找
            for (; it != _table.end(); it++) {
                if (it->second.real_path == realpath) {
                    *info = it->second;
                    pthread_rwlock_unlock(&_rwlock);
                    return true;
                }
            }
            pthread_rwlock_unlock(&_rwlock);
            return false;
        }

        //将所有的文件信息存放在array数组中
        bool GetAll(std::vector<BackupInfo> *array) {
            pthread_rwlock_wrlock(&_rwlock);
            auto it = _table.begin();
            for (; it != _table.end(); it++) {
                array->push_back(it->second);
            }
            pthread_rwlock_unlock(&_rwlock);
            return true;
        }

        bool Storage() {
            //1. 获取所有数据
            std::vector<BackupInfo> arry;
            this->GetAll(&arry);//将map中的文件信息全部保存在array中
            //2. 添加到Json::Value
            Json::Value root;
            for (int i = 0; i < arry.size(); i++) {
                Json::Value item;
                item["user_id"] = arry[i].user_id;
                item["pack_flag"] = arry[i].pack_flag;
                item["fsize"] = (Json::Int64) arry[i].fsize;
                item["atime"] = (Json::Int64) arry[i].atime;
                item["mtime"] = (Json::Int64) arry[i].mtime;
                item["real_path"] = arry[i].real_path;
                item["pack_path"] = arry[i].pack_path;
                item["url"] = arry[i].url;
                root.append(item);//添加数组元素
            }
            //3. 对Json::Value序列化
            std::string body;
            JsonUtil::Serialize(root, &body);
            //4. 写入到文件中
            FileUtil fu(_manage_file);//创建文件
            fu.SetContent(body);      //将body中的序列化数据写入到文件
            return true;
        }

        //读取back.dat文件中的内容到map _table中
        bool InitLoad() {
            //1. 将数据文件中的数据读取出来
            FileUtil fu(_manage_file);
            if (fu.Exists() == false) {
                std::cout << "InitLoad _manage_file failed" << std::endl;
                return true;
            }
            //将文件中的数据提取到body中
            std::string body;
            fu.GetContent(&body);

            //2. 反序列化
            Json::Value root;
            JsonUtil::UnSerialize(body, root);//将body中的数据传入root中
            //3. 将反序列化得到的Json::Value中的数据添加到table中
            for (int i = 0; i < root.size(); i++) {
                BackupInfo info;
                info.pack_flag = root[i]["pack_flag"].asBool();
                //Json::Value没有time_t类型，需要使用Json::Value支持的类型
                info.fsize = root[i]["fsize"].asInt64();
                info.atime = root[i]["atime"].asInt64();
                info.mtime = root[i]["mtime"].asInt64();
                info.pack_path = root[i]["pack_path"].asString();
                info.real_path = root[i]["real_path"].asString();
                info.url = root[i]["url"].asString();
                info.user_id = root[i]["user_id"].asString();
                Insert(info);//插入到map _table中
            }
            return true;
        }

        //从数据管理模块中删除文件信息
        bool Delete(const std::string &url) {
            pthread_rwlock_wrlock(&_rwlock);
            auto it = _table.find(url);
            if (it != _table.end()) {
                _table.erase(it);// 从unordered_map中删除对应的键值对
                pthread_rwlock_unlock(&_rwlock);
                Storage();// 更新文件信息
                return true;
            }
            pthread_rwlock_unlock(&_rwlock);
            return false;// 文件信息不存在
        }

        //根据用户ID获取文件信息
        std::unordered_map<std::string, BackupInfo> GetByUserId(const std::string &userId) {
            std::unordered_map<std::string, BackupInfo> result;
            pthread_rwlock_rdlock(&_rwlock);
            for (const auto &pair: _table) {
                //pair是BackupInfo
                if (pair.second.user_id == userId) {
                    result.insert(pair);
                }
            }
            pthread_rwlock_unlock(&_rwlock);
            return result;
        }

    private:
        std::string _manage_file;//数据存储持久化文件
        pthread_rwlock_t _rwlock;//读写锁
        std::unordered_map<std::string, BackupInfo> _table;
    };
}// namespace cloud