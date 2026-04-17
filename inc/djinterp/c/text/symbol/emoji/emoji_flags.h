/******************************************************************************
* djinterp [text]                                                emoji_flags.h
*
* Emoji flag sequences.
*   This header defines UTF-8 string constants for flag emoji using regional
* indicator letter pairs (ISO 3166-1 alpha-2), plus a small builder macro for
* composing flags from two regional indicator codepoints.
*
*
* path:      \inc\text\symbol\emoji\emoji_flags.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.08
******************************************************************************/

#ifndef DJINTERP_TEXT_SYMBOL_EMOJI_FLAGS_
#define DJINTERP_TEXT_SYMBOL_EMOJI_FLAGS_

#include "../../../unicode/unicode.h"


// D_EMOJI_FLAG
//   macro: constructs an emoji flag sequence from two regional indicator letters.
#define D_EMOJI_FLAG(L1, L2)  \
    (L1 L2)


// flags (pairs exactly as listed)

// A

// D_EMOJI_FLAG_ASCENSION_ISLAND
//   constant: emoji flag for Ascension Island (ac).
#define D_EMOJI_FLAG_ASCENSION_ISLAND                   \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_C)  // AC

// D_EMOJI_FLAG_ANDORRA
//   constant: emoji flag for Andorra (ad).
#define D_EMOJI_FLAG_ANDORRA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_D)  // AD

// D_EMOJI_FLAG_UNITED_ARAB_EMIRATES
//   constant: emoji flag for United Arab Emirates (ae).
#define D_EMOJI_FLAG_UNITED_ARAB_EMIRATES               \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_E)  // AE

// D_EMOJI_FLAG_AFGHANISTAN
//   constant: emoji flag for Afghanistan (af).
#define D_EMOJI_FLAG_AFGHANISTAN                        \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_F)  // AF

// D_EMOJI_FLAG_ANTIGUA_AND_BARBUDA
//   constant: emoji flag for Antigua and Barbuda (ag).
#define D_EMOJI_FLAG_ANTIGUA_AND_BARBUDA                \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_G)  // AG

// D_EMOJI_FLAG_ANGUILLA
//   constant: emoji flag for Anguilla (ai).
#define D_EMOJI_FLAG_ANGUILLA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_I)  // AI

// D_EMOJI_FLAG_ALBANIA
//   constant: emoji flag for Albania (al).
#define D_EMOJI_FLAG_ALBANIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_L)  // AL

// D_EMOJI_FLAG_ARMENIA
//   constant: emoji flag for Armenia (am).
#define D_EMOJI_FLAG_ARMENIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_M)  // AM

// D_EMOJI_FLAG_ANGOLA
//   constant: emoji flag for Angola (ao).
#define D_EMOJI_FLAG_ANGOLA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_O)  // AO

// D_EMOJI_FLAG_ANTARCTICA
//   constant: emoji flag for Antarctica (aq).
#define D_EMOJI_FLAG_ANTARCTICA                         \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_Q)  // AQ

// D_EMOJI_FLAG_ARGENTINA
//   constant: emoji flag for Argentina (ar).
#define D_EMOJI_FLAG_ARGENTINA                          \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_R)  // AR

// D_EMOJI_FLAG_AMERICAN_SAMOA
//   constant: emoji flag for American Samoa (as).
#define D_EMOJI_FLAG_AMERICAN_SAMOA                     \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_S)  // AS

// D_EMOJI_FLAG_AUSTRIA
//   constant: emoji flag for Austria (at).
#define D_EMOJI_FLAG_AUSTRIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_T)  // AT

// D_EMOJI_FLAG_AUSTRALIA
//   constant: emoji flag for Australia (au).
#define D_EMOJI_FLAG_AUSTRALIA                          \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_U)  // AU

// D_EMOJI_FLAG_ARUBA
//   constant: emoji flag for Aruba (aw).
#define D_EMOJI_FLAG_ARUBA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_W)  // AW

// D_EMOJI_FLAG_ALAND_ISLANDS
//   constant: emoji flag for �land Islands (ax).
#define D_EMOJI_FLAG_ALAND_ISLANDS                      \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_X)  // AX

// D_EMOJI_FLAG_AZERBAIJAN
//   constant: emoji flag for Azerbaijan (az).
#define D_EMOJI_FLAG_AZERBAIJAN                         \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_Z)  // AZ


// B

// D_EMOJI_FLAG_BOSNIA_AND_HERZEGOVINA
//   constant: emoji flag for Bosnia and Herzegovina (ba).
#define D_EMOJI_FLAG_BOSNIA_AND_HERZEGOVINA             \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_A)  // BA

// D_EMOJI_FLAG_BARBADOS
//   constant: emoji flag for Barbados (bb).
#define D_EMOJI_FLAG_BARBADOS                           \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_B)  // BB

// D_EMOJI_FLAG_BANGLADESH
//   constant: emoji flag for Bangladesh (bd).
#define D_EMOJI_FLAG_BANGLADESH                         \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_D)  // BD

// D_EMOJI_FLAG_BELGIUM
//   constant: emoji flag for Belgium (be).
#define D_EMOJI_FLAG_BELGIUM                            \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_E)  // BE

// D_EMOJI_FLAG_BURKINA_FASO
//   constant: emoji flag for Burkina Faso (bf).
#define D_EMOJI_FLAG_BURKINA_FASO                       \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_F)  // BF

// D_EMOJI_FLAG_BULGARIA
//   constant: emoji flag for Bulgaria (bg).
#define D_EMOJI_FLAG_BULGARIA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_G)  // BG

// D_EMOJI_FLAG_BAHRAIN
//   constant: emoji flag for Bahrain (bh).
#define D_EMOJI_FLAG_BAHRAIN                            \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_H)  // BH

// D_EMOJI_FLAG_BURUNDI
//   constant: emoji flag for Burundi (bi).
#define D_EMOJI_FLAG_BURUNDI                            \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_I)  // BI

// D_EMOJI_FLAG_BENIN
//   constant: emoji flag for Benin (bj).
#define D_EMOJI_FLAG_BENIN                              \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_J)  // BJ

// D_EMOJI_FLAG_SAINT_BARTHELEMY
//   constant: emoji flag for Saint Barth�lemy (bl).
#define D_EMOJI_FLAG_SAINT_BARTHELEMY                   \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_L)  // BL

// D_EMOJI_FLAG_BERMUDA
//   constant: emoji flag for Bermuda (bm).
#define D_EMOJI_FLAG_BERMUDA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_M)  // BM

// D_EMOJI_FLAG_BRUNEI
//   constant: emoji flag for Brunei (bn).
#define D_EMOJI_FLAG_BRUNEI                             \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_N)  // BN

// D_EMOJI_FLAG_BOLIVIA
//   constant: emoji flag for Bolivia (bo).
#define D_EMOJI_FLAG_BOLIVIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_O)  // BO

// D_EMOJI_FLAG_CARIBBEAN_NETHERLANDS
//   constant: emoji flag for Caribbean Netherlands (bq).
#define D_EMOJI_FLAG_CARIBBEAN_NETHERLANDS              \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_Q)  // BQ

