#pragma once

#include <iosfwd>
#include <string>

enum class OutputFormat { Text, Json, Xml, Yaml };

std::string jsonEscape(const std::string &value);
std::string xmlEscape(const std::string &value);
std::string yamlEscape(const std::string &value);

void printJsonSuccess(std::ostream &os, const std::string &action,
                      const std::string &payload = {});
void printJsonError(std::ostream &os, const std::string &action,
                    const std::string &message);
void printXmlSuccess(std::ostream &os, const std::string &action,
                     const std::string &payload = {});
void printXmlError(std::ostream &os, const std::string &action,
                   const std::string &message);
void printYamlSuccess(std::ostream &os, const std::string &action,
                      const std::string &payload = {});
void printYamlError(std::ostream &os, const std::string &action,
                    const std::string &message);
void printStructuredSuccess(std::ostream &os, OutputFormat format,
                            const std::string &action,
                            const std::string &jsonPayload,
                            const std::string &xmlPayload,
                            const std::string &yamlPayload);
void printStructuredError(std::ostream &os, OutputFormat format,
                          const std::string &action,
                          const std::string &message);
