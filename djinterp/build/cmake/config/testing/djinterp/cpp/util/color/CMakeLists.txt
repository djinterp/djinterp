# =============================================================================
# testing/djinterp/core/util/color/
#   AGGREGATE only -- recurses into the nine child leaf directories
# (color, color_cie_lab, color_cmyk, color_common, color_convert, color_hsl,
# color_hsv, color_rgb, color_ycbcr) and publishes all_djinterp_core_util_color
# over every test exe at or below here, so that the parent's recursion reaches
# them.
# =============================================================================

djinterp_add_test_subtree(all_djinterp_core_util_color)
