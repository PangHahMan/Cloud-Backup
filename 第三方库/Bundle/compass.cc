#include "bundle.h"
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    std::cout << "argv[1]是原始文件路径名称" << std::endl;
    std::cout << "argv[2]是压缩包名称" << std::endl;
    if (argc < 3) {
        return -1;
    }
    std::string ifilename = argv[1];//原始文件名称
    std::string ofilename = argv[2];//压缩包文件名称

    std::ifstream ifs;
    ifs.open(ifilename, std::ios::binary);//打开原始文件,二进制方式打开
    ifs.seekg(0, std::ios::end);          //跳转到文件末尾
    size_t fsize = ifs.tellg();           //tellg获取当前位置相对于起始位置的偏移量,也就是字符串长度
    ifs.seekg(0, std::ios::beg);          //回到起始位置

    std::string body;
    body.resize(fsize);       //调整body大小
    ifs.read(&body[0], fsize);//读取文件所有数据到body中,&body[0] 取首地址

    std::string packed = bundle::pack(bundle::LZIP, body);  //文件压缩成LZIP格式,从body读取

    std::ofstream ofs;
    ofs.open(ofilename, std::ios::binary);//打开压缩包文件
    ofs.write(&packed[0], packed.size()); //将压缩后的数据写入压缩包文件

    //关闭文件
    ifs.close();
    ofs.close();
    return 0;
}