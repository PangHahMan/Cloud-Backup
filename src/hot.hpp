#pragma once
#include "datamanager.hpp"

extern cloud::DataManager *_data;//全局对象 不可以放在命名空间里面
namespace cloud {
    class HotManager {
    private:
        std::string _back_dir;  //备份文件存放目录
        std::string _pack_dir;  //压缩包存放目录
        std::string _arc_suffix;//压缩包后缀名
        int _hot_time;          //热点时间

        //非热点文件-返回真；热点文件-返回假
        bool HotJudge(const std::string &filename) {
            FileUtil fu(filename);
            time_t last_atime = fu.LastAtime();
            time_t cur_time = time(nullptr);
            //如果当前时间-文件最后访问时间 > 热点事件,说明他是一个非热点文件
            if (cur_time - last_atime > _hot_time) {
                return true;
            }
            return false;
        }

    public:
        HotManager() {
            //获取config.conf里的信息,先构建Config对象
            Config *config = Config::GetInstance();
            _back_dir = config->GetBackDir();
            _pack_dir = config->GetPackDir();
            _arc_suffix = config->GetArc_Suffix();
            _hot_time = config->GetHotTime();
            //创建_back_dir和_pack_dir的目录
            FileUtil tmp1(_back_dir);
            FileUtil tmp2(_pack_dir);
            tmp1.CreateDirectory();
            tmp2.CreateDirectory();
        }

        //对指定目录下的文件进行备份和处理操作
        bool RunModule() {
            while (1) {
                //1. 遍历备份目录，获取所有文件名
                FileUtil fu(_back_dir);
                std::vector<std::string> arry;
                fu.ScanDirectory(&arry);//扫描文件信息(带路径文件名)存放在array数组中
                //2. 遍历判断文件是否是非热点文件
                for (auto &a: arry) {
                    if (HotJudge(a) == false) {
                        continue;//热点文件则不需要特别处理
                    }
                    //3. 获取文件的备份信息
                    BackupInfo bi;
                    //根据文件路径获取文件信息
                    if (_data->GetOneByRealPath(a, &bi) == false) {
                        //现在有一个文件存在，但是没有备份信息
                        bi.NewBackupInfo(a, bi.user_id);//设置一个新的备份信息出来
                    }
                    //3. 对非热点文件进行压缩处理
                    FileUtil tmp(a);
                    tmp.Compress(bi.pack_path);//压缩路径设置为pack_path
                    //4. 删除源文件，修改备份信息
                    _data->Delete(bi.url);
                    tmp.Remove();

                    bi.pack_flag = true;
                    _data->Update(bi);//建立url 键值对
                }
                usleep(1000);//避免空目录循环遍历，消耗cpu资源过高
            }
            return true;
        }
    };
}// namespace cloud