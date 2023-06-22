#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <iterator>
#include <set>
#include <unordered_map>
#include "pugixml.hpp"
#include "cxxopts.hpp"

/*
class XMLAttribute {
private:
    std::string name;
    std::string value;

public:
    const std::string& getName() const {
        return this->name;
    }

    const std::string& getValue() const {
        return this->value;
    }

    void setName(std::string name) {
        this->name = name;
    }

    void setValue(std::string value) {
        this->value = value;
    }

    bool operator==(const XMLAttribute& other) const {
        return name == other.name && value == other.value;
    }

    bool operator<(const XMLAttribute& other) const {
        return name < other.name;
    }
};
*/

class XMLElement {
private:
    std::string xpath;
    std::string name;
    std::unordered_map<std::string, std::string> attributes;
    //std::set<XMLAttribute> attributes;
    //std::vector<XMLElement> children;
public:
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
        /*
        if (this->xpath == other.xpath) {
            for (const XMLAttribute& attrib: this->attributes) {
                if (other.attributes.find(attrib) != other.attributes.end()) {
                    return true;
                }
            } 
            return false;
        } 
        else {
            return false;
        }
        */
       return false;
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

void process_node(const pugi::xml_node& mpd1_node, const pugi::xml_document& mpd2, std::unordered_map<std::string, XMLElement>& diffset, std::string xpath="") {
    xpath = xpath + "/" + mpd1_node.name();
    XMLElement element;
    element.setXPath(xpath);
    element.setName(mpd1_node.name());

    std::stringstream elem_attr_filter_ss;
    std::stringstream id_info_ss;
    if (!mpd1_node.attributes().empty()) {
        std::cout << "XPath: " << xpath << std::endl;

        elem_attr_filter_ss << "[";
        for (auto it = mpd1_node.attributes().begin(); it != mpd1_node.attributes().end(); it++) {

            if (it->name() == (std::string)"id") {
                id_info_ss << "[@" << it->name() << "='" << it->value() << "']";
            }

            //XMLAttribute attrib;
            //attrib.setName(it->name());
            //attrib.setValue(it->value());
            //element.addAttribute(attrib);
            element.addAttribute(it->name(), it->value());

            elem_attr_filter_ss << "@" << it->name() << "=" << "'" << it->value() << "'";
            if (std::next(it) != mpd1_node.attributes().end()) {
                elem_attr_filter_ss << " and ";
            }
            
            // Print out object attributes
            std::cout << "  " << it->name() << ": " << it->value() << std::endl;
        }
        elem_attr_filter_ss << "]";

        std::string full_query = xpath + elem_attr_filter_ss.str();
        std::cout << "MPD2 Query: " << full_query << std::endl;

        //Check if node exists in mpd2
        pugi::xpath_query element_query(full_query.c_str());
        pugi::xpath_node_set results = element_query.evaluate_node_set(mpd2);

        if (results.empty()) {
            std::cout << "Element does not exist in MPD2!\n" << std::endl;
            //add to diffset
            //diffset.insert(element);
            diffset[xpath] = element;
        } else {  
            if (results.size() > 1) {
                std::cout << "ERROR! Elements exists " << results.size() << " times in MPD2!";
            } else {
                std::cout << "Element exists in MPD2.\n" << std::endl;
            }
        }

    }

    // If the 'id' field was present, add it to the xpath for recursive processing 
    if (!id_info_ss.str().empty()) {
        xpath = xpath + id_info_ss.str();
    }
    
    // Process child nodes recursivly
    for (pugi::xml_node mpd1_child : mpd1_node.children()) {
        process_node(mpd1_child, mpd2, diffset, xpath);
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
    std::unordered_map<std::string, XMLElement> current_missing;
    process_node(client_root, current_doc, current_missing);


    pugi::xml_node current_root = current_doc.child("MPD");
    // contains elements missing in client MPD
    std::unordered_map<std::string, XMLElement> client_missing;
    process_node(current_root, client_doc, client_missing);


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
    for (const auto& pair: delta_map) {
        std::cout << "XPath: " << pair.first.getXPath() << std::endl;
        std::cout << "Directive: " << pair.second << std::endl;
        std::cout << "Attributes:" << std::endl;
        for (const auto& attr: pair.first.getAttributes())
            std::cout << "  " << attr.first << ": " << attr.second << std::endl;
        std::cout << std::endl;
    }

    /*
        TODO/BUG NOTES:
        - Elements that can exist more than once on the same XPath are always being tagged as updated (i.e. Segments, Representation)
            - Need to reference RFC 5261 (https://datatracker.ietf.org/doc/html/rfc5261) for implementation details, can possiblibly solved with
            implementation of XMLElement.similar() function
            - Use examples from rfc5261 / Find a testing framework
        - Elements added in the current MPD on a new XPath are recorded successfully
        - Elements whos xpath are removed in the current MPD are recorded successfully
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


