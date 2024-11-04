/*
* class : Midas Comprehensive Design
* author : Kim youngmin, Son eonsang
* date : 2024/06/01
* brief : charging control, fan coltrol(by temperature), data send and receive(full duplex)
* 
* modification history
* date  |  name  | brief
* 05/29    ses     senser testing
* 06/01    kym     SoC, charging control function, overall file content
* 06/01    ses     temp sensing, current sensing demo by python
* 06/02    kym     temp, current, voltage sensing and location matching, multi threading
* 06/03    kym     socket communication
* 06/04    ses     socket communication add pwm
*/
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <wiringPiI2C.h>
#include <wiringPi.h>
#include <unistd.h>
#include <cstdint>
#include <dirent.h>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <algorithm>
#include <chrono>
#include <softPwm.h>
#include <cstring>
#include <cmath>

#define SERVER_PORT 9000
#define SERVER_IP "192.168.0.155"

#define TCA_ADDR 0x70   //TCA9548A ina219 default address

#define RELAY_PIN1 0    //discharge relay
#define RELAY_PIN2 2
#define RELAY_PIN3 3
#define RESISTER_FAN_PIN 13

#define BATTERY1_FAN_PIN 4
#define BATTERY2_FAN_PIN 5
#define BATTERY3_FAN_PIN 12

#define BATTERY1_PWM_PIN 1
#define BATTERY2_PWM_PIN 23
#define BATTERY3_PWM_PIN 16

#define MAX_CRITICAL_TEMPERATURE 60
#define MAX_SAFE_TEMPERATURE 50
#define TARGET_VOLTAGE 4.2

#define BAT1_TEMP_ADDR "/sys/bus/w1/devices/28-3ce1d44372ac/w1_slave"
#define BAT2_TEMP_ADDR "/sys/bus/w1/devices/28-3ce1d4431bf2/w1_slave"
#define BAT3_TEMP_ADDR "/sys/bus/w1/devices/28-0316611a16ff/w1_slave"

#define RESISTER1_TEMP_ADDR "/sys/bus/w1/devices/28-031660efe1ff/w1_slave"
#define RESISTER2_TEMP_ADDR "/sys/bus/w1/devices/28-0316612a37ff/w1_slave"
#define RESISTER3_TEMP_ADDR "/sys/bus/w1/devices/28-031661131fff/w1_slave"

//charging mode
enum ChargingMode {
    STOP_CHARGING,
    STANDARD_CHARGING,
    FAST_CHARGING
};

int relay_state[4];
std::mutex mtx;

class TCA9548A {  //tca9548a ina219 structure
public:
    TCA9548A(int address) : address(address) {
        fd = wiringPiI2CSetup(address);
        if (fd == -1) {
            std::cerr << "Failed to initialize I2C communication.\n";
            exit(1);
        }
        wiringPiI2CWriteReg16(fd, 0x00, 0x399F); // Initialize INA219
    }

    float readBusVoltage() {
        int16_t value = readRegister(0x02);
        return value * 0.0005;
    }

    float readShuntVoltage() {
        int16_t value = readRegister(0x01);
        return value * 0.01;
    }

    float readCurrent() {
        int16_t value = readRegister(0x04);
        return value * 0.1;
    }

private:
    int fd;
    int address;

    int16_t readRegister(uint8_t reg) {
        int16_t value = wiringPiI2CReadReg16(fd, reg);
        // Swap bytes to correct endianness
        value = (value << 8) | ((value >> 8) & 0xFF);
        return value;
    }
};

void selectTCA9548AChannel(int fd, int channel) {
    if (channel > 7) {
        std::cerr << "Invalid channel number.\n";
        exit(1);
    }
    wiringPiI2CWrite(fd, 1 << channel);
}

