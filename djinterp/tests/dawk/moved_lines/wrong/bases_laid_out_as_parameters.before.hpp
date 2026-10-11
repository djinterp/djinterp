/* bases_laid_out_as_parameters: a base's argument is not a parameter list */

template<unsigned Feat>
struct d_mixed
    : d_underline<d_has(Feat, 1)>
    , d_overline<d_has(Feat, 2)>
{
    int m_value;
};
