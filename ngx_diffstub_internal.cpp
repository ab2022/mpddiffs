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

Author: Erik Ponder, Jovan Rosario, Alex Balk
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
#include <tuple>
#include <vector>
#include "pugixml.hpp"
#include "ngx_diffstub_internal.hpp"
#include "diffstub_xml_node.hpp"


pugi::xml_node get_parent_directive(const XMLElement& element, const pugi::xml_document& diff_patch, std::string directive) {
    size_t delim_ct = 0;
    for (char c : element.getXPath()) {
        if (c == '/') {
            delim_ct++;
        }
    }

    auto pos = element.getXPath().find_last_of("/");
    std::string sel_xpath = element.getXPath().substr(0, pos);
    for (size_t i = 0; i < delim_ct - 2; i++) {   // Check for elements 1 level below MPD (cannot add MPD)
        std::stringstream query;
        query << "/Patch/" << directive << "[@sel=\"" << sel_xpath << "\"]";
        pugi::xpath_query diff_query(query.str().c_str());
        pugi::xpath_node_set results = diff_patch.select_nodes(diff_query);

        if (!results.empty()) {
            if (results.size() == 1) {
                return results.first().node(); // Parent node was found in diff_patch

            } else {
                std::cerr << "UNEXPECTED NUMBER OF RESULTS FOUND" << std::endl;
                exit(1);
            }
        }

        pos = sel_xpath.find_last_of("/");
        sel_xpath = sel_xpath.substr(0, pos);
    }

    return pugi::xml_node(); // Parent node was not found in diff_patch
}


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

    
    //POSSIBLY CREATE AN INDEX MAP like the one used in morph_diffs
    for (auto& element: deltas) {
        // Ignore any nested children with a 'remove' selector to a parent node already present
        // This logic applies to any node type
        if (element.first.type == "Element") {
            if (element.second == "REMOVE") {
                if (get_parent_directive(element.first, diff_patch, "remove").empty()) {  // If parent doesnt exist
                    pugi::xml_node remove_directive = patch.append_child("remove");
                    pugi::xml_attribute attr = remove_directive.append_attribute("sel");
                    attr.set_value(element.first.getXPath().c_str());
                }
            } else if (element.second == "ADD") {
                pugi::xml_node parent_directive = get_parent_directive(element.first, diff_patch, "add");
                if (!parent_directive.empty()) {   // If parent Exists
                    std::string parent_sel = parent_directive.attribute("sel").value();
                    std::cerr << "TAG: Entering Nested Directive Block!!" << std::endl;
                    std::cerr << "Element Name: " << element.first.getName() << std::endl;
                    std::cerr << "Full XPath: " << element.first.getXPath() << std::endl;
                    std::cerr << "Add Directive 'sel': " << parent_sel << std::endl;
                    std::string remaining_xpath = element.first.getXPath().substr(parent_sel.length());
                    std::cerr << "Remaining Path: " << remaining_xpath << std::endl;

                    auto split_pos = remaining_xpath.find_last_of("/");
                    std::string next_child_xpath = remaining_xpath.substr(1, split_pos - 1);
                    std::cerr << "Next Child: " << next_child_xpath << std::endl;

                    //TODO: THIS IS A BAND AID FIX, DOES NOT WORK IF THERE ARE MULTIPLE ELEMENTS ADDRESSED BY INDEX
                    //Band aid fix failing with attribute identifiers
                    //Make Functtion modular and use here? -> add child element
                    pugi::xpath_node_set results;
                    if (element.first.getName() == "S") {
                        std::cerr << "TAG: S Nested ELEMENT!" << std::endl;
                    } else {
                        std::cerr << "TAG: Standard Nested Element!" << std::endl;
                        auto last_bracket_start_pos = next_child_xpath.find_last_of("[");
                        results = parent_directive.select_nodes(next_child_xpath.substr(0, last_bracket_start_pos).c_str()); //TODO
                        std::cerr << next_child_xpath.substr(0, last_bracket_start_pos) << std::endl;
                    }

                    if (!results.empty()) {
                        if (results.size() == 1) {
                            std::cerr << "GOOD!" << std::endl;
                            pugi::xml_node baby =  results.first().node().append_child(element.first.getName().c_str());
                            // MODIFY XPATH?
                            for (auto& attrib: element.first.getAttributes()) {
                                pugi::xml_attribute baby_attr = baby.append_attribute(attrib.first.c_str());
                                baby_attr.set_value(attrib.second.c_str());
                            }
                        } else {
                            //TODO UTILIZE ELEMENT INDEX FOR QUERY? (is it possible?)
                            //Grab all generic templates,
                        }
                        
                    } else {
                        std::cerr << "EMPTY!" << std::endl;
                    }


                } else {
                    // Search document on this current xpath, to combine list items (i.e. segments) in one directive
                    if (element.first.getName() == "S") {
                        auto pos = element.first.getXPath().find_last_of("/");
                        std::string sel_xpath = element.first.getXPath().substr(0, pos);
                        std::stringstream query;
                        //query << "/Patch/add[starts-with(@sel, \"" << sel_xpath << "/S\")]";
                        query << "/Patch/add[starts-with(@sel, \"" << sel_xpath << "\")]";
                        pugi::xpath_query diff_query(query.str().c_str());
                        pugi::xpath_node_set results = diff_patch.select_nodes(diff_query);   // Grab elements existing in diff patch for possible modification 

                        bool new_node = true;

                        // IF we have not encountered this selector before, add a new Segment, otherwise edit the existing 'ADD' selector
                        if (!results.empty()) {
                            for (const pugi::xpath_node& matched_xpath_node: results) {
                                pugi::xml_node matched_patch_node = matched_xpath_node.node(); 

                                for(pugi::xml_node child: matched_patch_node.children()) {    // Iterate through elements that are already being added
                                    pugi::xml_attribute attr = child.attribute(element.first.selector_attrib.c_str());
                                    const char* attr_value = attr.value();
                                    const char* expected_value;

                                    if (strcmp(attr_value, "before") == 0) {
                                        expected_value = element.first.next_sibling_rel_val.c_str();
                                    } else {
                                        expected_value = element.first.prev_sibling_rel_val.c_str();
                                    } 
                                    
                                    pugi::xml_node segment_elem;
                                    if (strcmp(attr_value, expected_value) == 0) {              // Existing t||n attribute value in patch matches attr selector
                                        new_node = false;
                                        if(element.first.relative_pos == "before") {            // Add element to list before specific element
                                            segment_elem = matched_patch_node.insert_child_before(element.first.getName().c_str(), child);
                                        } else if (element.first.relative_pos == "after") {     // Add element to list after specific element
                                            segment_elem = matched_patch_node.insert_child_after(element.first.getName().c_str(), child);

                                            pugi::xml_attribute pos_attr = matched_patch_node.attribute("pos");
                                            if(strcmp(pos_attr.value(), "before") == 0) {       
                                                pugi::xml_attribute sel_attr = matched_patch_node.attribute("sel");
                                                std::string base_xpath = element.first.getXPath().substr(0, pos);
                                                
                                                if(element.first.next_sibling_rel_val != "") {  
                                                    std::stringstream sel_path_ss;                
                                                    sel_path_ss << base_xpath << "/S[@" << element.first.selector_attrib << "='" << element.first.next_sibling_rel_val << "']";
                                                    sel_attr.set_value(sel_path_ss.str().c_str());
                                                } else {                                   
                                                    sel_attr.set_value(base_xpath.c_str());       // Change selector from idividual segment to SegmentTimeline
                                                    matched_patch_node.remove_attribute("pos");   // remove positional attribute
                                                }
                                            }
                                        } else {
                                            std::cerr << "ERROR: Unexpected positional selector encountered! (" << element.first.relative_pos << ")" << std::endl;
                                            exit(1);
                                        }
                                        
                                        for (auto& attrib: element.first.getAttributes()) {
                                            pugi::xml_attribute attribute = segment_elem.append_attribute(attrib.first.c_str());
                                            attribute.set_value(attrib.second.c_str());
                                        }

                                        break;
                                    }

                                }

                                if (new_node == false) {
                                    break;
                                }
                            }
                        } 
                        
                        if (new_node) {                                                                    // Results are empty, Add a new segment
                            pugi::xml_node add_directive = patch.append_child("add");

                            pugi::xml_attribute sel_attr = add_directive.append_attribute("sel");
                            std::string base_xpath = element.first.getXPath().substr(0, pos);

                            std::string adjacent_sibling_rel_val;
                            if (element.first.relative_pos == "before") {
                                adjacent_sibling_rel_val = element.first.next_sibling_rel_val;
                            } else {
                                adjacent_sibling_rel_val = element.first.prev_sibling_rel_val;
                            }

                            
                            if (element.first.relative_pos == "singleton") {
                                sel_attr.set_value(base_xpath.c_str());
                            } else {
                                std::stringstream sel_path_ss;
                                sel_path_ss << base_xpath << "/S[@" << element.first.selector_attrib << "='" << adjacent_sibling_rel_val << "']";
                                sel_attr.set_value(sel_path_ss.str().c_str());
                                pugi::xml_attribute pos_attr = add_directive.append_attribute("pos");
                                pos_attr.set_value(element.first.relative_pos.c_str());
                            }

                            pugi::xml_node child = add_directive.append_child(element.first.getName().c_str());
                            for (auto& attrib: element.first.getAttributes()) {
                                pugi::xml_attribute attribute = child.append_attribute(attrib.first.c_str());
                                attribute.set_value(attrib.second.c_str());
                            }
                        } 

                    } else {
                        auto pos = element.first.getXPath().find_last_of("/");
                        std::string sel_xpath = element.first.getXPath().substr(0, pos);
                        std::stringstream query;
                        query << "/Patch/add[@sel=\"" << sel_xpath << "\"]";
                        pugi::xpath_query diff_query(query.str().c_str());
                        pugi::xpath_node_set results = diff_patch.select_nodes(diff_query);
                        // IF we have not encountered this selector before, add a new element, otherwise edit the existing 'ADD' selector
                        if (results.empty()) {
                            pugi::xml_node add_directive = patch.append_child("add");
                            pugi::xml_attribute attr = add_directive.append_attribute("sel");

                            attr.set_value(element.first.getXPath().substr(0, pos).c_str());

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
                    }
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

                attribute.set_value(element.first.getXPath().c_str());

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
                    sel_path << element.first.getXPath().c_str() << "/@" << attribute.first;
                    sel_attr.set_value(sel_path.str().c_str());
                    pugi::xml_node textNode = add_directive.append_child(pugi::node_pcdata);
                    textNode.text() = attribute.second.c_str();
                }
            } else if (element.second == "REMATTR") {
                for (auto& attribute : element.first.getAttributes()) {
                    pugi::xml_node rem_directive = patch.append_child("remove");
                    pugi::xml_attribute sel_attr = rem_directive.append_attribute("sel");
                    std::stringstream sel_path;
                    sel_path << element.first.getXPath().c_str() << "/@" << attribute.first;
                    sel_attr.set_value(sel_path.str().c_str());
                }
            } else if (element.second == "REPATTR") {
                for (auto& attribute : element.first.getAttributes()) {
                    pugi::xml_node replace_directive = patch.append_child("replace");
                    pugi::xml_attribute sel_attr = replace_directive.append_attribute("sel");
                    std::stringstream sel_path;
                    sel_path << element.first.getXPath().c_str() << "/@" << attribute.first;
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
                std::string xpath_delimiter = "/";
                std::string xpath_query = element.first.getXPath();

                size_t start = 0;
                size_t end = xpath_query.find(xpath_delimiter);
                std::vector<std::string> tokens;
                while (end != std::string::npos) {
                    tokens.push_back(xpath_query.substr(start, end - start));
                    start = end + xpath_delimiter.length();
                    end = xpath_query.find(xpath_delimiter, start);
                }
                tokens.push_back(xpath_query.substr(start, end));

                size_t cutout_chars = 0;
                // Pop the text() token and discard
                // +1 for the delimeter that gets cut out of path
                cutout_chars = cutout_chars + tokens.back().size() + 1;
                tokens.pop_back();

                std::string txt_parent_name = tokens.back();
                // +1 for the delimeter that gets cut out of path
                cutout_chars = cutout_chars + txt_parent_name.size() + 1;
                tokens.pop_back();

                xpath_query = xpath_query.substr(0, xpath_query.size() - cutout_chars);

                
                std::stringstream query_ss;
                query_ss << "/Patch/add[@sel=\"" << xpath_query << "\"]/" << txt_parent_name;

                pugi::xpath_query text_query(query_ss.str().c_str());
                pugi::xpath_node_set results = diff_patch.select_nodes(text_query);

                if (!results.empty()) {
                    if (results.size() == 1) {
                        pugi::xml_node parent_node = results.first().node();
                        pugi::xml_node text_node = parent_node.append_child(pugi::node_pcdata);
                        text_node.text() = element.first.getValue().c_str();
                    } else {
                        std::cerr << "ERROR: MULTIPLE PARENT ADD DIRECTIVES FOUND!" << std::endl;
                        exit(1);
                    }
                } else {
                    std::cerr << "ERROR: PARENT ADD DIRECTIVE NOT FOUND" << std::endl;
                    exit(1);
                }

            } else if (element.second == "REMOVE") {
                if (get_parent_directive(element.first, diff_patch, "remove").empty()) {  // If parent doesnt exist,
                    pugi::xml_node remove_directive = patch.append_child("remove");
                    pugi::xml_attribute sel_attr = remove_directive.append_attribute("sel");

                    std::stringstream sel_path;
                    sel_path << element.first.getXPath().c_str();
                    sel_attr.set_value(sel_path.str().c_str());
                }

            } else if (element.second == "REPLACE") {
                pugi::xml_node replace_directive = patch.append_child("replace");
                pugi::xml_attribute sel_attr = replace_directive.append_attribute("sel");

                std::stringstream sel_path;
                sel_path << element.first.getXPath().c_str();
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
    XMLElement element;
    element.relative_pos = "";                  // Only Relative to segment positioning
    element.prev_sibling_rel_val = "";          // Only Relative to segment positioning
    element.next_sibling_rel_val = "";          // Only Relative to segment positioning
    element.selector_attrib = "";               // Only Relative to segment positioning
    element.type = NodeTypeToString(mpd1_node.type());
    element.index = -1;

    if (mpd1_node.children().empty()) {
        element.has_children = false;
    } else {
        element.has_children = true;
    }

    if (mpd1_node.type() == pugi::node_element) {
        xpath = xpath + "/" + mpd1_node.name();
        index_map[xpath]++;
        element.index = index_map[xpath];

        element.setName(mpd1_node.name());
        std::string full_query;

        // If the element contains attributes
        if (!mpd1_node.attributes().empty()) {
            // Create a filter stream that populates based on attribute values,
            // Used for searching for identical node in other XML document
            std::stringstream elem_attr_filter_ss;
            elem_attr_filter_ss << "[";
            for (auto it = mpd1_node.attributes().begin(); it != mpd1_node.attributes().end(); it++) {
                element.addAttribute(it->name(), it->value());

                elem_attr_filter_ss << "@" << it->name() << "=" << "'" << it->value() << "'";
                if (std::next(it) != mpd1_node.attributes().end()) {
                    elem_attr_filter_ss << " and ";
                }
            }
            elem_attr_filter_ss << "]";

            full_query = xpath + elem_attr_filter_ss.str();

            auto id_itr = element.getAttributes().find("id");
            if (id_itr != element.getAttributes().end()) {             // 'id' attribute found
                std::string id_val = id_itr->second;
                element.selector_attrib = "id";
                std::stringstream id_ss;
                id_ss << "[@id='" << id_val << "']";
                xpath = xpath + id_ss.str();
            } else if (element.getName() == "S") {                     // is a Segment
                if (element.getAttributes().find("t") != element.getAttributes().end()) {          // If t is present in segment
                    element.selector_attrib = "t";
                } else if (element.getAttributes().find("n") != element.getAttributes().end()) {   // If n is present in segment
                    element.selector_attrib = "n";
                } else {
                    std::cerr << "ERROR: No valid attributes to assign for addressing Segment (S)!" << std::endl;
                    exit(1);
                }

                auto selector_attrib_itr = element.getAttributes().find(element.selector_attrib);
                std::string selector_attrib_val = selector_attrib_itr->second;
                std::stringstream selector_attrib_ss;
                selector_attrib_ss << "[@" << element.selector_attrib << "='" << selector_attrib_val << "']";
                xpath = xpath + selector_attrib_ss.str();


                if(mpd1_node.parent().first_child() == mpd1_node.parent().last_child()) { // This is the only element in the SegmentTimeline
                    element.relative_pos = "singleton";
                } else if(mpd1_node.parent().first_child() == mpd1_node) {     // If this is the first element of SegmentTimeline use 'before' positional directive
                    element.relative_pos = "before";
                    element.next_sibling_rel_val = mpd1_node.next_sibling().attribute(element.selector_attrib.c_str()).value();
                } else {                                                // Otherwise use 'after' positional directive
                    element.relative_pos = "after";
                    element.prev_sibling_rel_val = mpd1_node.previous_sibling().attribute(element.selector_attrib.c_str()).value();
                    if (mpd1_node.parent().last_child() != mpd1_node) {
                        element.next_sibling_rel_val = mpd1_node.next_sibling().attribute(element.selector_attrib.c_str()).value();
                    }
                }

            } else {                                                    // no addressing attributes found
                std::stringstream idx_ss;
                idx_ss << "[" << index_map[xpath] << "]";
                xpath = xpath + idx_ss.str();
            }

        } else {                                                        // no attributes present
            full_query = xpath;
            std::stringstream idx_ss;
            idx_ss << "[" << index_map[xpath] << "]";
            xpath = xpath + idx_ss.str();
        }

        pugi::xpath_query element_query(full_query.c_str());
        pugi::xpath_node_set results = mpd2.select_nodes(element_query);

        element.setXPath(xpath);

        if (results.empty()) {
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
                    
                // Matched element found but has different number of attributes
                if (current_attr_size != matched_attr_size) {
                    diffset[xpath] = element;
                }
                
            }
        }
        
        // Process child nodes recursivly
        // Here we could recursivly pass in the current element to add children to that element, could be useful for optimization (replace element + children in one directive)
        // Is it too inneficient to calculate the resulting string of both operations (replace element attributes + replace children vs replace element + children) and choose the smaller one?
        for (pugi::xml_node mpd1_child : mpd1_node.children()) {
            process_node(mpd1_child, mpd2, diffset, index_map, xpath);
        }
    } else if (mpd1_node.type() == pugi::node_pcdata) {
        //If node is a text node, we must process it differently
        xpath = xpath + "/text()";
        index_map[xpath]++;
        element.setXPath(xpath);
        element.setValue(mpd1_node.value());
        element.has_children = false;

        std::cerr << "Text Node Path: " << xpath << std::endl;
        pugi::xpath_query test_query(xpath.c_str());
        pugi::xpath_node_set results = mpd2.select_nodes(test_query);
        

        if (!results.empty()) {
            if (results.size() == 1) {
                std::string mpd1_text_content = mpd1_node.value();
                std::string mpd2_text_content = results.first().node().value();

                if (mpd1_text_content == mpd2_text_content) {
                    std::cerr << "Text Element " << xpath << " matches in MPD2." << std::endl;
                } else {
                    std::cerr << "Text Element " << xpath << " mismastch in MPD2." << std::endl;
                    diffset[xpath] = element;
                }
            } else {
                std::cerr << "ERROR: Multiple Results found for text node: " << xpath << std::endl;
                exit(1);
            }
                
        } else {
            std::cerr << "Text Element " << xpath << " does not exist in MPD2." << std::endl;
            diffset[xpath] = element;
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
    pugi::xml_document old_doc;
    if(!old_doc.load_file((const char*) old_mpd)) {
        std::cerr << "Failed to load old mpd!" << std::endl;
        exit(1);
    }

    pugi::xml_document new_doc;
    if (!new_doc.load_file((const char*) new_mpd)) {
        std::cerr << "Failed to load new mpd!" << std::endl;
        exit(1);
    }

    std::string old_ptime = old_doc.child("MPD").attribute("publishTime").value();
    std::string new_ptime = new_doc.child("MPD").attribute("publishTime").value();

    // publishTime is the same in current and incoming, return
    if (old_ptime == new_ptime)
    {
        std::cerr << "mpd1 ptime    " << old_ptime << std::endl;
        std::cerr << "current ptime " << new_ptime << std::endl;
        std::cerr << "they are the same, done" << std::endl;
        return "";
    }

    /***************
    Create Diff Sets
    ****************/
    pugi::xml_node old_root = old_doc.child("MPD");
    // contains elements missing in current MPD
    std::unordered_map<std::string, u_int> new_imap;
    std::unordered_map<std::string, XMLElement> new_missing;                       // Elements Present in old Doc, missing in new
    process_node(old_root, new_doc, new_missing, new_imap);


    pugi::xml_node new_root = new_doc.child("MPD");
    // contains elements missing in client MPD
    std::unordered_map<std::string, u_int> old_imap;
    std::unordered_map<std::string, XMLElement> old_missing;                       // Elements Present in New Doc, missing in old 
    process_node(new_root, old_doc, old_missing, old_imap);

    /*
        If we use a set instead of a map, the combination of S Path + attributes will make unique objects,
        however searching for keys in the map become dificulet when we only care abut xpath

        Unoredered map should be of Xpath -> List<XMLElement> (list is ordered)
        if list is > 0 compare object indepenedently for add/remove/replace
    */

    /***************
    Compare Diff Sets

    TODO - POSSIBLY Utilize XPATH OF Element to perform a lookup when adding/replacing grandparent elements (elements that contain multiple generations of child elements )
    ****************/

    std::map<XMLElement, std::string> delta_map;
    for (const auto& pair: new_missing) {
        auto it = old_missing.find(pair.first);
        // If entry exist in old missing
        if (it != old_missing.end()) {
            // Add element to update_map with 'REPLACE' operation
            if(it->second.has_children) {
                // Need to ADD/REM/REPLACE All Attributes for this element since it contains children
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
                    std::cerr << "Key " << add_attr.getXPath() << " exists but contains attribute deltas in the old MPD (w/ Children), Setting Tag to ADDATTR..." << std::endl;
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
                    std::cerr << "Key " << rem_attr.getXPath() << " exists but contains attribute deltas in the old MPD (w/ Children), Setting Tag to REMATTR..." << std::endl;
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
                    std::cerr << "Key " << rep_attr.getXPath() << " exists but contains attribute deltas in the old MPD (w/ Children), Setting Tag to REPATTR..." << std::endl;
                    delta_map[rep_attr] = "REPATTR";
                }
                
                
            } else {
                std::cerr << "Key " << pair.first << " exists but contains attribute deltas in the old MPD (no Children), Setting Tag to REPLACE..." << std::endl;
                delta_map[it->second] = "REPLACE";
            }


            auto miss_it = old_missing.find(it->first);
            if (miss_it != old_missing.end()) {
                std::cerr << "Found Key " << it->first << " was successfully cleaned from old missing" << std::endl;
                old_missing.erase(it);
            } else {
                // Element with key 'some_key' was not found in the map
                std::cerr << "ERROR: Key " << it->first << " was not found in old missing! Could not erase!" << std::endl;
                exit(1);
            }

            
        } else {
            std::cerr << "Key " << pair.first << " does not exist in the old MPD, Setting Tag to REMOVE..." << std::endl;
            // Add element to update_map with 'REMOVE' operation
            delta_map[pair.second] = "REMOVE";
        }
    }
    // What is left in the client_missing map should be 'ADDED'
    for (const auto& pair: old_missing) {
        std::cerr << "Key " << pair.first << " does not exist in the new MPD, Setting Tag to ADD..." << std::endl;
        delta_map[pair.second] = "ADD";
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
    std::string diff_patch_str = translate_deltas(delta_map, old_ptime, new_ptime);

    std::cerr << "OUTPUT PATCH:\n" << diff_patch_str << std::endl;

    // Allocate memory for the C-style string and copy the content, if we dont do this the string object is destroyed on
    // function return and we are left with a dangling pointer.  Need to make sure to free memory in NGINX module (or main for testing)!
    char* diff_patch_c_str = (char*) malloc(diff_patch_str.size() + 1);
    std::strcpy(diff_patch_c_str, diff_patch_str.c_str());

    // Return the C-style string
    return diff_patch_c_str;
}

const char* extractPublishTime(const char* mpd){
    // Load File
    pugi::xml_document mpd_xml;
    mpd_xml.load_file((const char*) mpd);

    // Add Patch Location Element as the first child
    pugi::xml_node mpd_elem = mpd_xml.child("MPD");
    std::string publish_time = mpd_elem.attribute("publishTime").value();

    char* xml_c_str = (char*) malloc(publish_time.size() + 1);
    std::strcpy(xml_c_str, publish_time.c_str());

    // Return the C-style string
    return xml_c_str;
}

// Changed 'ttl' to char* because it needs to be a string when setting the value of a text node
const char* add_patch_location(const char* mpd, const char* mpd_id, const char* patch_location, const char* ttl) {
    // Load File
    pugi::xml_document mpd_xml;
    mpd_xml.load_file((const char*) mpd);

    // Add Patch Location Element as the first child
    pugi::xml_node mpd_elem = mpd_xml.child("MPD");
    pugi::xml_attribute mpd_id_attr = mpd_elem.append_attribute("id");
    mpd_id_attr.set_value(mpd_id);

    pugi::xml_node pl_elem = mpd_elem.insert_child_before("PatchLocation", mpd_elem.first_child());
    pugi::xml_attribute ttl_attr = pl_elem.append_attribute("ttl");
    ttl_attr.set_value(ttl);
    pugi::xml_node pl_text_field = pl_elem.append_child(pugi::node_pcdata);
    
    std::string patch_location_str = patch_location;
    pl_text_field.set_value(patch_location_str.c_str());

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