void setup() {
    // Initialize wiringPi
    if (wiringPiSetup() == -1) {
        std::cerr << "Failed to initialize wiringPi!" << std::endl;
        exit(1);
    }

    // Set the relay pin as an output
    pinMode(RELAY_PIN1, OUTPUT);
    pinMode(RELAY_PIN2, OUTPUT);
    pinMode(RELAY_PIN3, OUTPUT);

    pinMode(BATTERY1_FAN_PIN, OUTPUT);
    pinMode(BATTERY2_FAN_PIN, OUTPUT);
    pinMode(BATTERY3_FAN_PIN, OUTPUT);

    pinMode(RESISTER_FAN_PIN, OUTPUT);

    pinMode(BATTERY1_PWM_PIN, OUTPUT);
    pinMode(BATTERY2_PWM_PIN, OUTPUT);
    pinMode(BATTERY3_PWM_PIN, OUTPUT);

    // Set the initial state of the relay to off (LOW)
    digitalWrite(RELAY_PIN1, LOW);
    digitalWrite(RELAY_PIN2, LOW);
    digitalWrite(RELAY_PIN3, LOW);
    std::cout << "Relay initialized to off state." << std::endl;

    wiringPiSetup();

    softPwmCreate(BATTERY1_FAN_PIN, 0, 100);   //battery1 fan
    softPwmCreate(BATTERY2_FAN_PIN, 0, 100);   //battery2 fan
    softPwmCreate(BATTERY3_FAN_PIN, 0, 100);  //battery3 fan

    softPwmCreate(RESISTER_FAN_PIN, 0, 100);  //resister fan

    softPwmCreate(BATTERY1_PWM_PIN, 0, 100);   //battery1 charge control
    softPwmCreate(BATTERY2_PWM_PIN, 0, 100);  //battery2 charge control
    softPwmCreate(BATTERY3_PWM_PIN, 0, 100);  //battery3 charge control
}

double readTemperature(const std::string& sensorPath) { //read temperature data from file
    std::ifstream file(sensorPath);
    std::string line;
    double temperature = 0.0;

    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (line.find(" t=") != std::string::npos) {
                std::string tempStr = line.substr(line.find(" t=") + 3);
                temperature = std::stod(tempStr) / 1000.0;
                break;
            }
        }
        file.close();
    }
    else {
        std::cerr << "Could not open sensor file: " << sensorPath << "\n";
    }

    return temperature;
}

void controlRelay(char command, int relay_num) {
    if (command == '1') {
        if(relay_num == 1) {
            std::cout << "Turning the relay_1 ON" << std::endl;
            digitalWrite(RELAY_PIN1, HIGH); // Turn the relay ON
            relay_state[RELAY_PIN1] = 1;
        } else if(relay_num == 2) {
            std::cout << "Turning the relay_2 ON" << std::endl;
            digitalWrite(RELAY_PIN2, HIGH); // Turn the relay ON
            relay_state[RELAY_PIN2] = 1;
        } else if(relay_num == 3) {
            std::cout << "Turning the relay_3 ON" << std::endl;
            digitalWrite(RELAY_PIN3, HIGH); // Turn the relay ON
            relay_state[RELAY_PIN3] = 1;
        } else std::cout << "Invalid relay order" << std::endl;
    }
    else if (command == '0') {
        if(relay_num == 1) {
            std::cout << "Turning the relay_1 OFF" << std::endl;
            digitalWrite(RELAY_PIN1, LOW); // Turn the relay OFF
            relay_state[RELAY_PIN1] = 0;
        } else if(relay_num == 2) {
            std::cout << "Turning the relay_2 OFF" << std::endl;
            digitalWrite(RELAY_PIN2, LOW); // Turn the relay OFF
            relay_state[RELAY_PIN2] = 0;
        } else if(relay_num == 3) {
            std::cout << "Turning the relay_3 OFF" << std::endl;
            digitalWrite(RELAY_PIN3, LOW); // Turn the relay OFF
            relay_state[RELAY_PIN3] = 0;
        } else std::cout << "Invalid relay order" << std::endl;
    }
    else {
        std::cerr << "Invalid command!" << std::endl;
    }
}

