#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <iterator>
#include <set>
#include <unordered_map>
#include "pugixml.hpp"
#include "cxxopts.hpp"


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
        if (this->xpath == other.xpath) {
            
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
        else {
            return false;
        }
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

#ifdef __cplusplus
extern "C" {
#endif

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

/**
 * - Recursive function for processing the XMLTree and identifiying nodes that do not exist in mpd2
 * - Nodes that cannot be identified in mpd2 are copied into an XMLElement object in the pass by reference diffset
 * - The index_map is used to track index of identites that do not contain an "id" field
 * 
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

    std::cout << "MPD2 Query: " << full_query << std::endl;

    // Check if node exists in mpd2
    pugi::xpath_query element_query(full_query.c_str());
    pugi::xpath_node_set results = element_query.evaluate_node_set(mpd2);  

    if (results.empty()) {
        std::cout << "Element does not exist in MPD2!" << std::endl;
        //add to diffset
        std::stringstream idx_ss;
        idx_ss << "[" << index_map[xpath] << "]";
        xpath = xpath + idx_ss.str();
        element.setXPath(xpath);
        diffset[xpath] = element;
    } else {
        if (results.size() > 1) {
            std::cout << "ERROR! Elements exists " << results.size() << " times in MPD2!";
            exit(1);
        } else {
            std::cout << "Element exists in MPD2." << std::endl;
        }
    }
    std::cout << "XPath: " << xpath << std::endl << std::endl;

   /*
   TODO: Track child node index when parsing through the document, and record in XMLElement object
   Index will be used during comparison to inject/remove elements at certain indexes (i.e Segments <S\>).  Can possibly use logic like:
   if node.attributes.empty() && !node.children.empty()
        call process node with child index counter
        add child index to XPath ie. '/MPD/Period[@id='P0']/AdaptationSet[@id='0']/SegmentTemplate/SegmentTimeline/S[1]'
    
   */
    
    // Process child nodes recursivly
    for (pugi::xml_node mpd1_child : mpd1_node.children()) {
        process_node(mpd1_child, mpd2, diffset, index_map, xpath);
    }
}

// Utility function for debugging
void print_diffset(std::unordered_map<std::string, XMLElement>& diffset) {
    for (const auto& elem: diffset) {
        std::cout << "XPath: " << elem.second.getXPath() << std::endl;
        for (const auto& attrib: elem.second.getAttributes()) {
            std::cout << "  " << attrib.first << ": " << attrib.second << std::endl;
        }
        std::cout << std::endl;
    }
}


void morph_diffs(const char* client_mpd) {

    const char* current_mpd = "current.mpd";
    struct stat buffer;

    std::cout << "\n\n" << "Starting Diff..." << "\n\n";

    if (stat (current_mpd, &buffer) != 0)
    {
        //there is no current mpd, copy incoming to current and return
        std::cout << "creating current.mpd because it doesn't exist" << std::endl;
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
        std::cout << "mpd1 ptime    " << client_ptime << std::endl;
        std::cout << "current ptime " << current_ptime << std::endl;
        std::cout << "they are the same, done" << std::endl;
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


    std::cout << "\nElements present in Client MPD but missing in Current MPD: " << std::endl;
    print_diffset(current_missing);

    std::cout << "\nElements present in Current MPD but missing in Client MPD: " << std::endl;
    print_diffset(client_missing);

    /***************
    Compare Diff Sets
    ****************/
    std::map<XMLElement, std::string> delta_map;
    for (const auto& pair: current_missing) {
        auto it = client_missing.find(pair.first);

        if (it != client_missing.end()) {
            // Add element to update_map with 'REPLACE' operation
            delta_map[it->second] = "REPLACE";

            // Erase the entry from the client map
            client_missing.erase(it->first);
            
        } else {
            std::cout << "Key " << pair.first << " does not exist in the map." << std::endl;
            // Add element to update_map with 'REMOVE' operation
            delta_map[pair.second] = "REMOVE";
        }
    }
    // What is left in the client_missing map should be 'ADDED'
    for (const auto& pair: client_missing) {
        delta_map[pair.second] = "ADD";
    }

    // Print out Diffset
    std::cout << "\n----------------------------------------\n" << std::endl;
    for (const auto& pair: delta_map) {
        std::cout << "XPath: " << pair.first.getXPath() << std::endl;
        std::cout << "Directive: " << pair.second << std::endl;
        std::cout << "Attributes:" << std::endl;
        for (const auto& attr: pair.first.getAttributes())
            std::cout << "  " << attr.first << ": " << attr.second << std::endl;
        std::cout << std::endl;
    }

    /**
     * Proposed Fix for indexing
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
        - Single Elements are detected for Add, Remove, Update
        - No Testing done for nested elements
    */


    /***************
    Create Diff File
    ****************/
    //copy mpd1 to current
}

int main (int argc, char *argv[]) {

    std::string inmpd_o;

    try
    {
        cxxopts::Options options(argv[0], "diffstub VOD stand-alone tool");

        options.set_width(80).add_options()
            ("u", "mpd file, do mpd diffs with current", cxxopts::value<std::string>())
            ("h,help", "Print this help")
            ;

        auto result = options.parse(argc, argv);

        if ( result.count("help") || argc == 1 )
        {
            std::cout << options.help() << std::endl;
            exit(0);
        }

        if (result.count("u"))
            inmpd_o = result["u"].as<std::string>().c_str();
    }
    catch (const cxxopts::OptionException& e)
    {
        std::cout << "error parsing options: " << e.what() << std::endl;
        exit(1);
    }

    if (inmpd_o.length())
    {
        const char* inmpd = inmpd_o.c_str();
        morph_diffs(inmpd);
    }

    return EXIT_SUCCESS;
}

#ifdef __cplusplus
}
#endif


