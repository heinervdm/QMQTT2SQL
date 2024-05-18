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

#include "mqttsubscriber.h"
#include "qtjsonpath.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

/**
 * Convert QMqttClient::ClientError to a descriptive string.
 */
QString qMqttClientErrorToString(QMqttClient::ClientError error)
{
    // Error description taken from: https://doc.qt.io/qt-5/qmqttclient.html#ClientError-enum
    switch (error)
    {
    case QMqttClient::NoError: return "No error occurred."; break;
    case QMqttClient::InvalidProtocolVersion: return "Error: The broker does not accept a connection using the specified protocol version."; break;
    case QMqttClient::IdRejected: return "Error: The client ID is malformed. This might be related to its length."; break;
    case QMqttClient::ServerUnavailable: return "Error: The network connection has been established, but the service is unavailable on the broker side."; break;
    case QMqttClient::BadUsernameOrPassword: return "Error: The data in the username or password is malformed."; break;
    case QMqttClient::NotAuthorized: return "Error: The client is not authorized to connect."; break;

    case QMqttClient::TransportInvalid: return "Error: The underlying transport caused an error. For example, the connection might have been interrupted unexpectedly."; break;
    case QMqttClient::ProtocolViolation: return "Error: The client encountered a protocol violation, and therefore closed the connection."; break;
    case QMqttClient::UnknownError: return "Error: An unknown error occurred."; break;
    case QMqttClient::Mqtt5SpecificError: return "Error: The error is related to MQTT protocol level 5. A reason code might provide more details."; break;
    }
    return QString();
}

/**
 * Convert QMqttClient::ClientState to a descriptive string.
 */
QString qMqttClientStateToString(QMqttClient::ClientState state)
{
    // State description taken from: https://doc.qt.io/qt-5/qmqttclient.html#ClientState-enum
    switch (state)
    {
    case QMqttClient::Disconnected: return "The client is disconnected from the broker."; break;
    case QMqttClient::Connecting: return "A connection request has been made, but the broker has not approved the connection yet."; break;
    case QMqttClient::Connected: return "The client is connected to the broker."; break;
    }
    return QString();
}

/**
 * Convert QMqttSubscription::SubscriptionState to a descriptive string.
 */
QString qMqttSubscriptionState(QMqttSubscription::SubscriptionState state)
{
    // State description taken from: https://doc.qt.io/qt-5/qmqttsubscription.html#SubscriptionState-enum
    switch (state)
    {
    case QMqttSubscription::Unsubscribed: return "The topic has been unsubscribed from."; break;
    case QMqttSubscription::SubscriptionPending: return "A request for a subscription has been sent, but is has not been confirmed by the broker yet."; break;
    case QMqttSubscription::Subscribed: return "The subscription was successful and messages will be received."; break;
    case QMqttSubscription::UnsubscriptionPending: return "A request to unsubscribe from a topic has been sent, but it has not been confirmed by the broker yet."; break;
    case QMqttSubscription::Error: return "An error occured."; break;
    }
    return QString();
}