// D_EMOJI_FLAG_BRAZIL
//   constant: emoji flag for Brazil (br).
#define D_EMOJI_FLAG_BRAZIL                             \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_R)  // BR

// D_EMOJI_FLAG_BAHAMAS
//   constant: emoji flag for Bahamas (bs).
#define D_EMOJI_FLAG_BAHAMAS                            \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_S)  // BS

// D_EMOJI_FLAG_BHUTAN
//   constant: emoji flag for Bhutan (bt).
#define D_EMOJI_FLAG_BHUTAN                             \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_T)  // BT

// D_EMOJI_FLAG_BOUVET_ISLAND
//   constant: emoji flag for Bouvet Island (bv).
#define D_EMOJI_FLAG_BOUVET_ISLAND                      \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_V)  // BV

// D_EMOJI_FLAG_BOTSWANA
//   constant: emoji flag for Botswana (bw).
#define D_EMOJI_FLAG_BOTSWANA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_W)  // BW

// D_EMOJI_FLAG_BELARUS
//   constant: emoji flag for Belarus (by).
#define D_EMOJI_FLAG_BELARUS                            \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_Y)  // BY

// D_EMOJI_FLAG_BELIZE
//   constant: emoji flag for Belize (bz).
#define D_EMOJI_FLAG_BELIZE                             \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_Z)  // BZ


// C

// D_EMOJI_FLAG_CANADA
//   constant: emoji flag for Canada (ca).
#define D_EMOJI_FLAG_CANADA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_A)  // CA

// D_EMOJI_FLAG_COCOS_ISLANDS
//   constant: emoji flag for Cocos (Keeling) Islands (cc).
#define D_EMOJI_FLAG_COCOS_ISLANDS                      \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_C)  // CC

// D_EMOJI_FLAG_CONGO_KINSHASA
//   constant: emoji flag for Congo - Kinshasa (cd).
#define D_EMOJI_FLAG_CONGO_KINSHASA                     \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_D)  // CD

// D_EMOJI_FLAG_CENTRAL_AFRICAN_REPUBLIC
//   constant: emoji flag for Central African Republic (cf).
#define D_EMOJI_FLAG_CENTRAL_AFRICAN_REPUBLIC           \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_F)  // CF

// D_EMOJI_FLAG_CONGO_BRAZZAVILLE
//   constant: emoji flag for Congo - Brazzaville (cg).
#define D_EMOJI_FLAG_CONGO_BRAZZAVILLE                  \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_G)  // CG

// D_EMOJI_FLAG_SWITZERLAND
//   constant: emoji flag for Switzerland (ch).
#define D_EMOJI_FLAG_SWITZERLAND                        \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_H)  // CH

// D_EMOJI_FLAG_COTE_DIVOIRE
//   constant: emoji flag for C�te d�Ivoire (ci).
#define D_EMOJI_FLAG_COTE_DIVOIRE                       \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_I)  // CI

// D_EMOJI_FLAG_COOK_ISLANDS
//   constant: emoji flag for Cook Islands (ck).
#define D_EMOJI_FLAG_COOK_ISLANDS                       \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_K)  // CK

// D_EMOJI_FLAG_CHILE
//   constant: emoji flag for Chile (cl).
#define D_EMOJI_FLAG_CHILE                              \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_L)  // CL

// D_EMOJI_FLAG_CAMEROON
//   constant: emoji flag for Cameroon (cm).
#define D_EMOJI_FLAG_CAMEROON                           \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_M)  // CM

// D_EMOJI_FLAG_CHINA
//   constant: emoji flag for China (cn).
#define D_EMOJI_FLAG_CHINA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_N)  // CN

// D_EMOJI_FLAG_COLOMBIA
//   constant: emoji flag for Colombia (co).
#define D_EMOJI_FLAG_COLOMBIA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_O)  // CO

// D_EMOJI_FLAG_CLIPPERTON_ISLAND
//   constant: emoji flag for Clipperton Island (cp).
#define D_EMOJI_FLAG_CLIPPERTON_ISLAND                  \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_P)  // CP

// D_EMOJI_FLAG_COSTA_RICA
//   constant: emoji flag for Costa Rica (cr).
#define D_EMOJI_FLAG_COSTA_RICA                         \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_R)  // CR

// D_EMOJI_FLAG_CUBA
//   constant: emoji flag for Cuba (cu).
#define D_EMOJI_FLAG_CUBA                               \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_U)  // CU

// D_EMOJI_FLAG_CAPE_VERDE
//   constant: emoji flag for Cape Verde (cv).
#define D_EMOJI_FLAG_CAPE_VERDE                         \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_V)  // CV

// D_EMOJI_FLAG_CURACAO
//   constant: emoji flag for Cura�ao (cw).
#define D_EMOJI_FLAG_CURACAO                            \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_W)  // CW

// D_EMOJI_FLAG_CHRISTMAS_ISLAND
//   constant: emoji flag for Christmas Island (cx).
#define D_EMOJI_FLAG_CHRISTMAS_ISLAND                   \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_X)  // CX

// D_EMOJI_FLAG_CYPRUS
//   constant: emoji flag for Cyprus (cy).
#define D_EMOJI_FLAG_CYPRUS                             \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_Y)  // CY

// D_EMOJI_FLAG_CZECHIA
//   constant: emoji flag for Czechia (cz).
#define D_EMOJI_FLAG_CZECHIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_Z)  // CZ


// D

// D_EMOJI_FLAG_GERMANY
//   constant: emoji flag for Germany (de).
#define D_EMOJI_FLAG_GERMANY                            \
    D_EMOJI_FLAG(D_UNICODE_RI_D, D_UNICODE_RI_E)  // DE

// D_EMOJI_FLAG_DIEGO_GARCIA
//   constant: emoji flag for Diego Garcia (dg).
#define D_EMOJI_FLAG_DIEGO_GARCIA                       \
    D_EMOJI_FLAG(D_UNICODE_RI_D, D_UNICODE_RI_G)  // DG

// D_EMOJI_FLAG_DJIBOUTI
//   constant: emoji flag for Djibouti (dj).
#define D_EMOJI_FLAG_DJIBOUTI                           \
    D_EMOJI_FLAG(D_UNICODE_RI_D, D_UNICODE_RI_J)  // DJ

// D_EMOJI_FLAG_DENMARK
//   constant: emoji flag for Denmark (dk).
#define D_EMOJI_FLAG_DENMARK                            \
    D_EMOJI_FLAG(D_UNICODE_RI_D, D_UNICODE_RI_K)  // DK

// D_EMOJI_FLAG_DOMINICA
//   constant: emoji flag for Dominica (dm).
#define D_EMOJI_FLAG_DOMINICA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_D, D_UNICODE_RI_M)  // DM

// D_EMOJI_FLAG_DOMINICAN_REPUBLIC
//   constant: emoji flag for Dominican Republic (do).
#define D_EMOJI_FLAG_DOMINICAN_REPUBLIC                 \
    D_EMOJI_FLAG(D_UNICODE_RI_D, D_UNICODE_RI_O)  // DO

