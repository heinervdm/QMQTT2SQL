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

#include "mqtt2sqlconfig.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

Mqtt2SqlConfig::Mqtt2SqlConfig()
    : m_settings(nullptr)
{}

bool Mqtt2SqlConfig::parse(const QString &configFile)
{
    m_settings = new QSettings(configFile, QSettings::IniFormat);

    m_settings->beginGroup("psql");
    m_sqlHostname = m_settings->value("hostname").toString();
    m_sqlPort = m_settings->value("port", 5432).toUInt();
    m_sqlUsername = m_settings->value("username").toString();
    m_sqlPassword = m_settings->value("password").toString();
    m_sqlDatabase = m_settings->value("database").toString();
    m_sqlTablePrefix = m_settings->value("prefix", "mqtt").toString();
    m_sqlMaxStorageTime = std::chrono::hours(m_settings->value("maxstoragehours", 7*24).toInt());
    m_settings->endGroup();

    m_settings->beginGroup("mqtt");
    m_mqttHostname = m_settings->value("hostname").toString();
    if (m_mqttHostname.isEmpty())
    {
        m_lastError = "Error: hostname is empty!";
        m_settings->deleteLater();
        m_settings = nullptr;
        return false;
    }
    m_mqttPort = m_settings->value("port", 8883).toUInt();
    m_mqttUsername = m_settings->value("username").toString();
    m_mqttPassword = m_settings->value("password").toString();
    int version = m_settings->value("version", 3).toInt();
    switch (version)
    {
    case 3:
        m_mqttVersion = QMqttClient::MQTT_3_1;
        break;
    case 4:
        m_mqttVersion = QMqttClient::MQTT_3_1_1;
        break;
    case 5:
        m_mqttVersion = QMqttClient::MQTT_5_0;
        break;
    default:
        m_lastError = "Error: invalid MQTT version: " + m_settings->value("version").toString();
        m_settings->deleteLater();
        m_settings = nullptr;
        return false;
    }
    m_mqttUseTls = m_settings->value("usetls", false).toBool();

    QSqlDatabase db = QSqlDatabase::database();
    db.setHostName(sqlHostname());
    db.setDatabaseName(sqlDatabase());
    db.setPort(sqlPort());
    db.setUserName(sqlUsername());
    db.setPassword(sqlPassword());
    if (db.open())
    {
        QSqlQuery query;
        if (!query.exec("CREATE TABLE IF NOT EXISTS " + m_sqlTablePrefix + "_config (sensorId integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY, groupname varchar(100), sensor varchar(100), topic varchar(100), jsonpath varchar(100), datatype varchar(10), scaling real, unit varchar(10), lastdata text);"))
        {
            QTextStream(stderr) << "Error while creating " + m_sqlTablePrefix + "_config table: " << query.lastError().text() << Qt::endl;
        }

        if (query.exec("SELECT sensorId, topic, jsonpath, datatype FROM " + m_sqlTablePrefix + "_config"))
        {
            while (query.next())
            {
                MqttTopicConfig c;
                c.sensorId = query.value(0).toInt();
                c.topic = query.value(1).toString();
                c.jsonpath = query.value(2).toString();
                c.type = QVariant::nameToType(query.value(3).toString().toStdString().c_str());
                m_mqttTopicConfig.append(c);
            }
        }
        else
        {
            QTextStream(stderr) << "Error while getting config from " + m_sqlTablePrefix + "_config table: " << query.lastError().text() << Qt::endl;
        }
    }
    else
    {
        QTextStream(stderr) << "Error: Faild to open database: " << db.lastError().text() << Qt::endl;
        ::exit(2);
    }

    return true;
}
