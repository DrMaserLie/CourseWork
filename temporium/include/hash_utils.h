#ifndef HASH_UTILS_H
#define HASH_UTILS_H

#include <string>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

namespace Temporium {

class HashUtils {
public:
    // Хэширование пароля с использованием SHA-256
    static std::string hashPassword(const std::string& password, const std::string& salt = "") {
        std::string salted_password = salt + password + salt;
        return sha256(salted_password);
    }
    
    // Проверка пароля
    static bool verifyPassword(const std::string& password, const std::string& stored_hash, 
                               const std::string& salt = "") {
        std::string computed_hash = hashPassword(password, salt);
        return computed_hash == stored_hash;
    }
    
    // Хэширование строки SHA-256
    static std::string sha256(const std::string& data) {
        return sha256(data.c_str(), data.length());
    }
    
    // Хэширование бинарных данных SHA-256
    static std::string sha256(const char* data, size_t length) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        
        if (data != nullptr && length > 0) {
            SHA256_Update(&sha256, data, length);
        }
       
        
        SHA256_Final(hash, &sha256);
        
        return bytesToHex(hash, SHA256_DIGEST_LENGTH);
    }
    
    // Преобразование байтов в hex-строку
    static std::string bytesToHex(const unsigned char* data, size_t length) {
        std::stringstream ss;
        for (size_t i = 0; i < length; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
        }
        return ss.str();
    }
};

} 

#endif 