// D_EMOJI_FLAG_ALGERIA
//   constant: emoji flag for Algeria (dz).
#define D_EMOJI_FLAG_ALGERIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_D, D_UNICODE_RI_Z)  // DZ


// E

// D_EMOJI_FLAG_CEUTA_AND_MELILLA
//   constant: emoji flag for Ceuta and Melilla (ea).
#define D_EMOJI_FLAG_CEUTA_AND_MELILLA                  \
    D_EMOJI_FLAG(D_UNICODE_RI_E, D_UNICODE_RI_A)  // EA

// D_EMOJI_FLAG_ECUADOR
//   constant: emoji flag for Ecuador (ec).
#define D_EMOJI_FLAG_ECUADOR                            \
    D_EMOJI_FLAG(D_UNICODE_RI_E, D_UNICODE_RI_C)  // EC

// D_EMOJI_FLAG_ESTONIA
//   constant: emoji flag for Estonia (ee).
#define D_EMOJI_FLAG_ESTONIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_E, D_UNICODE_RI_E)  // EE

// D_EMOJI_FLAG_EGYPT
//   constant: emoji flag for Egypt (eg).
#define D_EMOJI_FLAG_EGYPT                              \
    D_EMOJI_FLAG(D_UNICODE_RI_E, D_UNICODE_RI_G)  // EG

// D_EMOJI_FLAG_WESTERN_SAHARA
//   constant: emoji flag for Western Sahara (eh).
#define D_EMOJI_FLAG_WESTERN_SAHARA                     \
    D_EMOJI_FLAG(D_UNICODE_RI_E, D_UNICODE_RI_H)  // EH

// D_EMOJI_FLAG_ERITREA
//   constant: emoji flag for Eritrea (er).
#define D_EMOJI_FLAG_ERITREA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_E, D_UNICODE_RI_R)  // ER

// D_EMOJI_FLAG_SPAIN
//   constant: emoji flag for Spain (es).
#define D_EMOJI_FLAG_SPAIN                              \
    D_EMOJI_FLAG(D_UNICODE_RI_E, D_UNICODE_RI_S)  // ES

// D_EMOJI_FLAG_ETHIOPIA
//   constant: emoji flag for Ethiopia (et).
#define D_EMOJI_FLAG_ETHIOPIA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_E, D_UNICODE_RI_T)  // ET

// D_EMOJI_FLAG_EUROPEAN_UNION
//   constant: emoji flag for European Union (eu).
#define D_EMOJI_FLAG_EUROPEAN_UNION                     \
    D_EMOJI_FLAG(D_UNICODE_RI_E, D_UNICODE_RI_U)  // EU


// F

// D_EMOJI_FLAG_FINLAND
//   constant: emoji flag for Finland (fi).
#define D_EMOJI_FLAG_FINLAND                            \
    D_EMOJI_FLAG(D_UNICODE_RI_F, D_UNICODE_RI_I)  // FI

// D_EMOJI_FLAG_FIJI
//   constant: emoji flag for Fiji (fj).
#define D_EMOJI_FLAG_FIJI                               \
    D_EMOJI_FLAG(D_UNICODE_RI_F, D_UNICODE_RI_J)  // FJ

// D_EMOJI_FLAG_FALKLAND_ISLANDS
//   constant: emoji flag for Falkland Islands (fk).
#define D_EMOJI_FLAG_FALKLAND_ISLANDS                   \
    D_EMOJI_FLAG(D_UNICODE_RI_F, D_UNICODE_RI_K)  // FK

// D_EMOJI_FLAG_MICRONESIA
//   constant: emoji flag for Micronesia (fm).
#define D_EMOJI_FLAG_MICRONESIA                         \
    D_EMOJI_FLAG(D_UNICODE_RI_F, D_UNICODE_RI_M)  // FM

// D_EMOJI_FLAG_FAROE_ISLANDS
//   constant: emoji flag for Faroe Islands (fo).
#define D_EMOJI_FLAG_FAROE_ISLANDS                      \
    D_EMOJI_FLAG(D_UNICODE_RI_F, D_UNICODE_RI_O)  // FO

// D_EMOJI_FLAG_FRANCE
//   constant: emoji flag for France (fr).
#define D_EMOJI_FLAG_FRANCE                             \
    D_EMOJI_FLAG(D_UNICODE_RI_F, D_UNICODE_RI_R)  // FR


// G

// D_EMOJI_FLAG_GABON
//   constant: emoji flag for Gabon (ga).
#define D_EMOJI_FLAG_GABON                              \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_A)  // GA

// D_EMOJI_FLAG_UNITED_KINGDOM
//   constant: emoji flag for United Kingdom (gb).
#define D_EMOJI_FLAG_UNITED_KINGDOM                     \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_B)  // GB

// D_EMOJI_FLAG_GRENADA
//   constant: emoji flag for Grenada (gd).
#define D_EMOJI_FLAG_GRENADA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_D)  // GD

// D_EMOJI_FLAG_GEORGIA
//   constant: emoji flag for Georgia (ge).
#define D_EMOJI_FLAG_GEORGIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_E)  // GE

// D_EMOJI_FLAG_FRENCH_GUIANA
//   constant: emoji flag for French Guiana (gf).
#define D_EMOJI_FLAG_FRENCH_GUIANA                      \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_F)  // GF

// D_EMOJI_FLAG_GUERNSEY
//   constant: emoji flag for Guernsey (gg).
#define D_EMOJI_FLAG_GUERNSEY                           \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_G)  // GG

// D_EMOJI_FLAG_GHANA
//   constant: emoji flag for Ghana (gh).
#define D_EMOJI_FLAG_GHANA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_H)  // GH

// D_EMOJI_FLAG_GIBRALTAR
//   constant: emoji flag for Gibraltar (gi).
#define D_EMOJI_FLAG_GIBRALTAR                          \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_I)  // GI

// D_EMOJI_FLAG_GREENLAND
//   constant: emoji flag for Greenland (gl).
#define D_EMOJI_FLAG_GREENLAND                          \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_L)  // GL

// D_EMOJI_FLAG_GAMBIA
//   constant: emoji flag for Gambia (gm).
#define D_EMOJI_FLAG_GAMBIA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_M)  // GM

// D_EMOJI_FLAG_GUINEA
//   constant: emoji flag for Guinea (gn).
#define D_EMOJI_FLAG_GUINEA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_N)  // GN

// D_EMOJI_FLAG_GUADELOUPE
//   constant: emoji flag for Guadeloupe (gp).
#define D_EMOJI_FLAG_GUADELOUPE                         \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_P)  // GP

// D_EMOJI_FLAG_EQUATORIAL_GUINEA
//   constant: emoji flag for Equatorial Guinea (gq).
#define D_EMOJI_FLAG_EQUATORIAL_GUINEA                  \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_Q)  // GQ

// D_EMOJI_FLAG_GREECE
//   constant: emoji flag for Greece (gr).
#define D_EMOJI_FLAG_GREECE                             \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_R)  // GR