float calculate_SoC(float voltage, int relay_state) {    //estimate SoC by voltage
    if(relay_state == 1) voltage += (voltage - 2.9) * 1.3;
    if (voltage >= 4.20) return 100.0;
    else if (voltage >= 4.08) {
        if (voltage >= 4.16) return 98.0;
        else if (voltage >= 4.14) return 96.0;
        else if (voltage >= 4.12) return 94.0;
        else if (voltage >= 4.10) return 92.0;
        else return 90.0;
    }
    else if (voltage >= 3.96) {
        if (voltage >= 4.06) return 88.0;
        else if (voltage >= 4.04) return 86.0;
        else if (voltage >= 4.02) return 84.0;
        else if (voltage >= 3.99) return 82.0;
        else return 80.0;
    }
    else if (voltage >= 3.85) {
        if (voltage >= 3.94) return 78.0;
        else if (voltage >= 3.91) return 76.0;
        else if (voltage >= 3.89) return 74.0;
        else if (voltage >= 3.87) return 72.0;
        else return 70.0;
    }
    else if (voltage >= 3.75) {
        if (voltage >= 3.83) return 68.0;
        else if (voltage >= 3.81) return 66.0;
        else if (voltage >= 3.79) return 64.0;
        else if (voltage >= 3.77) return 62.0;
        else return 60.0;
    }
    else if (voltage >= 3.65) {
        if (voltage >= 3.73) return 58.0;
        else if (voltage >= 3.71) return 56.0;
        else if (voltage >= 3.69) return 54.0;
        else if (voltage >= 3.67) return 52.0;
        else return 50.0;
    }
    else if (voltage >= 3.6) {
        if (voltage >= 3.64) return 48.0;
        else if (voltage >= 3.63) return 46.0;
        else if (voltage >= 3.62) return 44.0;
        else if (voltage >= 3.61) return 42.0;
        else return 40.0;
    }
    else if (voltage >= 3.55) {
        if (voltage >= 3.59) return 38.0;
        else if (voltage >= 3.58) return 36.0;
        else if (voltage >= 3.57) return 34.0;
        else if (voltage >= 3.56) return 32.0;
        else return 30.0;
    }
    else if (voltage >= 3.5) {
        if (voltage >= 3.54) return 28.0;
        else if (voltage >= 3.53) return 26.0;
        else if (voltage >= 3.52) return 24.0;
        else if (voltage >= 3.51) return 22.0;
        else return 20.0;
    }
    else if (voltage >= 3.43) {
        if (voltage >= 3.48) return 18.0;
        else if (voltage >= 3.46) return 16.0;
        else if (voltage >= 3.44) return 14.0;
        else if (voltage >= 3.42) return 12.0;
        else return 10.0;
    }
    else {
        if (voltage >= 3.4) return 8.0;
        else if (voltage >= 3.3) return 6.0;
        else if (voltage >= 3.1) return 4.0;
        else if (voltage >= 3.0) return 2.0;
        else return 0.0;
    }
}

