#include "config.hpp"
#include "datamanager.hpp"
#include "hot.hpp"
#include "server.hpp"
#include "util.hpp"
#include <thread>
void FileUtilTest(const std::string &filename) {
    cloud::FileUtil fu(filename);
    std::cout << fu.FileSize() << std::endl;
    std::cout << "Modify:" << fu.LastMtime() << std::endl;
    std::cout << "Access:" << fu.LastAtime() << std::endl;
    std::cout << fu.FileName() << std::endl;
}

void FileUtilTest2(const std::string &filename) {
    cloud::FileUtil fu(filename);
    std::string body;
    fu.GetContent(&body);

    cloud::FileUtil nfu("./hello.txt");
    nfu.SetContent(body);
}

void FileUtilTest3(const std::string &filename) {
    std::string packname = filename + ".lz";//packname是压缩后的名称
    cloud::FileUtil fu(filename);
    fu.Compress(packname);//对filename进行压缩，会看compress函数中重新构建文件packname
    cloud::FileUtil pfu(packname);
    pfu.Uncompress("hello.txt");//将packname进行解压缩，文件名为hello.txt
}

void FileUtilTest4(const std::string &filename) {
    cloud::FileUtil fu(filename);
    fu.CreateDirectory();//不存在则创建目录
    std::vector<std::string> array;
    fu.ScanDirectory(&array);//扫描目录文件到array向量中
    for (auto &a: array) {
        //打印向量中的文件名
        std::cout << a << std::endl;
    }
}

void JsonUtilTest() {
    const char *name = "小明";
    int age = 19;
    float score[] = {85, 88.5, 99};
    Json::Value root;
    root["姓名"] = name;
    root["年龄"] = age;
    root["成绩"].append(score[0]);
    root["成绩"].append(score[1]);
    root["成绩"].append(score[2]);
    std::string json_str;
    cloud::JsonUtil::Serialize(root, &json_str);
    std::cout << json_str << std::endl;

    Json::Value val;
    cloud::JsonUtil::UnSerialize(json_str, val);

    std::cout << val["姓名"].asString() << std::endl;
    std::cout << val["年龄"].asInt() << std::endl;
    for (int i = 0; i < val["成绩"].size(); i++) {
        std::cout << val["成绩"][i].asFloat() << " ";
    }
    std::cout << std::endl;
}

void ConfigTest() {
    cloud::Config *config = cloud::Config::GetInstance();
    std::cout << config->GetHotTime() << std::endl;
    std::cout << config->GetServerIp() << std::endl;
    std::cout << config->GetServerPort() << std::endl;
    std::cout << config->GetUrlPrefix() << std::endl;
    std::cout << config->GetArc_Suffix() << std::endl;
    std::cout << config->GetPackDir() << std::endl;
    std::cout << config->GetBackDir() << std::endl;
    std::cout << config->GetManagerFile() << std::endl;
}

void DataTest(const std::string &filename) {
    cloud::BackupInfo info;
    //info.NewBackupInfo(filename);
    std::cout << info.pack_flag << std::endl;
    std::cout << info.fsize << std::endl;
    std::cout << info.mtime << std::endl;
    std::cout << info.atime << std::endl;
    std::cout << info.real_path << std::endl;
    std::cout << info.pack_path << std::endl;
    std::cout << info.url << std::endl;
}

void DataTest2(const std::string &filename) {
    cloud::BackupInfo info;
    //info.NewBackupInfo(filename);//填充filename文件的文件信息

    cloud::DataManager data;
    std::cout << "-----------insert and GetOneByURL--------\n";
    data.Insert(info);//插入文件信息到map中,并构建了<url,BackupInfo>的映射

    cloud::BackupInfo tmp;
    data.GetOneByURL("/download/bundle.h", &tmp);//通过URL获取文件信息,传入到tmp中

    std::cout << tmp.pack_flag << std::endl;
    std::cout << tmp.fsize << std::endl;
    std::cout << tmp.mtime << std::endl;
    std::cout << tmp.atime << std::endl;
    std::cout << tmp.real_path << std::endl;
    std::cout << tmp.pack_path << std::endl;
    std::cout << tmp.url << std::endl;

    std::cout << "-----------update and getall--------\n";
    info.pack_flag = true;
    data.Update(info);
    std::vector<cloud::BackupInfo> arry;
    data.GetAll(&arry);
    for (auto &a: arry) {
        std::cout << a.pack_flag << std::endl;
        std::cout << a.fsize << std::endl;
        std::cout << a.mtime << std::endl;
        std::cout << a.atime << std::endl;
        std::cout << a.real_path << std::endl;
        std::cout << a.pack_path << std::endl;
        std::cout << a.url << std::endl;
    }
    std::cout << "-----------GetOneByRealPath--------\n";

    data.GetOneByRealPath(filename, &tmp);
    std::cout << tmp.pack_flag << std::endl;
    std::cout << tmp.fsize << std::endl;
    std::cout << tmp.mtime << std::endl;
    std::cout << tmp.atime << std::endl;
    std::cout << tmp.real_path << std::endl;
    std::cout << tmp.pack_path << std::endl;
    std::cout << tmp.url << std::endl;
}

void DataTest3() {
    cloud::DataManager data;//构造函数会进行初始化Init函数会将back.dat中的数据存放在map中
    std::vector<cloud::BackupInfo> array;
    data.GetAll(&array);//将data类中的文件信息全部读到array中

    for (auto &a: array) {
        std::cout << a.pack_flag << std::endl;
        std::cout << a.fsize << std::endl;
        std::cout << a.mtime << std::endl;
        std::cout << a.atime << std::endl;
        std::cout << a.real_path << std::endl;
        std::cout << a.pack_path << std::endl;
        std::cout << a.url << std::endl;
    }
}

cloud::DataManager *_data = new cloud::DataManager();//全局变量 hot文件下 extern引入
void HotTest() {
    cloud::HotManager hot;
    hot.RunModule();
}

void ServerTest() {
    cloud::Server srv;
    srv.RunModule();
}

int main(int argc, char *argv[]) {
    //创建多线程并发运行
    std::thread thread_hot_manager(HotTest);
    std::thread thread_service(ServerTest);
    thread_hot_manager.join();
    thread_service.join();
    return 0;
}