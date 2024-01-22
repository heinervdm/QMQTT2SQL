/**
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

#include "mqttsubscriber.h"

#include <QSqlDatabase>
#include <QSqlQuery>

MqttSubscriber::MqttSubscriber(const Mqtt2SqlConfig & config, QObject *parent)
    : QObject{parent}
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
    db.setHostName("localhost");
    db.setDatabaseName("mqtt");
    db.setUserName("dbmas");
    db.setPassword("dbmas");
    if (db.open())
    {
        QSqlQuery query("CREATE TABLE IF NOT EXISTS mqtt (ts timestamp with time zone, topic varchar(255), data jsonb)");
        query.exec();
    }
}

void MqttSubscriber::subscribe()
{
    QTextStream(stdout) << "MQTT connection established" << Qt::endl;

    m_subscription = m_client.subscribe(m_topic);
    if (!m_subscription) {
        QTextStream(stderr) << "Failed to subscribe to " << m_topic << Qt::endl;
        emit errorOccured(m_client.error());
    }

    connect(m_subscription, &QMqttSubscription::stateChanged, this,
            [](QMqttSubscription::SubscriptionState s) {
        QTextStream(stdout) << "Subscription state changed: " << s << Qt::endl;
    });

    connect(m_subscription, &QMqttSubscription::messageReceived, this,
            [this](QMqttMessage msg) {
        handleMessage(msg);
    });
}

void MqttSubscriber::onConnectionError(QMqttClient::ClientError error)
{
    if (error != QMqttClient::NoError)
    {
        QTextStream(stderr) << "MQTT error: " << error << Qt::endl;
        emit errorOccured(error);
    }
}

void MqttSubscriber::handleMessage(const QMqttMessage &msg)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (db.isValid() && db.isOpen())
    {
        QSqlQuery query;
        if (query.prepare("INSERT INTO person (ts, topic, data) VALUES (:ts, :topic, :data)"))
        {
            query.bindValue(":ts", QDateTime::currentDateTime());
            query.bindValue(":topic", msg.topic().name());
            query.bindValue(":data", msg.payload());
            query.exec();
        }
    }
}
