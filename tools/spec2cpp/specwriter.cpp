#include "specwriter.h"

#include <QFile>
#include <QTextStream>

#include <algorithm>
#include <unordered_set>

struct QStringHash
{
    size_t operator()(const QString &string) const { return qHash(string); }
};

SpecWriter::SpecWriter(Data data)
    : m_data(std::move(data))
{
}

static QString methodToName(const QString &method)
{
    auto names = method.split('/');

    // Remove some specifica method prefix : $, textDocument, window, workspace, client
    if (names.first() == "$" || names.first() == "textDocument" || names.first() == "window"
        || names.first() == "workspace" || names.first() == "client")
        names.removeFirst();

    QString name;
    for (const auto &word : names) {
        name += word[0].toUpper() + word.mid(1);
    }
    return name;
}

static const char NotificationHeader[] = R"(// File generated by spec2cpp tool
// DO NOT MAKE ANY CHANGES HERE

#pragma once

#include "notificationmessage.h"
#include "types.h"

namespace Lsp {
%1
}
)";

// %1 notification name
// %2 notification method
// %3 notification params
static const char NotificationImpl[] = R"(
inline constexpr char %1Name[] = "%2";
struct %1Notification : public NotificationMessage<%1Name, %3>
{};
)";

void SpecWriter::saveNotifications()
{
    QFile file(LSP_SOURCE_PATH "/notifications.h");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    QTextStream stream(&file);

    QString text;
    for (const auto &notification : m_data.notifications) {
        const QString name = methodToName(notification.method);
        text += QString(NotificationImpl).arg(name, notification.method, notification.params);
    }
    stream << QString(NotificationHeader).arg(text);
}

static const char RequestHeader[] = R"(// File generated by spec2cpp tool
// DO NOT MAKE ANY CHANGES HERE

#pragma once

#include "requestmessage.h"
#include "types.h"

namespace Lsp {
%1
}
)";

// %1 request name
// %2 request method
// %3 request params
// %4 request result
// %5 request error
static const char RequestImpl[] = R"(
inline constexpr char %1Name[] = "%2";
struct %1Request : public RequestMessage<%1Name, %3, %4, %5>
{};
)";

void SpecWriter::saveRequests()
{
    QFile file(LSP_SOURCE_PATH "/requests.h");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    QTextStream stream(&file);

    QString text;
    for (const auto &request : m_data.requests) {
        const QString name = methodToName(request.method);
        text += QString(RequestImpl).arg(name, request.method, request.params, request.result, request.error);
    }
    stream << QString(RequestHeader).arg(text);
    ;
}

static const char CodeHeader[] = R"(// File generated by spec2cpp tool
// DO NOT MAKE ANY CHANGES HERE

#pragma once

#include <nlohmann/json.hpp>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

namespace Lsp {
%1
}
)";

static const char CodeJsonHeader[] = R"(// File generated by spec2cpp tool
// DO NOT MAKE ANY CHANGES HERE

#pragma once

#include "json.h"
#include "types.h"

namespace Lsp {
%1
}
)";

void SpecWriter::saveCode()
{
    cleanCode();

    {
        QFile file(LSP_SOURCE_PATH "/types.h");
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return;
        QTextStream stream(&file);

        QString text;
        text += writeEnums();
        text += writeTypesAndInterfaces();

        stream << QString(CodeHeader).arg(text);
    }
    {
        QFile file(LSP_SOURCE_PATH "/types_json.h");
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return;
        QTextStream stream(&file);

        QString text;
        text += writeJsonEnums();
        for (const auto &interface : m_data.interfaces)
            text += writeJsonInterface(interface);

        stream << QString(CodeJsonHeader).arg(text);
    }
}

// %1 comment
// %2 enum name
// %3 enum content
static const char EnumImpl[] = R"(
%1enum class %2 {
%3};
)";

QString SpecWriter::writeEnums()
{
    QString result;
    for (const auto &enumeration : m_data.enumerations) {
        QString content;
        for (const auto &value : enumeration.values) {
            if (enumeration.isString)
                content += QString("%1\t%2,\n").arg(value.comment, value.name);
            else
                content += QString("%1\t%2 = %3,\n").arg(value.comment, value.name, value.value);
        }
        result += QString(EnumImpl).arg(enumeration.comment, enumeration.name, content);
    }
    return result;
}

