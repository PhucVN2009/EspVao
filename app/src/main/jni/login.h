#pragma once

#include <curl/curl.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

using json = nlohmann::json;
std::string g_Token, g_Auth;
bool bValid = false;
struct MemoryStruct {
	char *memory;
	size_t size;
};

inline const char* GetAndroidID() {
    static char prop_value[PROP_VALUE_MAX];
    __system_property_get("ro.build.id", prop_value);
    return prop_value;
}

inline const char* GetDeviceModel() {
    static char prop_value[PROP_VALUE_MAX];
    __system_property_get("ro.product.model", prop_value);
    return prop_value;
}

inline const char* GetDeviceBrand() {
    static char prop_value[PROP_VALUE_MAX];
    __system_property_get("ro.product.model", prop_value);
    return prop_value;
}

inline const char* GetDeviceUniqueIdentifier(const std::string& uuid) {
    static std::string formattedUuid;
    std::stringstream ss;
    for (char c : uuid) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    formattedUuid = ss.str();
    return formattedUuid.c_str();
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *) userp;
	
	mem->memory = (char *) realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		return 0;
	}
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;
	return realsize;
}

std::string Login(const char *user_key) {
	JNIEnv *env;
    jvm->AttachCurrentThread(&env, 0);
	
	auto looperClass = env->FindClass(oxorany("android/os/Looper"));
	auto prepareMethod = env->GetStaticMethodID(looperClass, oxorany("prepare"), oxorany("()V"));
	env->CallStaticVoidMethod(looperClass, prepareMethod);
	
	jclass activityThreadClass = env->FindClass(oxorany("android/app/ActivityThread"));
	jfieldID sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, oxorany("sCurrentActivityThread"), oxorany("Landroid/app/ActivityThread;"));
	jobject sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass, sCurrentActivityThreadField);
	
	jfieldID mInitialApplicationField = env->GetFieldID(activityThreadClass, oxorany("mInitialApplication"), oxorany("Landroid/app/Application;"));
	jobject mInitialApplication = env->GetObjectField(sCurrentActivityThread, mInitialApplicationField);
	
	std::string hwid = user_key;
	hwid += GetAndroidID();
	hwid += GetDeviceModel();
	hwid += GetDeviceBrand();
	std::string UUID = GetDeviceUniqueIdentifier(hwid.c_str());
//	jvm->DetachCurrentThread();
	std::string errMsg;
	
	struct MemoryStruct chunk{};
	chunk.memory = (char *) malloc(1);
	chunk.size = 0;
	
	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	     
	if (curl) {
        std::string api_key = oxorany("https://baoios.com/public/auth-login-java.php");
        const char *version = "3";
		const char *package_game = "com.muchmod.kgvn";

		curl_easy_setopt(curl, CURLOPT_URL, (api_key.c_str()));
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
		
		struct curl_slist *headers = NULL;
		headers = curl_slist_append(headers, oxorany("Content-Type: application/x-www-form-urlencoded"));
		
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		char data[4096];
		sprintf(data, oxorany("auth=%s&phienban=%s&device=%s&package=%s"), user_key, version, UUID.c_str(), package_game);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		
		res = curl_easy_perform(curl);
		if (res == CURLE_OK) {
			try {
				json result = json::parse(chunk.memory);
				std::string kq = result[std::string(oxorany("ketqua"))].get<std::string>();
				if (!kq.compare("thanhcong")) {
					EXP = result[std::string(oxorany("hansudung"))].get<std::string>();

					g_Token = result[std::string(oxorany("auth"))].get<std::string>();
                    g_Auth = result[std::string(oxorany("auth"))].get<std::string>();
					bValid = true;
				} else {
					errMsg = result[std::string(oxorany("mes"))].get<std::string>();
				}
			} catch (json::exception &e) {
				errMsg = "{";
				errMsg += e.what();
				errMsg += "}\n{";
				errMsg += chunk.memory;
				errMsg += "}";
			}
		} else {
			errMsg = curl_easy_strerror(res);
		}
	}
	curl_easy_cleanup(curl);
//	jvm->DetachCurrentThread();
	return bValid ? "OK" : errMsg;
}


void saveKey()
{
    nlohmann::json configJson;
    configJson["Key"] = s;

    std::ofstream file("/storage/emulated/0/Android/data/lqmhax.online/files/key.txt");
    if (!file.is_open()) {
        return;
    }

    file << std::setw(4) << configJson << std::endl;
    file.close();
}

void loadKey() {
    std::ifstream file("/storage/emulated/0/Android/data/lqmhax.online/files/key.txt");
    if (!file.is_open()) {
        return;
    }

    nlohmann::json configJson;
    file >> configJson;
    file.close();

    std::string savedKey = configJson.value("Key", "");
    strncpy(s, savedKey.c_str(), sizeof(s));
}
