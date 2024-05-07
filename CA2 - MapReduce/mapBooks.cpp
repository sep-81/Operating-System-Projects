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
#include <unordered_map>
#include <stdio.h>
#include <string.h>

using namespace std;

int main(int argc, char* argv[]) {
    cout << "mapping\n";
    if( argc == 1) {
        cout << "Usage: " << argv[0] << " <filename>" << endl;
        return 1;
    }
    unordered_map<string, int> genres;
    string genrStr;
    // get all genres from parent that is in argv[2]
    char* allGenres = argv[2];
    cout << allGenres << argv[1] << "map\n";
    char* genr = strtok(allGenres, ",");
    // genrStr = genr;
    // cout << "map 30: " << genrStr << endl;
    // genres[genrStr] = 0;
    while(genr) {
        cout << "map 34: " << genr << endl;
        genrStr = genr;
        genres[genrStr] = 0;
        // cout << "..\n";
        genr = strtok(NULL, ",");
    }
    cout << "map 38 \n";
    // read in the csv file

    cout << argc << argv[1] << endl;
    fstream fin;
    fin.open(argv[1], ios::in);
    string line,word;
    
    // genres.clear();
    if(fin.is_open())
    {
        while(getline(fin, line))
        {    
        stringstream str(line);
        getline(str, word, ',');
        while(getline(str, word, ','))
            genres[word]++;
        }
    } else {
        cout << "File not found" << endl;
        return 1;
    }
    for( auto gen : genres ) {
        cout << gen.first << " " << gen.second << endl;
    }
    for (auto itr = genres.begin(); itr != genres.end(); itr++) {
    int fd = open(itr->first.c_str(), O_RDWR);
    string num = to_string(itr->second);
    int res = write(fd, num.c_str(), num.length()+1);
    cout << argv[1] << '\t' + itr->first + '\t' << num << '\t' << res << "\t68 map\n";
    close(fd);
    }
    cout << "map: end\n";
}