// D_EMOJI_FLAG_SOUTH_GEORGIA_SANDWICH_ISLANDS
//   constant: emoji flag for South Georgia and the South Sandwich Islands
//             (gs).
#define D_EMOJI_FLAG_SOUTH_GEORGIA_SANDWICH_ISLANDS     \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_S)  // GS

// D_EMOJI_FLAG_GUATEMALA
//   constant: emoji flag for Guatemala (gt).
#define D_EMOJI_FLAG_GUATEMALA                          \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_T)  // GT

// D_EMOJI_FLAG_GUAM
//   constant: emoji flag for Guam (gu).
#define D_EMOJI_FLAG_GUAM                               \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_U)  // GU

// D_EMOJI_FLAG_GUINEA_BISSAU
//   constant: emoji flag for Guinea-Bissau (gw).
#define D_EMOJI_FLAG_GUINEA_BISSAU                      \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_W)  // GW

// D_EMOJI_FLAG_GUYANA
//   constant: emoji flag for Guyana (gy).
#define D_EMOJI_FLAG_GUYANA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_Y)  // GY


// H

// D_EMOJI_FLAG_HONG_KONG
//   constant: emoji flag for Hong Kong (hk).
#define D_EMOJI_FLAG_HONG_KONG                          \
    D_EMOJI_FLAG(D_UNICODE_RI_H, D_UNICODE_RI_K)  // HK

// D_EMOJI_FLAG_HEARD_MCDONALD_ISLANDS
//   constant: emoji flag for Heard and McDonald Islands (hm).
#define D_EMOJI_FLAG_HEARD_MCDONALD_ISLANDS             \
    D_EMOJI_FLAG(D_UNICODE_RI_H, D_UNICODE_RI_M)  // HM

// D_EMOJI_FLAG_HONDURAS
//   constant: emoji flag for Honduras (hn).
#define D_EMOJI_FLAG_HONDURAS                           \
    D_EMOJI_FLAG(D_UNICODE_RI_H, D_UNICODE_RI_N)  // HN

// D_EMOJI_FLAG_CROATIA
//   constant: emoji flag for Croatia (hr).
#define D_EMOJI_FLAG_CROATIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_H, D_UNICODE_RI_R)  // HR

// D_EMOJI_FLAG_HAITI
//   constant: emoji flag for Haiti (ht).
#define D_EMOJI_FLAG_HAITI                              \
    D_EMOJI_FLAG(D_UNICODE_RI_H, D_UNICODE_RI_T)  // HT

// D_EMOJI_FLAG_HUNGARY
//   constant: emoji flag for Hungary (hu).
#define D_EMOJI_FLAG_HUNGARY                            \
    D_EMOJI_FLAG(D_UNICODE_RI_H, D_UNICODE_RI_U)  // HU


// I

// D_EMOJI_FLAG_CANARY_ISLANDS
//   constant: emoji flag for Canary Islands (ic).
#define D_EMOJI_FLAG_CANARY_ISLANDS                     \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_C)  // IC

// D_EMOJI_FLAG_INDONESIA
//   constant: emoji flag for Indonesia (id).
#define D_EMOJI_FLAG_INDONESIA                          \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_D)  // ID

// D_EMOJI_FLAG_IRELAND
//   constant: emoji flag for Ireland (ie).
#define D_EMOJI_FLAG_IRELAND                            \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_E)  // IE

// D_EMOJI_FLAG_ISRAEL
//   constant: emoji flag for Israel (il).
#define D_EMOJI_FLAG_ISRAEL                             \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_L)  // IL

// D_EMOJI_FLAG_ISLE_OF_MAN
//   constant: emoji flag for Isle of Man (im).
#define D_EMOJI_FLAG_ISLE_OF_MAN                        \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_M)  // IM

// D_EMOJI_FLAG_INDIA
//   constant: emoji flag for India (in).
#define D_EMOJI_FLAG_INDIA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_N)  // IN

// D_EMOJI_FLAG_BRITISH_INDIAN_OCEAN_TERRITORY
//   constant: emoji flag for British Indian Ocean Territory (io).
#define D_EMOJI_FLAG_BRITISH_INDIAN_OCEAN_TERRITORY     \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_O)  // IO

// D_EMOJI_FLAG_IRAQ
//   constant: emoji flag for Iraq (iq).
#define D_EMOJI_FLAG_IRAQ                               \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_Q)  // IQ

// D_EMOJI_FLAG_IRAN
//   constant: emoji flag for Iran (ir).
#define D_EMOJI_FLAG_IRAN                               \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_R)  // IR

// D_EMOJI_FLAG_ICELAND
//   constant: emoji flag for Iceland (is).
#define D_EMOJI_FLAG_ICELAND                            \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_S)  // IS

// D_EMOJI_FLAG_ITALY
//   constant: emoji flag for Italy (it).
#define D_EMOJI_FLAG_ITALY                              \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_T)  // IT


// J

// D_EMOJI_FLAG_JERSEY
//   constant: emoji flag for Jersey (je).
#define D_EMOJI_FLAG_JERSEY                             \
    D_EMOJI_FLAG(D_UNICODE_RI_J, D_UNICODE_RI_E)  // JE

// D_EMOJI_FLAG_JAMAICA
//   constant: emoji flag for Jamaica (jm).
#define D_EMOJI_FLAG_JAMAICA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_J, D_UNICODE_RI_M)  // JM

// D_EMOJI_FLAG_JORDAN
//   constant: emoji flag for Jordan (jo).
#define D_EMOJI_FLAG_JORDAN                             \
    D_EMOJI_FLAG(D_UNICODE_RI_J, D_UNICODE_RI_O)  // JO

// D_EMOJI_FLAG_JAPAN
//   constant: emoji flag for Japan (jp).
#define D_EMOJI_FLAG_JAPAN                              \
    D_EMOJI_FLAG(D_UNICODE_RI_J, D_UNICODE_RI_P)  // JP


// K

// D_EMOJI_FLAG_KENYA
//   constant: emoji flag for Kenya (ke).
#define D_EMOJI_FLAG_KENYA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_E)  // KE

// D_EMOJI_FLAG_KYRGYZSTAN
//   constant: emoji flag for Kyrgyzstan (kg).
#define D_EMOJI_FLAG_KYRGYZSTAN                         \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_G)  // KG

// D_EMOJI_FLAG_CAMBODIA
//   constant: emoji flag for Cambodia (kh).
#define D_EMOJI_FLAG_CAMBODIA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_H)  // KH

// D_EMOJI_FLAG_KIRIBATI
//   constant: emoji flag for Kiribati (ki).
#define D_EMOJI_FLAG_KIRIBATI                           \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_I)  // KI

// D_EMOJI_FLAG_COMOROS
//   constant: emoji flag for Comoros (km).
#define D_EMOJI_FLAG_COMOROS                            \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_M)  // KM

// D_EMOJI_FLAG_SAINT_KITTS_AND_NEVIS
//   constant: emoji flag for Saint Kitts and Nevis (kn).
#define D_EMOJI_FLAG_SAINT_KITTS_AND_NEVIS              \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_N)  // KN

