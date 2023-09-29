#include "bundle.h"
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cout << "argc[1]是压缩包名称" << std::endl;
        std::cout << "argc[2]是解压后的文件名称" << std::endl;
        return -1;
    }

    std::string ifilename = argv[1];   //压缩包名称
    std::string ofilename = argv[2];   //解压后的文件名称

    std::ifstream ifs;
    ifs.open(ifilename, std::ios::binary);   //二进制打开压缩文件
    ifs.seekg(0, std::ios::end); 
    size_t fsize = ifs.tellg();      //获取压缩文件大小
    ifs.seekg(0, std::ios::beg);

    std::string body;              //用于解压文件保存的string
    body.resize(fsize); 
    ifs.read(&body[0], fsize);     //将文件中fsize中的数据写入到body中
    ifs.close(); 

    std::string unpacked = bundle::unpack(body);   //对body解压缩到unpack
    std::ofstream ofs;
    ofs.open(ofilename, std::ios::binary);
    ofs.write(&unpacked[0], unpacked.size());  //将unpack里面的数据写入到ofilename文件中
    ofs.close();

    return 0;
}