//
// Created by gnezd on 2022-09-19.
//

#include "Logging.h"
#include "AsyncLogging.h"
#include <iostream>

using namespace std;

int main() {
    AsyncLogging log("./ddmuduo.log");
    log.start();
    cout << "i am dd" << endl;
    for (int i = 0; i < 100000; i++) {
        LOG << "ddddddd";
    }
    cout << "i am dd" << endl;


    return 0;
}