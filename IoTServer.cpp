// #include <mysql/mysql.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <array>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <memory>
#include <utility>
#include <chrono>
#include <ctime>
#include <map>
#define PORT 7001 // Server's Port number
#define MAXLENGTH 3000
#define LogDIR "/home/pi/Desktop/log/"

using namespace std;
using namespace boost::asio;

io_service global_io_service;
string logFName("server");
fstream csv;

int filesize(string fname) {
    ifstream in_file(fname, ios::binary);
    in_file.seekg(0, ios::end);
    int file_size = in_file.tellg();
    return file_size;
}

class IoTSession : public enable_shared_from_this<IoTSession> {
private:
    map<string, int> connectionCount;
    ip::tcp::socket         _socket;
    array<char, MAXLENGTH>  _data;
    chrono::system_clock::time_point startTime, endTime;
    string      fr_name;
    fstream fr, flog;
    // char        rfID[24];
    // MYSQL       *conn;
    // MYSQL_RES   *res;
    // MYSQL_ROW   row;

public:
    // Declare constructor
    IoTSession(ip::tcp::socket socket) : _socket(move(socket)) {
        connectionCount.clear();
        fr_name = _socket.remote_endpoint().address().to_string();
        // Insert ACK
        /* insertDB(); */
        // open received file descriptor with trunc mode
        fr.open (fr_name + "_" + to_string(connectionCount[fr_name]) , std::fstream::in | std::fstream::out | std::fstream::trunc);
        // open log file descriptor with trunc mode
        flog.open (LogDIR + logFName + ".log", std::fstream::in | std::fstream::out | std::fstream::app);
        startTime = chrono::system_clock::now(); 
        flog << "----- Session: " + fr_name + "-----\n";
        flog << "Started computation at: " << toTime(startTime);
    }

    void start() {
        do_read();
    }

    //Declare destructor
    ~IoTSession() {
        // Count end time and data rate
        endTime = chrono::system_clock::now();
        chrono::duration<double> elapsed_seconds = endTime-startTime;
        flog << "finished computation at:\t" << read_gps() << "\t" << toTime(endTime);
        double fsize = double (filesize(fr_name)) / 1000000; // MB
        flog << "file size:\t\t\t" << to_string(fsize) << "MB\t" << elapsed_seconds.count() << "s\t" << to_string(fsize/elapsed_seconds.count()) << "Mbps\n";
        csv << fr_name + "," + to_string(connectionCount[fr_name]++) + ","  + to_string(fsize) + "," +  to_string(elapsed_seconds.count()) + "," + to_string(fsize / elapsed_seconds.count()) << "\n";

        // close received file descriptor (bug for fr.close before do_read finish???????)
        fr.close();
        flog.close();
    };

private:
    void do_read() {

        auto self(shared_from_this());
        _socket.async_read_some(
                buffer(_data, MAXLENGTH),
                [this, self](boost::system::error_code ec, std::size_t length) {
                    if (!ec) {
                        string content;
                        content.append(_data.data(), _data.data()+length);
                        fr << content;
                        do_read();
                    }
                });
    }
    // *****************************
    // Read GPS from file,
    // But we don't have the file
    // *****************************
    string read_gps() {
        fstream     fgps;
        string dir = LogDIR, lat, lng, result;
        fgps.open (dir + "gps.log", std::fstream::in);
        fgps >> lat  >> lng;
        result = lat + " " + lng;
        //cout << result << endl;
        fgps.close();
        return result;
    }
    // void connDB() {
    //     char server[17], user[20], password[20], database[20];

    //     // Setting MYSQL server
    //     strcpy(server, "140.113.216.91");
    //     strcpy(user, "cloud");
    //     strcpy(password, "cloud2016");
    //     strcpy(database, "ray");

    //     // Connect to database
    //     conn = mysql_init(NULL);
    //     if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)) {
    //         fprintf(stderr, "%s\n", mysql_error(conn));
    //         exit(1);
    //     }
    // }
    // void insertUAVack() {
    //     // Get local time
    //     struct tm * timeinfo;
    //     char timestamp[20], str[40960];
    //     time_t now;

