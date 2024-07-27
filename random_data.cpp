#include <iostream>
#include <fstream>
#include <stdint.h>
#include <time.h>
#include <chrono>

#define SBUF_SIZE 512
struct Sensor_Buffer {
    uint8_t time;                       // timestamp
    uint8_t id;                        // index, changes to sensor label when full
    int16_t data[255];
} __attribute__((packed));

int get_time () { // return timestamp 11b
    using namespace std::chrono;
    auto now = system_clock::now();
    auto now_ms = time_point_cast<microseconds>(now);
    auto value = now_ms.time_since_epoch();
    return value.count() % 2048;
}

void fillBuff (Sensor_Buffer* sbuf) {
    int t = get_time();
    int type = rand() % 3;
    sbuf->time = t % 256;
    sbuf->id = ((t / 256) << 5) | ((rand() % 7) << 2) | type;
    for (int i = 0; i < 255; i++) {
        sbuf->data[i] = rand() % 2000 - 1000;
    }
}

int main () {
    std::ofstream binFile;
    binFile.open("binFile.bin", std::ios::binary);
    if(!binFile) {
        std::cout << "Unable to open file \n";
        return 1; 
    }

    Sensor_Buffer sensBuf;
    for (int i = 0; i < 10; i++) {
        fillBuff(&sensBuf);
        std::cout << static_cast<int>(sensBuf.id) % 4 << "\n";
        binFile.write(reinterpret_cast<char *>(&sensBuf), SBUF_SIZE);
    }

    binFile.close();
    return 0;
}
