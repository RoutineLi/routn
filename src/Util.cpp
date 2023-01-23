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
#include <google/protobuf/unknown_field_set.h>

#include <dirent.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
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
	std::string ToUpper(const std::string& name){
		std::string rt = name;
		std::transform(rt.begin(), rt.end(), rt.begin(), ::toupper);
		return rt;
	}

	std::string ToLower(const std::string& name){
		std::string rt = name;
		std::transform(rt.begin(), rt.end(), rt.begin(), ::tolower);
		return rt;
	}


	std::string TimerToString(time_t ts, const std::string& format){
		struct tm tm;
		localtime_r(&ts, &tm);
		char buff[64];
		strftime(buff, sizeof(buff), format.c_str(), &tm);
		return std::string(buff);
	}

	time_t StringToTimer(const char* str, const char* format){
		struct tm t;
		memset(&t, 0, sizeof(t));
		if(!strptime(str, format, &t)){
			return 0;
		}
		return mktime(&t);
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


	static const char uri_chars[256] = {
		/* 0 */
		0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 1, 1, 0,
		1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 0, 0, 0, 1, 0, 0,
		/* 64 */
		0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 0, 1,
		0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 1, 0,
		/* 128 */
		0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
		/* 192 */
		0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	};

	static const char xdigit_chars[256] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,
		0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	};

