/* reserved.hpp: a template parameter's name is not the implementation's */

template<typename _Type,
         typename Kept,
         class    _Second,
         typename... _Rest>
struct d_reserved
{
    typedef typename _Type::value_type value_type;
};

template<typename Type>
struct d_fine
{
    typedef typename Type::value_type value_type;
};
