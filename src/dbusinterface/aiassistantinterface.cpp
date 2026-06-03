// Copyright (C) 2020 ~ 2021 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "aiassistantinterface.h"

AiAssistantInterface::AiAssistantInterface(const QString &serviceName, const QString &ObjectPath,
                           const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(serviceName, ObjectPath, staticInterfaceName(), connection, parent)
{
    qDebug() << "AiAssistantInterface created for service:" << serviceName << "path:" << ObjectPath;
}

AiAssistantInterface::~AiAssistantInterface()
{
    qDebug() << "Destroying AiAssistantInterface";
}