// D_EMOJI_FLAG_NORTH_KOREA
//   constant: emoji flag for North Korea (kp).
#define D_EMOJI_FLAG_NORTH_KOREA                        \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_P)  // KP

// D_EMOJI_FLAG_SOUTH_KOREA
//   constant: emoji flag for South Korea (kr).
#define D_EMOJI_FLAG_SOUTH_KOREA                        \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_R)  // KR

// D_EMOJI_FLAG_KUWAIT
//   constant: emoji flag for Kuwait (kw).
#define D_EMOJI_FLAG_KUWAIT                             \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_W)  // KW

// D_EMOJI_FLAG_CAYMAN_ISLANDS
//   constant: emoji flag for Cayman Islands (ky).
#define D_EMOJI_FLAG_CAYMAN_ISLANDS                     \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_Y)  // KY

// D_EMOJI_FLAG_KAZAKHSTAN
//   constant: emoji flag for Kazakhstan (kz).
#define D_EMOJI_FLAG_KAZAKHSTAN                         \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_Z)  // KZ


// L

// D_EMOJI_FLAG_LEBANON
//   constant: emoji flag for Lebanon (lb).
#define D_EMOJI_FLAG_LEBANON                            \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_B)  // LB

// D_EMOJI_FLAG_SAINT_LUCIA
//   constant: emoji flag for Saint Lucia (lc).
#define D_EMOJI_FLAG_SAINT_LUCIA                        \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_C)  // LC

// D_EMOJI_FLAG_LIECHTENSTEIN
//   constant: emoji flag for Liechtenstein (li).
#define D_EMOJI_FLAG_LIECHTENSTEIN                      \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_I)  // LI

// D_EMOJI_FLAG_SRI_LANKA
//   constant: emoji flag for Sri Lanka (lk).
#define D_EMOJI_FLAG_SRI_LANKA                          \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_K)  // LK

// D_EMOJI_FLAG_LIBERIA
//   constant: emoji flag for Liberia (lr).
#define D_EMOJI_FLAG_LIBERIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_R)  // LR

// D_EMOJI_FLAG_LESOTHO
//   constant: emoji flag for Lesotho (ls).
#define D_EMOJI_FLAG_LESOTHO                            \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_S)  // LS

// D_EMOJI_FLAG_LITHUANIA
//   constant: emoji flag for Lithuania (lt).
#define D_EMOJI_FLAG_LITHUANIA                          \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_T)  // LT

// D_EMOJI_FLAG_LUXEMBOURG
//   constant: emoji flag for Luxembourg (lu).
#define D_EMOJI_FLAG_LUXEMBOURG                         \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_U)  // LU

// D_EMOJI_FLAG_LATVIA
//   constant: emoji flag for Latvia (lv).
#define D_EMOJI_FLAG_LATVIA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_V)  // LV

// D_EMOJI_FLAG_LIBYA
//   constant: emoji flag for Libya (ly).
#define D_EMOJI_FLAG_LIBYA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_Y)  // LY


// M

// D_EMOJI_FLAG_MOROCCO
//   constant: emoji flag for Morocco (ma).
#define D_EMOJI_FLAG_MOROCCO                            \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_A)  // MA

// D_EMOJI_FLAG_MONACO
//   constant: emoji flag for Monaco (mc).
#define D_EMOJI_FLAG_MONACO                             \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_C)  // MC

// D_EMOJI_FLAG_MOLDOVA
//   constant: emoji flag for Moldova (md).
#define D_EMOJI_FLAG_MOLDOVA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_D)  // MD

// D_EMOJI_FLAG_MONTENEGRO
//   constant: emoji flag for Montenegro (me).
#define D_EMOJI_FLAG_MONTENEGRO                         \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_E)  // ME

// D_EMOJI_FLAG_SAINT_MARTIN
//   constant: emoji flag for Saint Martin (mf).
#define D_EMOJI_FLAG_SAINT_MARTIN                       \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_F)  // MF

// D_EMOJI_FLAG_MADAGASCAR
//   constant: emoji flag for Madagascar (mg).
#define D_EMOJI_FLAG_MADAGASCAR                         \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_G)  // MG

// D_EMOJI_FLAG_MARSHALL_ISLANDS
//   constant: emoji flag for Marshall Islands (mh).
#define D_EMOJI_FLAG_MARSHALL_ISLANDS                   \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_H)  // MH

// D_EMOJI_FLAG_NORTH_MACEDONIA
//   constant: emoji flag for North Macedonia (mk).
#define D_EMOJI_FLAG_NORTH_MACEDONIA                    \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_K)  // MK

// D_EMOJI_FLAG_MALI
//   constant: emoji flag for Mali (ml).
#define D_EMOJI_FLAG_MALI                               \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_L)  // ML

// D_EMOJI_FLAG_MYANMAR
//   constant: emoji flag for Myanmar (mm).
#define D_EMOJI_FLAG_MYANMAR                            \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_M)  // MM

// D_EMOJI_FLAG_MONGOLIA
//   constant: emoji flag for Mongolia (mn).
#define D_EMOJI_FLAG_MONGOLIA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_N)  // MN

// D_EMOJI_FLAG_MACAO
//   constant: emoji flag for Macao (mo).
#define D_EMOJI_FLAG_MACAO                              \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_O)  // MO

// D_EMOJI_FLAG_NORTHERN_MARIANA_ISLANDS
//   constant: emoji flag for Northern Mariana Islands (mp).
#define D_EMOJI_FLAG_NORTHERN_MARIANA_ISLANDS           \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_P)  // MP

// D_EMOJI_FLAG_MARTINIQUE
//   constant: emoji flag for Martinique (mq).
#define D_EMOJI_FLAG_MARTINIQUE                         \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_Q)  // MQ

// D_EMOJI_FLAG_MAURITANIA
//   constant: emoji flag for Mauritania (mr).
#define D_EMOJI_FLAG_MAURITANIA                         \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_R)  // MR

// D_EMOJI_FLAG_MONTSERRAT
//   constant: emoji flag for Montserrat (ms).
#define D_EMOJI_FLAG_MONTSERRAT                         \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_S)  // MS

// D_EMOJI_FLAG_MALTA
//   constant: emoji flag for Malta (mt).
#define D_EMOJI_FLAG_MALTA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_T)  // MT

// D_EMOJI_FLAG_MAURITIUS
//   constant: emoji flag for Mauritius (mu).
#define D_EMOJI_FLAG_MAURITIUS                          \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_U)  // MU

// D_EMOJI_FLAG_MALDIVES
//   constant: emoji flag for Maldives (mv).
#define D_EMOJI_FLAG_MALDIVES                           \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_V)  // MV

// D_EMOJI_FLAG_MALAWI
//   constant: emoji flag for Malawi (mw).
#define D_EMOJI_FLAG_MALAWI                             \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_W)  // MW

// D_EMOJI_FLAG_MEXICO
//   constant: emoji flag for Mexico (mx).
#define D_EMOJI_FLAG_MEXICO                             \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_X)  // MX

