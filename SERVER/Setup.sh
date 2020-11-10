#!/bin/bash 
#This is script is just used for a reference of the setup for the server and dependencies!

#Change Device to Static IP Via
#sudo nano /etc/dhcpcd.conf

sudo apt install mariadb-server mariadb-client -y
#Configuration Setup
#sudo nano /etc/mysql/mariadb.conf.d/50-server.cnf
#comment out bind-address
#sudo systemctl restart mariadb
#sudo mysql -u root -p
#CREATE USER 'e30b67be'@'%' IDENTIFIED BY '25d258f3';
#GRANT ALL PRIVILEGES ON *.* TO 'e30b67be'@'%';
#FLUSH PRIVILEGES;

sudo apt install nodejs npm -y
