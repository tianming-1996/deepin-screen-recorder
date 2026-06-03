// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbusutils.h"
#include <QDBusConnection>
#include <QDBusReply>
#include <QDBusInterface>
#include <QDebug>
#include <QDBusError>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusConnectionInterface>

DBusUtils::DBusUtils()
{

}

QVariant DBusUtils::redDBusProperty(const QString &service, const QString &path, const QString &interface, const char *propert)
{
    // 创建QDBusInterface接口
    QDBusInterface ainterface(service, path,
                              interface,
                              QDBusConnection::sessionBus());
    if (!ainterface.isValid()) {
        qDebug() << qPrintable(QDBusConnection::sessionBus().lastError().message());
        QVariant v(0) ;
        return  v;
    }
    //调用远程的value方法
    QVariant v = ainterface.property(propert);
    return  v;
}
QVariant DBusUtils::redDBusMethod(const QString &service, const QString &path, const QString &interface, const char *method)
{
    // 创建QDBusInterface接口
    QDBusInterface ainterface(service, path,
                              interface,
                              QDBusConnection::sessionBus());
    if (!ainterface.isValid()) {
        qDebug() <<  "error:" << qPrintable(QDBusConnection::sessionBus().lastError().message());
        QVariant v(0) ;
        return  v;
    }
    //调用远程的value方法
    QDBusReply<QDBusVariant> reply = ainterface.call(method);
    if (reply.isValid()) {
//        return reply.value();
        QVariant v(0) ;
        return  v;
    } else {
        qDebug() << "error1:" << qPrintable(QDBusConnection::sessionBus().lastError().message());
        QVariant v(0) ;
        return  v;
    }
}

bool DBusUtils::isAiAssistantAvailable()
{
    return !aiAssistantServiceName().isEmpty();
}

QString DBusUtils::aiAssistantServiceName()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qDebug() << "Session bus is not connected, AI assistant is unavailable.";
        return QString();
    }

    const QString copilotService = QStringLiteral("com.deepin.copilot");
    const QString iflytekService = QStringLiteral("com.iflytek.aiassistant");
    const QString objectPath = QStringLiteral("/com/deepin/copilot");
    const QString copilotInterface = QStringLiteral("com.deepin.copilot");

    QDBusInterface copilot(copilotService,
                           objectPath,
                           copilotInterface,
                           bus);
    if (copilot.isValid()) {
        copilot.call(QStringLiteral("version"));
    }

    auto hasAiAssistantMethods = [&](const QString &service) -> bool {
        QDBusInterface introspect(service,
                                  objectPath,
                                  QStringLiteral("org.freedesktop.DBus.Introspectable"),
                                  bus);
        if (!introspect.isValid()) {
            qDebug() << "AI assistant introspection interface is invalid for service:" << service
                     << "error:" << bus.lastError().message();
            return false;
        }

        QDBusReply<QString> xml = introspect.call(QStringLiteral("Introspect"));
        if (!xml.isValid()) {
            qDebug() << "AI assistant introspection failed for service:" << service
                     << "error:" << xml.error().message();
            return false;
        }

        const QString xmlStr = xml.value();
        const bool hasQuickOcr = xmlStr.contains(QStringLiteral("launchAiQuickOCR"));
        const bool hasChatUpload = xmlStr.contains(QStringLiteral("launchChatUploadImage"));
        qDebug() << "AI assistant service:" << service
                 << "has launchAiQuickOCR:" << hasQuickOcr
                 << "has launchChatUploadImage:" << hasChatUpload;
        return hasQuickOcr && hasChatUpload;
    };

    if (hasAiAssistantMethods(copilotService)) {
        return copilotService;
    }

    if (hasAiAssistantMethods(iflytekService)) {
        return iflytekService;
    }

    return QString();
}
