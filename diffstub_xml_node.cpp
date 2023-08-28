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
    std::stringstream this_compare_ss;
    this_compare_ss << this->xpath << " ";
    for (const auto& attr: this->attributes) {
        this_compare_ss << attr.first << "=" << attr.second;
    }

    std::stringstream other_compare_ss;
    other_compare_ss << other.xpath << " ";
    for (const auto& attr: other.attributes) {
        other_compare_ss << attr.first << "=" << attr.second;
    } 

    return this_compare_ss.str() < other_compare_ss.str();
}