/*******************************************************************************
* djinterp [c][interp]                                               djin_desc.c
*
*   A djin, a frame and a djin's command as desc's facts, declared in
* djin_desc.h. The settings are the option core's own reading of them
* (option_env_desc.h), so that describe, help and `get --show-origin` cannot
* disagree; this unit adds only what a djin record holds.
*
*
* path:      /src/djinterp/c/modules/interp/djin_desc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.19
*                                                            revised: 2026.11.19
*******************************************************************************/

#include "../../../../../inc/djinterp/c/modules/interp/djin_desc.h"  // corresponding header
// djinterp
#include "../../../../../inc/djinterp/c/option/option_env_desc.h"  // d_option_env_describe


#if RE_STD_HAS_INT64


//==============================================================================
// 1.  THE FACTS
//==============================================================================

/*
d_internal_djin_desc_text
  One text fact, left out where the text is null.
*/
static int
d_internal_djin_desc_text(
    struct dj_desc_sink* _sink,
    const char*          _key,
    const char*          _text,
    uint32_t             _layer,
    uint32_t             _importance
)
{
    if (_text == NULL)
    {
        return (_sink->stopped != 0u) ? 1 : 0;
    }

    return d_desc_emit(_sink, _key, NULL, d_ostream_value_text(_text),
                       _layer, _importance,
                       (uint32_t)D_DESC_TRUTH_IMPLEMENTATION);
}

/*
d_internal_djin_desc_settings
  A frame's settings, a level of their own, with what each accepts; nothing
for a null frame.
*/
static int
d_internal_djin_desc_settings(
    const struct dj_option_env* _frame,
    struct dj_desc_sink*        _sink
)
{
    int stopped;

    if ( (_frame == NULL) ||
         (d_desc_enter(_sink, "settings", NULL) == 0) )
    {
        return (_sink->stopped != 0u) ? 1 : 0;
    }

    stopped = d_option_env_describe(_frame, _sink, D_OPTION_ENV_DESC_ACCEPTS);
    d_desc_leave(_sink);

    return ( (stopped == 1) ||
             (_sink->stopped != 0u) ) ? 1 : 0;
}

/*
d_internal_djin_desc_commands
  Each command, a level keyed by its name, holding its summary; nothing for
a djin with none.
*/
static int
d_internal_djin_desc_commands(
    const struct dj_djin* _djin,
    struct dj_desc_sink*  _sink
)
{
    int stopped = 0;

    if ( (_djin->commands == NULL) ||
         (_djin->command_count == 0u) ||
         (d_desc_enter(_sink, "commands", NULL) == 0) )
    {
        return (_sink->stopped != 0u) ? 1 : 0;
    }

    for (size_t i = 0u; (stopped == 0) && (i < _djin->command_count); ++i)
    {
        if (d_desc_enter(_sink, _djin->commands[i].name, NULL) != 0)
        {
            stopped = d_internal_djin_desc_text(_sink, "summary",
                                                _djin->commands[i].summary,
                                                (uint32_t)D_DESC_LAYER_INTENT,
                                                0u);
            d_desc_leave(_sink);
        }

        stopped = (_sink->stopped != 0u) ? 1 : stopped;
    }

    d_desc_leave(_sink);

    return ( (stopped != 0) ||
             (_sink->stopped != 0u) ) ? 1 : 0;
}


//==============================================================================
// 2.  THE DESCRIBERS
//==============================================================================

/*
d_djin_describer
  Identity, intent, the commands, the settings, then the djin's own.
*/
int
d_djin_describer(
    const void*          _object,
    struct dj_desc_sink* _sink
)
{
    const struct dj_djin* const djin = (const struct dj_djin*)_object;

    if ( (djin == NULL) ||
         (_sink == NULL) )
    {
        return -1;
    }

    if ( (d_internal_djin_desc_text(_sink, "name", djin->name,
                                    (uint32_t)D_DESC_LAYER_IDENTITY,
                                    0u) != 0) ||
         (d_internal_djin_desc_text(_sink, "version", djin->version,
                                    (uint32_t)D_DESC_LAYER_IDENTITY,
                                    0u) != 0) ||
         (d_internal_djin_desc_text(_sink, "summary", djin->summary,
                                    (uint32_t)D_DESC_LAYER_INTENT,
                                    0u) != 0) ||
         (d_internal_djin_desc_commands(djin, _sink) != 0) ||
         (d_internal_djin_desc_settings(djin->settings, _sink) != 0) )
    {
        return 1;
    }

    if (djin->describe != NULL)
    {
        (void)djin->describe(djin, _sink);
    }

    return (_sink->stopped != 0u) ? 1 : 0;
}

/*
d_djin_frame_describer
  A frame's name, then its settings.
*/
int
d_djin_frame_describer(
    const void*          _object,
    struct dj_desc_sink* _sink
)
{
    const struct dj_option_env* const frame =
        (const struct dj_option_env*)_object;

    if ( (frame == NULL) ||
         (_sink == NULL) )
    {
        return -1;
    }

    if (d_internal_djin_desc_text(_sink, "name", frame->name,
                                  (uint32_t)D_DESC_LAYER_IDENTITY,
                                  0u) != 0)
    {
        return 1;
    }

    return d_internal_djin_desc_settings(frame, _sink);
}

/*
d_djin_command_describer
  The djin's name, then the command's name and summary.
*/
int
d_djin_command_describer(
    const void*          _object,
    struct dj_desc_sink* _sink
)
{
    const struct dj_djin_command_of* const of =
        (const struct dj_djin_command_of*)_object;

    if ( (of == NULL) ||
         (of->djin == NULL) ||
         (of->command == NULL) ||
         (_sink == NULL) )
    {
        return -1;
    }

    if ( (d_internal_djin_desc_text(_sink, "djin", of->djin->name,
                                    (uint32_t)D_DESC_LAYER_IDENTITY,
                                    0u) != 0) ||
         (d_internal_djin_desc_text(_sink, "name", of->command->name,
                                    (uint32_t)D_DESC_LAYER_IDENTITY,
                                    0u) != 0) )
    {
        return 1;
    }

    return d_internal_djin_desc_text(_sink, "summary", of->command->summary,
                                     (uint32_t)D_DESC_LAYER_INTENT, 0u);
}


#endif  // RE_STD_HAS_INT64
