#include <wiringPiI2C.h>
#include <wiringPi.h>
#include <stdio.h>

#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <pthread.h> 

//BlueZ
#include "bluez-5.52/lib/bluetooth.h"
#include "bluez-5.52/lib/bluetooth/hci.h"
#include "bluez-5.52/lib/bluetooth/hci_lib.h"
#include "bluez-5.52/lib/l2cap.h"

#include <vector>
#include <algorithm>
#include <unordered_map>

#include <string>

int fd;

void displayToggleEnable(int bits)
{
  delayMicroseconds(500);
  wiringPiI2CReadReg8(fd, (bits | 0b00000100));
  delayMicroseconds(500);
  wiringPiI2CReadReg8(fd, (bits & ~0b00000100));
  delayMicroseconds(500);
}

void sendBytes(int bits, int mode)
{
  int highBits;
  int lowBits;

  highBits = mode | (bits & 0xF0) | 0x08;
  lowBits = mode | ((bits << 4) & 0xF0) | 0x08;

  wiringPiI2CReadReg8(fd, highBits);
  displayToggleEnable(highBits);

  wiringPiI2CReadReg8(fd, lowBits);
  displayToggleEnable(lowBits);
}

int initializeDisplay()
{
    if(wiringPiSetup() == -1)
        return -1;

    fd = wiringPiI2CSetup(0x27);

      sendBytes(0x33, 0);
      sendBytes(0x32, 0);
      sendBytes(0x06, 0);
      sendBytes(0x0C, 0);
      sendBytes(0x28, 0);
      sendBytes(0x01, 0);
      delayMicroseconds(500);
      return 0;
}

void clearDisplay()
{
  sendBytes(0x01, 0);
  sendBytes(0x02, 0);
}

int displayWrite(const char *s, int line)
{
  if(line == 0)
    line = 0x80;
  else if (line == 1)
    line = 0xC0;
  else
    return -1;

  sendBytes(line, 0);

  while(*s)
    sendBytes(*(s++), 1);

  return 0;
}


void displayWriteDouble(double value , int line)
{
    char buffer[20];
    sprintf(buffer, "%4.1f", value);
    displayWrite(buffer, line);
}

typedef unsigned char byte;

//Bluetooth Address of BalanceBoard
char WiiFitBoardDeviceAddress[18];

//Sockets
int controlPipe;
int dataPipe;

//Balance Board Calibration Data used in final mass calculation
double calibration0[4];
double calibration17[4];
double calibration34[4];

int frontButtonPressed = 0;
int calcMSG = 0; //Display Calcuating MSG
int lock = 0; //A lock so we don't hit a weight measuring infinte loop

//Sample Data
int samples = 0;
std::vector<double> massSamples;

void requestCalibrationData()
{
    byte calibrateSequence[] = {0x52, 0x17, 0x04, 0xA4, 0x00, 0x24, 0x00, 0x18};
    write(controlPipe, &calibrateSequence, 8);
}

void requestMassData()
{
    byte extensionSequence[] = {0x52, 0x12, 0x04, 0x32}; //0x4 contious reporting
    write(controlPipe, &extensionSequence, 4);
}

void EnableLED()
{
    byte ledEnabledSequence[] = {0x52, 0x11, 0x10};
    write(controlPipe,&ledEnabledSequence, 3);
}

void DisableLED()
{
    byte ledDisabledSequence[] = {0x52, 0x11, 0x00};
    write(controlPipe,&ledDisabledSequence, 3);
}

void processCalibrationData(byte* data, int length)
{
    if(length == 16)
    {
        calibration0[0] = (double)(((data[0] & 0xFF) << 8)+ (data[1] & 0xFF));
        calibration0[1] = (double)(((data[2] & 0xFF) << 8)+ (data[3] & 0xFF));
        calibration0[2] = (double)(((data[4] & 0xFF) << 8)+ (data[5] & 0xFF));
        calibration0[3] = (double)(((data[6] & 0xFF) << 8)+ (data[7] & 0xFF));

        calibration17[0] = (double)(((data[8] & 0xFF) << 8)+ (data[9] & 0xFF));
        calibration17[1] = (double)(((data[10] & 0xFF) << 8)+ (data[11] & 0xFF));
        calibration17[2] = (double)(((data[12] & 0xFF) << 8)+ (data[13] & 0xFF));
        calibration17[3] = (double)(((data[14] & 0xFF) << 8)+ (data[15] & 0xFF));

    }
    else
    {
        calibration34[0] = (double)(((data[0] & 0xFF) << 8)+ (data[1] & 0xFF));
        calibration34[1] = (double)(((data[2] & 0xFF) << 8)+ (data[3] & 0xFF));
        calibration34[2] = (double)(((data[4] & 0xFF) << 8)+ (data[5] & 0xFF));
        calibration34[3] = (double)(((data[6] & 0xFF) << 8)+ (data[7] & 0xFF));
    }
}

