#include <iostream>

using namespace std;

int main (){
    int a = 90;
    int b = 99;
    int &rf = a;
    int &rf1 = a;
    rf1++;
    rf++;
    cout << rf << endl;
    cout << a << endl;
    rf = b;
    rf++;
    cout << b << endl;
}