#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include "pugixml.hpp"
#include "cxxopts.hpp"

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

void print_query_result(pugi::xpath_node_set results) {
    for (const auto& node: results) {
        std::cout << node.node().path() << std::endl;
        for (pugi::xml_attribute& attr: node.node().attributes()) {
            std::cout << attr.name() << ": " << attr.value() << std::endl;
        }
    }
}

// TODO: Change Naming convention (mpd)
void processNode(const pugi::xml_node& mpd1_node, const pugi::xml_document& mpd2, std::string xpath="") {
    xpath = xpath + "/" + mpd1_node.name();

    std::stringstream elem_attr_filter_ss;
    std::stringstream id_info_ss;
    if (!mpd1_node.attributes().empty()) {
        std::cout << "XPath: " << xpath << std::endl;

        elem_attr_filter_ss << "[";
        for (auto it = mpd1_node.attributes().begin(); it != mpd1_node.attributes().end(); it++) {

            if (it->name() == (std::string)"id") {
                id_info_ss << "[@" << it->name() << "='" << it->value() << "']";
            }

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
        } else {
            std::cout << "Element exists in MPD2.\n" << std::endl;
        }

    }

    // If the 'id' field was present, add it to the xpath for recursive processing 
    if (!id_info_ss.str().empty()) {
        xpath = xpath + id_info_ss.str();
    }
    
    // Process child nodes recursivly
    for (pugi::xml_node mpd1_child : mpd1_node.children()) {
        processNode(mpd1_child, mpd2, xpath);
    }
}


void morph_diffs(const char* mpd1) {

    const char* current_mpd = "current.mpd";
    struct stat buffer;

    std::cout << "\n\n" << "Starting Diff..." << "\n\n";

    if (stat (current_mpd, &buffer) != 0)
    {
        //there is no current mpd, copy incoming to current and return
        std::cout << "creating current.mpd because it doesn't exist" << std::endl;
        std::ifstream src(mpd1);
        std::ofstream dst(current_mpd);
        dst << src.rdbuf();
        return;
    }

    //TBD: check if mpd1 exists, is accessible

    pugi::xml_document mpd1_doc;
    mpd1_doc.load_file((const char*)mpd1);

    pugi::xml_document current_doc;
    current_doc.load_file((const char*)current_mpd);

    std::string mpd1_ptime = mpd1_doc.child("MPD").attribute("publishTime").value();
    std::string current_ptime = current_doc.child("MPD").attribute("publishTime").value();

    if (mpd1_ptime == current_ptime)
    {
        //publishTime is the same in current and incoming, return
        std::cout << "mpd1 ptime    " << mpd1_ptime << std::endl;
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

    pugi::xml_node root = current_doc.child("MPD");
    processNode(root, mpd1_doc); // Start recursive parsing from the root node

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