MqttSubscriber::MqttSubscriber(const Mqtt2SqlConfig & config, QObject *parent)
    : QObject{parent}
    , m_config(config)
{
    QSqlDatabase db = QSqlDatabase::database();
    db.setHostName(config.sqlHostname());
    db.setDatabaseName(config.sqlDatabase());
    db.setPort(config.sqlPort());
    db.setUserName(config.sqlUsername());
    db.setPassword(config.sqlPassword());
    if (db.open())
    {
        {
            QSqlQuery query;
            if (!query.exec("CREATE TABLE IF NOT EXISTS " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::String) + " (stringId integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY, ts timestamp with time zone, sensorId integer, value text)"))
            {
                QTextStream(stderr) << "Error while creating " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::String) + " table: " << query.lastError().text() << Qt::endl;
            }
            if (!query.exec("CREATE INDEX IF NOT EXISTS " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::String) + "_sensorId_idx ON " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::String) + " (sensorId)"))
            {
                QTextStream(stderr) << "Error while creating index: " << query.lastError().text() << Qt::endl;
            }
            if (!query.exec("CREATE INDEX IF NOT EXISTS " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::String) + "_ts_idx ON " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::String) + " (ts)"))
            {
                QTextStream(stderr) << "Error while creating index: " << query.lastError().text() << Qt::endl;
            }
        }
        {
            QSqlQuery query;
            if (!query.exec("CREATE TABLE IF NOT EXISTS " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::Bool) + " (boolId integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY, ts timestamp with time zone, sensorId integer, value boolean);"))
            {
                QTextStream(stderr) << "Error while creating " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::Bool) + " table: " << query.lastError().text() << Qt::endl;
            }
            if (!query.exec("CREATE INDEX IF NOT EXISTS " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::Bool) + "_sensorId_idx ON " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::Bool) + " (sensorId);"))
            {
                QTextStream(stderr) << "Error while creating index: " << query.lastError().text() << Qt::endl;
            }
            if (!query.exec("CREATE INDEX IF NOT EXISTS " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::Bool) + "_ts_idx ON " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::Bool) + " (ts);"))
            {
                QTextStream(stderr) << "Error while creating index: " << query.lastError().text() << Qt::endl;
            }
        }
        {
            QSqlQuery query;
            if (!query.exec("CREATE TABLE IF NOT EXISTS " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::Int) + " (integerId integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY, ts timestamp with time zone, sensorId integer, value integer);"))
            {
                QTextStream(stderr) << "Error while creating " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::Int) + " table: " << query.lastError().text() << Qt::endl;
            }
            if (!query.exec("CREATE INDEX IF NOT EXISTS " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::Int) + "_sensorId_idx ON " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::Int) + " (sensorId);"))
            {
                QTextStream(stderr) << "Error while creating index: " << query.lastError().text() << Qt::endl;
            }
            if (!query.exec("CREATE INDEX IF NOT EXISTS " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::Int) + "_ts_idx ON " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::Int) + " (ts);"))
            {
                QTextStream(stderr) << "Error while creating index: " << query.lastError().text() << Qt::endl;
            }
        }
        {
            QSqlQuery query;
            if (!query.exec("CREATE TABLE IF NOT EXISTS " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::Double) + " (doubleId integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY, ts timestamp with time zone, sensorId integer, value real);"))
            {
                QTextStream(stderr) << "Error while creating " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::Double) + " table: " << query.lastError().text() << Qt::endl;
            }
            if (!query.exec("CREATE INDEX IF NOT EXISTS " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::Double) + "_sensorId_idx ON " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::Double) + " (sensorId);"))
            {
                QTextStream(stderr) << "Error while creating index: " << query.lastError().text() << Qt::endl;
            }
            if (!query.exec("CREATE INDEX IF NOT EXISTS " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::Double) + "_ts_idx ON " + config.sqlTablePrefix() + "_" + QVariant::typeToName(QVariant::Double) + " (ts);"))
            {
                QTextStream(stderr) << "Error while creating index: " << query.lastError().text() << Qt::endl;
            }
        }
        {
            QSqlQuery query;
            if (!query.exec("CREATE TABLE IF NOT EXISTS " + config.sqlTablePrefix() + "_sensors_seen (lastseen timestamp with time zone, topic varchar(255) PRIMARY KEY, data jsonb)"))
            {
                QTextStream(stderr) << "Error while creating " + config.sqlTablePrefix() + "_sensors_seen table: " << query.lastError().text() << Qt::endl;
            }
        }
    }
    else
    {
        QTextStream(stderr) << "Error: Faild to open database: " << db.lastError().text() << Qt::endl;
        ::exit(2);
    }

    m_client.setProtocolVersion(config.mqttVersion());
    m_client.setHostname(config.mqttHostname());
    m_client.setPort(config.mqttPort());
    QMqttConnectionProperties props;
    m_client.setConnectionProperties(props);
    connect(&m_client, &QMqttClient::errorChanged, this, &MqttSubscriber::onConnectionError);
    connect(&m_client, &QMqttClient::connected, this, &MqttSubscriber::subscribe);

    if (!config.mqttUsername().isEmpty() && !config.mqttPassword().isEmpty())
    {
        m_client.setUsername(config.mqttUsername());
        m_client.setPassword(config.mqttPassword());
    }

    if (config.mqttUseTls())
    {
        QSslConfiguration sslconfig;
        sslconfig.defaultConfiguration();
        sslconfig.setProtocol(QSsl::TlsV1_2);
        sslconfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        m_client.connectToHostEncrypted(sslconfig);
    }
    else
    {
        m_client.connectToHost();
    }

    m_cleanupTimer.setInterval(60*60*1000);
    m_cleanupTimer.setSingleShot(false);
    connect(&m_cleanupTimer, &QTimer::timeout, this, &MqttSubscriber::cleanup);
    m_cleanupTimer.start();
}

