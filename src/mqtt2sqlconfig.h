/*
    QMQTT2SQL subscribes to a MQTT broker and stores all messages in a PostgreSQL database.
    Copyright (C) 2024  Thomas Zimmermann

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MQTT2SQLCONFIG_H
#define MQTT2SQLCONFIG_H

#include <QSettings>
#include <QMqttClient>
#include <chrono>

class Mqtt2SqlConfig
{
public:
    Mqtt2SqlConfig();

    bool parse(const QString & configFile);

    bool isValid() const { return m_settings != nullptr; }
    const QString & lastError() const { return m_lastError; }
    const QString & mqttHostname() const { return m_mqttHostname; }
    quint16 mqttPort() const { return m_mqttPort; }
    const QString & mqttUsername() const { return m_mqttUsername; }
    const QString & mqttPassword() const { return m_mqttPassword; }
    QMqttClient::ProtocolVersion mqttVersion() const { return m_mqttVersion; }
    bool mqttUseTls() const { return m_mqttUseTls; }
    const QString & mqttTopic() const  { return m_mqttTopic; }

    const QString & sqlHostname() const { return m_sqlHostname; }
    quint16 sqlPort() const { return m_sqlPort; }
    const QString & sqlUsername() const { return m_sqlUsername; }
    const QString & sqlPassword() const { return m_sqlPassword; }
    const QString & sqlDatabase() const { return m_sqlDatabase; }
    std::chrono::hours sqlMaxStroageTime() const { return m_sqlMaxStorageTime; }

private:
    QSettings * m_settings;
    QString m_lastError;
    QString m_mqttHostname;
    quint16 m_mqttPort = 8883;
    QString m_mqttUsername;
    QString m_mqttPassword;
    QMqttClient::ProtocolVersion m_mqttVersion = QMqttClient::MQTT_3_1;
    bool m_mqttUseTls = false;
    QString m_mqttTopic;

    QString m_sqlHostname;
    quint16 m_sqlPort = 5432;
    QString m_sqlUsername;
    QString m_sqlPassword;
    QString m_sqlDatabase;
    std::chrono::hours m_sqlMaxStorageTime;
};

#endif // MQTT2SQLCONFIG_H