// D_EMOJI_FLAG_MALAYSIA
//   constant: emoji flag for Malaysia (my).
#define D_EMOJI_FLAG_MALAYSIA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_Y)  // MY

// D_EMOJI_FLAG_MOZAMBIQUE
//   constant: emoji flag for Mozambique (mz).
#define D_EMOJI_FLAG_MOZAMBIQUE                         \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_Z)  // MZ


// N

// D_EMOJI_FLAG_NEW_CALEDONIA
//   constant: emoji flag for New Caledonia (nc).
#define D_EMOJI_FLAG_NEW_CALEDONIA                      \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_C)  // NC

// D_EMOJI_FLAG_NIGER
//   constant: emoji flag for Niger (ne).
#define D_EMOJI_FLAG_NIGER                              \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_E)  // NE

// D_EMOJI_FLAG_NORFOLK_ISLAND
//   constant: emoji flag for Norfolk Island (nf).
#define D_EMOJI_FLAG_NORFOLK_ISLAND                     \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_F)  // NF

// D_EMOJI_FLAG_NIGERIA
//   constant: emoji flag for Nigeria (ng).
#define D_EMOJI_FLAG_NIGERIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_G)  // NG

// D_EMOJI_FLAG_NICARAGUA
//   constant: emoji flag for Nicaragua (ni).
#define D_EMOJI_FLAG_NICARAGUA                          \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_I)  // NI

// D_EMOJI_FLAG_NETHERLANDS
//   constant: emoji flag for Netherlands (nl).
#define D_EMOJI_FLAG_NETHERLANDS                        \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_L)  // NL

// D_EMOJI_FLAG_NORWAY
//   constant: emoji flag for Norway (no).
#define D_EMOJI_FLAG_NORWAY                             \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_O)  // NO

// D_EMOJI_FLAG_NEPAL
//   constant: emoji flag for Nepal (np).
#define D_EMOJI_FLAG_NEPAL                              \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_P)  // NP

// D_EMOJI_FLAG_NAURU
//   constant: emoji flag for Nauru (nr).
#define D_EMOJI_FLAG_NAURU                              \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_R)  // NR

// D_EMOJI_FLAG_NIUE
//   constant: emoji flag for Niue (nu).
#define D_EMOJI_FLAG_NIUE                               \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_U)  // NU

// D_EMOJI_FLAG_NEW_ZEALAND
//   constant: emoji flag for New Zealand (nz).
#define D_EMOJI_FLAG_NEW_ZEALAND                        \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_Z)  // NZ


// O

// D_EMOJI_FLAG_OMAN
//   constant: emoji flag for Oman (om).
#define D_EMOJI_FLAG_OMAN                               \
    D_EMOJI_FLAG(D_UNICODE_RI_O, D_UNICODE_RI_M)  // OM


// P

// D_EMOJI_FLAG_PANAMA
//   constant: emoji flag for Panama (pa).
#define D_EMOJI_FLAG_PANAMA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_A)  // PA

// D_EMOJI_FLAG_PERU
//   constant: emoji flag for Peru (pe).
#define D_EMOJI_FLAG_PERU                               \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_E)  // PE

// D_EMOJI_FLAG_FRENCH_POLYNESIA
//   constant: emoji flag for French Polynesia (pf).
#define D_EMOJI_FLAG_FRENCH_POLYNESIA                   \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_F)  // PF

// D_EMOJI_FLAG_PAPUA_NEW_GUINEA
//   constant: emoji flag for Papua New Guinea (pg).
#define D_EMOJI_FLAG_PAPUA_NEW_GUINEA                   \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_G)  // PG

// D_EMOJI_FLAG_PHILIPPINES
//   constant: emoji flag for Philippines (ph).
#define D_EMOJI_FLAG_PHILIPPINES                        \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_H)  // PH

// D_EMOJI_FLAG_PAKISTAN
//   constant: emoji flag for Pakistan (pk).
#define D_EMOJI_FLAG_PAKISTAN                           \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_K)  // PK

// D_EMOJI_FLAG_POLAND
//   constant: emoji flag for Poland (pl).
#define D_EMOJI_FLAG_POLAND                             \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_L)  // PL

// D_EMOJI_FLAG_SAINT_PIERRE_MIQUELON
//   constant: emoji flag for Saint Pierre and Miquelon (pm).
#define D_EMOJI_FLAG_SAINT_PIERRE_MIQUELON              \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_M)  // PM

// D_EMOJI_FLAG_PITCAIRN_ISLANDS
//   constant: emoji flag for Pitcairn Islands (pn).
#define D_EMOJI_FLAG_PITCAIRN_ISLANDS                   \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_N)  // PN

// D_EMOJI_FLAG_PUERTO_RICO
//   constant: emoji flag for Puerto Rico (pr).
#define D_EMOJI_FLAG_PUERTO_RICO                        \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_R)  // PR

// D_EMOJI_FLAG_PALESTINIAN_TERRITORIES
//   constant: emoji flag for Palestinian Territories (ps).
#define D_EMOJI_FLAG_PALESTINIAN_TERRITORIES            \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_S)  // PS

// D_EMOJI_FLAG_PORTUGAL
//   constant: emoji flag for Portugal (pt).
#define D_EMOJI_FLAG_PORTUGAL                           \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_T)  // PT

// D_EMOJI_FLAG_PALAU
//   constant: emoji flag for Palau (pw).
#define D_EMOJI_FLAG_PALAU                              \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_W)  // PW

// D_EMOJI_FLAG_PARAGUAY
//   constant: emoji flag for Paraguay (py).
#define D_EMOJI_FLAG_PARAGUAY                           \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_Y)  // PY


// Q

// D_EMOJI_FLAG_QATAR
//   constant: emoji flag for Qatar (qa).
#define D_EMOJI_FLAG_QATAR                              \
    D_EMOJI_FLAG(D_UNICODE_RI_Q, D_UNICODE_RI_A)  // QA


// R

// D_EMOJI_FLAG_REUNION
//   constant: emoji flag for R�union (re).
#define D_EMOJI_FLAG_REUNION                            \
    D_EMOJI_FLAG(D_UNICODE_RI_R, D_UNICODE_RI_E)  // RE

// D_EMOJI_FLAG_ROMANIA
//   constant: emoji flag for Romania (ro).
#define D_EMOJI_FLAG_ROMANIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_R, D_UNICODE_RI_O)  // RO

// D_EMOJI_FLAG_SERBIA
//   constant: emoji flag for Serbia (rs).
#define D_EMOJI_FLAG_SERBIA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_R, D_UNICODE_RI_S)  // RS

// D_EMOJI_FLAG_RUSSIA
//   constant: emoji flag for Russia (ru).
#define D_EMOJI_FLAG_RUSSIA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_R, D_UNICODE_RI_U)  // RU

// D_EMOJI_FLAG_RWANDA
//   constant: emoji flag for Rwanda (rw).
#define D_EMOJI_FLAG_RWANDA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_R, D_UNICODE_RI_W)  // RW


// S

