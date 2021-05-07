#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <array>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/signal_set.hpp>
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
#include <vector>
#define PORT 7001 // Server's Port number
#define MAXLENGTH 3000
#define LogDIR "/home/pi/Desktop/log/"
#define fileDIR "/home/pi/Desktop/received/"

using namespace std;
using namespace boost::asio;

map<string, std::chrono::system_clock::time_point> files;
io_service global_io_service;
fstream csv;
int cur_round;
void handler(int signo);
string fr_name;

int filesize(string fname)
{
    ifstream in_file(fname, ios::binary);
    in_file.seekg(0, ios::end);
    int file_size = in_file.tellg();
    return file_size;
}

char *toTime(std::chrono::system_clock::time_point target)
{
    time_t temp = std::chrono::system_clock::to_time_t(target);
    char *result = ctime(&temp);
    for (char *ptr = result; *ptr != '\0'; ptr++)
    {
        if (*ptr == '\n' || *ptr == '\r')
            *ptr = '\0';
    }
    return result;
}

class IoTSession : public enable_shared_from_this<IoTSession>
{
private:
    ip::tcp::socket _socket;
    array<char, MAXLENGTH> _data;
    std::chrono::system_clock::time_point startTime, endTime;
    fstream fr;

public:
    // Declare constructor
    IoTSession(ip::tcp::socket socket) : _socket(move(socket))
    {
        char temp[5];
        sprintf(temp, "%d", cur_round);
        fr_name = _socket.remote_endpoint().address().to_string() + "_" + temp;

        // open log file descriptor with app mode
        fr.open(fr_name, std::fstream::in | std::fstream::out | std::fstream::trunc);
        startTime = std::chrono::system_clock::now();
        files.insert(pair<string, std::chrono::system_clock::time_point>(fr_name, startTime));
    }

    void start()
    {
        do_read();
    }

    //Declare destructor
    ~IoTSession()
    {
        // Count end time and data rate
        endTime = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = endTime - startTime;
        if (files.erase(fr_name) != 0)
        {
            double fsize = double(filesize(fr_name)) / 128; // kb

            cout << fr_name << " received file size:" << fsize << endl;
            // printf("fr_name received file size:%lf\n", fsize);

            csv << "\"" << fr_name + "\"," << cur_round
                << ",\"" << toTime(endTime) << "\"," + to_string(fsize)
                << " \r\n"
                << flush;
        }
        fr.close();
    };

private:
    void do_read()
    {

        auto self(shared_from_this());
        _socket.async_read_some(
            buffer(_data, MAXLENGTH),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec)
                {
                    string content;
                    content.append(_data.data(), _data.data() + length);
                    fr << content;
                    do_read();
                }
            });
    }
};

class IoTServer
{
private:
    ip::tcp::acceptor _acceptor;
    ip::tcp::socket _socket;

public:
    IoTServer(short port)
        : _acceptor(global_io_service, ip::tcp::endpoint(ip::tcp::v4(), port)),
          _socket(global_io_service)
    {
        do_accept();
    }

private:
    void do_accept()
    {
        _acceptor.async_accept(_socket, [this](boost::system::error_code ec) {
            if (!ec)
            {
                global_io_service.notify_fork(io_service::fork_prepare);

                int pid = fork();
                // child process
                if (pid == 0)
                {
                    // Only child register the handler
                    signal(SIGINT, handler);
                    // inform io_service fork finished (child)
                    global_io_service.notify_fork(io_service::fork_child);

                    // close acceptor
                    _acceptor.close();

                    // parse the request
                    make_shared<IoTSession>(move(_socket))->start();
                }
                else
                {
                    // parent process
                    // inform io_service fork finished (parent)
                    global_io_service.notify_fork(io_service::fork_parent);

                    cout << "----- " + _socket.remote_endpoint().address().to_string() +
                                ":" + to_string(_socket.remote_endpoint().port()) + " pid:" + to_string(pid) + " -----\n";

                    _socket.close();
                    do_accept();
                }
            }
            else
            {
                cerr << "accept error:" << ec.message() << endl;
                do_accept();
            }
        });
    }
};

// void handler(const boost::system::error_code &error, int signal_number)
void handler(int signo)
{
    // std::cout << to_string(getpid()) << " handling signal " << signal_number << std::endl;
    std::cout << " Handling signal " << signo << std::endl;
    // std::cout << to_string(getpid()) << " Map size: " << files.size() << std::endl;
    for (map<string, std::chrono::system_clock::time_point>::iterator it = files.begin(); it != files.end();)
    {
        std::chrono::system_clock::time_point endTime = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = endTime - it->second;
        double fsize = double(filesize(it->first)) / 128; // kb
	// printf("Received file size:%lf\n", fsize);
        cout << fr_name << " received file size:" << fsize << endl;
        csv << "\"" << it->first + "\"," << cur_round
            << ",\"" << toTime(endTime) << "\"," + to_string(fsize)
            << " \r\n"
            << flush;
        it = files.erase(it);
    }
    csv.flush();
    csv.close();
    exit(1);
}

int main(int argc, char *argv[])
{
    signal(SIGCHLD, SIG_IGN);
    // signal(SIGINT, handler);
    // set port
    short port = PORT; // default PORT = 7001
    string logFName("tcp_");
    if (argc == 3)
    {
        cout << "Now Usage: IoTServer logName cur_round" << endl;
        logFName += argv[1];
        cur_round = atoi(argv[2]);
    }
    else
    {
        cout << "[ERROR] Usage: IoTServer logName cur_round" << endl;
        exit(EXIT_FAILURE);
    }
    csv.open(LogDIR + logFName + ".csv", std::fstream::in | std::fstream::out | std::fstream::app);

    // boost::asio::signal_set signals(global_io_service, SIGINT);
    // signals.async_wait(handler);

    try
    {
        IoTServer server(port);
        global_io_service.run();
    }
    catch (exception &e)
    {
        cerr << "Exception: " << e.what() << "\n";
        // csv.close();
    }

    return 0;
}
