# QMQTT2SQL
QMQTT2SQL subscribes to a MQTT broker and stores all messages in a PostgreSQL database.
It is publiched under the GPL3.0+ license.

## Dependencies
| Dependency  | Link                                                             |
| ----------- | ---------------------------------------------------------------- |
| MQTT:       | https://mqtt.org/                                                |
| QtMQTT:     | https://doc.qt.io/qt-5/qtmqtt-index.html                         |

### Install QtMQTT on raspbian

```Bash
sudo apt-get install qtbase5-private-dev qt5-qtwebsockets-dev
git clone https://github.com/qt/qtmqtt.git --branch 5.15.2
cd qtmqtt
qmake-qt5 qtmqtt.pro
make
sudo make install
```

## Configuration
The configuration is given in an INI file.

The MQTT connection parameters are given by the _hostname_, _port_, _username_, _password_, _version_ and _usetls_ attributes.
If no authentification is needed, _username_ and _password_ need to be empty.
The _version_ attribute excepts three values: 3 for MQTT 3.1, 4 for MQTT 3.1.1 and 5 for MQTT 5.0.
If the MQTT connection is TLS encrypted the _usetls_ attribute should be set to true, false otherwise.

The PostgreSQL connection parameters are given by the _hostname_, _port_, _username_ and _password_ attributes.


```INI
[mqtt]
hostname=example.com
port=8883
username=USER
password=PASSWORT
version=3
usetls=true

[psql]
hostname=example.com
port=123
username=USER
password=PASSWORT
```


