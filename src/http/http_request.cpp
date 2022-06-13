#include "http_request.h"
#include "../log/log.h"

using namespace std;

const unordered_set<string> HttpRequest::default_html{
    "/index", "/register","/login","/welcome","/video", "/picture"
};

const unordered_map<string, int> HttpRequest::default_html_tag{
    {"/register.html", 0}, {"/login.html", 1}
};

void HttpRequest::Init(){
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpRequest::IsKeepAlive() const {
    if(header_.count("Connection") ==1){
        return header_.find("Connection")->second =="keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::parse(Buffer& buff){
    const char CRLF[] = "\r\n";
    if(buff.ReadableBytes()<=0){ 
        return false;
    }
    while(buff.ReadableBytes() && state_!=FINISH){
        const char* line_end = search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF+2);
        string line(buff.Peek(), line_end);
        switch(state_){
            case REQUEST_LINE:
                if(!ParseRequestLine_(line)){
                    return false;
                }
                ParsePath_();
                break;
            case HEADERS:
                ParseHeader_(line);
                if(buff.ReadableBytes()<=2){
                    state_ = FINISH;
                }
                break;
            case BODY:
                ParseBody_(line);
                break;
            default:
                break;
        }
        
        if(line_end == buff.BeginWrite()){break;}
        buff.RetrieveUntil(line_end+2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

void HttpRequest::ParsePath_(){
    if(path_ == "/"){
        path_ = "/index.html";
    }else{
        for(auto &item: default_html){
            if(item == path_){
                path_ += ".html";
                break;
            }
        }
    }
}

bool HttpRequest::ParseRequestLine_(const string& line){
    // GET /index.html HTTP/1.1
    regex pattern ("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch match_result;
    if(regex_match(line, match_result, pattern)){
        method_ = match_result[1];
        path_ = match_result[2];
        version_ = match_result[3];
        state_ = HEADERS;
        return true; 
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::ParseHeader_(const string& line){
    regex pattern("^([^:]*): ?(.*)$");
    smatch match_result;
    if(regex_match(line, match_result, pattern)){
        header_[match_result[1]] = match_result[2];
    }else{
        state_ = BODY;
    }
}

void HttpRequest::ParseBody_(const string& line){
    body_ = line;
    ParsePost_();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

int HttpRequest::ConvertToHex(char ch){
    if(ch>='A' && ch <= 'F') return ch-'A'+10; 
    if(ch>='a' && ch <= 'f') return ch-'a'+10;
    return ch;
}

void HttpRequest::ParsePost_(){
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded"){
        ParseFromEncodedUrl_();
        if(default_html_tag.count(path_)){
            int tag = default_html_tag.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            if(tag == 0||tag==1){
                bool is_login = (tag==1);
                if(UserVerify(post_["username"], post_["password"], is_login)){
                    path_ = "/welcome.html";
                }else{
                    path_ = "/error.html";
                }
            }
        }
    }
}

void HttpRequest::ParseFromEncodedUrl_(){
    if(body_.size() == 0){return;}
    std::string key, value;
    int num=0, n = body_.size();
    int i=0, j=0;
    while(i++<n){
        char ch = body_[i];
        switch(ch){
            case '=':
                key = body_.substr(j, i-j);
                j = i+1;
                break;
            case '+':
                body_[i] = ' ';
                break;
            case '%':
                num = ConvertToHex(body_[i+1])*16+ConvertToHex(body_[i+2]); // simple encrption
                body_[i+2] = num%10+'0';  
                body_[i+1] = num/10+'0';
                i+=2;
                break;
            case '&':
                value = body_.substr(j, i-j);
                j = i+1;
                post_[key] = value;
                LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
                break;
            default:
                break;
        }
    }
    assert(j<=i);
    if(post_.count(key)==0 && j<i){
        value = body_.substr(j, i-j);
        post_[key] = value;
    }
}


bool HttpRequest::UserVerify(const string &name, const string& passwd, bool is_login){
    if(name== ""||passwd == ""){return false;}
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), passwd.c_str());
    MYSQL* sql_connection;
    SqlConnectionRaii(&sql_connection, SqlConnectionPool::Instance());
    assert(sql_connection); 
    bool is_verify_success = false;
    char order[256] = {0};
    MYSQL_RES* res = nullptr;
    if(!is_login){
        is_verify_success = true;
    }
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);
    if(mysql_query(sql_connection, order)){ 
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql_connection);
    while(MYSQL_ROW row = mysql_fetch_row(res)){
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        string db_passwd(row[1]);
        if(is_login){// user wants to log in
            if(passwd == db_passwd){
                is_verify_success = true;
            }else{
                is_verify_success = false;
                LOG_DEBUG("passwd not correct!");
                
            }
        }else{
            is_verify_success = false;
            LOG_DEBUG("user used!");
        }
    }
    mysql_free_result(res);

    if(!is_login && is_verify_success ){
        LOG_DEBUG("register!");
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s', '%s')", name.c_str(), passwd.c_str());
        LOG_DEBUG( "%s", order);
        if(mysql_query(sql_connection, order)){
            LOG_DEBUG( "Insert error!");
            is_verify_success = false;
        }
        is_verify_success = true;
    }
    SqlConnectionPool::Instance()->FreeConnection(sql_connection);
    LOG_DEBUG( "UserVerify success!!"); 
    return is_verify_success;
}


string HttpRequest::Path() const {
    return path_;
}
string& HttpRequest::Path(){
    return path_;
}
string HttpRequest::Method() const{
    return method_;
}
string HttpRequest::Version() const{
    return version_;
}

string HttpRequest::GetPost(const string& key) const{
    assert(key!="");
    if(post_.count(key)==1){
        return post_.find(key)->second;
    }
    return "";
}

string HttpRequest::GetPost(const char* key) const{
    assert(key!=nullptr);
    if(post_.count(key)==1){
        return post_.find(key)->second;
    }
    return "";
}