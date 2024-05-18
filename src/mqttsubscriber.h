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

#ifndef MQTTSUBSCRIBER_H
#define MQTTSUBSCRIBER_H

#include <QObject>
#include <QTimer>
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
    /// Is emitted when an error occurs.
    void errorOccured(const QString & error, int exitcode);

private slots:
    void subscribe();
    void onConnectionError(QMqttClient::ClientError error);
    void handleMessage(const QMqttMessage &msg, const MqttTopicConfig &config);
    void handleAnyMessage(const QMqttMessage &msg);
    void cleanup();

private:
    bool compareToPreviousValue(const QString &table, int sensorId, const QVariant &newValue);
    bool wasTopicSeen(const QString & topic) { return m_seenTopics.contains(topic); }

private:
    QMqttClient m_client;
    QMqttSubscription *m_subscription;
    QTimer m_cleanupTimer;
    Mqtt2SqlConfig m_config;
    QList<QMqttSubscription*> m_subscriptions;
    QMap<int,QVariant> m_lastvalues; // sensorId -> Last sensor value
    QList<QString> m_seenTopics; // List of seen topics
};

#endif // MQTTSUBSCRIBER_H
