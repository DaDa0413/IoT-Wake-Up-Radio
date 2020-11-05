// #include <mysql/mysql.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <array>
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
int filesize(string fname)
{
    ifstream in_file(fname, ios::binary);
    in_file.seekg(0, ios::end);
    int file_size = in_file.tellg();
    return file_size;
}

int main (int argc, char *argv[])
{
    cout << to_string((double)(filesize("1mb")) / 1000000); // MB

    return 0;
}
