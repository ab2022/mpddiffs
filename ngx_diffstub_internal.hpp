#ifndef NGX_DIFFSTUB_INTERNAL_HPP
#define NGX_DIFFSTUB_INTERNAL_HPP

#ifdef __cplusplus
extern "C" {
#endif

const char* morph_diffs(const char* old_mpd, const char* new_mpd);

const char* add_patch_location(const char* mpd, const char* patch_location, const char* ttl);

#ifdef __cplusplus
}
#endif

#endif //NGX_DIFFSTUB_INTERNAL_HPP