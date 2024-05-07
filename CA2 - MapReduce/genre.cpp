#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

using namespace std;

int main(int argc, char* argv[]) {
    if( argc == 1) {
        cout << "Usage: " << argv[0] << " <filename>" << endl;
        return 1;
    }
    string genreName = argv[1];
    cout << "20: GENRE: " + genreName << endl;
    int main_fd = open("mainfifo", O_RDWR);
    int genre_fd = open(genreName.c_str(), O_RDWR);
    char buf[1024];
    int num_genre = 0;
    int size = stoi(string(argv[2]));
    cout << "number of parts: " <<size << endl;
    for ( int i = 0 ; i < size; i++) {
        read(genre_fd, buf, 1024);
        num_genre += stoi(string(buf));
        cout << "GENRE 28: "<< genreName << i << '\t' << buf << "\t" << "\t" << num_genre << endl;
        buf[0] = '\0';
    }
    string num_genre_str = to_string(num_genre);
    num_genre_str = genreName + " " + num_genre_str;
    cout << num_genre_str << "\tgenre********" << endl;
    int res = write(main_fd, num_genre_str.c_str(), num_genre_str.length()+1);
    cout << genreName << res << "\t genre: end\n";
    close(genre_fd);
    close(main_fd);
}