/**
 * @brief Called when the connection to the MQTT brocker is established and will subscribe to the topic.
 */
void MqttSubscriber::subscribe()
{
    QTextStream(stdout) << "MQTT connection established" << Qt::endl;

    const QList<MqttTopicConfig> & configs = m_config.mqttTopicConfig();
    for (const MqttTopicConfig & c : configs)
    {
        QMqttTopicFilter topic(c.topic);
        QMqttSubscription *subscription = m_client.subscribe(topic);
        if (!subscription) {
            QTextStream(stderr) << "Failed to subscribe to " << topic.filter() << Qt::endl;
            emit errorOccured("Failed to subscribe to " + topic.filter(), 1);
        }
        else
        {
            QTextStream(stdout) << "Subscribed to " << topic.filter() << Qt::endl;
            connect(subscription, &QMqttSubscription::stateChanged, this,
                    [topic](QMqttSubscription::SubscriptionState s) {
                QTextStream(stdout) << "Subscription state changed [topic " << topic.filter() << "]: " << qMqttSubscriptionState(s) << Qt::endl;
            });

            connect(subscription, &QMqttSubscription::messageReceived, this,
                    [this](const QMqttMessage & msg) {
                handleMessage(msg);
            });
            subscription->setProperty("config", QVariant::fromValue(c));
            m_subscriptions.append(subscription);
        }
    }
    {
        QMqttTopicFilter topic("#");
        QMqttSubscription *subscription = m_client.subscribe(topic);
        if (!subscription) {
            QTextStream(stderr) << "Failed to subscribe to " << topic.filter() << Qt::endl;
            emit errorOccured("Failed to subscribe to " + topic.filter(), 1);
        }
        else
        {
            QTextStream(stdout) << "Subscribed to " << topic.filter() << Qt::endl;
            connect(subscription, &QMqttSubscription::stateChanged, this,
                    [topic](QMqttSubscription::SubscriptionState s) {
                        QTextStream(stdout) << "Subscription state changed [topic " << topic.filter() << "]: " << qMqttSubscriptionState(s) << Qt::endl;
                    });

            connect(subscription, &QMqttSubscription::messageReceived, this,
                    [this](const QMqttMessage & msg) {
                        handleAnyMessage(msg);
                    });
            m_subscriptions.append(subscription);
        }
    }
}

/**
 * @brief Called when an error occurs in the MQTT client.
 *
 * Will print the error via \ref qMqttClientErrorToString and emit the signal \ref errorOccured.
 */
void MqttSubscriber::onConnectionError(QMqttClient::ClientError error)
{
    if (error != QMqttClient::NoError)
    {
        QTextStream(stderr) << "MQTT error: " << qMqttClientErrorToString(error) << Qt::endl;
        emit errorOccured(qMqttClientErrorToString(error), 3);
    }
}

/**
 * @brief Compare a value to the lastest one for the given sensorId
 * @param table Table to look for the given sensorId
 * @param sensorId Id of the sensor to look up
 * @param newValue New value to compare with
 * @return True if the values are identical, false otherwise
 */
bool MqttSubscriber::compareToPreviousValue(const QString & table, int sensorId, const QVariant & newValue)
{
    if (m_lastvalues.contains(sensorId))
    {
        return newValue == m_lastvalues[sensorId];
    }

    QSqlQuery squery;
    if (squery.exec("SELECT value FROM " + table + " WHERE sensorId=" + QString::number(sensorId) + " ORDER BY ts DESC LIMIT 1;"))
    {
        while (squery.next())
        {
            QVariant lastValue = squery.value(0);
            if (newValue.type() == QVariant::Double)
            {
                return qFuzzyCompare(newValue.toFloat(),lastValue.toFloat());
            }
            else
            {
                return (newValue == lastValue);
            }
        }
    }
    else
    {
        QTextStream(stderr) << "Error: Failed to execute statement: " << squery.lastError().text() << Qt::endl;
    }
    return false;
}

