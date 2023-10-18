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

#include "diffstub_xml_node.hpp"

const std::string& XMLElement::getValue() const {
    return this->value;
}

const std::string& XMLElement::getXPath() const {
    return this->xpath;
}

const std::string& XMLElement::getName() const {
    return this->name;
}

const std::unordered_map<std::string, std::string>& XMLElement::getAttributes() const {
    return this->attributes;
}

const std::string XMLElement::get_compare_str() const {
    std::stringstream compare_ss;

    if (this->type == "Text") {                            
        compare_ss << this->xpath;                         //Text Nodes have no index (Not supported by dash.js?)
    } else if (this->name == "S") {    
        auto pos = this->xpath.find_last_of("/");          //Element is a Segment, modify the xpath using Segment Index
        compare_ss << this->xpath.substr(0, pos)  << "/" << this->name << "/" << this->index;
        auto it = this->attributes.find(this->selector_attrib);
        compare_ss << "/@" << it->first << "=" << it->second;
    } else {
        compare_ss << this->xpath << "/" << this->index;   //All other elements, Use index to maintain Element order
    }

    return compare_ss.str();
}

void XMLElement::setValue(std::string value) {
    this->value = value;
}

void XMLElement::setXPath(std::string xpath) {
    this->xpath = xpath;
}

void XMLElement::setName(std::string name) {
    this->name = name;
}

void XMLElement::addAttribute(std::string name, std::string value) {
    this->attributes[name] = value;
}

bool XMLElement::similar(const XMLElement& other) const {
    size_t total_attrib = this->attributes.size();
    size_t similar_attrib = 0;

    for (const auto& pair: this->attributes) {
        auto it = other.getAttributes().find(pair.first);

        if (it != other.getAttributes().end()) {
            if (pair.second == it->second) {
                similar_attrib++;
            }
        }
    } 

    if (static_cast<float>(similar_attrib)/total_attrib > .50) {
        return true;
    } else {
        return false;
    }
}

bool XMLElement::operator==(const XMLElement& other) const {
    // If name or number of attributes or children are not the same, 
    //if (name != other.name || attributes.size() != other.attributes.size() || children.size() != other.children.size()) 
    if (this->name != other.name || this->xpath != other.xpath || this->attributes.size() != other.attributes.size())
    {
        return false;
    }

    // Compare attributes
    if (this->attributes != other.attributes) {
        return false;
    }

    /*
    // Compare children recursively
    for (size_t i = 0; i < children.size(); ++i) {
        if (children[i] != other.children[i]) {
            return false;
        }
    }
    */

    return true;
}


bool XMLElement::operator<(const XMLElement& other) const {    
    /*
    this_compare_ss << this->xpath << " ";
    for (const auto& attr: this->attributes) {
        this_compare_ss << attr.first << "=" << attr.second;
    }
    */
    //std::cerr << "THIS XPATH: " << this->xpath << std::endl;
    //std::cerr << "OTHER XPATH: " << other.getXPath() << std::endl;

    std::string this_compare = this->get_compare_str();
    std::string other_compare = other.get_compare_str();

    //std::cerr << "THIS Compare: " << this_compare << std::endl;
    //std::cerr << "OTHER Compare: " << other_compare << std::endl;
    
    /*
    other_compare_ss << other.xpath << " ";
    for (const auto& attr: other.attributes) {
        other_compare_ss << attr.first << "=" << attr.second;
    }
    */

    return this_compare < other_compare;
}