// %1 enum name
// %2 content: lines like {ValueName, ValueString},
static const char EnumJsonImpl[] = R"(
JSONIFY_ENUM( %1, {
%2})
)";

QString SpecWriter::writeJsonEnums()
{
    QString result;
    for (const auto &enumeration : m_data.enumerations) {
        if (enumeration.isString) {
            QString content;
            for (const auto &value : enumeration.values)
                content += QString("    {%1::%2, \"%3\"},\n").arg(enumeration.name, value.name, value.value);
            result += QString(EnumJsonImpl).arg(enumeration.name, content);
        }
    }
    return result;
}

QString SpecWriter::writeTypesAndInterfaces()
{
    QString result;
    QStringList existingNames;

    auto &types = m_data.types;
    auto startType = types.begin();
    auto endType = types.begin();

    auto &interfaces = m_data.interfaces;
    auto startStruct = interfaces.begin();
    auto endStruct = interfaces.begin();

    while (startType != types.end() || startStruct != interfaces.end()) {
        // Write only types without any dependencies
        if (startType != types.end()) {
            endType = std::stable_partition(startType, types.end(), [](const auto &type) {
                return type.dependencies.isEmpty();
            });
            while (startType != endType) {
                result += writeType(*startType);
                existingNames.push_back(startType->name);
                ++startType;
            }
        }

        // Write only interfaces without any dependencies
        if (startStruct != interfaces.end()) {
            endStruct = std::stable_partition(startStruct, interfaces.end(), [](const auto &interface) {
                return interface.dependencies.isEmpty();
            });
            while (startStruct != endStruct) {
                result += writeMainInterface(*startStruct);
                existingNames.push_back(startStruct->name);
                ++startStruct;
            }
        }

        // Cleanup dependencies
        for (auto it = endType; it != types.end(); ++it) {
            for (const auto &name : existingNames)
                it->dependencies.removeAll(name);
        }
        for (auto it = endStruct; it != interfaces.end(); ++it) {
            for (const auto &name : existingNames)
                it->dependencies.removeAll(name);
        }
        existingNames.clear();
    }
    return result;
}

// %1 comment
// %2 type name
// %3 type datatype
static const char TypeImpl[] = R"(
%1using %2 = %3;
)";
QString SpecWriter::writeType(const Data::Type &type)
{
    static std::unordered_set<QString, QStringHash> exceptions = {"integer", "uinteger", "decimal"};
    if (exceptions.contains(type.name))
        return {};
    return QString(TypeImpl).arg(type.comment, type.name, type.dataType);
}

static QString writeProperty(const Data::Interface::Property &property, const QString &interface)
{
    QString name = property.name;
    name.remove("readonly ");

    bool isOptional = property.name.contains('?');
    bool isConstString = property.dataType.startsWith('\'');
    bool isPtr = property.dataType == interface;

    if (isPtr) {
        return QString("%1std::unique_ptr<%2> %3;\n").arg(property.comment, property.dataType, name.remove('?'));
    } else if (isOptional) {
        return QString("%1std::optional<%2> %3;\n").arg(property.comment, property.dataType, name.remove('?'));
    } else if (isConstString) {
        QString text = property.dataType;
        return QString("%1static inline const std::string %2 = %3;\n")
            .arg(property.comment, name, text.replace('\'', '\"'));
    }
    return QString("%1%2 %3;\n").arg(property.comment, property.dataType, name);
}

// %1 interface name
// %2 child interfaces and properties
static const char ChildInterfaceImpl[] = R"(struct %1 {
%2};
)";
static QString writeChildInterface(const Data::Interface &interface, QStringList parent)
{
    parent.push_back(interface.name);

    QString content;

    for (const auto &child : interface.children)
        content += writeChildInterface(child, parent);
    for (const auto &property : interface.properties)
        content += writeProperty(property, interface.name);

    return QString(ChildInterfaceImpl).arg(interface.name, content);
}

// %1 comment
// %2 interface name
// %3 interface extend: ": public Extend1, public Extend2
// %4 child interfaces and properties
static const char MainInterfaceImpl[] = R"(
%1struct %2%3 {
%4};
)";
QString SpecWriter::writeMainInterface(const Data::Interface &interface)
{
    QString extends = interface.extends.join(", public ");
    if (!extends.isEmpty())
        extends.prepend(": public ");

    QString content;
    for (const auto &child : interface.children)
        content += writeChildInterface(child, {interface.name});
    for (const auto &property : interface.properties)
        content += writeProperty(property, interface.name);

    return QString(MainInterfaceImpl).arg(interface.comment, interface.name, extends, content);
}

