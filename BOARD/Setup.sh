#Stuff I didn't want to script
#Change Keyboard To US in /etc/default/keyboard XKBLAYOUT="us"
#Enable I2C via sudo raspi-config

#Enable ssh
sudo systemctl enable ssh
sudo systemctl start ssh

#LCD Dependency
sudo apt-get install wiringpi -y

#Bluetooth Dependency
sudo apt-get install libbluetooth-dev -y

#Build BlueZ from soruce
sudo apt-get install wget
wget www.kernel.org/pub/linux/bluetooth/bluez-5.52.tar.xz
tar xvf bluez-5.52.tar.xz
rm bluez-5.52.tar.xz
cd bluez-5.52
sudo apt-get install -y libusb-dev libdbus-1-dev libglib2.0-dev libudev-dev libical-dev libreadline-dev
./configure --enable-library
make
sudo make install

#Install SQL Client
sudo apt-get install libssl-dev -y
sudo apt-get install mariadb-client -y
sudo apt-get install libmariadb-dev -y

#Compile Board Logic using g++
g++ -I/usr/include/mariadb -lmariadbclient -lpthread -lbluetooth -lwiringPi -o PiSmartScale main.cpp

sudo apt-get install g++ -y
