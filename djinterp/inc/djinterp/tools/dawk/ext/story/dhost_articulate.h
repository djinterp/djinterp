/*******************************************************************************
* djinterp [tools]                                            dhost_articulate.h
*
* The Articulate Storyline driver of the storyline extension.
*   A .story file is a ZIP package of XML parts and media, in the manner of
* Open Packaging: relationship parts beside the parts they describe say which
* part uses which. Articulate publishes nothing of it. What this driver reads
* is what users have worked out, and each assumption is one function, so the
* first real .story it meets either confirms it or shows where it differs:
*     the story part is story/story.xml, and its scenes are the <scene>
*     elements of its <sceneLst>, in order, each listing its slides as
*     <sldId> references resolved through story/_rels/story.xml.rels;
*     a slide is a part whose root <sld> names it; and
*     the media a slide uses are the relationships of type "media" in the
*     slide's own relationship part, each targeting a part under story/media.
* When the scene list gives no slides, the slides are taken in the order of
* their part names, slide1, slide2, ..., and the story says so in its notes.
*   A replacement overwrites the media part's bytes, so every slide-level use
* keeps pointing where it did. The XML records each media part's MD5, as hex
* or as base64; every copy of the old checksum, in every XML part, is
* rewritten to the new one, which keeps the duplicate list theme.xml holds in
* step without knowing where it is. A media part used by any other part --
* another slide, a layout, a master -- is refused.
*   Not known yet, so not changed: anything recording a clip's duration. A
* replacement of a different length may need its timeline adjusted in
* Storyline.
*
* path:      /inc/djinterp/tools/dawk/ext/story/dhost_articulate.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  OPERATIONS
    ----------
    1.  The driver
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_STORY_DHOST_ARTICULATE_H
#define DJINTERP_TOOLS_DAWK_EXT_STORY_DHOST_ARTICULATE_H 1

// djinterp
#include "./dhost_story.h"  // d_story_driver


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    The driver
//------------------------------------------------------------------------------
// The driver claims a file whose name ends in .story, in any case; the
// descriptor is static, and outlives every story opened with it.
const struct d_story_driver* d_articulate_driver(void);


#endif  // DJINTERP_TOOLS_DAWK_EXT_STORY_DHOST_ARTICULATE_H