int DiscoverWiiFitBoard()
{
    int deviceID = hci_get_route(NULL);
    int sock = hci_open_dev(deviceID);

    if (deviceID < 0 || sock < 0)
        return -1;

    int maxResponses = 255;
    inquiry_info* inquiryInfo = NULL;
    inquiryInfo = (inquiry_info*)malloc(maxResponses * sizeof(inquiry_info));

    int numResponses = hci_inquiry(deviceID, 8, maxResponses, NULL, &inquiryInfo, IREQ_CACHE_FLUSH);

    if(numResponses < 0 )
        return -1;

    for (int i = 0; i < numResponses; i++)
    {
        char name[249];
        char address[18];

        hci_read_remote_name(sock, &(inquiryInfo+i)->bdaddr, sizeof(name), name, 0);
        if(strcmp(name, "Nintendo RVL-WBC-01") == 0)
        {
            ba2str(&(inquiryInfo+i)->bdaddr, address);
            memcpy(WiiFitBoardDeviceAddress, address,sizeof(WiiFitBoardDeviceAddress));
            free(inquiryInfo);
            close(sock);
            return 0;
        }
    }

    free(inquiryInfo);
    close(sock);
    return -1;
}



double calcualteMass(int rawMassData, int calPos)
{
    if(rawMassData < calibration0[calPos])
        return 0.00;

    if(rawMassData < calibration17[calPos])
        return 17 * ((rawMassData - calibration0[calPos]) / (calibration17[calPos] - calibration0[calPos]));

    if(rawMassData > calibration17[calPos])
        return 17 + 17 * ((rawMassData - calibration17[calPos]) / (calibration34[calPos] - calibration17[calPos]));

    return 0.00;
}

int sampleCount = 500;
int maxFreq = 0, freqVal = 0;
bool isFreq(int x)
{
    return x == freqVal;
}

void calcuateWeight(double sample)
{
    if(sample > 25.00 && lock == 1)
        return;

    if(sample < 25.00 && lock == 1)
        lock = 0;

    if(sample > 25.00)
    {
        if(calcMSG == 0 && samples < sampleCount)
        {
            calcMSG = 1;
            clearDisplay();
            displayWrite("Calcuating...", 0);
            sleep(4);
        }

        if(samples == sampleCount)
        {
            printf("Log: Mass Samples Collected\n");

            std::vector<int> wholeNumTable(massSamples.begin(), massSamples.end());
            std::unordered_map<int, int> freqTable;
            for(auto &i : wholeNumTable)
                freqTable[i]++;
            for(auto &i : freqTable)
            {
                if(i.second > maxFreq)
                {
                    maxFreq = i.second;
                    freqVal = i.first;
                }
            }

            std::vector<int>::iterator iter = wholeNumTable.begin();
            double sum = 0.00;

            while ((iter = std::find_if(iter, wholeNumTable.end(), isFreq)) != wholeNumTable.end())
            {
                sum += massSamples.at(std::distance(wholeNumTable.begin(), iter));
                iter++;
            }

            double weight = 0.0;
            weight = sum / maxFreq;

            printf("Log: Weight Calculated: %lf\n",  weight);
            clearDisplay();
            displayWrite("Your Weight:", 0);
            displayWriteDouble(weight, 1);

            //Cleanup
            massSamples.clear();
            maxFreq = 0;
            freqVal = 0;

            sleep(4);
            clearDisplay();
            displayWrite("Please Step On", 0);
            displayWrite("To Begin...", 1);
            samples = 0;
            lock = 1;

        }

        massSamples.push_back(sample);
        samples++;
    }
    else
    {
        if(calcMSG == 1)
        {
            clearDisplay();
            displayWrite("Please Step On", 0);
            displayWrite("To Begin...", 1);
        }

        calcMSG = 0;
        samples = 0;
        return;
    }
}

void processMassData(byte* data, int length)
{
    double rawTopRight = (double)(((data[0] & 0xFF) << 8)+ (data[1] & 0xFF));
    double rawBotRight = (double)(((data[2] & 0xFF) << 8)+ (data[3] & 0xFF));
    double rawTopLeft = (double)(((data[4] & 0xFF) << 8)+ (data[5] & 0xFF));
    double rawBotLeft = (double)(((data[6] & 0xFF) << 8)+ (data[7] & 0xFF));

    double topRight = calcualteMass(rawTopRight, 0);
    double topLeft = calcualteMass(rawTopLeft, 2);
    double bottomRight = calcualteMass(rawBotRight, 1);
    double bottomLeft = calcualteMass(rawBotLeft, 3);

    double total = topLeft + topRight + bottomLeft + bottomRight;

    total *= 2.2046;
    calcuateWeight(total);
}

