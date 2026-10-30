# djinterp/test — pure aggregate over the per-suite leaves + the umbrella leaf.
#   Every runner lives in its own <suite>/ subdir (a leaf); this dir only
# recurses and rolls them up.  (Replaces the old cluster, which mismatched the
# subdir layout and collided with the leaves.)
djinterp_add_test_subtree(all_djinterp_test)