void control_fan_speed(float temperature[], int fan_pwm[]) { //pwm fan control by temperature
    while (true) {
        mtx.lock();
        temperature[0] = readTemperature(BAT1_TEMP_ADDR);
      temperature[1] = readTemperature(BAT2_TEMP_ADDR);
      temperature[2] = readTemperature(BAT3_TEMP_ADDR);
      temperature[3] = readTemperature(RESISTER1_TEMP_ADDR);
      temperature[4] = readTemperature(RESISTER2_TEMP_ADDR);
      temperature[5] = readTemperature(RESISTER3_TEMP_ADDR);
        mtx.unlock();
        
        int fan_speed = 0;

        //bat1
        if (temperature[0] <= 20.0) {   //all 3 batteries each 20'C->0%, 40'C->100%
            fan_speed = 0;
        }
        else if (temperature[0] >= 40.0) {
            fan_speed = 100;
        }
        else {
            fan_speed = static_cast<int>((temperature[0] - 20.0) / 20.0 * 100);
        }

        softPwmWrite(BATTERY1_FAN_PIN, fan_speed);
        fan_pwm[0]=fan_speed;
        printf("battery-1 Temperature: %.2f C, Fan Speed: %d\n", temperature[0], fan_speed);

        //bat2
        if (temperature[1] <= 20.0) {
            fan_speed = 0;
        }
        else if (temperature[1] >= 40.0) {
            fan_speed = 100;
        }
        else {
            fan_speed = static_cast<int>((temperature[1] - 20.0) / 20.0 * 100);
        }

        softPwmWrite(BATTERY2_FAN_PIN, fan_speed);
        fan_pwm[1]=fan_speed;
        printf("battery-2 Temperature: %.2f C, Fan Speed: %d\n", temperature[1], fan_speed);

        //bat3
        if (temperature[2] <= 20.0) {
            fan_speed = 0;
        }
        else if (temperature[2] >= 40.0) {
            fan_speed = 100;
        }
        else {
            fan_speed = static_cast<int>((temperature[2] - 20.0) / 20.0 * 100);
        }

        softPwmWrite(BATTERY3_FAN_PIN, fan_speed);
        fan_pwm[2]=fan_speed;
        printf("battery-3 Temperature: %.2f C, Fan Speed: %d\n", temperature[2], fan_speed);

        float max_temp = temperature[3];

        if (temperature[4] > max_temp) max_temp = temperature[4];
        if (temperature[5] > max_temp) max_temp = temperature[5];

        if (max_temp <= 20.0) { //20~50 20'C->0%, 50'C->100%
            fan_speed = 0;
        }
        else if (max_temp >= 50.0) {
            fan_speed = 100;
        }
        else {
            fan_speed = static_cast<int>((max_temp - 20.0) / 30.0 * 100);
        }

        softPwmWrite(RESISTER_FAN_PIN, fan_speed);
        fan_pwm[3]=fan_speed;
        printf("Discharge Resistor Max Temperature: %.2f C, Fan Speed: %d\n", max_temp, fan_speed);
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void control_charging(TCA9548A& sensor, float temperature[], float bat_data[]) {
    int tca_fd = wiringPiI2CSetup(TCA_ADDR);
    int duty_cycle1 = 0;
    int duty_cycle2 = 0;
    int duty_cycle3 = 0;
    int counter = 0;
    std::vector<float> current1;
    std::vector<float> voltage1;
    std::vector<float> current2;
    std::vector<float> voltage2;
    std::vector<float> current3;
    std::vector<float> voltage3;

    std::vector<int> SoC_array_1;
    std::vector<int> SoC_array_2;
    std::vector<int> SoC_array_3;
    
    int soc_1 = 0;
    int soc_2 = 0;
    int soc_3 = 0;
    while (true) {

        //mtx.lock();

        //bat1
        selectTCA9548AChannel(tca_fd, 5);

        int charge_mode[3];
        int count = 0;
        
        

        if (relay_state[RELAY_PIN1] == 1) {    //stop charging while discharging
            charge_mode[0] = STOP_CHARGING;
            duty_cycle1 = 0;
            softPwmWrite(BATTERY1_PWM_PIN, duty_cycle1);
            
            float shuntVoltage = sensor.readShuntVoltage();

            current1.push_back(sensor.readCurrent());
            if (current1.size() > 10) {
                current1.erase(current1.begin());
            }
            float avg_current = 0.0;
            count = 0;
            for (float amph : current1) {
                if(abs(amph) >= duty_cycle1){
                    avg_current += amph;
                    count++;
                }
            }
            avg_current /= count;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            voltage1.push_back(sensor.readBusVoltage());
            if (voltage1.size() > 10) voltage1.erase(voltage1.begin());

            float avg_voltage = 0.0;
            count = 0;
            for (float volt : voltage1) {
                if(volt >= 0.1){
                    avg_voltage += volt;
                    count++;
                }
            }
            avg_voltage /= count;

            //calculate SoC
            SoC_array_1.push_back(calculate_SoC(avg_voltage, relay_state[RELAY_PIN1]));
            if(SoC_array_1.size() > 5) SoC_array_1.erase(SoC_array_1.begin());
                
            soc_1 = 0;
            for(int soc : SoC_array_1) {
                soc_1 += soc;
            }
            soc_1 /= SoC_array_1.size();
                
            std::cout << std::fixed;
            std::cout.precision(2);
            std::cout << "Battery(1) Voltage: " << avg_voltage << " V, Current: " << avg_current << " mA, SoC: " << soc_1 << "%, duty cycle: " << duty_cycle1 << "%" << std::endl;
            bat_data[0] = avg_voltage;
            bat_data[1] = avg_current;
            bat_data[2] = soc_1;
            bat_data[3] = duty_cycle1;
        }
        else {  //set charging mode by temperature
            if (temperature[0] > MAX_CRITICAL_TEMPERATURE) {   //over 60'C stop charging
                charge_mode[0] = STOP_CHARGING;
                duty_cycle1 = 0;
                softPwmWrite(BATTERY1_PWM_PIN, duty_cycle1);
            }
            else if (soc_1 == 100) { charge_mode[0] = STOP_CHARGING; duty_cycle1 = 0; softPwmWrite(BATTERY1_PWM_PIN, duty_cycle1); }
            else if (temperature[0] > MAX_SAFE_TEMPERATURE) {  //over 50'C slow down charging
                charge_mode[0] = STANDARD_CHARGING;
            }
            else {
                charge_mode[0] = FAST_CHARGING;
            }
            bat_data[4] = charge_mode[0];

            if (charge_mode[0] != STOP_CHARGING) {
                float target_current = (charge_mode[0] == FAST_CHARGING) ? 1000.0 : 500.0;
                float shuntVoltage = sensor.readShuntVoltage();

                current1.push_back(sensor.readCurrent());
                if (current1.size() > 10) {
                    current1.erase(current1.begin());
                }
                float avg_current = 0.0;
                count = 0;
                for (float amph : current1) {
                    if(abs(amph) >= duty_cycle1){
                        avg_current += amph;
                        count++;
                    }
                }
                avg_current /= count;

                softPwmWrite(BATTERY1_PWM_PIN, 0);   //shut charging for measure voltage for SoC
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                voltage1.push_back(sensor.readBusVoltage());
                if (voltage1.size() > 10) voltage1.erase(voltage1.begin());

                float avg_voltage = 0.0;
                count = 0;
                for (float volt : voltage1) {
                    if(volt >= 0.1){
                        avg_voltage += volt;
                        count++;
                    }
                }
                avg_voltage /= count;

                //calculate SoC
                SoC_array_1.push_back(calculate_SoC(avg_voltage, relay_state[RELAY_PIN1]));
                if(SoC_array_1.size() > 5) SoC_array_1.erase(SoC_array_1.begin());
                
                soc_1 = 0;
                for(int soc : SoC_array_1) {
                    soc_1 += soc;
                }
                soc_1 /= SoC_array_1.size();
                
                std::cout << std::fixed;
                std::cout.precision(2);
                std::cout << "Battery(1) Voltage: " << avg_voltage << " V, Current: " << avg_current << " mA, SoC: " << soc_1 << "%, duty cycle: " << duty_cycle1 << "%" << std::endl;
                bat_data[0] = avg_voltage;
                bat_data[1] = avg_current;
                bat_data[2] = soc_1;
                bat_data[3] = duty_cycle1;

                softPwmWrite(BATTERY1_PWM_PIN, duty_cycle1);  //charging continue

                if (avg_voltage < TARGET_VOLTAGE) {
                    //CC charging
                    if (avg_current < target_current) {
                        duty_cycle1 = std::min(100, (duty_cycle1 + 1) + counter); //increase duty-cycle
                    }
                    else {
                        duty_cycle1 = std::max(0, (duty_cycle1 - 1) - counter);   //decrease duty-cycle
                    }
                }
                else {
                    //CV charging
                    duty_cycle1 = std::max(0, (duty_cycle1 - 1) + counter);   //decrease duty-cycle
                }
                counter = std::max(0, counter - 1);
                softPwmWrite(BATTERY1_PWM_PIN, duty_cycle1);
            }
        }

        //bat2
        selectTCA9548AChannel(tca_fd, 6);

        if (relay_state[RELAY_PIN2] == 1) {    //stop charging while discharging
            charge_mode[1] = STOP_CHARGING;
            duty_cycle2 = 0;
            softPwmWrite(BATTERY2_PWM_PIN, duty_cycle2);
            
            float shuntVoltage = sensor.readShuntVoltage();

            current2.push_back(sensor.readCurrent());
            if (current2.size() > 10) {
                current2.erase(current2.begin());
            }
            float avg_current = 0.0;
            count = 0;
            for (float amph : current2) {
                if(abs(amph) >= duty_cycle2){
                    avg_current += amph;
                    count++;
                }
            }
            avg_current /= count;

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            voltage2.push_back(sensor.readBusVoltage());
            if (voltage2.size() > 10) voltage2.erase(voltage2.begin());

            float avg_voltage = 0.0;
            count=0;
            for (float volt : voltage2) {
                if(volt >= 0.1){
                    avg_voltage += volt;
                    count++;
                }
            }
            avg_voltage /= count;

            //calculate SoC
            SoC_array_2.push_back(calculate_SoC(avg_voltage, relay_state[RELAY_PIN2]));
            if(SoC_array_2.size() > 5) SoC_array_2.erase(SoC_array_2.begin());
                
            soc_2 = 0;
            for(int soc : SoC_array_2) {
                soc_2 += soc;
            }
            soc_2 /= SoC_array_2.size();
                
            std::cout << std::fixed;
            std::cout.precision(2);
            std::cout << "Battery(2) Voltage: " << avg_voltage << " V, Current: " << avg_current << " mA, SoC: " << soc_2 << "%, duty cycle: " << duty_cycle2 << "%" << std::endl;
            bat_data[5] = avg_voltage;
            bat_data[6] = avg_current;
            bat_data[7] = soc_2;
            bat_data[8] = duty_cycle2;
        }
        else {  //set charging mode by temperature
            if (temperature[1] > MAX_CRITICAL_TEMPERATURE) {   //over 60'C stop charging
                charge_mode[1] = STOP_CHARGING;
                duty_cycle2 = 0;
                softPwmWrite(BATTERY2_PWM_PIN, duty_cycle2);
            }
            else if (soc_2 == 100) { charge_mode[1] = STOP_CHARGING; duty_cycle2 = 0; softPwmWrite(BATTERY2_PWM_PIN, duty_cycle2); }
            else if (temperature[1] > MAX_SAFE_TEMPERATURE) {  //over 50'C slow down charging
                charge_mode[1] = STANDARD_CHARGING;
            }
            else {
                charge_mode[1] = FAST_CHARGING;
            }
            bat_data[9] = charge_mode[1];

            if (charge_mode[1] != STOP_CHARGING) {
                float target_current = (charge_mode[1] == FAST_CHARGING) ? 1000.0 : 500.0;
                float shuntVoltage = sensor.readShuntVoltage();

                current2.push_back(sensor.readCurrent());
                if (current2.size() > 10) {
                    current2.erase(current2.begin());
                }
                float avg_current = 0.0;
                count = 0;
                for (float amph : current2) {
                    if(abs(amph) >= duty_cycle2){
                        avg_current += amph;
                        count++;
                    }
                }
                avg_current /= count;

                softPwmWrite(BATTERY2_PWM_PIN, 0);   //shut charging for measure voltage for SoC
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                voltage2.push_back(sensor.readBusVoltage());
                if (voltage2.size() > 10) voltage2.erase(voltage2.begin());

                float avg_voltage = 0.0;
                count=0;
                for (float volt : voltage2) {
                    if(volt >= 0.1){
                        avg_voltage += volt;
                        count++;
                    }
                }
                avg_voltage /= count;

                //calculate SoC
                SoC_array_2.push_back(calculate_SoC(avg_voltage, relay_state[RELAY_PIN2]));
                if(SoC_array_2.size() > 5) SoC_array_2.erase(SoC_array_2.begin());
                
                soc_2 = 0;
                for(int soc : SoC_array_2) {
                    soc_2 += soc;
                }
                soc_2 /= SoC_array_2.size();
                
                std::cout << std::fixed;
                std::cout.precision(2);
                std::cout << "Battery(2) Voltage: " << avg_voltage << " V, Current: " << avg_current << " mA, SoC: " << soc_2 << "%, duty cycle: " << duty_cycle2 << "%" << std::endl;
                bat_data[5] = avg_voltage;
                bat_data[6] = avg_current;
                bat_data[7] = soc_2;
                bat_data[8] = duty_cycle2;

                softPwmWrite(BATTERY2_PWM_PIN, duty_cycle2);  //charging continue

                if (avg_voltage < TARGET_VOLTAGE) {
                    //CC charging
                    if (avg_current < target_current) {
                        duty_cycle2 = std::min(100, (duty_cycle2 + 1) + counter); //increase duty-cycle
                    }
                    else {
                        duty_cycle2 = std::max(0, (duty_cycle2 - 1) - counter);   //decrease duty-cycle
                    }
                }
                else {
                    //CV charging
                    duty_cycle2 = std::max(0, (duty_cycle2 - 1) + counter);   //decrease duty-cycle
                }
                counter = std::max(0, counter - 1);
                softPwmWrite(BATTERY2_PWM_PIN, duty_cycle2);
            }
        }

        //bat3
        selectTCA9548AChannel(tca_fd, 7);

        if (relay_state[RELAY_PIN3] == 1) {    //stop charging while discharging
            charge_mode[2] = STOP_CHARGING;
            duty_cycle3 = 0;
            softPwmWrite(BATTERY3_PWM_PIN, duty_cycle3);
            
            float shuntVoltage = sensor.readShuntVoltage();

            current3.push_back(sensor.readCurrent());
            if (current3.size() > 10) {
                current3.erase(current3.begin());
            }
            float avg_current = 0.0;
            count = 0;
            for (float amph : current3) {
                if(abs(amph) >= duty_cycle3){
                    avg_current += amph;
                    count++;
                }
            }
            avg_current /= count;

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            voltage3.push_back(sensor.readBusVoltage());
            if (voltage3.size() > 10) voltage3.erase(voltage3.begin());

            float avg_voltage = 0.0;
            count = 0;
            for (float volt : voltage3) {
                if(volt >= 0.1){
                    avg_voltage += volt;
                    count++;
                }
            }
            avg_voltage /= count;

            //calculate SoC
            SoC_array_3.push_back(calculate_SoC(avg_voltage, relay_state[RELAY_PIN3]));
            if(SoC_array_3.size() > 5) SoC_array_3.erase(SoC_array_3.begin());
            
            soc_3 = 0;
            for(int soc : SoC_array_3) {
                soc_3 += soc;
            }
            soc_3 /= SoC_array_3.size();
                
            std::cout << std::fixed;
            std::cout.precision(2);
            std::cout << "Battery(3) Voltage: " << avg_voltage << " V, Current: " << avg_current << " mA, SoC: " << soc_3 << "%, duty cycle: " << duty_cycle3 << "%" << std::endl;
            bat_data[10] = avg_voltage;
            bat_data[11] = avg_current;
            bat_data[12] = soc_3;
            bat_data[13] = duty_cycle3;
        }
        else {  //set charging mode by temperature
            if (temperature[2] > MAX_CRITICAL_TEMPERATURE) {   //over 60'C stop charging
                charge_mode[2] = STOP_CHARGING;
                duty_cycle3 = 0;
                softPwmWrite(BATTERY3_PWM_PIN, duty_cycle3);
            }
            else if (soc_3 == 100) { charge_mode[2] = STOP_CHARGING; duty_cycle3 = 0; softPwmWrite(BATTERY3_PWM_PIN, duty_cycle3); }
            else if (temperature[2] > MAX_SAFE_TEMPERATURE) {  //over 50'C slow down charging
                charge_mode[2] = STANDARD_CHARGING;
            }
            else {
                charge_mode[2] = FAST_CHARGING;
            }
            bat_data[14] = charge_mode[2];

            if (charge_mode[2] != STOP_CHARGING) {
                float target_current = (charge_mode[2] == FAST_CHARGING) ? 1000.0 : 500.0;
                float shuntVoltage = sensor.readShuntVoltage();

                current3.push_back(sensor.readCurrent());
                if (current3.size() > 10) {
                    current3.erase(current3.begin());
                }
                float avg_current = 0.0;
                count = 0;
                for (float amph : current3) {
                    if(abs(amph) >= duty_cycle3){
                        avg_current += amph;
                        count++;
                    }
                }
                avg_current /= count;

                softPwmWrite(BATTERY3_PWM_PIN, 0);   //shut charging for measure voltage for SoC
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                voltage3.push_back(sensor.readBusVoltage());
                if (voltage3.size() > 10) voltage3.erase(voltage3.begin());

                float avg_voltage = 0.0;
                count = 0;
                for (float volt : voltage3) {
                    if(volt >= 0.1){
                        avg_voltage += volt;
                        count++;
                    }
                }
                avg_voltage /= count;

                //calculate SoC
                SoC_array_3.push_back(calculate_SoC(avg_voltage, relay_state[RELAY_PIN3]));
                if(SoC_array_3.size() > 5) SoC_array_3.erase(SoC_array_3.begin());
                
                soc_3 = 0;
                for(int soc : SoC_array_3) {
                    soc_3 += soc;
                }
                soc_3 /= SoC_array_3.size();
                
                std::cout << std::fixed;
                std::cout.precision(2);
                std::cout << "Battery(3) Voltage: " << avg_voltage << " V, Current: " << avg_current << " mA, SoC: " << soc_3 << "%, duty cycle: " << duty_cycle3 << "%" << std::endl;
                bat_data[10] = avg_voltage;
                bat_data[11] = avg_current;
                bat_data[12] = soc_3;
                bat_data[13] = duty_cycle3;

                softPwmWrite(BATTERY3_PWM_PIN, duty_cycle3);  //charging continue

                if (avg_voltage < TARGET_VOLTAGE) {
                    //CC charging
                    if (avg_current < target_current) {
                        duty_cycle3 = std::min(100, (duty_cycle3 + 1) + counter); //increase duty-cycle
                    }
                    else {
                        duty_cycle3 = std::max(0, (duty_cycle3 - 1) - counter);   //decrease duty-cycle
                    }
                }
                else {
                    //CV charging
                    duty_cycle3 = std::max(0, (duty_cycle3 - 1) + counter);   //decrease duty-cycle
                }
                counter = std::max(0, counter - 1);
                softPwmWrite(BATTERY3_PWM_PIN, duty_cycle3);
            }
        }
        //mtx.unlock();
    }
}

void send_data(float bat_data[], float temperature[], int fan_pwm[], int relay_state[], int sock){
    while(1){
        char buffer[1024];
        int length = snprintf(buffer, sizeof(buffer), "{\"voltage_1\": %.2f, \"current_1\": %.2f, \"soc_1\": %d, \"temperature_1\": %.2f, \"charge_mode_1\": %d, \"relay_state_1\": %d, \"fan_pwm_1\": %d, \"duty_cycle1\": %d, "
        "\"voltage_2\": %.2f, \"current_2\": %.2f, \"soc_2\": %d, \"temperature_2\": %.2f, \"charge_mode_2\": %d, \"relay_state_2\": %d, \"fan_pwm_2\": %d, \"duty_cycle2\": %d, "
        "\"voltage_3\": %.2f, \"current_3\": %.2f, \"soc_3\": %d, \"temperature_3\": %.2f, \"charge_mode_3\": %d, \"relay_state_3\": %d, \"fan_pwm_3\": %d, \"duty_cycle3\": %d, "
        "\"resister_temp_1\": %.2f, \"resister_temp_2\": %.2f, \"resister_temp_3\": %.2f, \"resister_fan_pwm\": %d}",
        bat_data[0], bat_data[1], (int)bat_data[2], temperature[0], (int)bat_data[4], relay_state[0], fan_pwm[0], (int)bat_data[3],
        bat_data[5], bat_data[6], (int)bat_data[7], temperature[1], (int)bat_data[9], relay_state[2], fan_pwm[1], (int)bat_data[8],
        bat_data[10], bat_data[11], (int)bat_data[12], temperature[2], (int)bat_data[14], relay_state[3], fan_pwm[2], (int)bat_data[13],
        temperature[3], temperature[4], temperature[5], fan_pwm[3]);
        
        if(length < 0 || length >= sizeof(buffer)){
            std::cerr << "error: buffer size in insufficient" << std::endl;
        } else {
            std::cout << "json generated" << buffer << std::endl;
            send(sock, buffer, strlen(buffer), 0);
            std::cout << "Data sent to the server" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void receive_data(int sock) {
    char buffer[4] = {0};

    while (true) {
        int buffLength = read(sock, buffer, sizeof(buffer) - 1);
        if (buffLength > 0) {
            std::cout << "Data received from the server: " << buffer[0] << "-" << buffer[1] << "-" << buffer[2] << std::endl;
            for(int i = 0;i < 3;i++) {
                controlRelay(buffer[i], i + 1);
            }
        }
    }
}

int main() {
    setup();    //rasp sensor, pin setup;

    float temperature[6];
    float bat_data[15];
    int fan_pwm[4];
    int sock = 0;
    struct sockaddr_in server_addr;
    
    TCA9548A ina219(0x40);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return 1;
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return 1;
    }
    
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return 1;
    }
    std::cout << "1" << std::endl;
    std::thread ctrlFanThread(control_fan_speed, temperature, fan_pwm);
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    std::cout << "read done" << std::endl;
    
    std::thread ctrlChargingBatThread(control_charging, std::ref(ina219), temperature, bat_data);
    std::thread sendThread(send_data, bat_data, temperature, fan_pwm, relay_state, sock);
    std::thread receiveThread(receive_data, sock);
    
    while (1) {
        printf("----------------\nsend data %.2f, %.2f, %d, %d, %d\n-----------------\n", bat_data[0], bat_data[1], (int)bat_data[2], (int)bat_data[3], (int)bat_data[4]);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    ctrlFanThread.join();
    ctrlChargingBatThread.join();
    sendThread.join();
    receiveThread.join();
    
    return 0;
}
