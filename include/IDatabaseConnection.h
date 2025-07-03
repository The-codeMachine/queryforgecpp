#pragma once
#include <memory>
#include <vector>
#include <string>

class IPreparedStatement;
class IResultSet;

class IDatabaseConnection {
public:
    virtual std::shared_ptr<IPreparedStatement> prepareStatement(const std::string& sql) = 0;
    virtual bool isValid() const = 0;
    virtual void reconnect() = 0;
    virtual ~IDatabaseConnection() = default;
};
