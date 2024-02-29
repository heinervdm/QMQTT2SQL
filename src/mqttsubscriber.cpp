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

    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName(config.sqlHostname());
    db.setDatabaseName(config.sqlDatabase());
    db.setPort(config.sqlPort());
    db.setUserName(config.sqlUsername());
    db.setPassword(config.sqlPassword());
    if (db.open())
    {
        QSqlQuery query("CREATE TABLE IF NOT EXISTS mqtt (ts timestamp with time zone, topic varchar(255), data jsonb);");
        if (!query.exec())
        {
            QTextStream(stderr) << "Error while creating mqtt table: " << query.lastError().text() << Qt::endl;
        }
        QSqlQuery query2("CREATE INDEX mqtt_topic_idx ON mqtt (topic DESC);");
        if (!query2.exec())
        {
            QTextStream(stderr) << "Error while creating index: " << query2.lastError().text() << Qt::endl;
        }
    }
    else
    {
        QTextStream(stderr) << "Error: Faild to open database: " << db.lastError().text() << Qt::endl;
        emit errorOccured(db.lastError().text(), 2);
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

    QMqttTopicFilter topic(m_config.mqttTopic());
    m_subscription = m_client.subscribe(topic);
    if (!m_subscription) {
        QTextStream(stderr) << "Failed to subscribe to " << topic.filter() << Qt::endl;
        emit errorOccured("Failed to subscribe to " + topic.filter(), 1);
    }

    connect(m_subscription, &QMqttSubscription::stateChanged, this,
            [](QMqttSubscription::SubscriptionState s) {
        QTextStream(stdout) << "Subscription state changed: " << qMqttSubscriptionState(s) << Qt::endl;
    });

    connect(m_subscription, &QMqttSubscription::messageReceived, this,
            [this](const QMqttMessage & msg) {
        handleMessage(msg);
    });
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
 * @brief Called when a MQTT message is received.
 *
 * Inserts the received message in the QSqlDatabase, with current timestamp as ts,
 * the messages topic as topic and the messages payload as data.
 */
void MqttSubscriber::handleMessage(const QMqttMessage &msg)
{
    QTextStream(stdout) << "Message received. Topic: " << msg.topic().name() << ", Message: " << msg.payload() << Qt::endl;
    QSqlDatabase db = QSqlDatabase::database();
    if (db.isValid() && db.isOpen())
    {
        QSqlQuery query;
        if (query.prepare("INSERT INTO mqtt (ts, topic, data) VALUES (:ts, :topic, :data);"))
        {
            query.bindValue(":ts", QDateTime::currentDateTime());
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
 * @brief Delete all outdated SQL entires
 */
void MqttSubscriber::cleanup()
{
    QTextStream(stdout) << "Cleaning up SQL database." << Qt::endl;
    QSqlDatabase db = QSqlDatabase::database();
    if (db.isValid() && db.isOpen())
    {
        QSqlQuery query;
        if (query.prepare("DELETE FROM mqtt WHERE ts < :ts;"))
        {
            query.bindValue(":ts", QDateTime::currentDateTime().addSecs(m_config.sqlMaxStroageTime().count()*60*60*-1));
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
