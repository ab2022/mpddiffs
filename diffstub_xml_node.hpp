/*
Copyright 2023 Comcast

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Author: Erik Ponder, Jovan Rosario
 */

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