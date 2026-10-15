/*******************************************************************************
* bitwisedom [app]                                                     fonts.hpp
*
* The two fonts bitwisedom draws with, as ImGui's compressed TTF data.
*
* path:      /src/djinterp/tools/bitwisedom/app/fonts/fonts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef BITWISEDOM_APP_FONTS_FONTS_HPP
#define BITWISEDOM_APP_FONTS_FONTS_HPP 1


namespace bitwisedom
{
namespace fonts
{

// the data -- for ImFontAtlas::AddFontFromMemoryCompressedTTF
const void*  interface_data();
int          interface_size();
const void*  mono_data();
int          mono_size();

}  // namespace fonts
}  // namespace bitwisedom


#endif  // BITWISEDOM_APP_FONTS_FONTS_HPP