QStringList interfaceProperties(const Data::Interface &interface, const std::vector<Data::Interface> &interfaces)
{
    QStringList properties;
    for (const auto &prop : interface.properties) {
        QString name = prop.name;
        name.remove('?');
        name.remove("readonly ");
        properties.push_back(name);
    }

    for (const auto &extend : interface.extends) {
        auto it = std::find_if(interfaces.cbegin(), interfaces.cend(), [extend](const auto &interface) {
            return interface.name == extend;
        });
        if (it != interfaces.cend()) {
            properties.append(interfaceProperties(*it, interfaces));
        }
    }
    return properties;
}

QString SpecWriter::writeJsonInterface(const Data::Interface &interface, QStringList parent)
{
    static std::unordered_set<QString> exceptions = {"SelectionRange", "FormattingOptions", "ChangeAnnotationsType"};

    parent.push_back(interface.name);
    if (exceptions.contains(interface.name))
        return QString("\nJSONIFY_FWD(%1)\n").arg(parent.join("::"));

    QString result;
    if (parent.count() == 1)
        result += "\n";

    QStringList properties = interfaceProperties(interface, m_data.interfaces);

    for (const auto &child : interface.children)
        result += writeJsonInterface(child, parent);

    if (properties.empty())
        result += QString("JSONIFY_EMPTY(%1)\n").arg(parent.join("::"));
    else
        result += QString("JSONIFY(%1, %2)\n").arg(parent.join("::"), properties.join(", "));
    return result;
}

void SpecWriter::cleanCode()
{
    auto &enumerations = m_data.enumerations;
    auto &types = m_data.types;
    auto &interfaces = m_data.interfaces;

    // Remove enumeration duplicates
    {
        std::unordered_set<QString, QStringHash> enumSet;
        auto itEnum = std::remove_if(enumerations.begin(), enumerations.end(), [&enumSet](const auto &e) {
            return !enumSet.insert(e.name).second;
        });
        enumerations.erase(itEnum, enumerations.end());
    }

    // Cleanup enumeration names and enumeration values
    {
        std::map<QString, QString> specialEnumNames = {{"InitializeError", "InitializeErrorCodes"}};
        for (auto &enumeration : enumerations) {
            if (specialEnumNames.contains(enumeration.name))
                enumeration.name = specialEnumNames.at(enumeration.name);

            for (auto &value : enumeration.values) {
                value.name[0] = value.name[0].toUpper();
                if (enumeration.isString)
                    value.value = value.value.remove('\'');
                else
                    value.value[0] = value.value[0].toUpper();
            }
        }
    }

    // Remove some specific struct
    {
        std::unordered_set<QString, QStringHash> removeStructNames = {"Message", "RequestMessage", "ResponseMessage",
                                                                      "ResponseError", "NotificationMessage"};
        auto it = interfaces.begin();
        while (it != interfaces.end()) {
            if (removeStructNames.contains(it->name))
                it = interfaces.erase(it);
            else
                ++it;
        }
        // Remove first TextDocumentSyncOptions
        it = std::find_if(interfaces.begin(), interfaces.end(), [](const auto &interface) {
            return interface.name == "TextDocumentSyncOptions";
        });
        interfaces.erase(it);
    }

    // Remove duplicate types (from enums or interfaces)
    QStringList enumNames;
    {
        for (auto &enumeration : enumerations)
            enumNames.push_back(enumeration.name);

        QStringList existingTypeNames = enumNames;
        for (const auto &interface : interfaces)
            existingTypeNames.push_back(interface.name);

        auto itRemove = std::remove_if(types.begin(), types.end(), [existingTypeNames](const auto &t) {
            return existingTypeNames.contains(t.name);
        });
        types.erase(itRemove, types.end());
    }

    // Cleanup dependencies for types and interfaces
    {
        for (auto &type : types) {
            for (const auto &enumName : qAsConst(enumNames))
                type.dependencies.removeAll(enumName);
        }
        for (auto &interface : m_data.interfaces) {
            for (const auto &enumName : qAsConst(enumNames))
                interface.dependencies.removeAll(enumName);
        }
    }
}