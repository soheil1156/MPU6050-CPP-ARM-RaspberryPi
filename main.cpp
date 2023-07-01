/**
 * @file main.cpp
 * @brief MPU6050-CPP-ARM-RaspberryPi - Vibration Analysis Software
 * @author Soheil Nazari
 * @date July 1, 2023
 *
 * This software is a vibration analysis tool that utilizes the MPU6050 sensor on ARM or Raspberry Pi devices.
 * It reads the accelerometer data, performs analysis, and outputs the results to a file.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
 * to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <iostream>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <fstream>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <err.h>
#include <chrono>
#include <ctime>

#define IOEX_SUCCESS 0
#define IOEX_FAILURE -1

#define IOEX_DEBUG

#ifdef IOEX_DEBUG

int IOEX_debug_counter;
#define IOEX_Debug(args...) {IOEX_debug_counter ++; printf("\r\n#%s-%03d\t",__FILE__,IOEX_debug_counter);printf(args) ;}

#define IOEX_Debug_sec(args...)  {printf("\r\n*************************************************\r\n*\r\n* %s\t",__FILE__);printf(args);\
							printf("\r\n*\r\n*************************************************");}

#else

#define IOEX_Debug(args...)
#define IOEX_Debug_sec(args...)

#endif

#define IOEX_PINOUT 0
#define IOEX_PININ 1
#define IOEX_PINSET 1
#define IOEX_PINCLR 0

#define PWR_MGMT_1    0x6B
#define SMPLRT_DIV    0x19
#define CONFIG        0x1A
#define GYRO_CONFIG   0x1B
#define INT_ENABLE    0x38
#define ACCEL_XOUT_H  0x3B
#define ACCEL_YOUT_H  0x3D
#define ACCEL_ZOUT_H  0x3F
#define GYRO_XOUT_H   0x43
#define GYRO_YOUT_H   0x45
#define GYRO_ZOUT_H   0x47

int fdiic = 0;
int rciic = 0;
uint8_t devAddr = 0x68;
char * path = NULL;
uint8_t dir = 0xff;
uint8_t GPIO = 0x00;

int init(const char *path, uint8_t devAddr) {
    path = strdup(path);
    devAddr = devAddr;
    fdiic = open(path, O_RDWR);
    if (fdiic < 0) {
        err(errno, "Tried to open '%s'", path);
        return IOEX_FAILURE;
    }
    return IOEX_SUCCESS;
}

int iicwrite(uint8_t regAddr, union i2c_smbus_data *value) {
    if (ioctl(fdiic, I2C_SLAVE,  devAddr) < 0) {
        IOEX_Debug("write Erro");
        return IOEX_FAILURE;
    }

    struct i2c_smbus_ioctl_data args;
    args.read_write = I2C_SMBUS_WRITE;
    args.command = regAddr;
    args.size = I2C_SMBUS_BYTE_DATA;
    args.data = value;
    return ioctl(fdiic, I2C_SMBUS, &args);
}

int iicread(uint8_t regAddr, union i2c_smbus_data *value) {
    if (ioctl(fdiic, I2C_SLAVE, devAddr) < 0) {
        IOEX_Debug("write Erro");
        return IOEX_FAILURE;
    }

    struct i2c_smbus_ioctl_data args;
    args.read_write = I2C_SMBUS_READ;
    args.command = regAddr;
    args.size = I2C_SMBUS_BYTE_DATA;
    args.data = value;

    return ioctl(fdiic, I2C_SMBUS, &args);
}

void MPU_Init() {
    union i2c_smbus_data value;
    // Write to sample rate register
    value.byte = 7;
    iicwrite(SMPLRT_DIV, &value);

    // Write to power management register
    value.byte = 1;
    iicwrite(PWR_MGMT_1, &value);

    // Write to Configuration register
    value.byte = 0;
    iicwrite(CONFIG, &value);

    // Write to Gyro configuration register
    value.byte = 24;
    iicwrite(GYRO_CONFIG, &value);

    // Write to interrupt enable register
    value.byte = 1;
    iicwrite(INT_ENABLE, &value);

    union i2c_smbus_data value2;
    value2.byte = 0;
    iicread(ACCEL_XOUT_H, &value2);
    printf("**** INIT DONE data_read =%02X %d\r\n", devAddr, value2.byte);
}

int read_raw_data(uint8_t addr) {
    // Accelero and Gyro value are 16-bit
    int value, high, low;
    union i2c_smbus_data i2cvalue;
    iicread(addr, &i2cvalue);
    high = i2cvalue.byte;
    iicread(addr + 1, &i2cvalue);
    low = i2cvalue.byte;

    value = ((high << 8) | low);

    if (value > 32768)
        value = value - 65536;
    return value;
}

int main() {
    // Open the I2C bus
    const char *device = "/dev/i2c-1";
    init(device, 0x68);
    MPU_Init();

    int acc_x;
    int acc_y;
    int acc_z;
    float Ax, Ay, Az;

    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point lastTime = startTime;

    std::chrono::steady_clock::time_point currentTime;
    std::chrono::duration<double> totalTime;

    FILE *outputFile = fopen("dataCpp.txt", "w");
    if (outputFile == nullptr) {
        std::cout << "Error opening file!" << std::endl;
        return 1;
    }

    while (1) {
        int acc_x = read_raw_data(ACCEL_XOUT_H);
        int acc_y = read_raw_data(ACCEL_YOUT_H);
        int acc_z = read_raw_data(ACCEL_ZOUT_H);

        Ax = acc_x / 16384.0;
        Ay = acc_y / 16384.0;
        Az = acc_z / 16384.0;

        currentTime = std::chrono::steady_clock::now();
        totalTime = currentTime - startTime;

        fprintf(outputFile, "\t%.4f \t%.4f \t%.4f \t%.4f\r\n", Ax, Ay, Az, totalTime);
    }

    fclose(outputFile);
    return 0;
}