// D_EMOJI_FLAG_SAUDI_ARABIA
//   constant: emoji flag for Saudi Arabia (sa).
#define D_EMOJI_FLAG_SAUDI_ARABIA                       \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_A)  // SA

// D_EMOJI_FLAG_SOLOMON_ISLANDS
//   constant: emoji flag for Solomon Islands (sb).
#define D_EMOJI_FLAG_SOLOMON_ISLANDS                    \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_B)  // SB

// D_EMOJI_FLAG_SEYCHELLES
//   constant: emoji flag for Seychelles (sc).
#define D_EMOJI_FLAG_SEYCHELLES                         \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_C)  // SC

// D_EMOJI_FLAG_SUDAN
//   constant: emoji flag for Sudan (sd).
#define D_EMOJI_FLAG_SUDAN                              \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_D)  // SD

// D_EMOJI_FLAG_SWEDEN
//   constant: emoji flag for Sweden (se).
#define D_EMOJI_FLAG_SWEDEN                             \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_E)  // SE

// D_EMOJI_FLAG_SINGAPORE
//   constant: emoji flag for Singapore (sg).
#define D_EMOJI_FLAG_SINGAPORE                          \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_G)  // SG

// D_EMOJI_FLAG_SAINT_HELENA
//   constant: emoji flag for Saint Helena (sh).
#define D_EMOJI_FLAG_SAINT_HELENA                       \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_H)  // SH

// D_EMOJI_FLAG_SLOVENIA
//   constant: emoji flag for Slovenia (si).
#define D_EMOJI_FLAG_SLOVENIA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_I)  // SI

// D_EMOJI_FLAG_SVALBARD_JAN_MAYEN
//   constant: emoji flag for Svalbard and Jan Mayen (sj).
#define D_EMOJI_FLAG_SVALBARD_JAN_MAYEN                 \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_J)  // SJ

// D_EMOJI_FLAG_SLOVAKIA
//   constant: emoji flag for Slovakia (sk).
#define D_EMOJI_FLAG_SLOVAKIA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_K)  // SK

// D_EMOJI_FLAG_SIERRA_LEONE
//   constant: emoji flag for Sierra Leone (sl).
#define D_EMOJI_FLAG_SIERRA_LEONE                       \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_L)  // SL

// D_EMOJI_FLAG_SAN_MARINO
//   constant: emoji flag for San Marino (sm).
#define D_EMOJI_FLAG_SAN_MARINO                         \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_M)  // SM

// D_EMOJI_FLAG_SENEGAL
//   constant: emoji flag for Senegal (sn).
#define D_EMOJI_FLAG_SENEGAL                            \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_N)  // SN

// D_EMOJI_FLAG_SOMALIA
//   constant: emoji flag for Somalia (so).
#define D_EMOJI_FLAG_SOMALIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_O)  // SO

// D_EMOJI_FLAG_SURINAME
//   constant: emoji flag for Suriname (sr).
#define D_EMOJI_FLAG_SURINAME                           \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_R)  // SR

// D_EMOJI_FLAG_SOUTH_SUDAN
//   constant: emoji flag for South Sudan (ss).
#define D_EMOJI_FLAG_SOUTH_SUDAN                        \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_S)  // SS

// D_EMOJI_FLAG_SAO_TOME_PRINCIPE
//   constant: emoji flag for S�o Tom� and Pr�ncipe (st).
#define D_EMOJI_FLAG_SAO_TOME_PRINCIPE                  \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_T)  // ST

// D_EMOJI_FLAG_EL_SALVADOR
//   constant: emoji flag for El Salvador (sv).
#define D_EMOJI_FLAG_EL_SALVADOR                        \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_V)  // SV

// D_EMOJI_FLAG_SINT_MAARTEN
//   constant: emoji flag for Sint Maarten (sx).
#define D_EMOJI_FLAG_SINT_MAARTEN                       \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_X)  // SX

// D_EMOJI_FLAG_SYRIA
//   constant: emoji flag for Syria (sy).
#define D_EMOJI_FLAG_SYRIA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_Y)  // SY

// D_EMOJI_FLAG_ESWATINI
//   constant: emoji flag for Eswatini (sz).
#define D_EMOJI_FLAG_ESWATINI                           \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_Z)  // SZ


// T

// D_EMOJI_FLAG_TRISTAN_DA_CUNHA
//   constant: emoji flag for Tristan da Cunha (ta).
#define D_EMOJI_FLAG_TRISTAN_DA_CUNHA                   \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_A)  // TA

// D_EMOJI_FLAG_TURKS_CAICOS_ISLANDS
//   constant: emoji flag for Turks and Caicos Islands (tc).
#define D_EMOJI_FLAG_TURKS_CAICOS_ISLANDS               \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_C)  // TC

// D_EMOJI_FLAG_CHAD
//   constant: emoji flag for Chad (td).
#define D_EMOJI_FLAG_CHAD                               \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_D)  // TD

// D_EMOJI_FLAG_FRENCH_SOUTHERN_TERRITORIES
//   constant: emoji flag for French Southern Territories (tf).
#define D_EMOJI_FLAG_FRENCH_SOUTHERN_TERRITORIES        \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_F)  // TF

// D_EMOJI_FLAG_TOGO
//   constant: emoji flag for Togo (tg).
#define D_EMOJI_FLAG_TOGO                               \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_G)  // TG

// D_EMOJI_FLAG_THAILAND
//   constant: emoji flag for Thailand (th).
#define D_EMOJI_FLAG_THAILAND                           \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_H)  // TH

// D_EMOJI_FLAG_TAJIKISTAN
//   constant: emoji flag for Tajikistan (tj).
#define D_EMOJI_FLAG_TAJIKISTAN                         \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_J)  // TJ

// D_EMOJI_FLAG_TOKELAU
//   constant: emoji flag for Tokelau (tk).
#define D_EMOJI_FLAG_TOKELAU                            \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_K)  // TK

// D_EMOJI_FLAG_TIMOR_LESTE
//   constant: emoji flag for Timor-Leste (tl).
#define D_EMOJI_FLAG_TIMOR_LESTE                        \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_L)  // TL

// D_EMOJI_FLAG_TURKMENISTAN
//   constant: emoji flag for Turkmenistan (tm).
#define D_EMOJI_FLAG_TURKMENISTAN                       \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_M)  // TM

// D_EMOJI_FLAG_TUNISIA
//   constant: emoji flag for Tunisia (tn).
#define D_EMOJI_FLAG_TUNISIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_N)  // TN

// D_EMOJI_FLAG_TONGA
//   constant: emoji flag for Tonga (to).
#define D_EMOJI_FLAG_TONGA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_O)  // TO

// D_EMOJI_FLAG_TURKEY
//   constant: emoji flag for Turkey (tr).
#define D_EMOJI_FLAG_TURKEY                             \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_R)  // TR

// D_EMOJI_FLAG_TRINIDAD_AND_TOBAGO
//   constant: emoji flag for Trinidad and Tobago (tt).
#define D_EMOJI_FLAG_TRINIDAD_AND_TOBAGO                \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_T)  // TT

