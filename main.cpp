#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include "pugixml.hpp"
#include "cxxopts.hpp"


#ifdef __cplusplus
extern "C" {
#endif


void morph_diffs(const char* client_mpd);

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


