/*
 * Copyright (C) ab
 */
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <sys/stat.h>
#include <fstream>
#include <set>
#include <unordered_map>
#include <map>
#include "pugixml.hpp"
#include "ngx_diffstub_internal.hpp"
#include "diffstub_xml_node.hpp"


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
std::string translate_deltas(const std::map<XMLElement, std::string>& deltas, std::string& old_pub_time, std::string& new_pub_time) {
    pugi::xml_document diff_patch;

    // Set the version and encoding attributes of the XML declaration
    pugi::xml_node xml_declaration = diff_patch.append_child(pugi::node_declaration);
    xml_declaration.append_attribute("version") = "1.0";
    xml_declaration.append_attribute("encoding") = "UTF-8";
    xml_declaration.append_attribute("standalone") = "yes";


    pugi::xml_node patch = diff_patch.append_child("Patch");
    // TODO: Replace with reading attributes from Patch Schema file, but we only want to do once, not for every call (pass in header obj?)
    pugi::xml_attribute xmlns = patch.append_attribute("xmlns");
    xmlns.set_value("urn:mpeg:dash:schema:mpd-patch:2020");
    pugi::xml_attribute xmlns_xsi = patch.append_attribute("xmlns:xsi");
    xmlns_xsi.set_value("http://www.w3.org/2001/XMLSchema-instance");
    pugi::xml_attribute xsi_schema_loc = patch.append_attribute("xsi:schemaLocation");
    xsi_schema_loc.set_value("urn:mpeg:dash:schema:mpd-patch:2020 DASH-MPD-PATCH.xsd");

    // Where to get this in real time?
    pugi::xml_attribute mpd_id = patch.append_attribute("mpdId");
    mpd_id.set_value("?");
    pugi::xml_attribute orig_pub_time = patch.append_attribute("originalPublishTime");
    orig_pub_time.set_value(old_pub_time.c_str());
    pugi::xml_attribute pub_time = patch.append_attribute("publishTime");
    pub_time.set_value(new_pub_time.c_str());

    

    for (auto& element: deltas) {
        if (element.first.type == "Element") {
            if (element.second == "REMOVE") {
                pugi::xml_node remove_directive = patch.append_child("remove");
                //<remove sel="doc/foo[@a='1']" ws="after"/>
                pugi::xml_attribute attr = remove_directive.append_attribute("sel");
                attr.set_value(element.first.getXPath().substr(1).c_str());
            } else if (element.second == "ADD") {
                // Search document on this current xpath, to combine list items (i.e. segments) in one directive
                auto pos = element.first.getXPath().find_last_of("/");
                std::string sel_xpath = element.first.getXPath().substr(1, pos - 1);

                std::stringstream query;
                query << "Patch/add[@sel=\"" << sel_xpath << "\"]";
                pugi::xpath_query diff_query(query.str().c_str());
                pugi::xpath_node_set results = diff_patch.select_nodes(diff_query);

                // IF we have not encountered this selector before, add a new element, otherwise edit the existing 'ADD' selector
                if (results.empty()) {
                    pugi::xml_node add_directive = patch.append_child("add");
                    pugi::xml_attribute attr = add_directive.append_attribute("sel");

                    auto pos = element.first.getXPath().find_last_of("/");
                    attr.set_value(element.first.getXPath().substr(1, pos - 1).c_str());

                    pugi::xml_node child = add_directive.append_child(element.first.getName().c_str());
                    for (auto& attrib: element.first.getAttributes()) {
                        pugi::xml_attribute attribute = child.append_attribute(attrib.first.c_str());
                        attribute.set_value(attrib.second.c_str());
                    }
                } else if (results.size() == 1) {
                    pugi::xml_node matched_node = results.first().node();

                    pugi::xml_node child = matched_node.append_child(element.first.getName().c_str());
                    for (auto& attrib: element.first.getAttributes()) {
                        pugi::xml_attribute attribute = child.append_attribute(attrib.first.c_str());
                        attribute.set_value(attrib.second.c_str());
                    }
                } else {
                    std::cerr << "ERROR: Result size in Diff Patch is > 1!" << std::endl;
                    exit(1);
                }
            } else if (element.second == "REPLACE") {
                /*
                When using the replace directive on an element, you need to provide child nodes if they exist
                so behavior needs to be changed according to the following rules:
                    1. If the element has changed and no children are present, replace the element
                        - For optimization: If sum(size_of(add_attribute_directives)) < size_of(add_element_directive), then use smaller option add_attribute, 
                            else add_element
                    2. If just the element attributes values changed, and children are present, replace/add/remove ATTRIBUTES
                    (Optimization) - More efficient to replace all nested elements in a single element replacement directive, if there are also multiple changes occuring in 
                        the child nodes (which may also be nested). i.e size of accumulative attribute replacements are larger than the nested element as a whole
                */
                
                pugi::xml_node replace_directive = patch.append_child("replace");
                pugi::xml_attribute attribute = replace_directive.append_attribute("sel");
                attribute.set_value(element.first.getXPath().substr(1).c_str());
                pugi::xml_node child = replace_directive.append_child(element.first.getName().c_str());
                for (auto& attrib: element.first.getAttributes()) {
                    pugi::xml_attribute attribute = child.append_attribute(attrib.first.c_str());
                    attribute.set_value(attrib.second.c_str());
                }
            } else if (element.second == "ADDATTR") {
                for (auto& attribute : element.first.getAttributes()) {
                    pugi::xml_node add_directive = patch.append_child("add");
                    pugi::xml_attribute sel_attr = add_directive.append_attribute("sel");
                    std::stringstream sel_path;
                    sel_path << element.first.getXPath().substr(1).c_str() << "/@" << attribute.first;
                    sel_attr.set_value(sel_path.str().c_str());
                    pugi::xml_node textNode = add_directive.append_child(pugi::node_pcdata);
                    textNode.text() = attribute.second.c_str();
                }
            } else if (element.second == "REMATTR") {
                for (auto& attribute : element.first.getAttributes()) {
                    pugi::xml_node rem_directive = patch.append_child("remove");
                    pugi::xml_attribute sel_attr = rem_directive.append_attribute("sel");
                    std::stringstream sel_path;
                    sel_path << element.first.getXPath().substr(1).c_str() << "/@" << attribute.first;
                    sel_attr.set_value(sel_path.str().c_str());
                    pugi::xml_node textNode = rem_directive.append_child(pugi::node_pcdata);
                    textNode.text() = attribute.second.c_str();
                }
            } else if (element.second == "REPATTR") {
                for (auto& attribute : element.first.getAttributes()) {
                    pugi::xml_node replace_directive = patch.append_child("replace");
                    pugi::xml_attribute sel_attr = replace_directive.append_attribute("sel");
                    std::stringstream sel_path;
                    sel_path << element.first.getXPath().substr(1).c_str() << "/@" << attribute.first;
                    sel_attr.set_value(sel_path.str().c_str());
                    pugi::xml_node text_node = replace_directive.append_child(pugi::node_pcdata);
                    text_node.text() = attribute.second.c_str();
                }
            } else {
                std::cerr << "Unrecognized Directive for Element Node: " << element.second << std::endl;
                exit(1);
            } 
        } else if (element.first.type == "Text") {
            if (element.second == "ADD") {
                std::cerr << "Text Node XPath: " << element.first.getXPath() << std::endl;
                
                pugi::xml_node add_directive = patch.append_child("add");
                pugi::xml_attribute sel_attr = add_directive.append_attribute("sel");

                /*
                std::stringstream sel_path;
                // Strip beginning '/' from XPath for text replace selector
                sel_path << element.first.getXPath().substr(1).c_str();
                sel_attr.set_value(sel_path.str().c_str());
                */

            } else if (element.second == "REMOVE") {
                pugi::xml_node remove_directive = patch.append_child("remove");
                pugi::xml_attribute sel_attr = remove_directive.append_attribute("sel");

                std::stringstream sel_path;
                // Strip beginning '/' from XPath for text replace selector
                sel_path << element.first.getXPath().substr(1).c_str();
                sel_attr.set_value(sel_path.str().c_str());
            } else if (element.second == "REPLACE") {
                pugi::xml_node replace_directive = patch.append_child("replace");
                pugi::xml_attribute sel_attr = replace_directive.append_attribute("sel");

                std::stringstream sel_path;
                // Strip beginning '/' from XPath for text replace selector
                sel_path << element.first.getXPath().substr(1).c_str();
                sel_attr.set_value(sel_path.str().c_str());
                pugi::xml_node text_node = replace_directive.append_child(pugi::node_pcdata);
                text_node.text() = element.first.getValue().c_str();
            } else {
                std::cerr << "Unrecognized Directive For Text Node: " << element.second << std::endl;
                exit(1);
            }
        } else {
            std::cerr << "Unrecognized Node Type! " << element.first.type << std::endl;
            exit(1);
        }
        

    }

    // Save the XML document to a string
    std::ostringstream oss;
    diff_patch.save(oss, "\t");
    std::string diff_patch_str = oss.str();

    return diff_patch_str;
}

