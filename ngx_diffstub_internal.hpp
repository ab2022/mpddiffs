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

Author: Erik Ponder, Jovan Rosario
 */

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
const char* extractPublishTime(const char* mpd);

/*
* The caller is responsible for freeing the memory allocated for the returned string using free().
*/
const char* add_patch_location(const char* mpd, const char* mpd_id, const char* patch_location, const char* ttl);


#ifdef __cplusplus
}
#endif

#endif //NGX_DIFFSTUB_INTERNAL_HPP