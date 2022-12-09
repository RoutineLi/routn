#include <iostream>
#include <fstream>

int main(){
    std::ifstream fs;
    std::ofstream ofs;
    fs.open("/home/rotun-li/routn/bin/love.html");
    std::string buff;
    if(fs.is_open()){
        std::string tmp;
        while(getline(fs, tmp)){
            buff += tmp;
            buff += "\n";
        }
    }
    ofs.open("/home/rotun-li/routn/bin/love2.html", std::ios::app | std::ios::out);
    ofs << buff;
    ofs.close();
	fs.close();
		return 0;
}