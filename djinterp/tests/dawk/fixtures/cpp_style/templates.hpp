/* templates.hpp: typename, never class, for a type template parameter */

template<class Type>
struct d_first;

template<typename First,
         class    Second>
struct d_second;

template<typename Type>
struct d_named;

template<template<typename> class Holder,
         typename                 Type>
struct d_held;

template<typename Type>
struct d_based : public d_named<Type>, public d_first<Type>
{};