int shutdownCounter = 0;
void* Listen(void* arg) 
{ 
    massSamples.reserve(sampleCount);

    printf("Log: PiSmartScale Ready...\n");

    clearDisplay();
    displayWrite("Please Step On", 0);
    displayWrite("To Begin...", 1);

    byte memoryBuffer[23] = { 0 };
    while(1)
    {
        int bytes_read = read(dataPipe, memoryBuffer, sizeof(memoryBuffer));

        if( bytes_read > 0 )
        {
            if(memoryBuffer[1] == 0x21) //Calibration Packet Recived
            {
                int dataSize = (memoryBuffer[4] >> 4) + 1;
                byte parsedBuffer[dataSize];
                memcpy(parsedBuffer, memoryBuffer + 7, dataSize); //8th byte starts data
                processCalibrationData(parsedBuffer, dataSize);
                requestMassData();
            }
            if(memoryBuffer[1] == 0x32) //Board Event
            {
                //Front Button Pressed we need to determine when it pushed down and when it comes back up
                if(memoryBuffer[3] == 0x08 && frontButtonPressed == 0) 
                {
                    frontButtonPressed = 1;
                }

                if(memoryBuffer[3] == 0x08 && frontButtonPressed == 1)
                {
                    shutdownCounter++;
                    if(shutdownCounter == 1000)
                    {
                        //Shutdown the board
                    }
                }

                if(memoryBuffer[3] == 0x00 && frontButtonPressed == 1)
                {
                    shutdownCounter = 0;
                    frontButtonPressed = 0;
                }

                //Process Mass Data
                byte parsedBuffer[8];
                memcpy(parsedBuffer, memoryBuffer + 4, 8);
                processMassData(parsedBuffer, 8);
            }
        }
    }
    return(NULL);
}

int ConnectWiiFitBoard()
{

    //Creation & Descritpition of Control Pipe (Tramission End)
    struct sockaddr_l2 addr = { 0 };
    int controlPipeStatus;
    controlPipe = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP); //Proto
    addr.l2_family = AF_BLUETOOTH;
    addr.l2_psm = htobs(0x11); //port#
    str2ba(WiiFitBoardDeviceAddress, &addr.l2_bdaddr); //convient convert func
    controlPipeStatus = connect(controlPipe,(struct sockaddr *)&addr, sizeof(addr));

    //Creation & Description of Data Pipe (Receiving End)
    struct sockaddr_l2 addr2 = { 0 };
    int dataPipeStatus;
    dataPipe = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP); //Proto
    addr2.l2_family = AF_BLUETOOTH;
    addr2.l2_psm = htobs(0x13); //port#
    str2ba(WiiFitBoardDeviceAddress, &addr2.l2_bdaddr);
    dataPipeStatus = connect(dataPipe,(struct sockaddr *)&addr2, sizeof(addr2));


    if(dataPipeStatus == 0 && controlPipeStatus == 0) 
    {
        printf("Log: Connected to Bluetooth Device\n");
        clearDisplay();
        displayWrite("Device Connected", 0);
        sleep(2);
        printf("Log: Spawning Listening Thread\n");
        pthread_t receivingThread; 
        pthread_create(&receivingThread, NULL, &Listen, NULL); //Spawn Reciving Thread
        printf("Log: Enabling LED\n");
        EnableLED();
        printf("Log: Requesting Calibration Data\n");
        requestCalibrationData();
        printf("Log: Calibration Data Completed\n");
        pthread_join(receivingThread, NULL); //Await thread to complete before exiting
        close(controlPipe);
        close(dataPipe);
        return 0;
    }
    else
    {
        printf("Log: Failed to establish a Bluetooth Connection\n");
        clearDisplay();
        displayWrite("Connection Failed", 0);
        displayWrite(WiiFitBoardDeviceAddress, 1);
        close(controlPipe);
        close(dataPipe);
        return -1;
    }
}

int main(int argc, char* argv[])
{
    printf("Log: intializing display\n");
    initializeDisplay();
    clearDisplay();


    displayWrite("Welcome To", 0);
    displayWrite("Pi Smart Scale!", 1);
    sleep(3);

    clearDisplay();
    displayWrite("Discovery Mode",0);
    displayWrite("Seraching...",1);

    printf("Log: Searching For Bluetooth Device...\n");
    while(1)
    {
        if(DiscoverWiiFitBoard() == 0)
        {
            printf("Log: Found Device\n");
            printf("Log: Address: %s\n", WiiFitBoardDeviceAddress);
            clearDisplay();
            displayWrite("Found Device!", 0);
            sleep(2);

            if(ConnectWiiFitBoard() != 0)
                return -1;

            break;
        }
    }
    return 0;
}