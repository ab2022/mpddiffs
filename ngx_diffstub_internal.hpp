#ifndef NGX_DIFFSTUB_INTERNAL_HPP
#define NGX_DIFFSTUB_INTERNAL_HPP

#ifdef __cplusplus
extern "C" {
#endif

/*
* The caller is responsible for freeing the memory allocated for the returned string using free().
*/
const char* morph_diffs(const char* old_mpd, const char* new_mpd);

/*
* The caller is responsible for freeing the memory allocated for the returned string using free().
*/
const char* add_patch_location(const char* mpd, const char* patch_location, const char* ttl);


#ifdef __cplusplus
}
#endif

#endif //NGX_DIFFSTUB_INTERNAL_HPP