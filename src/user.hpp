#include <fstream>
#include <sstream>
#include <unordered_map>
namespace cloud {
    class UserManager {
    private:
        std::unordered_map<std::string, std::string> userMap;
        std::string filename = "users.csv";

    public:
        UserManager() {
            // 从文件中加载用户信息
            std::ifstream file(filename);
            if (!file) {
                throw std::runtime_error("Unable to open user file");
            }

            std::string line;
            while (std::getline(file, line)) {
                std::istringstream ss(line);
                std::string username, password;
                std::getline(ss, username, ',');
                std::getline(ss, password, ',');
                userMap[username] = password;
            }
        }

        bool addUser(const std::string &username, const std::string &password) {
            std::cout << "添加用户" << std::endl;

            // 检查用户是否已存在
            if (userMap.find(username) != userMap.end()) {
                std::cout << "用户已存在" << std::endl;
                return false;
            }

            userMap[username] = password;
            std::cout << "添加文件" << std::endl;
            // 将新用户添加到文件
            std::ofstream file(filename, std::ios_base::app);
            if (!file) {
                std::cout << "写入文件失败" << std::endl;
            }
            file << username << "," << password << "\n";
            return true;
        }


        bool verifyUser(const std::string &username, const std::string &password) {
            return userMap.count(username) > 0 && userMap[username] == password;
        }
    };
}// namespace cloud