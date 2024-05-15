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

    QStringList groups = m_settings->childGroups();
    for (const QString & group : groups)
    {
        m_settings->beginGroup(group);
        MqttTopicConfig c;
        c.topic = m_settings->value("topic").toString();
        c.jsonquery = m_settings->value("jsonquery").toString();
        c.type = QVariant::nameToType(m_settings->value("type").toString().toStdString().c_str());
        c.scale = std::numeric_limits<float>::quiet_NaN();
        bool ok;
        float tmp = m_settings->value("scale").toFloat(&ok);
        if (ok)
        {
            c.scale = tmp;
        }
        c.group = m_settings->value("group").toString();
        c.name = m_settings->value("name").toString();
        m_mqttTopicConfig.append(c);

        m_settings->endGroup();
    }
    m_settings->endGroup();

    return true;
}
