# Introduction

This is a multi-threading high-concurrency webserver implemented using C++. A demo example can be accessed at 
[http://164.92.79.241:8000/](http://164.92.79.241:8000). Webbench Pressure tests shows that the server can hold
more than 10K QPS at the same time on a Linux machine with `2vCPUs`(shared) and `4GB` memory.

## Architecture

- IO multiplexing Epoll and Threading pool is used to accomplish the High Concurrency.
- The server is implemented using Reactor mode.
- Mysql Connection pool is implemented using RAII(Resource Acquisition Is Initialization) to achieve higher performance in sql connection.
- HTTP requests(static resources) are pared using regular patterns combined with Finite State Machine technique.
- STL vector is used to encapsulate a Buffer class which can have auto-increment in size when needed.
- STL vector is used to encapsulate a (min) Heap Timer class to clear non-active HTTP connections 
- Asynchronous logging system is implemented using an encapsulated blocking queue in singleton mode.


## Installation

step1: create database in mysql
```mysql
create database webserver;
USE webserver;
CREATE TABLE user(
    username char(50) NULL,
    password char(50) NULL
)ENGINE=InnoDB;

INSERT INTO user(username, password) VALUES('test', 'test');
```

step2:set mysql root password in the environment variables(in the ~/.bash_profile, and `source ~/.bash_profile` after that)

```shell
export webserver_mysql_passwod=<webserver_mysql_passwod>
```

step3: start the server:(default to use port 8000)
```shell
make
./server
```


## Pressure Test Result 
Test is performed on Ubuntu 20.04 (LTS) x64 with `2vCPUs`(shared) and `4GB` memory.

```shell
$sh pressure-test.sh 
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://localhost:8000/
10000 clients, running 10 sec.

Speed=105018 pages/min, 5629513 bytes/sec.
Requests: 17503 susceed, 0 failed.
```
