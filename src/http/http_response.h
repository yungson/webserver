#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <fcntl.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <sys/mman.h> // map, unmap
#include "../buffer/buffer.h"
#include <fcntl.h> 
#include <unistd.h> // close
#include <assert.h>
#include "../log/log.h"
class HttpResponse{

public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string& src_root, std::string& path, bool is_keep_alive = false, int code = -1);
    void MakeResponse(Buffer& buff);
    void UnMapFile();
    char* MappedFile();
    size_t FileLen() const;
    void ErrorContent(Buffer& buff, std::string message);
    int Code() const {return code_;}

private:
    void AddStatusLine_(Buffer& buff);
    void AddHeader_(Buffer& buff);
    void AddContent_(Buffer& buff);
    void ErrorHtml_();

    std::string GetFileType_();

    int code_;
    bool is_keep_alive_;
    std::string path_;
    std::string src_root_;
    char* mm_file_addr_;
    struct stat mm_file_stat_;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPES;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH; 

};

#endif // HTTP_RESPONSE_H