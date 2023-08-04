#include <string>
#include "cxxopts.hpp"
#include "ngx_diffstub_internal.hpp"


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
        const char* mpd_patch = morph_diffs(inmpd, "mpd_samples/current.mpd");
        //const char* mpd_patch = morph_diffs("mpd_samples/manifest_2_pl.mpd", "mpd_samples/manifest_1_pl.mpd");
        std::cerr << "MPD Patch: \n" << mpd_patch << "\n\n";

        // save the patch, other implentation operations, etc..

        // When done using the mpd_patch, free the memory associated with it
        free((void*) mpd_patch);

        /* Sample of Adding the patch location to an mpd file
        const char* modified_mpd = add_patch_location(inmpd, "live-stream/patch.mpd?publishTime=2022-05-22T17:24:00Z", "60");
        std::cerr << "Modified MPD w/ PatchLocation: \n" << modified_mpd << std::endl;
        free((void*) modified_mpd);
        */
    }

    return EXIT_SUCCESS;
}