std::string NodeTypeToString(pugi::xml_node_type type) {
    switch (type) {
        case pugi::node_element:    return "Element";
        case pugi::node_pcdata:     return "Text";
        case pugi::node_cdata:      return "CDATA";
        case pugi::node_comment:    return "Comment";
        case pugi::node_pi:         return "Processing Instruction";
        case pugi::node_declaration: return "Declaration";
        case pugi::node_doctype:    return "Document Type";
        default:                    return "Unknown";
    }
}

/**
 * - Recursive function for processing the XMLTree and identifiying nodes that do not exist in mpd2
 * - Nodes that cannot be identified in mpd2 are copied into an XMLElement object in the pass by reference diffset
 * - The index_map is used to track index of identites that do not contain an "id" field, this is later referenced 
 * - in XPath queuries for elements that do not contain an "id" attribute
*/
void process_node(const pugi::xml_node& mpd1_node, const pugi::xml_document& mpd2, std::unordered_map<std::string, XMLElement>& diffset, std::unordered_map<std::string, u_int>& index_map, std::string xpath="") {
    if (mpd1_node.type() == pugi::node_element) {
        xpath = xpath + "/" + mpd1_node.name();
        XMLElement element;
        element.type = NodeTypeToString(mpd1_node.type());

        if (mpd1_node.children().empty()) {
            element.has_children = false;
        } else {
            element.has_children = true;
        }
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
        // TODO:  This is currently throwing an error for Text Nodes (i.e. Patch Location))
        // Need to write logic that addreses this (check if a child is a text note before proceeding?)
        // i.e. If node has only 1 child and is of type PCDATA...

        pugi::xpath_query element_query(full_query.c_str());
        pugi::xpath_node_set results = mpd2.select_nodes(element_query);

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
                std::cerr << "ERROR! Queried element " << full_query << " exists " << results.size() << " times in MPD2!" << std::endl;
                exit(1);
            } else {
                // Further Inspect the matching element for matching attribute values
                // This is required in the case where all attributes are equal in value, but one may have been added/removed from the set
                pugi::xml_node matched_node = results.first().node();

                size_t current_attr_size = std::distance(mpd1_node.attributes_begin(), mpd1_node.attributes_end());
                size_t matched_attr_size = std::distance(matched_node.attributes_begin(), matched_node.attributes_end());
                if (current_attr_size == matched_attr_size) {
                    std::cerr << "Element " << full_query << " exists in MPD2." << std::endl;
                } else {
                    if (element.getAttributes().find("id") == element.getAttributes().end()) {
                        std::stringstream idx_ss;
                        idx_ss << "[" << index_map[xpath] << "]";
                        xpath = xpath + idx_ss.str();
                    }
                    element.setXPath(xpath);
                    diffset[xpath] = element;
                }

                /*
                for (pugi::xml_attribute attr : matched_node.attributes()) {
                    const char* attr_name = attr.name();
                    const char* attr_value = attr.value();
                    std::cout << "Attribute name: " << attr_name << ", Attribute value: " << attr_value << std::endl;
                }
                */

                
            }
        }
        std::cerr << "XPath: " << xpath << std::endl << std::endl;
        
        // Process child nodes recursivly
        // Here we could recursivly pass in the current element to add children to that element, could be useful for optimization (replace element + children in one directive)
        // Is it too inneficient to calculate the resulting string of both operations (replace element attributes + replace children vs replace element + children) and choose the smaller one?
        for (pugi::xml_node mpd1_child : mpd1_node.children()) {
            process_node(mpd1_child, mpd2, diffset, index_map, xpath);
        }
    } else if (mpd1_node.type() == pugi::node_pcdata) {
        //If node is a text node, we must process it differently
        std::stringstream idx_ss;
        idx_ss << "[" << index_map[xpath] << "]";
        xpath = xpath + idx_ss.str() + "/text()";
        std::cerr << "Text Node Path: " << xpath << std::endl;

        pugi::xpath_query test_query(xpath.c_str());
        pugi::xpath_node_set results = mpd2.select_nodes(test_query);

        if (!results.empty()) {
            if (results.size() == 1) {
                std::string mpd1_text_content = mpd1_node.value();
                std::string mpd2_text_content = results.first().node().value();

                if (mpd1_text_content == mpd2_text_content) {
                    std::cerr << "Element " << xpath << " matches in MPD2." << std::endl;
                } else {
                    XMLElement element;
                    element.type = NodeTypeToString(mpd1_node.type());
                    element.setXPath(xpath);
                    element.setValue(mpd1_node.value());
                    diffset[xpath] = element;
                }
            } else {
                std::cerr << "ERROR: Multiple Results found for text node: " << xpath << std::endl;
                exit(1);
            }
                
        } else {
            std::cout << "XPath query result is empty." << std::endl;
        }

    } else {
        std::cerr << "ERROR: Unhandled Node Type: " << NodeTypeToString(mpd1_node.type()) << std::endl;
        exit(1);
    }
    
}