#define CHAR_IS_UNRESERVED(c)           \
    (uri_chars[(unsigned char)(c)])

	//-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~
	std::string UrlEncode(const std::string& str, bool space_as_plus) {
		static const char *hexdigits = "0123456789ABCDEF";
		std::string* ss = nullptr;
		const char* end = str.c_str() + str.length();
		for(const char* c = str.c_str() ; c < end; ++c) {
			if(!CHAR_IS_UNRESERVED(*c)) {
				if(!ss) {
					ss = new std::string;
					ss->reserve(str.size() * 1.2);
					ss->append(str.c_str(), c - str.c_str());
				}
				if(*c == ' ' && space_as_plus) {
					ss->append(1, '+');
				} else {
					ss->append(1, '%');
					ss->append(1, hexdigits[(uint8_t)*c >> 4]);
					ss->append(1, hexdigits[*c & 0xf]);
				}
			} else if(ss) {
				ss->append(1, *c);
			}
		}
		if(!ss) {
			return str;
		} else {
			std::string rt = *ss;
			delete ss;
			return rt;
		}
	}

	std::string UrlDecode(const std::string& str, bool space_as_plus) {
		std::string* ss = nullptr;
		const char* end = str.c_str() + str.length();
		for(const char* c = str.c_str(); c < end; ++c) {
			if(*c == '+' && space_as_plus) {
				if(!ss) {
					ss = new std::string;
					ss->append(str.c_str(), c - str.c_str());
				}
				ss->append(1, ' ');
			} else if(*c == '%' && (c + 2) < end
						&& isxdigit(*(c + 1)) && isxdigit(*(c + 2))){
				if(!ss) {
					ss = new std::string;
					ss->append(str.c_str(), c - str.c_str());
				}
				ss->append(1, (char)(xdigit_chars[(int)*(c + 1)] << 4 | xdigit_chars[(int)*(c + 2)]));
				c += 2;
			} else if(ss) {
				ss->append(1, *c);
			}
		}
		if(!ss) {
			return str;
		} else {
			std::string rt = *ss;
			delete ss;
			return rt;
		}
	}


	std::string Trim(const std::string& str, const std::string& delimit) {
    auto begin = str.find_first_not_of(delimit);
    if(begin == std::string::npos) {
        return "";
    }
    auto end = str.find_last_not_of(delimit);
    return str.substr(begin, end - begin + 1);
}

	std::string TrimLeft(const std::string& str, const std::string& delimit) {
		auto begin = str.find_first_not_of(delimit);
		if(begin == std::string::npos) {
			return "";
		}
		return str.substr(begin);
	}

	std::string TrimRight(const std::string& str, const std::string& delimit) {
		auto end = str.find_last_not_of(delimit);
		if(end == std::string::npos) {
			return "";
		};
		return str.substr(0, end);
	}


	static void serialize_unknownfieldset(const google::protobuf::UnknownFieldSet& ufs, json::value& jnode){
		std::map<int, std::vector<json::value> > kvs;
		for(int i = 0; i < ufs.field_count(); ++i){
			const auto& uf = ufs.field(i);
			switch((int)uf.type()){
				case google::protobuf::UnknownField::TYPE_VARINT:
					kvs[uf.number()].push_back((long long)uf.varint());
					break;
				case google::protobuf::UnknownField::TYPE_FIXED32:
					kvs[uf.number()].push_back((uint32_t)uf.fixed32());
					break;
				case google::protobuf::UnknownField::TYPE_FIXED64:
					kvs[uf.number()].push_back((uint64_t)uf.fixed64());
					break;
				case google::protobuf::UnknownField::TYPE_LENGTH_DELIMITED:
					google::protobuf::UnknownFieldSet tmp;
					auto& v = uf.length_delimited();
					if(!v.empty() && tmp.ParseFromString(v)){
						json::value vv;
						serialize_unknownfieldset(tmp, vv);
						kvs[uf.number()].push_back(vv);
					}else{
						kvs[uf.number()].push_back(v);
					}
					break;
			}
		}

		for(auto& i : kvs){
			if(i.second.size() > 1){
				for(auto& n : i.second){
					jnode[std::to_string(i.first)].push_back(n);
				}
			}else{
				jnode[std::to_string(i.first)] = i.second[0];
			}
		}
	}

	static void serialize_message(const google::protobuf::Message& message, json::value& jnode){
		const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
		const google::protobuf::Reflection* reflection = message.GetReflection();

		for(int i = 0; i < descriptor->field_count(); ++i){
			const google::protobuf::FieldDescriptor* field = descriptor->field(i);

			if(field->is_repeated()){
				if(!reflection->FieldSize(message, field)){
					continue;
				}
			}else{
				if(!reflection->HasField(message, field)){
					continue;
				}
			}

			if(field->is_repeated()){
				switch(field->cpp_type()){
	#define XX(cpptype, method, valuetype, jsontype)	\
					case google::protobuf::FieldDescriptor::CPPTYPE_##cpptype: {	\
						int size = reflection->FieldSize(message, field);	\
						for(int n = 0; n < size; ++n){	\
							jnode[field->name()].push_back(	\
								(jsontype)reflection->GetRepeated##method(	\
								message, field, n));	\
						}	\
						break;	\
					}
				XX(INT32, Int32, int32_t, int);
				XX(UINT32, UInt32, uint32_t, uint32_t);
				XX(FLOAT, Float, float, float);
				XX(DOUBLE, Double, double, double);
				XX(BOOL, Bool, bool, bool);
				XX(INT64, Int64, int64_t, long long);
				XX(UINT64, UInt64, uint64_t, unsigned long long);
	#undef XX
					case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:{
						int size = reflection->FieldSize(message, field);
						for(int n = 0; n < size; ++n){
							jnode[field->name()].push_back(reflection->GetRepeatedEnum(message, field, n)->number());
						}
						break;
					}
					case google::protobuf::FieldDescriptor::CPPTYPE_STRING:{
						int size = reflection->FieldSize(message, field);
						for(int n = 0; n < size; ++n){
							jnode[field->name()].push_back(reflection->GetRepeatedString(message, field, n));
						}
						break;
					}
					case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:{
						int size = reflection->FieldSize(message, field);
						for(int n = 0; n < size; ++n){
							json::value vv;
							serialize_message(reflection->GetRepeatedMessage(message, field, n), vv);
							jnode[field->name()].push_back(vv);
						}
						break;
					}
				}
				continue;
			}

			switch(field->cpp_type()){
	#define XX(cpptype, method, valuetype, jsontype)\
				case google::protobuf::FieldDescriptor::CPPTYPE_##cpptype:{	\
					jnode[field->name()] = (jsontype)reflection->Get##method(message, field); \
					break; \
				}
				XX(INT32, Int32, int32_t, int32_t);
				XX(UINT32, UInt32, uint32_t, uint32_t);
				XX(FLOAT, Float, float, double);
				XX(DOUBLE, Double, double, double);
				XX(BOOL, Bool, bool, bool);
				XX(INT64, Int64, int64_t, int64_t);
				XX(UINT64, UInt64, uint64_t, uint64_t);
	#undef XX
				case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:{
					jnode[field->name()] = reflection->GetEnum(message, field)->number();
					break;
				}
				case google::protobuf::FieldDescriptor::CPPTYPE_STRING:{
					jnode[field->name()] = reflection->GetString(message, field);
					break;
				}
				case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:{
					serialize_message(reflection->GetMessage(message, field), jnode[field->name()]);
				}
			}
		}

		const auto& ufs = reflection->GetUnknownFields(message);
		serialize_unknownfieldset(ufs, jnode);
	}

	std::string PBToJsonString(const google::protobuf::Message& message){
		json::value jnode;
		serialize_message(message, jnode);
		return Routn::JsonUtil::ToString(jnode);
	}


	std::vector<std::string> split(const std::string &str, char delim, size_t max) {
    	std::vector<std::string> result;
		if (str.empty()) {
			return result;
		}

		size_t last = 0;
		size_t pos = str.find(delim);
		while (pos != std::string::npos) {
			result.push_back(str.substr(last, pos - last));
			last = pos + 1;
			if (--max == 1)
				break;
			pos = str.find(delim, last);
		}
		result.push_back(str.substr(last));
		return result;
	}

	std::vector<std::string> split(const std::string &str, const char *delims, size_t max) {
		std::vector<std::string> result;
		if (str.empty()) {
			return result;
		}

		size_t last = 0;
		size_t pos = str.find_first_of(delims);
		while (pos != std::string::npos) {
			result.push_back(str.substr(last, pos - last));
			last = pos + 1;
			if (--max == 1)
				break;
			pos = str.find_first_of(delims, last);
		}
		result.push_back(str.substr(last));
		return result;
	}

	in_addr_t GetIPv4Inet() {
		struct ifaddrs* ifas = nullptr;
		struct ifaddrs* ifa = nullptr;

		in_addr_t localhost = inet_addr("127.0.0.1");
		if(getifaddrs(&ifas)) {
			ROUTN_LOG_ERROR(g_logger) << "getifaddrs errno=" << errno
				<< " errstr=" << strerror(errno);
			return localhost;
		}

		in_addr_t ipv4 = localhost;

		for(ifa = ifas; ifa && ifa->ifa_addr; ifa = ifa->ifa_next) {
			if(ifa->ifa_addr->sa_family != AF_INET) {
				continue;
			}
			if(!strncasecmp(ifa->ifa_name, "lo", 2)) {
				continue;
			}
			ipv4 = ((struct sockaddr_in*)ifa->ifa_addr)->sin_addr.s_addr;
			if(ipv4 == localhost) {
				continue;
			}
		}
		if(ifas != nullptr) {
			freeifaddrs(ifas);
		}
		return ipv4;
	}

	std::string _GetIPv4() {
		char ipv4[INET_ADDRSTRLEN] = {0};
		memset(ipv4, 0, INET_ADDRSTRLEN);
		auto ia = GetIPv4Inet();
		inet_ntop(AF_INET, &ia, ipv4, INET_ADDRSTRLEN);
		return ipv4;
	}

	std::string GetIPv4() {
		static const std::string ip = _GetIPv4();
		return ip;
	}

	std::string GetHostName() {
		char host[512] = {0};
		gethostname(host, 511);
    	return host;
	}


	std::string replace(const std::string& str1, char find, char replaceWith){
		auto str = str1;
		size_t index = str.find(find);
		while(index != std::string::npos){
			str[index] = replaceWith;
			index = str.find(find, index + 1);
		}
		return str;
	}

	
	std::string replace(const std::string& str1, char find, const std::string& replaceWith){
		auto str = str1;
		size_t index = str.find(find);
		while(index != std::string::npos){
			str = str.substr(0, index) + replaceWith + str.substr(index + 1);
			index = str.find(find, index + replaceWith.size());
		}
		return str;
	}

	std::string replace(const std::string& str1, const std::string& find, const std::string& replaceWith){
		auto str = str1;
		size_t index = str.find(find);
		while(index != std::string::npos){
			str = str.substr(0, index) + replaceWith + str.substr(index + find.size());
			index = str.find(find, index + replaceWith.size());
		}
		return str; 
	}

}	
