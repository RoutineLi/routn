/*************************************************************************
	> File Name: Util.cpp
	> Author: lpj
	> Mail: lipeijie@vrvmail.com.cn
	> Created Time: 2022年10月25日 星期二 18时46分55秒
 ************************************************************************/

#include "Util.h"
#include "Log.h"
#include "Fiber.h"

#include <openssl/sha.h>

#include <dirent.h>
#include <signal.h>
#include <unistd.h>

namespace Routn
{
	Logger::ptr g_logger = ROUTN_LOG_NAME("system");
	pid_t GetThreadId()
	{
		return syscall(SYS_gettid);
	}
	uint32_t GetFiberId()
	{
		return Fiber::GetFiberId();
	}
	void Backtrace(std::vector<std::string> &bt, int size, int skip)
	{
		void **array = (void **)malloc((sizeof(void *) * size));
		size_t s = ::backtrace(array, size);

		char **strings = backtrace_symbols(array, s);
		if (strings == NULL)
		{
			ROUTN_LOG_ERROR(g_logger) << "backtrace_symbols error";
			return;
		}
		for (size_t i = skip; i < s; ++i)
		{
			bt.push_back(strings[i]);
		}
		free(strings);
		free(array);
	}
	std::string BacktraceToString(int size, int skip, const std::string &prefix)
	{
		std::vector<std::string> bt;
		Backtrace(bt, size, skip);
		std::stringstream ss;
		for (size_t i = 0; i < bt.size(); ++i)
		{
			ss << prefix << bt[i] << std::endl;
		}
		return ss.str();
	}

	//获取高精度时间-ms
	uint64_t GetCurrentMs(){
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
	}
	
	//获取高精度时间-us
	uint64_t GetCurrentUs(){
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;		
	}


	std::string TimerToString(time_t ts, const std::string& format){
		struct tm tm;
		localtime_r(&ts, &tm);
		char buff[64];
		strftime(buff, sizeof(buff), format.c_str(), &tm);
		return std::string(buff);
	}

	static int __lstat(const char* file, struct stat* st = nullptr) {
		struct stat lst;
		int ret = lstat(file, &lst);
		if(st) {
			*st = lst;
		}
		return ret;
	}

	static int __mkdir(const char* dirname) {
		if(access(dirname, F_OK) == 0) {
			return 0;
		}
		return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}


	/**
	 * 打开一个目录的所有文件并将其输入到容器中
	 * [in] files: 容器
	 * [in] path: 路径
	 * [in] subfix: 可以指定一个文件的后缀名
	 */
    void FSUtil::ListAllFile(std::vector<std::string>& files
								, const std::string& path
								, const std::string& subfix){
		if(access(path.c_str(), 0) != 0){
			return ;
		}
		DIR* dir = opendir(path.c_str());
		if(!dir) return;
		struct dirent* dp = nullptr;
		while((dp = readdir(dir)) != nullptr){
			if(dp->d_type == DT_DIR){
				if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")){
					continue;
				}
				ListAllFile(files, path + "/" + dp->d_name, subfix);
			}else if(dp->d_type == DT_REG){
				std::string filename(dp->d_name);
				if(subfix.empty()){
					files.push_back(path + "/" + filename);
				}else{
					if(filename.size() < subfix.size()){
						continue;
					}
					if(filename.substr(filename.length() - subfix.size()) == subfix){
						files.push_back(path + "/" + filename);
					}
				}
			}
		}
		closedir(dir);
	}

	/**
	 * 判断当前pid的进程是否正在运行
	 */
	bool FSUtil::IsRunningPidfile(const std::string& pidfile){
	if(__lstat(pidfile.c_str()) != 0) {
        return false;
    }
    std::ifstream ifs(pidfile);
    std::string line;
    if(!ifs || !std::getline(ifs, line)) {
        return false;
    }
    if(line.empty()) {
        return false;
    }
    pid_t pid = atoi(line.c_str());
    if(pid <= 1) {
        return false;
    }
    if(kill(pid, 0) != 0) {
        return false;
    }
    return true;
	}

	
	bool FSUtil::Mkdir(const std::string& dirname) {
    if(__lstat(dirname.c_str()) == 0) {
        return true;
    }
    char* path = strdup(dirname.c_str());
    char* ptr = strchr(path + 1, '/');
    do {
        for(; ptr; *ptr = '/', ptr = strchr(ptr + 1, '/')) {
            *ptr = '\0';
            if(__mkdir(path) != 0) {
                break;
            }
        }
        if(ptr != nullptr) {
            break;
        } else if(__mkdir(path) != 0) {
            break;
        }
        free(path);
        return true;
    } while(0);
    free(path);
    return false;
}



	/**
	 * @brief Base64编码
	 * 
	 * @param data 
	 * @param url 
	 * @return std::string 
	 */	
	std::string base64encode(const void* data, size_t len, bool url) {
		const char* base64 = url ?
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"
			: "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

		std::string ret;
		ret.reserve(len * 4 / 3 + 2);

		const unsigned char* ptr = (const unsigned char*)data;
		const unsigned char* end = ptr + len;

		while(ptr < end) {
			unsigned int packed = 0;
			int i = 0;
			int padding = 0;
			for(; i < 3 && ptr < end; ++i, ++ptr) {
				packed = (packed << 8) | *ptr;
			}
			if(i == 2) {
				padding = 1;
			} else if (i == 1) {
				padding = 2;
			}
			for(; i < 3; ++i) {
				packed <<= 8;
			}

			ret.append(1, base64[packed >> 18]);
			ret.append(1, base64[(packed >> 12) & 0x3f]);
			if(padding != 2) {
				ret.append(1, base64[(packed >> 6) & 0x3f]);
			}
			if(padding == 0) {
				ret.append(1, base64[packed & 0x3f]);
			}
			ret.append(padding, '=');
		}

		return ret;
	}

	std::string base64encode(const std::string& data, bool url) {
		return base64encode(data.c_str(), data.size(), url);
	}

	/**
	 * @brief sha1sum算法加密
	 * 
	 * @param data 
	 * @param len 
	 * @return std::string 
	 */
	std::string sha1sum(const void *data, size_t len) {
		SHA_CTX ctx;
		SHA1_Init(&ctx);
		SHA1_Update(&ctx, data, len);
		std::string result;
		result.resize(SHA_DIGEST_LENGTH);
		SHA1_Final((unsigned char*)&result[0], &ctx);
		return result;
	}

	std::string sha1sum(const std::string &data) {
		return sha1sum(data.c_str(), data.size());
	}

	std::string random_string(size_t len, const std::string& chars){
		if(len == 0 || chars.empty()) {
			return "";
		}
		std::string rt;
		rt.resize(len);
		int count = chars.size();
		for(size_t i = 0; i < len; ++i) {
			rt[i] = chars[rand() % count];
		}
		return rt;	
	}
};
