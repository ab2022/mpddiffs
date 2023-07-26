/*
 * Copyright (C) ab
 */
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <fstream>
#include <set>
#include <unordered_map>
#include <map>
#include "pugixml.hpp"
#include "ngx_diffstub_internal.hpp"

class XMLElement {
private:
    std::string xpath;
    std::string name;
    std::unordered_map<std::string, std::string> attributes;
    //std::vector<XMLElement> children;
public:
    size_t index;

    const std::string& getXPath() const {
        return this->xpath;
    }

    const std::string& getName() const {
        return this->name;
    }

    const std::unordered_map<std::string, std::string>& getAttributes() const {
        return this->attributes;
    }

    void setXPath(std::string xpath) {
        this->xpath = xpath;
    }

    void setName(std::string name) {
        this->name = name;
    }

    void addAttribute(std::string name, std::string value) {
        this->attributes[name] = value;
    }

    bool similar(const XMLElement& other) const {
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

    std::set<std::string> attrib_diff(const XMLElement& other) const {
        std::set<std::string> diff;
        return diff;
    }

    bool operator==(const XMLElement& other) const {
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

    bool operator<(const XMLElement& other) const {
        return this->xpath < other.xpath;
    }
};

/*
#define TOK "/VAL/"
#define TOKLEN 5

struct simple_walker: pugi::xml_tree_walker
{
    int curdep = 0;
    vector<string> all_elements;

    virtual bool for_each(pugi::xml_node& node)
    {
        for (pugi::xml_attribute attr: node.attributes())
        {
            curdep = depth(); 
            vector<string> path_elements;
            pugi::xml_node temp_node = node.parent();

            stringstream xps; //xpath string

            for (int i = curdep; i > 0; i--) {
                path_elements.insert(path_elements.begin(), temp_node.name());
                temp_node = temp_node.parent();
            }

            xps << "/";
            for (int i = 0; i < (int)path_elements.size(); i++)
                xps << path_elements[i] << "/";

            xps << node.name() << "[@" << attr.name() << "]" << TOK << attr.value();

            cout << xps.str() << endl;

            //cout << xps.str();
            all_elements.push_back(xps.str()); //one string for each mpd item
        }
        return true; // continue traversal
    }
};
*/

/* Tranlate the delta map into XML Patch format */
void translate_deltas(const std::map<XMLElement, std::string>& deltas) {
    pugi::xml_document diff_patch;

    // Set the version and encoding attributes of the XML declaration
    pugi::xml_node xml_declaration = diff_patch.append_child(pugi::node_declaration);
    xml_declaration.append_attribute("version") = "1.0";
    xml_declaration.append_attribute("encoding") = "UTF-8";


    pugi::xml_node diff = diff_patch.append_child("diff");
    for (auto& element: deltas) {
        
        if (element.second == "REM/ADD") {
            //REMOVE + ADD LOGIC
        } else if (element.second == "REMOVE") {
            pugi::xml_node remove_directive = diff.append_child("remove");
            //<remove sel="doc/foo[@a='1']" ws="after"/>
            pugi::xml_attribute attr = remove_directive.append_attribute("sel");
            attr.set_value(element.first.getXPath().substr(1).c_str());
        } else if (element.second == "ADD") {
            pugi::xml_node add_directive = diff.append_child("add");
            pugi::xml_attribute attr = add_directive.append_attribute("sel");

            auto pos = element.first.getXPath().find_last_of("/");
            attr.set_value(element.first.getXPath().substr(1, pos - 1).c_str());

            pugi::xml_node child = add_directive.append_child(element.first.getName().c_str());
            for (auto& attrib: element.first.getAttributes()) {
                pugi::xml_attribute attribute = child.append_attribute(attrib.first.c_str());
                attribute.set_value(attrib.second.c_str());
            }
        } else if (element.second == "REPLACE") {
            /*
            TODO: NEED TO CHANGE THE LOGIC FOR REPLACE, when using the replace directive on an element, you need to provide child nodes if they exist
            so behavior needs to be changed according to the following rules:
                1. If the element has changed and no children are present, replace the element
                    - For optimization: If sum(size_of(add_attribute_directives)) < size_of(add_element_directive), then use smaller option add_attribute, 
                        else add_element
                2. If just the element attributes values changed, and children are present, replace/add/remove ATTRIBUTES
                (Optimization) - More efficient to replace all nested elements in a single element replacement directive, if there are also multiple changes occuring in 
                    the child nodes (which may also be nested). i.e size of accumulative attribute replacements are larger than the nested element as a whole
            */
            pugi::xml_node replace_directive = diff.append_child("replace");
            // Special Logic for MPD Node, May need to be utilized for optimization (Replace single attribute instead of node)
            if (element.first.getName() == "MPD") {
                pugi::xml_attribute attribute = replace_directive.append_attribute("sel");
                attribute.set_value("MPD/@publishTime");
                pugi::xml_node textNode = replace_directive.append_child(pugi::node_pcdata);
                textNode.text() = element.first.getAttributes().at("publishTime").c_str();
            } else {
                pugi::xml_attribute attribute = replace_directive.append_attribute("sel");
                attribute.set_value(element.first.getXPath().substr(1).c_str());
                pugi::xml_node child = replace_directive.append_child(element.first.getName().c_str());
                for (auto& attrib: element.first.getAttributes()) {
                    pugi::xml_attribute attribute = child.append_attribute(attrib.first.c_str());
                    attribute.set_value(attrib.second.c_str());
                }
            }
        } else {
            //ERROR!
            std::cerr << "Unrecognized Directive: " << element.second << std::endl;
        }

    }

    // Save the XML document to a string
    std::ostringstream oss;
    // Use Tab indentation for readability
    diff_patch.save(oss, "    ");
    // save diff_patch to file
    diff_patch.save_file("diff_patch.xml");
    std::string xmlString = oss.str();
    // Output the XML string
    std::cerr << xmlString << std::endl;
}

/**
 * - Recursive function for processing the XMLTree and identifiying nodes that do not exist in mpd2
 * - Nodes that cannot be identified in mpd2 are copied into an XMLElement object in the pass by reference diffset
 * - The index_map is used to track index of identites that do not contain an "id" field, this is later referenced 
 * - in XPath queuries for elements that do not contain an "id" attribute
*/
void process_node(const pugi::xml_node& mpd1_node, const pugi::xml_document& mpd2, std::unordered_map<std::string, XMLElement>& diffset, std::unordered_map<std::string, u_int>& index_map, std::string xpath="") {
    xpath = xpath + "/" + mpd1_node.name();
    XMLElement element;
    element.setName(mpd1_node.name());
    std::string full_query;

    // If the element contains attributes
    if (!mpd1_node.attributes().empty()) {
        std::stringstream elem_attr_filter_ss;
        std::stringstream id_info_ss;

        // Create a filter stream that populates based on attribute values,
        // Used for searching for identical node in other XML document
        elem_attr_filter_ss << "[";
        for (auto it = mpd1_node.attributes().begin(); it != mpd1_node.attributes().end(); it++) {

            if (it->name() == (std::string)"id") {
                id_info_ss << "[@" << it->name() << "='" << it->value() << "']";
            }
            element.addAttribute(it->name(), it->value());

            elem_attr_filter_ss << "@" << it->name() << "=" << "'" << it->value() << "'";
            if (std::next(it) != mpd1_node.attributes().end()) {
                elem_attr_filter_ss << " and ";
            }
        }
        elem_attr_filter_ss << "]";

        full_query = xpath + elem_attr_filter_ss.str();
        

        if (!id_info_ss.str().empty()) {
            xpath = xpath + id_info_ss.str();
        } else {
            index_map[xpath]++;
            element.index = index_map[xpath];
        }

    } else {
        index_map[xpath]++;
        element.index = index_map[xpath];
        full_query = xpath;
    }

    std::cerr << "MPD2 Query: " << full_query << std::endl;

    // Check if node exists in mpd2
    pugi::xpath_query element_query(full_query.c_str());
    pugi::xpath_node_set results = element_query.evaluate_node_set(mpd2);  

    if (results.empty()) {
        std::cerr << "Element does not exist in MPD2!" << std::endl;

        // Add [@id="<val>"] to XPath if the attribute is present for future XPath queries referencing this element
        if (element.getAttributes().find("id") == element.getAttributes().end()) {
            std::stringstream idx_ss;
            idx_ss << "[" << index_map[xpath] << "]";
            xpath = xpath + idx_ss.str();
        }
        element.setXPath(xpath);
        diffset[xpath] = element;
    } else {
        if (results.size() > 1) {
            std::cerr << "ERROR! Elements exists " << results.size() << " times in MPD2!";
            exit(1);
        } else {
            std::cerr << "Element exists in MPD2." << std::endl;
        }
    }
    std::cerr << "XPath: " << xpath << std::endl << std::endl;
    
    // Process child nodes recursivly
    for (pugi::xml_node mpd1_child : mpd1_node.children()) {
        process_node(mpd1_child, mpd2, diffset, index_map, xpath);
    }
}

// Utility function for debugging
void print_diffset(std::unordered_map<std::string, XMLElement>& diffset) {
    for (const auto& elem: diffset) {
        std::cerr << "XPath: " << elem.second.getXPath() << std::endl;
        for (const auto& attrib: elem.second.getAttributes()) {
            std::cerr << "  " << attrib.first << ": " << attrib.second << std::endl;
        }
        std::cerr << std::endl;
    }
}

#ifdef __cplusplus
extern "C" {
#endif

void morph_diffs(const char* client_mpd) {

    const char* current_mpd = "current.mpd";
    struct stat buffer;

    std::cerr << "\n\n" << "Starting Diff..." << "\n\n";

    if (stat (current_mpd, &buffer) != 0)
    {
        //there is no current mpd, copy incoming to current and return
        std::cerr << "creating current.mpd because it doesn't exist" << std::endl;
        std::ifstream src(client_mpd);
        std::ofstream dst(current_mpd);
        dst << src.rdbuf();
        return;
    }

    //TBD: check if mpd1 exists, is accessible
    pugi::xml_document client_doc;
    client_doc.load_file((const char*)client_mpd);

    pugi::xml_document current_doc;
    current_doc.load_file((const char*)current_mpd);

    std::string client_ptime = client_doc.child("MPD").attribute("publishTime").value();
    std::string current_ptime = current_doc.child("MPD").attribute("publishTime").value();

    // publishTime is the same in current and incoming, return
    if (client_ptime == current_ptime)
    {
        std::cerr << "mpd1 ptime    " << client_ptime << std::endl;
        std::cerr << "current ptime " << current_ptime << std::endl;
        std::cerr << "they are the same, done" << std::endl;
        return;
    }

    /*
    simple_walker walker;
    mpd1_doc.traverse(walker); //array of xpaths in walker.all_elements

    size_t pos = 0;
    for (int i = 0; i < (int)walker.all_elements.size(); i++)
    {
        cout << endl;

        pos = walker.all_elements[i].find(TOK);

        string elpath = walker.all_elements[i].substr(0, pos);
        string value = walker.all_elements[i].substr(pos + TOKLEN);

        //cout << elpath << "\n";
        //out << value << endl;
        
        //std::string val = node.node().attribute("scanType").value();
    }
    */

    /***************
    Create Diff Sets
    ****************/
    pugi::xml_node client_root = client_doc.child("MPD");
    // contains elements missing in current MPD
    std::unordered_map<std::string, u_int> current_imap;
    std::unordered_map<std::string, XMLElement> current_missing;
    process_node(client_root, current_doc, current_missing, current_imap);


    pugi::xml_node current_root = current_doc.child("MPD");
    // contains elements missing in client MPD
    std::unordered_map<std::string, u_int> client_imap;
    std::unordered_map<std::string, XMLElement> client_missing;
    process_node(current_root, client_doc, client_missing, client_imap);

    /*
        If we use a set instead of a map, the combination of S Path + attributes will make unique objects,
        however searching for keys in the map become dificulet when we only care abut xpath

        Unoredered map should be of Xpath -> List<XMLElement> (list is ordered)
        if list is > 0 compare object indepenedently for add/remove/replace
    */

    std::cerr << "\nElements present in Client MPD but missing in Current MPD: " << std::endl;
    print_diffset(current_missing);

    std::cerr << "\nElements present in Current MPD but missing in Client MPD: " << std::endl;
    print_diffset(client_missing);

    /***************
    Compare Diff Sets

    TODO - POSSIBLY Utilize XPATH OF Element to perform a lookup when adding/replacing grandparent elements (elements that contain multiple generations of child elements )
    ****************/

    std::map<XMLElement, std::string> delta_map;
    for (const auto& pair: current_missing) {
        auto it = client_missing.find(pair.first);

        // If entry exist in client missing
        if (it != client_missing.end()) {
            // Add element to update_map with 'REPLACE' operation
            delta_map[it->second] = "REPLACE";
            // Erase the entry from the client map
            client_missing.erase(it->first);

            /*
            if (it->second.similar(pair.second)) {
                // Add element to update_map with 'REPLACE' operation
                delta_map[it->second] = "REPLACE";

                // Erase the entry from the client map
                client_missing.erase(it->first);
            } else { 
                // Remove + add directive

                std::cerr << "Key " << it->first << " found but element is not similar!" << std::endl;
                delta_map[it->second] = "REM/ADD";
                client_missing.erase(it->first);
            }
            */
            
        } else {
            std::cerr << "Key " << pair.first << " does not exist in the map." << std::endl;
            // Add element to update_map with 'REMOVE' operation
            delta_map[pair.second] = "REMOVE";
        }
    }
    // What is left in the client_missing map should be 'ADDED'
    for (const auto& pair: client_missing) {
        delta_map[pair.second] = "ADD";
    }

    // Print out Diffset
    std::cerr << "\n----------------------------------------\n" << std::endl;
    for (const auto& pair: delta_map) {
        std::cerr << "XPath: " << pair.first.getXPath() << std::endl;
        std::cerr << "Directive: " << pair.second << std::endl;
        std::cerr << "Attributes:" << std::endl;
        for (const auto& attr: pair.first.getAttributes())
            std::cerr << "  " << attr.first << ": " << attr.second << std::endl;
        std::cerr << std::endl;
    }

    /**
     * Proposed Fix for indexing
     * (IMPLEMENTED)
     * 1) Keep map of xpath with associated element index relative to the parent
     * Pros: Fast Map access
     * Cons: Memory Consumption (may be an issue with exxeccively large mpd)
     * 
     * 
     * 2) For each missing element, search Xpath of own document, Match element with index in list returned to determine XPath Selector Index
     * Pros: Only execute for missing elements
     * Cons: Each time we execute an expath query it requires computational resources, to minimize this would
     * Want to check the existance of a similar element 
    */

    /*
        TODO/BUG NOTES:
        - Use examples from rfc5261 / Find a testing framework
        - No Testing done for nested elements
    */


    /***************
    Create Diff File
    ****************/
    std::cerr << "-------------------------------------------" << std::endl;
    translate_deltas(delta_map);
}

#ifdef __cplusplus
}
#endif
