/*
一、序列化
1.将所有的数据保存在Json::Value对象中
2.使用Json::StreamWriter
*/

#include <iostream>
#include <jsoncpp/json/json.h>
#include <memory>
#include <sstream>
#include <string>

//序列化
int test1() {
    const char *name = "小明";
    int age = 18;
    float score[] = {77.5, 88, 93.6};
    Json::Value root;
    root["姓名"] = name;
    root["年龄"] = age;
    root["成绩"].append(score[0]);
    root["成绩"].append(score[1]);
    root["成绩"].append(score[2]);

    //swb.newStreamWriter返回类型是StreamWriter*
    Json::StreamWriterBuilder swb;
    std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
    //用ss来接收序列化的数据
    std::stringstream ss;
    sw->write(root, &ss);//将root序列化的数据写入到ss string流中
    std::cout << ss.str() << std::endl;
}

//反序列化
int test2() {
    std::string str = R"({"姓名":"小黑","年龄":19,"成绩":[58.5,56,59]})";
    //C++11引入的Raw String Literal（原始字符串字面量）语法,R"(" 开头，")" 结尾的部分定义了一个原始字符串字面量。
    //这意味着在这个字符串中，你不需要对引号、反斜杠等进行额外的转义，直接按照你在字符串中看到的方式写就可以了。
    Json::Value root;

    //newCharReader的返回值为CharReader
    Json::CharReaderBuilder crb;
    std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
    std::string err;//保存错误信息
    //virtual bool Json::CharReader::parse(const char *beginDoc, const char *endDoc, Json::Value *root, std::string *errs)
    /*  
    beginDoc：指向 JSON 字符串的起始位置的指针。
    endDoc：指向 JSON 字符串的结束位置的指针。
    root：一个指向 Json::Value 对象的指针，用于存储解析后的 JSON 数据。
    errs：一个可选的指向 std::string 的指针，用于存储解析过程中的错误信息。 
    */
    //parse函数将str中的信息解析到Json变量root中
    bool ret = cr->parse(str.c_str(), str.c_str() + str.size(), &root, &err);
    if (ret == false) {
        std::cout << "parse error:" << err << std::endl;
        return -1;
    }

    //反序列化asString转换成C++字符串
    std::cout << root["姓名"].asString() << std::endl;
    std::cout << root["年龄"].asString() << std::endl;
    int sz = root["成绩"].size();//获取成绩的个数
    for (int i = 0; i < sz; i++) {
        std::cout << root["成绩"][i] << std::endl;
    }
}

int main() {
    int i = test2();
    return 0;
}