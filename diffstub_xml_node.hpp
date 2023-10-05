#ifndef DIFFSTUB_XML_NODE_HPP
#define DIFFSTUB_XML_NODE_HPP

#include <string>
#include <iostream>
#include <unordered_map>
#include <set>
#include <sstream>

class XMLElement {
private:
    std::string xpath;
    std::string name;
    std::unordered_map<std::string, std::string> attributes;
    std::string value;
    //std::vector<XMLElement> children;
public:
    int index;
    std::string type;
    std::string relative_pos;
    std::string prev_sibling_rel_val;
    std::string next_sibling_rel_val;
    std::string selector_attrib;

    bool has_children;

    const std::string& getValue() const;

    const std::string& getXPath() const;

    const std::string& getName() const;

    const std::unordered_map<std::string, std::string>& getAttributes() const;

    const std::string get_compare_str() const;

    void setValue(std::string value);

    void setXPath(std::string xpath);

    void setName(std::string name);

    void addAttribute(std::string name, std::string value);

    bool similar(const XMLElement& other) const;

    bool operator==(const XMLElement& other) const;

    bool operator<(const XMLElement& other) const;
};

#endif //DIFFSTUB_XML_NODE_HPP