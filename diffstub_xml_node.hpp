#ifndef DIFFSTUB_XML_NODE_HPP
#define DIFFSTUB_XML_NODE_HPP

#include <string>
#include <unordered_map>
#include <set>
#include <sstream>

class XMLElement {
private:
    std::string xpath;
    std::string name;
    std::unordered_map<std::string, std::string> attributes;
    //std::vector<XMLElement> children;
public:
    size_t index;
    bool has_children;

    const std::string& getXPath() const;

    const std::string& getName() const;

    const std::unordered_map<std::string, std::string>& getAttributes() const;

    void setXPath(std::string xpath);

    void setName(std::string name);

    void addAttribute(std::string name, std::string value);

    bool similar(const XMLElement& other) const;

    bool operator==(const XMLElement& other) const;

    bool operator<(const XMLElement& other) const;
};

#endif //DIFFSTUB_XML_NODE_HPP