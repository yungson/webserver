#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include "../buffer/buffer.h"
#include <regex>
#include "../pool/sqlconnection_raii.h"

using namespace std;
class HttpRequest{

public:
    enum PARSE_STATE{REQUEST_LINE, HEADERS, BODY, FINISH};
    enum HTTP_CODE {NO_REQUEST= 0, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION};
    HttpRequest() {Init();}
    ~HttpRequest() = default; 

    void Init();
    bool parse(Buffer& buff);
    string Path() const;
    string& Path();
    string Method() const;
    string Version() const; 
    string GetPost(const string& key) const;
    string GetPost(const char* key) const;

    bool IsKeepAlive() const;

private:

    bool ParseRequestLine_(const string& line);
    void ParseHeader_(const string& line);
    void ParseBody_(const string& line);
    void ParsePath_();
    void ParsePost_();
    void ParseFromEncodedUrl_();
    static bool UserVerify(const string& name, const string& passwd, bool is_login);
    PARSE_STATE state_;
    string method_, path_, version_, body_;
    unordered_map<string, string> header_;
    unordered_map<string, string> post_; // post_: request form data
    static const unordered_set<string> default_html;
    static const unordered_map<string, int> default_html_tag;
    static int ConvertToHex(char ch);
};

#endif