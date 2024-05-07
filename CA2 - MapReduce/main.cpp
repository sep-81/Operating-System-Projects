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

int main(int argc, char *argv[]) {
    if( argc != 2 ) { 
        std::cout << "Usage: " << argv[0] << "<filename>" << std::endl;
        return 1;
    }
    cout << argc << argv[1] << endl;
    vector<string>csvNames;
    string genPath;
    for (const auto & entry : filesystem::directory_iterator(argv[1])) {
        // std::cout << entry.path().filename() << std::endl;
        auto temp = entry.path();
        if(temp.filename() != "genres.csv") {
            csvNames.push_back(entry.path());
            cout << temp << endl;
        } else {
            genPath = temp;
        }   
    }
    // cout << csvNames.size() << endl;
    int numPart = csvNames.size();
    cout << "numparts: " << numPart << endl;
    fstream fin;
    fin.open(genPath, ios::in);
    string line,word, allGenres = "";
    vector<string> genres;
    genres.clear();
    if(fin.is_open())
    {
        while(getline(fin, line))
        {    
        stringstream str(line);
        allGenres += line;
        while(getline(str, word, ','))
            genres.push_back(word);
        }
    } else {
        cout << "File not found" << endl;
        return 1;
    }
    // cout << genres.size() << endl;
    // for ( auto gen : genres ) {
    //     cout << gen << endl;
    // }

    // make main process fifio

    cout << allGenres <<"main 59 "<< endl;
    char* mainFifoName = "mainfifo";
    int result = mkfifo(mainFifoName, 0666);
    cout << result << '\n';
    int numGen = genres.size();
    int main_fd = open(mainFifoName, O_RDWR);
    cout << main_fd << endl;
    for( int i = 0 ; i < numGen; i++) { // genres process
        string gen = genres[i];
        mkfifo(gen.c_str(), 0666);
        pid_t pid = fork();
        if(pid < 0) {
            cout << "fork error" << endl;
            return 1;
        } 
        if( pid == 0) { // child
            execl("./genre", "genre", gen.c_str(), to_string(numPart).c_str(), (char*)NULL);
        } else { // parent
            continue;
        }

    }

    cout << allGenres << "\t main78"<< endl;
    for( int i = 0 ; i < numPart; i++) { // parts process
        int p[2];
        if( pipe(p) < 0 ) {
            cout << "Pipe error" << endl;
            return 1;
        }

        pid_t pid = fork();
        if(pid < 0) {
            cout << "fork error" << endl;
            return 1;
        }
        if ( pid == 0) { // child
            close(p[1]);
            char buf[allGenres.size()+1];
            read(p[0], buf, allGenres.size()+1);
            cout << buf << ", part proc 99"<< '\n';
            execl("./mapBooks", "mapBooks", csvNames[i].c_str(), buf, NULL);            

            break;
        } else {
            // parent
            close(p[0]);
            // parents sends mes to child
            const char* msg = allGenres.c_str();
            cout <<  msg << "108" <<endl;
            // cout << allGenres << endl;
            write(p[1], msg, allGenres.size()+1);

            close(p[1]);
        }
    }
    
    char buf[1024];
    cout << "main 118\n";
    for ( int i = 0 ; i < numGen; i++) {
        read(main_fd, buf, 1024);
        cout << "121*******" << buf << endl;
        buf[0] = '\0';
    }
    // for ( int i = 0 ; i < numGen + numPart; i++) {
    //     wait(NULL);
    // }
    close(main_fd);
}