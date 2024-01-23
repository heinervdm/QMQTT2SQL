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

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "mqtt2sqlconfig.h"
#include "mqttsubscriber.h"

static constexpr const char * version = "0.0.1";
static constexpr const char * applicationname = "QMQTT2SQL";
static constexpr const char * author = "Thomas Zimmermann";
static constexpr int copyrightyear = 2024;
static constexpr int configErrorExitCode = 1;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setApplicationName(applicationname);
    a.setApplicationVersion(version);

    QCommandLineParser parser;
    parser.setApplicationDescription("Subscribes to a MQTT broker and stores all messages in a PostgreSQL database.");
    QCommandLineOption helpOption = parser.addHelpOption();

    QCommandLineOption versionOption({"v", "version"}, "Show version and exit.");
    parser.addOption(versionOption);
    QCommandLineOption configOption({"c", "config"}, "Path to the config file.", "config",
                                    QString("%1/%2").arg(QCoreApplication::applicationDirPath(), "qdeconz2mqtt.ini"));
    parser.addOption(configOption);

    parser.process(a);

    bool showVersion = parser.isSet(versionOption);
    if (showVersion)
    {
        QTextStream(stdout) << applicationname << " " << version << Qt::endl;
        QTextStream(stdout) << "Copyright (C) " << copyrightyear << " " << author << "." << Qt::endl;
        QTextStream(stdout) << "License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>." << Qt::endl;
        QTextStream(stdout) << "This is free software: you are free to change and redistribute it." << Qt::endl;
        QTextStream(stdout) << "There is NO WARRANTY, to the extent permitted by law." << Qt::endl;
        ::exit(0);
    }

    bool showHelp = parser.isSet(helpOption);
    if (showHelp)
    {
        parser.showHelp();
    }

    QString configFile = parser.value("config");
    Mqtt2SqlConfig config;
    if (!config.parse(configFile))
    {
        QTextStream(stdout) << "Error while reading config file: " << configFile << Qt::endl;
        QTextStream(stdout) << config.lastError() << Qt::endl;
        ::exit(configErrorExitCode);
    }

    MqttSubscriber mc(config);
    QObject::connect(&mc, &MqttSubscriber::errorOccured, qApp, [](const QString & error, int exitcode){
        QTextStream(stderr) << error << Qt::endl;
        if (exitcode != 0)
        {
            ::exit(exitcode);
        }
    });

    return a.exec();
}
