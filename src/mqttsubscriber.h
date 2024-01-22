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

#ifndef MQTTSUBSCRIBER_H
#define MQTTSUBSCRIBER_H

#include <QObject>
#include <QMqttClient>
#include <QMqttSubscription>
#include <QMqttMessage>

#include "mqtt2sqlconfig.h"

class MqttSubscriber : public QObject
{
    Q_OBJECT
public:
    explicit MqttSubscriber(const Mqtt2SqlConfig &config, QObject *parent = nullptr);

signals:
    void errorOccured(QMqttClient::ClientError error);

private slots:
    void subscribe();
    void onConnectionError(QMqttClient::ClientError error);
    void handleMessage(const QMqttMessage &msg);

private:
    QMqttClient m_client;
    QMqttSubscription *m_subscription;
    QString m_topic;

};

#endif // MQTTSUBSCRIBER_H
