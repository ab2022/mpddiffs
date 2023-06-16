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

using namespace std;

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

            xps << node.name() << "@" << attr.name() << TOK << attr.value();

            cout << xps.str() << endl;

            //cout << xps.str();
            all_elements.push_back(xps.str()); //one string for each mpd item
        }
        return true; // continue traversal
    }
};


void morph_diffs(const char* mpd1) {

    const char* current_mpd = "current.mpd";
    struct stat buffer;

    cout << "\n\n" << "Starting Diff..." << "\n\n";

    if (stat (current_mpd, &buffer) != 0)
    {
        //there is no current mpd, copy incoming to current and return
        cout << "creating current.mpd because it doesn't exist" << endl;
        ifstream src(mpd1);
        ofstream dst(current_mpd);
        dst << src.rdbuf();
        return;
    }

    //TBD: check if mpd1 exists, is accessible

    pugi::xml_document mpd1_doc;
    mpd1_doc.load_file((const char*)mpd1);

    pugi::xml_document current_doc;
    current_doc.load_file((const char*)current_mpd);

    string mpd1_ptime = mpd1_doc.child("MPD").attribute("publishTime").value();
    string current_ptime = current_doc.child("MPD").attribute("publishTime").value();

    if (mpd1_ptime == current_ptime)
    {
        //publishTime is the same in current and incoming, return
        cout << "mpd1 ptime    " << mpd1_ptime << endl;
        cout << "current ptime " << current_ptime << endl;
        cout << "they are the same, done" << endl;
        return;
    }
    
    simple_walker walker;
    mpd1_doc.traverse(walker); //array of xpaths in walker.all_elements
    
    size_t pos = 0;
    //for (int i = 0; i < (int)walker.all_elements.size(); i++) 
    for (int i = 0; i < 2; i++)
    {
        cout << endl;

        pos = walker.all_elements[i].find(TOK);

        string elpath = walker.all_elements[i].substr(0, pos);
        string value = walker.all_elements[i].substr(pos + TOKLEN);

        cout << elpath << "\n";
        cout << value << endl;
        
        //std::string val = node.node().attribute("scanType").value();
    }
    
    //Create Diff File

    //copy mpd1 to current
}

int main (int argc, char *argv[]) {

    string inmpd_o;

    try
    {
        cxxopts::Options options(argv[0], "diffstub VOD stand-alone tool");

        options.set_width(80).add_options()
            ("u", "mpd file, do mpd diffs with current", cxxopts::value<string>())
            ("h,help", "Print this help")
            ;

        auto result = options.parse(argc, argv);

        if ( result.count("help") || argc == 1 )
        {
            cout << options.help() << endl;
            exit(0);
        }

        if (result.count("u"))
            inmpd_o = result["u"].as<string>().c_str();
    }
    catch (const cxxopts::OptionException& e)
    {
        cout << "error parsing options: " << e.what() << endl;
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