// D_EMOJI_FLAG_TUVALU
//   constant: emoji flag for Tuvalu (tv).
#define D_EMOJI_FLAG_TUVALU                             \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_V)  // TV

// D_EMOJI_FLAG_TAIWAN
//   constant: emoji flag for Taiwan (tw).
#define D_EMOJI_FLAG_TAIWAN                             \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_W)  // TW

// D_EMOJI_FLAG_TANZANIA
//   constant: emoji flag for Tanzania (tz).
#define D_EMOJI_FLAG_TANZANIA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_Z)  // TZ


// U

// D_EMOJI_FLAG_UKRAINE
//   constant: emoji flag for Ukraine (ua).
#define D_EMOJI_FLAG_UKRAINE                            \
    D_EMOJI_FLAG(D_UNICODE_RI_U, D_UNICODE_RI_A)  // UA

// D_EMOJI_FLAG_UGANDA
//   constant: emoji flag for Uganda (ug).
#define D_EMOJI_FLAG_UGANDA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_U, D_UNICODE_RI_G)  // UG

// D_EMOJI_FLAG_US_OUTLYING_ISLANDS
//   constant: emoji flag for U.S. Outlying Islands (um).
#define D_EMOJI_FLAG_US_OUTLYING_ISLANDS                \
    D_EMOJI_FLAG(D_UNICODE_RI_U, D_UNICODE_RI_M)  // UM

// D_EMOJI_FLAG_UNITED_NATIONS
//   constant: emoji flag for United Nations (un).
#define D_EMOJI_FLAG_UNITED_NATIONS                     \
    D_EMOJI_FLAG(D_UNICODE_RI_U, D_UNICODE_RI_N)  // UN

// D_EMOJI_FLAG_UNITED_STATES
//   constant: emoji flag for United States (us).
#define D_EMOJI_FLAG_UNITED_STATES                      \
    D_EMOJI_FLAG(D_UNICODE_RI_U, D_UNICODE_RI_S)  // US

// D_EMOJI_FLAG_URUGUAY
//   constant: emoji flag for Uruguay (uy).
#define D_EMOJI_FLAG_URUGUAY                            \
    D_EMOJI_FLAG(D_UNICODE_RI_U, D_UNICODE_RI_Y)  // UY

// D_EMOJI_FLAG_UZBEKISTAN
//   constant: emoji flag for Uzbekistan (uz).
#define D_EMOJI_FLAG_UZBEKISTAN                         \
    D_EMOJI_FLAG(D_UNICODE_RI_U, D_UNICODE_RI_Z)  // UZ


// V

// D_EMOJI_FLAG_VATICAN_CITY
//   constant: emoji flag for Vatican City (va).
#define D_EMOJI_FLAG_VATICAN_CITY                       \
    D_EMOJI_FLAG(D_UNICODE_RI_V, D_UNICODE_RI_A)  // VA

// D_EMOJI_FLAG_SAINT_VINCENT_GRENADINES
//   constant: emoji flag for Saint Vincent and the Grenadines (vc).
#define D_EMOJI_FLAG_SAINT_VINCENT_GRENADINES           \
    D_EMOJI_FLAG(D_UNICODE_RI_V, D_UNICODE_RI_C)  // VC

// D_EMOJI_FLAG_VENEZUELA
//   constant: emoji flag for Venezuela (ve).
#define D_EMOJI_FLAG_VENEZUELA                          \
    D_EMOJI_FLAG(D_UNICODE_RI_V, D_UNICODE_RI_E)  // VE

// D_EMOJI_FLAG_BRITISH_VIRGIN_ISLANDS
//   constant: emoji flag for British Virgin Islands (vg).
#define D_EMOJI_FLAG_BRITISH_VIRGIN_ISLANDS             \
    D_EMOJI_FLAG(D_UNICODE_RI_V, D_UNICODE_RI_G)  // VG

// D_EMOJI_FLAG_US_VIRGIN_ISLANDS
//   constant: emoji flag for U.S. Virgin Islands (vi).
#define D_EMOJI_FLAG_US_VIRGIN_ISLANDS                  \
    D_EMOJI_FLAG(D_UNICODE_RI_V, D_UNICODE_RI_I)  // VI

// D_EMOJI_FLAG_VIETNAM
//   constant: emoji flag for Vietnam (vn).
#define D_EMOJI_FLAG_VIETNAM                            \
    D_EMOJI_FLAG(D_UNICODE_RI_V, D_UNICODE_RI_N)  // VN

// D_EMOJI_FLAG_VANUATU
//   constant: emoji flag for Vanuatu (vu).
#define D_EMOJI_FLAG_VANUATU                            \
    D_EMOJI_FLAG(D_UNICODE_RI_V, D_UNICODE_RI_U)  // VU


// W

// D_EMOJI_FLAG_WALLIS_AND_FUTUNA
//   constant: emoji flag for Wallis and Futuna (wf).
#define D_EMOJI_FLAG_WALLIS_AND_FUTUNA                  \
    D_EMOJI_FLAG(D_UNICODE_RI_W, D_UNICODE_RI_F)  // WF

// D_EMOJI_FLAG_SAMOA
//   constant: emoji flag for Samoa (ws).
#define D_EMOJI_FLAG_SAMOA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_W, D_UNICODE_RI_S)  // WS


// X

// D_EMOJI_FLAG_KOSOVO
//   constant: emoji flag for Kosovo (xk).
#define D_EMOJI_FLAG_KOSOVO                             \
    D_EMOJI_FLAG(D_UNICODE_RI_X, D_UNICODE_RI_K)  // XK


// Y

// D_EMOJI_FLAG_YEMEN
//   constant: emoji flag for Yemen (ye).
#define D_EMOJI_FLAG_YEMEN                              \
    D_EMOJI_FLAG(D_UNICODE_RI_Y, D_UNICODE_RI_E)  // YE

// D_EMOJI_FLAG_MAYOTTE
//   constant: emoji flag for Mayotte (yt).
#define D_EMOJI_FLAG_MAYOTTE                            \
    D_EMOJI_FLAG(D_UNICODE_RI_Y, D_UNICODE_RI_T)  // YT


// Z

// D_EMOJI_FLAG_SOUTH_AFRICA
//   constant: emoji flag for South Africa (za).
#define D_EMOJI_FLAG_SOUTH_AFRICA                       \
    D_EMOJI_FLAG(D_UNICODE_RI_Z, D_UNICODE_RI_A)  // ZA

// D_EMOJI_FLAG_ZAMBIA
//   constant: emoji flag for Zambia (zm).
#define D_EMOJI_FLAG_ZAMBIA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_Z, D_UNICODE_RI_M)  // ZM

// D_EMOJI_FLAG_ZIMBABWE
//   constant: emoji flag for Zimbabwe (zw).
#define D_EMOJI_FLAG_ZIMBABWE                           \
    D_EMOJI_FLAG(D_UNICODE_RI_Z, D_UNICODE_RI_W)  // ZW


#endif	// DJINTERP_TEXT_SYMBOL_EMOJI_FLAGS_