/*******************************************************************************
* bitwisedom [ui]                                                      fonts.hpp
*
* The two fonts bitwisedom draws with, as ImGui's compressed TTF data.
*
* path:      /src/ui/bitwisedom/fonts/fonts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef UI_BITWISEDOM_FONTS_FONTS_HPP
#define UI_BITWISEDOM_FONTS_FONTS_HPP 1


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


#endif  // UI_BITWISEDOM_FONTS_FONTS_HPP
