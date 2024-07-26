#include <iostream>
#include <cmath>
#include <algorithm>
#include <string>
#include <time.h>

#define LEN 85 // length of buffer
#define TIME_SIZE 2048 // max value of timestamp

/* Helper Functions for Debugging */
void print(std::string s, int i) {
    std::cout << s << ": " << i << "\n";
}

void print(std::string s, float i)
{
    std::cout << s << ": " << i << "\n";
}

void printArr(int *arr, int size, std::string name) {
    std::cout << name << ": ";
    for (int i = 0; i < size; i++) {
        std::cout << arr[i] << " ";
    }
    std::cout << "\n";
}

void createData (int* data) {
    srand(time(NULL));
    for (int i = 0; i < LEN; i++) {
        data[i] = int(100 * sin(i / 100) + rand() % 10);
    }
}

/*  Useful Functions */

/*
return time between t0 and t1 in ms, sr is in Hz
sr: expected sampling rate of device in Hz (not desired sampling rate)
t0 / t1: timestamp before / after data collection
*/
int interpret_time(int sr, int t0, int t1) {
    int time = LEN * 1000 / sr + t0;      // time between t0 and t1 in ms
    int expT = time % TIME_SIZE;          // expected value for t1                        
    int offset = t1 - expT;               // offset between expected and actual time
    time += offset;
    if (abs(offset) < (TIME_SIZE / 2)) {  // no overflow offset
        print("sampling rate calculated", LEN / (float) time * 1000);
        return time; 
    } 
    if (offset < 0) {                     // t1 is fast and wrapped around to low number
        time += TIME_SIZE;
        print("sampling rate calculated", LEN / (float) time * 1000);
        return time;
     }
     time -= TIME_SIZE;                   // t1 is slow and didn't wrap around yet
     print("sampling rate calculated", LEN / (float) time * 1000);
     return time - TIME_SIZE;
}

/*
return expected number of samples given time and desired sampling rate
time: duration of sampling period in ms
goalSR: desired sampling rate
*/
int get_size(int time, int goalSR) { // return expected number of samples given timestamps and sampling rate
    return (time) * goalSR / 1000;
}

/*
copy data to output by applying hold or removing entries
data: raw data of length LEN
output: empty array of length goal
goal: desired length of data
*/
void fix_SR(int *data, int *output, int goal) {
    int err = LEN - goal; // amount of data to be created / removed
    if (!err) { // avoid dividing by 0
        std::copy_n(data, LEN, output);
        return;
    }
    int add = 0; // bool to add / remove data
    if (err < 0) {
        err = -err;
        add = 1; // add data
    }
    float copySize = float(LEN) / err; // amount of data to copy at a time w/o adjustment
    float count = 0; // index for data
    float next; // end index
    int c, n; // hold int count and next

    for (int i = 0; i < err; i++) { // for each time output must be adjusted
        next = count + copySize;
        c = int(count + 0.00001);   // prevents rounding down due to imprecise floating point addition
        n = int(next + 0.00001); 
        if (add) {
            std::copy_n(data + c, n - c, output + c + i); // copy copySize elements from data to buffer
            output[n + i] = output[n - 1 + i];            // duplicate last element copied
        } else {
            std::copy_n(data + c, n - c - 1, output + c - i); // copy copySize - 1 elements from data to output
        }
        count = next;
    }
}

int main() {
    int stuff[LEN]; // sample data
    createData(stuff);

    srand(time(NULL));
    int t0 = rand() % 256; // sample timestamps
    int t1 = rand() % 256;
    print("t0", t0);
    print("t1", t1);

    int time = interpret_time(40, t0, t1); // calculate time
    int outLen = get_size(time, 40); // calculate desired data size
    print("time", time);
    print("goal", outLen);

    int output[outLen];
    fix_SR(stuff, output, outLen);

    printArr(stuff, LEN, "Stuff      ");
    printArr(output, outLen, "Main Output");

    return 0;
}
