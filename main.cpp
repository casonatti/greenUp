#include <iostream>
#include <string.h>
using namespace std;

#define PARTICIPANT 0;
#define MANAGER 1;

int main(int argc, char** argv) {
    int machine;
//---------------------------------------------------------- PARTICIPANT section ----------------------------------------------------------
    if(argc == 1) {
        machine = PARTICIPANT;

        // while(true) {

        // }
    }
//------------------------------------------------------------ MANAGER section ------------------------------------------------------------
    //argv[1] does exist.
    if(argc == 2) { 
        if(strcmp(argv[1], "manager") != 0) { //argv[1] != "manager"
            cout << "argv NOT OK" << endl;
            return -1;
        }
        
        machine = MANAGER;

        // while(true) {

        // }
    }

    return 0;
}