#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <string>
#include <cstring>
#include "doctest.h"
#include "ngx_diffstub_internal.hpp" // Include your library header
#include "pugixml.hpp"

/*
    Optimization: Consider moving into application to remove the XMLElement class
*/
bool compareXmlNodes(const pugi::xml_node& val_node, const pugi::xml_node& act_node) {
    if (std::strcmp(val_node.name(), act_node.name()) != 0) {
        return false; // Element names differ
    }

    size_t num_attr_val = std::distance(val_node.attributes().begin(), val_node.attributes().end());
    size_t num_attr_act = std::distance(act_node.attributes().begin(), act_node.attributes().end());

    if (num_attr_val == num_attr_act) {
        for (const pugi::xml_attribute& attr1 : val_node.attributes()) {
            pugi::xml_attribute attr2 = act_node.attribute(attr1.name());

            if (!attr2 || std::strcmp(attr1.value(), attr2.value())) {
                return false; // Attribute values differ or missing
            }
        }
    } else {
        return false; 
    }
    
    size_t num_child_val = std::distance(val_node.children().begin(), val_node.children().end());
    size_t num_child_act = std::distance(act_node.children().begin(), act_node.children().end());
    if (num_child_act == num_child_val) {
        for (pugi::xml_node child1 : val_node.children()) {
            bool foundMatchingChild = false;
            for (pugi::xml_node child2 : act_node.children()) {
                if (compareXmlNodes(child1, child2)) {
                    foundMatchingChild = true;
                    break;
                }
            }
            if (!foundMatchingChild) {
                return false; // Child element missing or different
            }
        }
    } else {
        return false;
    }

    // Compare text content, if no text content exists, an empty string is returned, so this works for non-text nodes aswell
    if (std::strcmp(val_node.text().get(), act_node.text().get()) != 0) {
        return false; // Text content differs
    }

    return true; // Everything matches
}

/*
 TODO: Current logic replaces entire element if no children are present.
 Better to replace single attribute instead of the entire element and update test case
 */
TEST_CASE("TestAddAttrNoChild") {
    const char* old_mpd = "./mpd_samples/test_cases/test_base_1.mpd";
    const char* new_mpd = "./mpd_samples/test_cases/test_add_attr_no_child.mpd";
    const char* mpd_patch_val_file = "./mpd_samples/test_cases/test_add_attr_no_child-patch.mpd";
    
    const char* mpd_patch_act = morph_diffs(old_mpd, new_mpd);

    pugi::xml_document validate_patch, actual_patch;
    if (!validate_patch.load_file(mpd_patch_val_file)) {
        FAIL("FAILED TO LOAD TEST VALIDATION  FILE");
    }
    if (!actual_patch.load_string(mpd_patch_act)) {
        FAIL("FAILED TO LOAD GENERATED PATCH STRING");
    }
    
    pugi::xml_node root_val = validate_patch.first_child();
    pugi::xml_node root_act = actual_patch.first_child();

    REQUIRE(compareXmlNodes(root_val, root_act));

    free((void*) mpd_patch_act);
}


TEST_CASE("TestAddAttrWithChild") {
    const char* old_mpd = "./mpd_samples/test_cases/test_base_1.mpd";
    const char* new_mpd = "./mpd_samples/test_cases/test_add_attr_with_child.mpd";
    const char* mpd_patch_val_file = "./mpd_samples/test_cases/test_add_attr_with_child-patch.mpd";
    
    const char* mpd_patch_act = morph_diffs(old_mpd, new_mpd);

    pugi::xml_document validate_patch, actual_patch;
    if (!validate_patch.load_file(mpd_patch_val_file)) {
        FAIL("FAILED TO LOAD TEST VALIDATION  FILE");
    }
    if (!actual_patch.load_string(mpd_patch_act)) {
        FAIL("FAILED TO LOAD GENERATED PATCH STRING");
    }
    
    pugi::xml_node root_val = validate_patch.first_child();
    pugi::xml_node root_act = actual_patch.first_child();

    REQUIRE(compareXmlNodes(root_val, root_act));

    free((void*) mpd_patch_act);
}

#if 0
TEST_CASE("NotATest") {
    REQUIRE(1 == 1);
}
#endif