/**
 * @brief Called when a MQTT message for the # topic is received.
 *
 * Updates or creates the entry in the mqtt_sensors_seen table with the current timestamp as ts,
 * the messages topic as topic and the messages payload as data.
 */
void MqttSubscriber::handleAnyMessage(const QMqttMessage &msg)
{
    if (wasTopicSeen(msg.topic().name()))
    {
        return;
    }
    m_seenTopics.append(msg.topic().name());
    QSqlDatabase db = QSqlDatabase::database();
    if (db.isValid() && db.isOpen())
    {
        QSqlQuery query;
        if (query.prepare("INSERT INTO " + m_config.sqlTablePrefix() + "_sensors_seen (lastseen, topic, data) VALUES (NOW(), :topic, :data) ON CONFLICT (topic) DO UPDATE SET lastseen = NOW(), data = :data;"))
        {
            query.bindValue(":topic", msg.topic().name());
            query.bindValue(":data", QString::fromUtf8(msg.payload()));

            if (!query.exec())
            {
                QTextStream(stderr) << "SQL error: can not execute statement: " << query.lastError().text() << Qt::endl;
            }
        }
        else
        {
            QTextStream(stderr) << "SQL error: can not prepare statement: " << query.lastError().text() << Qt::endl;
        }
    }
    else
    {
        QTextStream(stderr) << "SQL error: Database not open!" << Qt::endl;
    }
}

/**
 * @brief Called when a MQTT message is received.
 *
 * Inserts the received message in the QSqlDatabase, with current timestamp as ts,
 * the messages topic as topic and the messages payload as data.
 */
void MqttSubscriber::handleMessage(const QMqttMessage &msg)
{
    const MqttTopicConfig config = sender()->property("config").value<MqttTopicConfig>();
    QTextStream(stdout) << "Message received. Topic: " << msg.topic().name() << ", Message: " << msg.payload() << Qt::endl;

    QVariant v;
    if (config.jsonpath.isEmpty())
    {
        v = QString::fromUtf8(msg.payload());
    }
    else
    {
        QJsonParseError jerror;
        QJsonDocument doc = QJsonDocument::fromJson(msg.payload(), &jerror);
        if (jerror.error != QJsonParseError::NoError)
        {
            QTextStream(stderr) << "Error while parsing payload: " << jerror.errorString() << Qt::endl;
            return;
        }
        QtJsonPath jp(doc);
        QVariant v = jp.getValue(config.jsonpath);
        if (v.isNull())
        {
            QTextStream(stderr) << "Error: can not extract value with JSONPath: " << config.jsonpath << Qt::endl;
            return;
        }
    }

    QSqlDatabase db = QSqlDatabase::database();
    if (db.isValid() && db.isOpen())
    {
        QSqlQuery query;
        if (v.convert(config.type))
        {
            bool skip = true;
            bool prepared = false;

            if (!compareToPreviousValue(m_config.sqlTablePrefix() + "_" + QVariant::typeToName(config.type), config.sensorId, v))
            {
                skip = false;
                prepared = query.prepare("INSERT INTO " + m_config.sqlTablePrefix() + "_" + QVariant::typeToName(config.type) + " (ts, sensorId, value) VALUES (NOW(), :sensorId, :value);");
            }


            if (skip)
            {
                QTextStream(stdout) << "Skipping value, as it has not changed." << Qt::endl;
                return;
            }

            if (prepared)
            {
                query.bindValue(":sensorId", config.sensorId);
                query.bindValue(":value", v);

                if (!query.exec())
                {
                    QTextStream(stderr) << "SQL error: can not execute statement: " << query.lastError().text() << Qt::endl;
                }
                else
                {
                    m_lastvalues[config.sensorId] = v;
                }
            }
            else
            {
                QTextStream(stderr) << "SQL error: can not prepare statement: " << query.lastError().text() << Qt::endl;
            }
        }
    }
    else
    {
        QTextStream(stderr) << "SQL error: Database not open!" << Qt::endl;
    }
}

/**
 * @brief Delete all outdated SQL entires
 */
void MqttSubscriber::cleanup()
{}