// Utility function for debugging
void print_diffset(std::unordered_map<std::string, XMLElement>& diffset) {
    for (const auto& elem: diffset) {
        std::cerr << "XPath: " << elem.second.getXPath() << std::endl;
        std::cerr << "Has Children: " << elem.second.has_children << std::endl;
        for (const auto& attrib: elem.second.getAttributes()) {
            std::cerr << "  " << attrib.first << ": " << attrib.second << std::endl;
        }
        std::cerr << std::endl;
    }
}

#ifdef __cplusplus
extern "C" {
#endif


const char* morph_diffs(const char* old_mpd, const char* new_mpd) {
    std::cerr << "\n\n" << "Starting Diff..." << "\n\n";

    //TBD: check if mpd1 exists, is accessible
    pugi::xml_document client_doc;
    client_doc.load_file((const char*) old_mpd);

    pugi::xml_document current_doc;
    current_doc.load_file((const char*) new_mpd);

    std::string old_ptime = client_doc.child("MPD").attribute("publishTime").value();
    std::string new_ptime = current_doc.child("MPD").attribute("publishTime").value();

    // publishTime is the same in current and incoming, return
    if (old_ptime == new_ptime)
    {
        std::cerr << "mpd1 ptime    " << old_ptime << std::endl;
        std::cerr << "current ptime " << new_ptime << std::endl;
        std::cerr << "they are the same, done" << std::endl;
        return "";
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
            if(it->second.has_children) {
                // Need to ADD/REM/REPLACE All Attributes for this element since it contains children
                // CURRENTLY A BUG WHEN ADDED OR REMOVED ATTRIBUTE PRESENT, LOOK INTO
                XMLElement add_attr;
                for(const auto& attr: it->second.getAttributes()) {
                    auto old_it = pair.second.getAttributes().find(attr.first);
                    // If this attribute is not found at all
                    if (old_it == pair.second.getAttributes().end()) {
                        add_attr.addAttribute(attr.first, attr.second);
                    }
                }
                if (!add_attr.getAttributes().empty()) {
                    add_attr.type = pair.second.type;
                    add_attr.setXPath(it->second.getXPath());
                    add_attr.setName(it->second.getName());
                    add_attr.index = it->second.index;
                    add_attr.has_children = true;
                    delta_map[add_attr] = "ADDATTR";
                }
                

                XMLElement rem_attr;
                for(const auto& attr: pair.second.getAttributes()) {
                    auto new_it = it->second.getAttributes().find(attr.first);
                    // If this attribute is not found at all
                    if (new_it == it->second.getAttributes().end()) {
                        rem_attr.addAttribute(attr.first, attr.second);
                    }
                }
                if (!rem_attr.getAttributes().empty()) {
                    rem_attr.type = pair.second.type;
                    rem_attr.setXPath(it->second.getXPath());
                    rem_attr.setName(it->second.getName());
                    rem_attr.index = it->second.index;
                    rem_attr.has_children = true;
                    delta_map[rem_attr] = "REMATTR";
                }

                XMLElement rep_attr;
                for(const auto& attr: it->second.getAttributes()) {
                    auto rep_it = pair.second.getAttributes().find(attr.first);
                    // If this attribute is not found at all
                    if (rep_it != pair.second.getAttributes().end()) {
                        // Add to the element to replacement list if it is not equal
                        if (rep_it->second != attr.second) {
                            rep_attr.addAttribute(attr.first, attr.second);
                        }
                    }
                }
                if (!rep_attr.getAttributes().empty()) {
                    rep_attr.type = pair.second.type;
                    rep_attr.setXPath(it->second.getXPath());
                    rep_attr.setName(it->second.getName());
                    rep_attr.index = it->second.index;
                    rep_attr.has_children = true;
                    delta_map[rep_attr] = "REPATTR";
                }
                
                
            } else {
                delta_map[it->second] = "REPLACE";
            }

            // Erase the entry from the client map
            client_missing.erase(it->first);
            
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
    Create Diff File And Return
    ****************/
    std::cerr << "-------------------------------------------" << std::endl;
    std::string diff_patch_str = translate_deltas(delta_map, old_ptime, new_ptime);

    // Allocate memory for the C-style string and copy the content, if we dont do this the string object is destroyed on
    // function return and we are left with a dangling pointer.  Need to make sure to free memory in NGINX module (or main for testing)!
    char* diff_patch_c_str = (char*) malloc(diff_patch_str.size() + 1);
    std::strcpy(diff_patch_c_str, diff_patch_str.c_str());

    // Return the C-style string
    return diff_patch_c_str;
}

// Changed 'ttl' to char* because it needs to be a string when setting the value of a text node
const char* add_patch_location(const char* mpd, const char* patch_location, const char* ttl) {
    // Load File
    pugi::xml_document mpd_xml;
    mpd_xml.load_file((const char*) mpd);

    // Add Patch Location Element as the first child
    pugi::xml_node mpd_elem = mpd_xml.child("MPD");
    pugi::xml_node pl_elem = mpd_elem.insert_child_before("PatchLocation", mpd_elem.first_child());
    pugi::xml_attribute ttl_attr = pl_elem.append_attribute("ttl");
    ttl_attr.set_value(ttl);
    pugi::xml_node pl_text_field = pl_elem.append_child(pugi::node_pcdata);
    pl_text_field.text() = patch_location;

    // Save the XML document to a string
    std::ostringstream oss;
    mpd_xml.save(oss, "\t");
    std::string xml_str = oss.str();

    // Allocate memory for the C-style string and copy the content, if we dont do this the string object is destroyed on
    // function return and we are left with a dangling pointer.  Need to make sure to free memory in NGINX module (or main for testing)!
    char* xml_c_str = (char*) malloc(xml_str.size() + 1);
    std::strcpy(xml_c_str, xml_str.c_str());

    // Return the C-style string
    return xml_c_str;
}

#ifdef __cplusplus
}
#endif
