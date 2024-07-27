
// reads data from SD File, ensures data is of correct length, 
// scales data, and saves to 21 files (one for each data stream)

#include <algorithm>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <chrono>
#include <memory>
#include <string>

#define LEN 85
#define TIME_SIZE 2048

struct XYZ{
    int16_t xyz[3];
} __attribute__((packed));

struct Sensor_Buffer {
    uint8_t time; // timestamp
    uint8_t id;   // index, changes to sensor label when full
    XYZ data[LEN];
} __attribute__((packed));

class BNO_Buffer {
public:
    Sensor_Buffer sensBuf;

    BNO_Buffer() {
        set_SR(125, 116, 47);
    }

    void parseLabel(int *timeStamp, int *bno, int *agm) { // read timestamp, sensor ID, sensor type
        *timeStamp = (sensBuf.time) + (sensBuf.id >> 5) * 256;
        *bno = (sensBuf.id >> 5) % 8;
        *agm = sensBuf.id % 4;
    }

    int get_correct_length(int t0, int t1, int bno, int agm) { // return correct length of buffer given timestamps, id
        int interval = interpret_time(expSR[bno][agm], t0, t1);
        return (interval) * goalSR[agm] / 1000;
    }

    /*
    copy data to output by applying hold or removing entries
    data: raw data of length LEN
    output: empty array of length goal
    goal: desired length of data
    */
    void fix_SR(XYZ* output, int goal) {
        int err = LEN - goal; // amount of data to be created / removed
        if (!err) { // no need to add/remove
            output = sensBuf.data;
            return;
        }
        int add = 0; // bool to add / remove data
        if (err < 0) {
            err = -err;
            add = 1; // add data
        }
        float copySize = float(LEN) / err; // amount of data to copy at a time w/o adjustment
        copy_data(copySize, err, add, output);
    }

private:
    int expSR[7][3];
    int goalSR[3] = {100, 100, 40};

    void set_SR(int accSR, int gyrSR, int magSR) {
        int srs[3] = {accSR, gyrSR, magSR};
        for (int i = 0; i < 7; i++) {
            std::copy_n(srs, 3, expSR[i]);
        }
    }

    /*
    return time between t0 and t1 in ms, sr is in Hz
    sr: expected sampling rate of device in Hz (not desired sampling rate)
    t0 / t1: timestamp before / after data collection
    */
    int interpret_time(int sr, int t0, int t1) {
        int time = LEN * 1000 / sr;           // time between t0 and t1 in ms
        int expT = (time + t0) % TIME_SIZE;   // expected value for t1                        
        int offset = t1 - expT;               // offset between expected and actual time
        time += offset;
        if (abs(offset) < (TIME_SIZE / 2)) { return time; } // no overflow offset
        if (offset < 0) { return time + TIME_SIZE; } // t1 is fast and wrapped around to low number
        return time - TIME_SIZE;
    }


    void copy_data(float copySize, int err, int add, XYZ* output) { // loop through data and adjust length
        float next, count = 0; // non-int start/end index
        int c, n; // int index
        for (int i = 0; i < err; i++) { // for each time output must be adjusted
            next = count + copySize;
            c = int(count + 0.00001);   // prevents rounding down due to imprecise floating point addition
            n = int(next + 0.00001); 
            if (add) {
                std::copy_n(sensBuf.data + c, n - c, output + c + i); // copy copySize elements from data to buffer
                output[n + i] = output[n - 1 + i];            // duplicate last element copied
            } else {
                std::copy_n(sensBuf.data + c, n - c - 1, output + c - i); // copy copySize - 1 elements from data to output
            }
            count = next;
        }
    }
};

#define SBUF_SIZE 512
std::string outFiles[7][3];

void name_files() {
    char sensors[3][4] = {"acc", "gyr", "mag"};
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 3; j ++) {
            outFiles[i][j] = "imu" + std::to_string(i) + "_" + sensors[j] + ".bin";
        }
    }
}

void write_to_file (int bno, int agm, XYZ* data, int outSize) {
    float k;
    float wBuf[outSize * 3];
    switch (agm) {
        case 0: k = 256; break;
        case 1: k = 512; break;
        case 2: k = 16; break;
    }
    for (int i = 0; i < outSize; i++) {
        for (int j = 0; j < 3; j++) {
            wBuf[3 * i + j] = data[i].xyz[j] / k;
        }
    }
    std::ofstream dataFile(outFiles[bno][agm], std::ios::app | std::ios::binary);
    if (!dataFile){
        std::cout << "file won't open\n";
        return;
    }
    dataFile.write(reinterpret_cast<char *>(wBuf), outSize * 3 * sizeof(float));
    dataFile.close();
}

int main() {
    std::ifstream binFile("binFile.bin", std::ios::binary);
    if (!binFile) {
        std::cout << "file won't open\n";
        return 1;
    }

    name_files();


    BNO_Buffer sbuf;
    int timeStamp, lastTime, bno, agm, outSize;
    XYZ *outData = new XYZ[128];

    binFile.read(reinterpret_cast<char*>(&sbuf.sensBuf), SBUF_SIZE);
    sbuf.parseLabel(&lastTime, &bno, &agm);

    while (binFile.read(reinterpret_cast<char*>(&sbuf.sensBuf), SBUF_SIZE)) {
        sbuf.parseLabel(&timeStamp, &bno, &agm);
        std::cout << "time: " << timeStamp << " " << "bno: " << bno << " " << "agm: " << agm << "\n";
 
        outSize = sbuf.get_correct_length(lastTime, timeStamp, bno, agm);
        sbuf.fix_SR(outData, outSize);
        write_to_file(bno, agm, outData, outSize);
        lastTime = timeStamp;
    }

    delete outData;
    binFile.close();
    return 0;
}
