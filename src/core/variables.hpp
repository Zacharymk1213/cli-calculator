#pragma once

#include <map>
#include <optional>
#include <string>

class VariableStore
{
public:
    explicit VariableStore(std::string filePath = "vars.toml");

    bool load();
    bool save() const;

    const std::map<std::string, double> &variables() const;
    std::optional<double> find(const std::string &name) const;
    void set(const std::string &name, double value);
    bool remove(const std::string &name);

    static bool isValidName(const std::string &name);

private:
    static std::string normalizeName(const std::string &name);

    std::string filePath_;
    std::map<std::string, double> vars_;
};

VariableStore &globalVariableStore();