    //     time(&now);
    //     timeinfo = localtime(&now);
    //     sprintf(timestamp, "%d-%d-%d %d:%d:%d", timeinfo->tm_year+1900, timeinfo->tm_mon+1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    //     // Insert data
    //     sprintf(str,"INSERT INTO `uavData`(`ipAddr`, `rawdata`, `receiveGWTime`, `gwip`, `gwid`, `rssi`, `snr`) VALUES ");
    //     char temp[256];
    //     sprintf(temp, "('%s', 'NOOO', '%s', '140.113.216.75', '00001c497bcaae8a', -108, -1.3)", fr_name.c_str(), timestamp);
    //     strcat(str, temp);
    //     if(mysql_query(conn, str)) {
    //         fprintf(stderr, "1, %s\n", mysql_error(conn));
    //         exit(1);
    //     }
    //     //mysql_free_result(res);
    // }
    // void insertRFMack() {
    //     char str[40960];
    //     //char rfID[24];
    //     sprintf(str, "SELECT rfID FROM `info` WHERE ipAddr = \"%s\"", fr_name.c_str());
    //     if(mysql_query(conn, str)){
    //         fprintf(stderr, "2, %s\n", mysql_error(conn));
    //         exit(1);
    //     }
    //     if(res = mysql_use_result(conn)){
    //         if(row = mysql_fetch_row(res)){
    //             if(row[0] != NULL)
    //                 strcpy(rfID, row[0]);
    //         }
    //     }
    //     // cout << rfID << endl;
    //     mysql_free_result(res);

    //     // Get local time
    //     struct tm * timeinfo;
    //     char timestamp[20];
    //     time_t now;

    //     time(&now);
    //     timeinfo = localtime(&now);
    //     sprintf(timestamp, "%d-%d-%d %d:%d:%d", timeinfo->tm_year+1900, timeinfo->tm_mon+1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    //     // Insert data
    //     sprintf(str,"INSERT INTO `rfm69`(`rfID`, `ackTime`, `baseID`) VALUES ");
    //     char temp[256];
    //     sprintf(temp, "('%s', '%s', '22:22:22:22:22:22:22:22')", rfID, timestamp);
    //     strcat(str, temp);
    //     if(mysql_query(conn, str)) {
    //         fprintf(stderr, "3, %s\n", mysql_error(conn));
    //         exit(1);
    //     }
    //     //mysql_free_result(res);
    // }
    // int  insertDB() {
    //     connDB();
    //     insertUAVack();
    //     insertRFMack();
    // }
    char* toTime(chrono::system_clock::time_point target) {
        time_t temp = chrono::system_clock::to_time_t(target);
        char* result = ctime(&temp);
        return result;
    }
};

class IoTServer {
private:
    ip::tcp::acceptor _acceptor;
    ip::tcp::socket _socket;

public:
    IoTServer(short port)
            : _acceptor(global_io_service, ip::tcp::endpoint(ip::tcp::v4(), port)),
              _socket(global_io_service) {
        do_accept();
    }

private:
    void do_accept() {
        _acceptor.async_accept(_socket, [this](boost::system::error_code ec) {
            if (!ec) {
                global_io_service.notify_fork(io_service::fork_prepare);

                int pid = fork();
                // child process
                if(pid == 0)
                {
                    // inform io_service fork finished (child)
                    global_io_service.notify_fork(io_service::fork_child);

                    // close acceptor
                    _acceptor.close();

                    // parse the request
                    make_shared<IoTSession>(move(_socket))->start();


                } else {
                    // parent process
                    // inform io_service fork finished (parent)
                    global_io_service.notify_fork(io_service::fork_parent);

                    cout << "----- " + _socket.remote_endpoint().address().to_string() +
                            ":"+ to_string(_socket.remote_endpoint().port()) + " pid:" + to_string(pid) + " -----\n";

                    _socket.close();
                    do_accept();
                }
            } else {
                cerr << "accept error:" << ec.message() << endl;
                do_accept();
            }
        });
    }
};


int main (int argc, char *argv[])
{
    signal(SIGCHLD, SIG_IGN);
    // set port
    short port = PORT;      // default PORT = 7001
    if(argc == 2) {
        port = static_cast<short>(atoi(argv[1]));
    }
    else if (argc == 3)
    {
        port = static_cast<short>(atoi(argv[1]));
        logFName.assign(argv[2]);
    }
    csv.open(LogDIR + logFName + ".csv", std::fstream::in | std::fstream::out | std::fstream::app);

    try {
        IoTServer server(port);
        global_io_service.run();
    } catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
        csv.close();
    }

    return 0;
}
