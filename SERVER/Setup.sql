#Change Device to Static IP Via
#sudo nano /etc/dhcpcd.conf

sudo apt install mariadb-server mariadb-client -y
#sudo nano /etc/mysql/mariadb.conf.d/50-server.cnf
#comment out bind-address
#sudo systemctl restart mariadb
#sudo mysql -u root -p
#CREATE USER 'username'@'%' IDENTIFIED BY 'passcode';
#GRANT ALL PRIVILEGES ON *.* TO 'username'@'%';
#FLUSH PRIVILEGES;

CREATE TABLE users (
    id int NOT NULL AUTO_INCREMENT UNIQUE,
    name varchar(255) NOT NULL UNIQUE,
    password varchar(255) NOT NULL,
	PRIMARY KEY (id)
);


CREATE TABLE mass(
	id int NOT NULL,
	date_time varchar(255) NOT NULL,
	measurement FLOAT NOT NULL,
	FOREIGN KEY (id) REFERENCES users (id)
);

#Dummy Data
INSERT INTO users (name, password)
VALUES ('mitch', 'thisWillBeHashed123');

INSERT INTO mass (id, date_time, measurement)
VALUES ('1', '2019-03-10 09:55:05', '129');
