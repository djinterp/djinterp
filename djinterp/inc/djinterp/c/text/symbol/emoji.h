/******************************************************************************
* djinterp [text]                                                      emoji.h
*
*   
*   Works in plain C99+ without embedding non-ASCII in the source.
*
* Usage:
*  - printf("%s\n", EMOJI_UTF8(CP_PARTY_POPPER));
*    // (VS16 forces emoji style)
*  - printf("%s\n", EMOJI_SEQ(CP_CHECK_MARK, CP_VS16));
*    //  (woman technologist)
*  - printf("%s\n", EMOJI_SEQ(0x1F469u, CP_ZWJ, 0x1F4BBu));
* Notes:
*  - Output must be interpreted as UTF-8 (most terminals). On Windows console,
*    run `chcp 65001` or call SetConsoleOutputCP(CP_UTF8) in your program.
*  - EMOJI_UTF8 / EMOJI_SEQ return pointers to a thread-local buffer. Don't
*    use the result twice in the SAME printf argument list; store/copy first or
*    print in separate calls.
*
*
* path:      \inc\text\symbol\emoji.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.09.22
******************************************************************************/

#ifndef DJINTERP_TEXT_SYMBOL_EMOJI_
#define DJINTERP_TEXT_SYMBOL_EMOJI_

#include <stddef.h>         // size_t 
#include <stdint.h>         // uint32_t
#include "../../djinterp.h"
#include "../../dmemory.h"
#include "..\


// ============================================================================
// FACE EMOJIS
// ============================================================================
// core smileys                             
//   U+1F600-U+1F637                        
#define D_EMOJI_FACE_GRINNING                    "\xF0\x9F\x98\x80"  // U+1F600
#define D_EMOJI_FACE_BEAMING                     "\xF0\x9F\x98\x81"  // U+1F601
#define D_EMOJI_FACE_TEARS_OF_JOY                "\xF0\x9F\x98\x82"  // U+1F602
#define D_EMOJI_FACE_GRINNING_BIG_EYES           "\xF0\x9F\x98\x83"  // U+1F603
#define D_EMOJI_FACE_GRINNING_SMILING_EYES       "\xF0\x9F\x98\x84"  // U+1F604
#define D_EMOJI_FACE_GRINNING_SWEAT              "\xF0\x9F\x98\x85"  // U+1F605
#define D_EMOJI_FACE_GRINNING_SQUINTING          "\xF0\x9F\x98\x86"  // U+1F606
#define D_EMOJI_FACE_SMILING_HALO                "\xF0\x9F\x98\x87"  // U+1F607
#define D_EMOJI_FACE_SMILING_DEVIL               "\xF0\x9F\x98\x88"  // U+1F608
#define D_EMOJI_FACE_WINK                        "\xF0\x9F\x98\x89"  // U+1F609
#define D_EMOJI_FACE_SMILING_EYES                "\xF0\x9F\x98\x8A"  // U+1F60A
#define D_EMOJI_FACE_SAVORING_FOOD               "\xF0\x9F\x98\x8B"  // U+1F60B
#define D_EMOJI_FACE_RELIEVED                    "\xF0\x9F\x98\x8C"  // U+1F60C
#define D_EMOJI_FACE_HEART_EYES                  "\xF0\x9F\x98\x8D"  // U+1F60D
#define D_EMOJI_FACE_SUNGLASSES                  "\xF0\x9F\x98\x8E"  // U+1F60E
#define D_EMOJI_FACE_SMIRK                       "\xF0\x9F\x98\x8F"  // U+1F60F
#define D_EMOJI_FACE_NEUTRAL                     "\xF0\x9F\x98\x90"  // U+1F610
#define D_EMOJI_FACE_EXPRESSIONLESS              "\xF0\x9F\x98\x91"  // U+1F611
#define D_EMOJI_FACE_UNAMUSED                    "\xF0\x9F\x98\x92"  // U+1F612
#define D_EMOJI_FACE_DOWNCAST_SWEAT              "\xF0\x9F\x98\x93"  // U+1F613
#define D_EMOJI_FACE_PENSIVE                     "\xF0\x9F\x98\x94"  // U+1F614
#define D_EMOJI_FACE_CONFUSED                    "\xF0\x9F\x98\x95"  // U+1F615
#define D_EMOJI_FACE_CONFOUNDED                  "\xF0\x9F\x98\x96"  // U+1F616
#define D_EMOJI_FACE_KISSING                     "\xF0\x9F\x98\x97"  // U+1F617
#define D_EMOJI_FACE_KISS_HEART                  "\xF0\x9F\x98\x98"  // U+1F618
#define D_EMOJI_FACE_KISSING_SMILING_EYES        "\xF0\x9F\x98\x99"  // U+1F619
#define D_EMOJI_FACE_KISSING_CLOSED_EYES         "\xF0\x9F\x98\x9A"  // U+1F61A
#define D_EMOJI_FACE_TONGUE                      "\xF0\x9F\x98\x9B"  // U+1F61B
#define D_EMOJI_FACE_WINK_TONGUE                 "\xF0\x9F\x98\x9C"  // U+1F61C
#define D_EMOJI_FACE_SQUINT_TONGUE               "\xF0\x9F\x98\x9D"  // U+1F61D
#define D_EMOJI_FACE_DISAPPOINTED                "\xF0\x9F\x98\x9E"  // U+1F61E
#define D_EMOJI_FACE_WORRIED                     "\xF0\x9F\x98\x9F"  // U+1F61F
#define D_EMOJI_FACE_ANGRY                       "\xF0\x9F\x98\xA0"  // U+1F620
#define D_EMOJI_FACE_POUTING                     "\xF0\x9F\x98\xA1"  // U+1F621
#define D_EMOJI_FACE_CRYING                      "\xF0\x9F\x98\xA2"  // U+1F622
#define D_EMOJI_FACE_PERSEVERING                 "\xF0\x9F\x98\xA3"  // U+1F623
#define D_EMOJI_FACE_TRIUMPH                     "\xF0\x9F\x98\xA4"  // U+1F624
#define D_EMOJI_FACE_DISAPPOINTED_RELIEVED       "\xF0\x9F\x98\xA5"  // U+1F625
#define D_EMOJI_FACE_FROWNING                    "\xF0\x9F\x98\xA6"  // U+1F626
#define D_EMOJI_FACE_ANGUISHED                   "\xF0\x9F\x98\xA7"  // U+1F627
#define D_EMOJI_FACE_FEARFUL                     "\xF0\x9F\x98\xA8"  // U+1F628
#define D_EMOJI_FACE_WEARY                       "\xF0\x9F\x98\xA9"  // U+1F629
#define D_EMOJI_FACE_SLEEPY                      "\xF0\x9F\x98\xAA"  // U+1F62A
#define D_EMOJI_FACE_TIRED                       "\xF0\x9F\x98\xAB"  // U+1F62B
#define D_EMOJI_FACE_GRIMACING                   "\xF0\x9F\x98\xAC"  // U+1F62C
#define D_EMOJI_FACE_LOUDLY_CRYING               "\xF0\x9F\x98\xAD"  // U+1F62D
#define D_EMOJI_FACE_OPEN_MOUTH                  "\xF0\x9F\x98\xAE"  // U+1F62E
#define D_EMOJI_FACE_HUSHED                      "\xF0\x9F\x98\xAF"  // U+1F62F
#define D_EMOJI_FACE_COLD_SWEAT                  "\xF0\x9F\x98\xB0"  // U+1F630
#define D_EMOJI_FACE_SCREAM                      "\xF0\x9F\x98\xB1"  // U+1F631
#define D_EMOJI_FACE_ASTONISHED                  "\xF0\x9F\x98\xB2"  // U+1F632
#define D_EMOJI_FACE_FLUSHED                     "\xF0\x9F\x98\xB3"  // U+1F633
#define D_EMOJI_FACE_SLEEPING                    "\xF0\x9F\x98\xB4"  // U+1F634
#define D_EMOJI_FACE_DIZZY_FACE                  "\xF0\x9F\x98\xB5"  // U+1F635
#define D_EMOJI_FACE_NO_MOUTH                    "\xF0\x9F\x98\xB6"  // U+1F636
#define D_EMOJI_FACE_MASK                        "\xF0\x9F\x98\xB7"  // U+1F637
// additional commonly-used faces:                              
//   U+1F641-U+1F644                                            
//   U+1F914,                                                   
//   U+1F917                                                    
//   U+1F928                                                    
//   U+1F970-U+1F971                                            
//   U+1F929                                                    
//   U+1F92A-U+1F92E                                            
#define D_EMOJI_FACE_SLIGHTLY_FROWNING           "\xF0\x9F\x99\x81"  // U+1F641
#define D_EMOJI_FACE_SLIGHTLY_SMILING            "\xF0\x9F\x99\x82"  // U+1F642
#define D_EMOJI_FACE_UPSIDE_DOWN                 "\xF0\x9F\x99\x83"  // U+1F643
#define D_EMOJI_FACE_ROLLING_EYES                "\xF0\x9F\x99\x84"  // U+1F644
#define D_EMOJI_FACE_THINKING                    "\xF0\x9F\xA4\x94"  // U+1F914
#define D_EMOJI_FACE_HUGGING                     "\xF0\x9F\x99\x97"  // U+1F917
#define D_EMOJI_FACE_RAISED_EYEBROW              "\xF0\x9F\xA4\xA8"  // U+1F928
#define D_EMOJI_FACE_STAR_STRUCK                 "\xF0\x9F\xA4\xA9"  // U+1F929
#define D_EMOJI_FACE_ZANY                        "\xF0\x9F\xA4\xAA"  // U+1F92A
#define D_EMOJI_FACE_SHUSHING                    "\xF0\x9F\xA4\xAB"  // U+1F92B
#define D_EMOJI_FACE_VOMIT                       "\xF0\x9F\xA4\xAE"  // U+1F92E
#define D_EMOJI_FACE_SMILING_WITH_HEARTS         "\xF0\x9F\xA5\xB0"  // U+1F970
#define D_EMOJI_FACE_YAWNING                     "\xF0\x9F\xA5\xB1"  // U+1F971
#define D_EMOJI_FACE_DISTORTED                   "\xF0\x9F\xAB\xAA"  // U+1FAEA

// ============================================================================
// PEOPLE & HUMAN EMOJIS  
// ============================================================================
// basic people
#define D_EMOJI_PEOPLE_BABY                      "\xF0\x9F\x91\xB6"  // U+1F476
#define D_EMOJI_PEOPLE_CHILD                     "\xF0\x9F\xA7\x92"  // U+1F9D2
#define D_EMOJI_PEOPLE_BOY                       "\xF0\x9F\x91\xA6"  // U+1F466
#define D_EMOJI_PEOPLE_GIRL                      "\xF0\x9F\x91\xA7"  // U+1F467
#define D_EMOJI_PEOPLE_PERSON                    "\xF0\x9F\xA7\x91"  // U+1F9D1
#define D_EMOJI_PEOPLE_MAN                       "\xF0\x9F\x91\xA8"  // U+1F468
#define D_EMOJI_PEOPLE_WOMAN                     "\xF0\x9F\x91\xA9"  // U+1F469
#define D_EMOJI_PEOPLE_OLDER_PERSON              "\xF0\x9F\xA7\x93"  // U+1F9D3
#define D_EMOJI_PEOPLE_OLD_MAN                   "\xF0\x9F\x91\xB4"  // U+1F474
#define D_EMOJI_PEOPLE_OLD_WOMAN                 "\xF0\x9F\x91\xB5"  // U+1F475
// hand gestures
#define D_EMOJI_HAND_WAVING                      "\xF0\x9F\x91\x8B"  // U+1F44B
#define D_EMOJI_HAND_RAISED_PALM                 "\xE2\x9C\x8B"      // U+270B
#define D_EMOJI_HAND_VULCAN_SALUTE               "\xF0\x9F\x96\x96"  // U+1F596
#define D_EMOJI_HAND_OK                          "\xF0\x9F\x91\x8C"  // U+1F44C
#define D_EMOJI_HAND_PEACE_VICTORY               "\xE2\x9C\x8C"      // U+270C
#define D_EMOJI_HAND_LOVE_YOU                    "\xF0\x9F\xA4\x9F"  // U+1F91F
#define D_EMOJI_HAND_HORNS                       "\xF0\x9F\xA4\x98"  // U+1F918
#define D_EMOJI_HAND_CALL_ME                     "\xF0\x9F\xA4\x99"  // U+1F919
#define D_EMOJI_HAND_POINT_LEFT                  "\xF0\x9F\x91\x88"  // U+1F448
#define D_EMOJI_HAND_POINT_RIGHT                 "\xF0\x9F\x91\x89"  // U+1F449
#define D_EMOJI_HAND_POINT_UP_2                  "\xF0\x9F\x91\x86"  // U+1F446
#define D_EMOJI_HAND_POINT_DOWN                  "\xF0\x9F\x91\x87"  // U+1F447
#define D_EMOJI_HAND_POINT_UP                    "\xE2\x98\x9D"      // U+261D
#define D_EMOJI_HAND_THUMBS_UP                   "\xF0\x9F\x91\x8D"  // U+1F44D
#define D_EMOJI_HAND_THUMBS_DOWN                 "\xF0\x9F\x91\x8E"  // U+1F44E
#define D_EMOJI_HAND_FIST_RAISED                 "\xE2\x9C\x8A"      // U+270A
#define D_EMOJI_HAND_FIST_ONCOMING               "\xF0\x9F\x91\x8A"  // U+1F44A
#define D_EMOJI_HAND_CLAP                        "\xF0\x9F\x91\x8F"  // U+1F44F
#define D_EMOJI_HAND_OPEN_HANDS                  "\xF0\x9F\x91\x90"  // U+1F450
#define D_EMOJI_HAND_PALMS_UP                    "\xF0\x9F\xA4\xB2"  // U+1F932
#define D_EMOJI_HANDSHAKE                        "\xF0\x9F\xA4\x9D"  // U+1F91D
#define D_EMOJI_HANDS_FOLDED                     "\xF0\x9F\x99\x8F"  // U+1F64F
// body parts
#define D_EMOJI_BODY_FLEXED_BICEP                "\xF0\x9F\x92\xAA"  // U+1F4AA
#define D_EMOJI_BODY_MECHANICAL_ARM              "\xF0\x9F\xA6\xBE"  // U+1F9BE
#define D_EMOJI_BODY_MECHANICAL_LEG              "\xF0\x9F\xA6\xBF"  // U+1F9BF
#define D_EMOJI_BODY_LEG                         "\xF0\x9F\xA6\xB5"  // U+1F9B5
#define D_EMOJI_BODY_FOOT                        "\xF0\x9F\xA6\xB6"  // U+1F9B6
#define D_EMOJI_BODY_EAR                         "\xF0\x9F\x91\x82"  // U+1F442
#define D_EMOJI_BODY_EAR_WITH_HEARING_AID        "\xF0\x9F\xA6\xBB"  // U+1F9BB
#define D_EMOJI_BODY_NOSE                        "\xF0\x9F\x91\x83"  // U+1F443
#define D_EMOJI_BODY_BRAIN                       "\xF0\x9F\xA7\xA0"  // U+1F9E0
#define D_EMOJI_BODY_ANATOMICAL_HEART            "\xF0\x9F\xAB\x80"  // U+1FAC0
#define D_EMOJI_BODY_LUNGS                       "\xF0\x9F\xAB\x81"  // U+1FAC1
#define D_EMOJI_BODY_TOOTH                       "\xF0\x9F\xA6\xB7"  // U+1F9B7
#define D_EMOJI_BODY_BONE                        "\xF0\x9F\xA6\xB4"  // U+1F9B4
#define D_EMOJI_BODY_EYES                        "\xF0\x9F\x91\x80"  // U+1F440
#define D_EMOJI_BODY_EYE                         "\xF0\x9F\x91\x81"  // U+1F441
#define D_EMOJI_BODY_TONGUE                      "\xF0\x9F\x91\x85"  // U+1F445
#define D_EMOJI_BODY_LIPS                        "\xF0\x9F\x91\x84"  // U+1F444
// gestures and poses  
#define D_EMOJI_GESTURE_PERSON_FACEPALM          "\xF0\x9F\xA4\xA6"  // U+1F926
#define D_EMOJI_GESTURE_PERSON_SHRUG             "\xF0\x9F\xA4\xB7"  // U+1F937
#define D_EMOJI_GESTURE_PERSON_GESTURING_NO      "\xF0\x9F\x99\x85"  // U+1F645
#define D_EMOJI_GESTURE_PERSON_GESTURING_OK      "\xF0\x9F\x99\x86"  // U+1F646
#define D_EMOJI_GESTURE_PERSON_TIPPING_HAND      "\xF0\x9F\x92\x81"  // U+1F481
#define D_EMOJI_GESTURE_PERSON_RAISING_HAND      "\xF0\x9F\x99\x8B"  // U+1F64B
#define D_EMOJI_GESTURE_DEAF_PERSON              "\xF0\x9F\xA7\x8F"  // U+1F9CF
#define D_EMOJI_GESTURE_PERSON_BOWING            "\xF0\x9F\x99\x87"  // U+1F647
// professions
#define D_EMOJI_PROFESSION_HEALTH_WORKER         "\xF0\x9F\xA7\x91\xE2\x80\x8D\xE2\x9A\x95\xEF\xB8\x8F"  // U+1F9D1 U+200D U+2695 U+FE0F
#define D_EMOJI_PROFESSION_DOCTOR                "\xF0\x9F\x91\xA8\xE2\x80\x8D\xE2\x9A\x95\xEF\xB8\x8F"  // U+1F468 U+200D U+2695 U+FE0F
#define D_EMOJI_PROFESSION_NURSE                 "\xF0\x9F\x91\xA9\xE2\x80\x8D\xE2\x9A\x95\xEF\xB8\x8F"  // U+1F469 U+200D U+2695 U+FE0F
#define D_EMOJI_PROFESSION_TEACHER               "\xF0\x9F\xA7\x91\xE2\x80\x8D\xF0\x9F\x8F\xAB"  // U+1F9D1 U+200D U+1F3EB
#define D_EMOJI_PROFESSION_JUDGE                 "\xF0\x9F\xA7\x91\xE2\x80\x8D\xE2\x9A\x96\xEF\xB8\x8F"  // U+1F9D1 U+200D U+2696 U+FE0F
#define D_EMOJI_PROFESSION_FARMER                "\xF0\x9F\xA7\x91\xE2\x80\x8D\xF0\x9F\x8C\xBE"  // U+1F9D1 U+200D U+1F33E
#define D_EMOJI_PROFESSION_COOK                  "\xF0\x9F\xA7\x91\xE2\x80\x8D\xF0\x9F\x8D\xB3"  // U+1F9D1 U+200D U+1F373
#define D_EMOJI_PROFESSION_MECHANIC              "\xF0\x9F\xA7\x91\xE2\x80\x8D\xF0\x9F\x94\xA7"  // U+1F9D1 U+200D U+1F527
#define D_EMOJI_PROFESSION_FACTORY_WORKER        "\xF0\x9F\xA7\x91\xE2\x80\x8D\xF0\x9F\x8F\xAD"  // U+1F9D1 U+200D U+1F3ED
#define D_EMOJI_PROFESSION_OFFICE_WORKER         "\xF0\x9F\xA7\x91\xE2\x80\x8D\xF0\x9F\x92\xBC"  // U+1F9D1 U+200D U+1F4BC
#define D_EMOJI_PROFESSION_SCIENTIST             "\xF0\x9F\xA7\x91\xE2\x80\x8D\xF0\x9F\x94\xAC"  // U+1F9D1 U+200D U+1F52C
#define D_EMOJI_PROFESSION_TECHNOLOGIST          "\xF0\x9F\xA7\x91\xE2\x80\x8D\xF0\x9F\x92\xBB"  // U+1F9D1 U+200D U+1F4BB
#define D_EMOJI_PROFESSION_SINGER                "\xF0\x9F\xA7\x91\xE2\x80\x8D\xF0\x9F\x8E\xA4"  // U+1F9D1 U+200D U+1F3A4
#define D_EMOJI_PROFESSION_ARTIST                "\xF0\x9F\xA7\x91\xE2\x80\x8D\xF0\x9F\x8E\xA8"  // U+1F9D1 U+200D U+1F3A8
#define D_EMOJI_PROFESSION_PILOT                 "\xF0\x9F\xA7\x91\xE2\x80\x8D\xE2\x9C\x88\xEF\xB8\x8F"  // U+1F9D1 U+200D U+2708 U+FE0F
#define D_EMOJI_PROFESSION_ASTRONAUT             "\xF0\x9F\xA7\x91\xE2\x80\x8D\xF0\x9F\x9A\x80"  // U+1F9D1 U+200D U+1F680
#define D_EMOJI_PROFESSION_FIREFIGHTER           "\xF0\x9F\xA7\x91\xE2\x80\x8D\xF0\x9F\x9A\x92"  // U+1F9D1 U+200D U+1F692
#define D_EMOJI_PROFESSION_STUDENT               "\xF0\x9F\xA7\x91\xE2\x80\x8D\xF0\x9F\x8E\x93"  // U+1F9D1 U+200D U+1F393
#define D_EMOJI_PROFESSION_POLICE_OFFICER        "\xF0\x9F\x91\xAE"  // U+1F46E
#define D_EMOJI_PROFESSION_DETECTIVE             "\xF0\x9F\x95\xB5"  // U+1F575
#define D_EMOJI_PROFESSION_GUARD                 "\xF0\x9F\x92\x82"  // U+1F482
#define D_EMOJI_PROFESSION_CONSTRUCTION_WORKER   "\xF0\x9F\x91\xB7"  // U+1F477
#define D_EMOJI_PROFESSION_PRINCE                "\xF0\x9F\xA4\xB4"  // U+1F934
#define D_EMOJI_PROFESSION_PRINCESS              "\xF0\x9F\x91\xB8"  // U+1F478
#define D_EMOJI_PROFESSION_PERSON_TURBAN         "\xF0\x9F\x91\xB3"  // U+1F473
#define D_EMOJI_PROFESSION_PERSON_SKULLCAP       "\xF0\x9F\x91\xB2"  // U+1F472
#define D_EMOJI_PROFESSION_BABY_ANGEL            "\xF0\x9F\x91\xBC"  // U+1F47C
// fantasy/roleplay
#define D_EMOJI_FANTASY_MAGE                     "\xF0\x9F\xA7\x99"  // U+1F9D9
#define D_EMOJI_FANTASY_FAIRY                    "\xF0\x9F\xA7\x9A"  // U+1F9DA
#define D_EMOJI_FANTASY_VAMPIRE                  "\xF0\x9F\xA7\x9B"  // U+1F9DB
#define D_EMOJI_FANTASY_MERPERSON                "\xF0\x9F\xA7\x9C"  // U+1F9DC
#define D_EMOJI_FANTASY_ELF                      "\xF0\x9F\xA7\x9D"  // U+1F9DD
#define D_EMOJI_FANTASY_GENIE                    "\xF0\x9F\xA7\x9E"  // U+1F9DE
#define D_EMOJI_FANTASY_ZOMBIE                   "\xF0\x9F\xA7\x9F"  // U+1F9DF
#define D_EMOJI_FANTASY_HAIRY_CREATURE           "\xF0\x9F\xAB\x88"  // U+1FAC8
// activities
#define D_EMOJI_ACTIVITY_PERSON_WALKING          "\xF0\x9F\x9A\xB6"  // U+1F6B6
#define D_EMOJI_ACTIVITY_PERSON_RUNNING          "\xF0\x9F\x8F\x83"  // U+1F3C3
#define D_EMOJI_ACTIVITY_PERSON_DANCING          "\xF0\x9F\x95\xBA"  // U+1F57A
#define D_EMOJI_ACTIVITY_WOMAN_DANCING           "\xF0\x9F\x92\x83"  // U+1F483
#define D_EMOJI_ACTIVITY_PEOPLE_DANCING          "\xF0\x9F\x91\xAF"  // U+1F46F
// family groups
// traditional families (man-woman + children)

// D_EMOJI_FAMILY_MAN_WOMAN_BOY
//   pre-defined constant: emoji code for family - man, woman, boy
// ZWJ sequence:
//    U+1F468 U+200D U+1F469 U+200D U+1F466
//    D_EMOJI_PEOPLE_MAN
//    
//    D_EMOJI_PEOPLE_WOMAN
//    D_EMOJI_PEOPLE_BOY
#define D_EMOJI_FAMILY_MAN_WOMAN_BOY             \
    "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA6"
#define D_EMOJI_FAMILY_MAN_WOMAN_GIRL            "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7"  // U+1F468 U+200D U+1F469 U+200D U+1F467
#define D_EMOJI_FAMILY_MAN_WOMAN_GIRL_BOY        "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7\xE2\x80\x8D\xF0\x9F\x91\xA6"  // U+1F468 U+200D U+1F469 U+200D U+1F467 U+200D U+1F466
#define D_EMOJI_FAMILY_MAN_WOMAN_BOY_BOY         "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA6\xE2\x80\x8D\xF0\x9F\x91\xA6"  // U+1F468 U+200D U+1F469 U+200D U+1F466 U+200D U+1F466
#define D_EMOJI_FAMILY_MAN_WOMAN_GIRL_GIRL       "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7\xE2\x80\x8D\xF0\x9F\x91\xA7"  // U+1F468 U+200D U+1F469 U+200D U+1F467 U+200D U+1F467
// same-sex families (man-man + children)
#define D_EMOJI_FAMILY_MAN_MAN_BOY               "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA6"  // U+1F468 U+200D U+1F468 U+200D U+1F466
#define D_EMOJI_FAMILY_MAN_MAN_GIRL              "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA7"  // U+1F468 U+200D U+1F468 U+200D U+1F467
#define D_EMOJI_FAMILY_MAN_MAN_GIRL_BOY          "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA7\xE2\x80\x8D\xF0\x9F\x91\xA6"  // U+1F468 U+200D U+1F468 U+200D U+1F467 U+200D U+1F466
#define D_EMOJI_FAMILY_MAN_MAN_BOY_BOY           "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA6\xE2\x80\x8D\xF0\x9F\x91\xA6"  // U+1F468 U+200D U+1F468 U+200D U+1F466 U+200D U+1F466
#define D_EMOJI_FAMILY_MAN_MAN_GIRL_GIRL         "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA7\xE2\x80\x8D\xF0\x9F\x91\xA7"  // U+1F468 U+200D U+1F468 U+200D U+1F467 U+200D U+1F467
// same-sex families (woman-woman + children)
#define D_EMOJI_FAMILY_WOMAN_WOMAN_BOY           "\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA6"  // U+1F469 U+200D U+1F469 U+200D U+1F466
#define D_EMOJI_FAMILY_WOMAN_WOMAN_GIRL          "\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7"  // U+1F469 U+200D U+1F469 U+200D U+1F467
#define D_EMOJI_FAMILY_WOMAN_WOMAN_GIRL_BOY      "\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7\xE2\x80\x8D\xF0\x9F\x91\xA6"  // U+1F469 U+200D U+1F469 U+200D U+1F467 U+200D U+1F466
#define D_EMOJI_FAMILY_WOMAN_WOMAN_BOY_BOY       "\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA6\xE2\x80\x8D\xF0\x9F\x91\xA6"  // U+1F469 U+200D U+1F469 U+200D U+1F466 U+200D U+1F466
#define D_EMOJI_FAMILY_WOMAN_WOMAN_GIRL_GIRL     "\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7\xE2\x80\x8D\xF0\x9F\x91\xA7"  // U+1F469 U+200D U+1F469 U+200D U+1F467 U+200D U+1F467
// single parent families
#define D_EMOJI_FAMILY_MAN_BOY                   "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA6"  // U+1F468 U+200D U+1F466
#define D_EMOJI_FAMILY_MAN_GIRL                  "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA7"  // U+1F468 U+200D U+1F467
#define D_EMOJI_FAMILY_MAN_GIRL_BOY              "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA7\xE2\x80\x8D\xF0\x9F\x91\xA6"  // U+1F468 U+200D U+1F467 U+200D U+1F466
#define D_EMOJI_FAMILY_MAN_BOY_BOY               "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA6\xE2\x80\x8D\xF0\x9F\x91\xA6"  // U+1F468 U+200D U+1F466 U+200D U+1F466
#define D_EMOJI_FAMILY_MAN_GIRL_GIRL             "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA7\xE2\x80\x8D\xF0\x9F\x91\xA7"  // U+1F468 U+200D U+1F467 U+200D U+1F467
#define D_EMOJI_FAMILY_WOMAN_BOY                 "\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA6"  // U+1F469 U+200D U+1F466
#define D_EMOJI_FAMILY_WOMAN_GIRL                "\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7"  // U+1F469 U+200D U+1F467
#define D_EMOJI_FAMILY_WOMAN_GIRL_BOY            "\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7\xE2\x80\x8D\xF0\x9F\x91\xA6"  // U+1F469 U+200D U+1F467 U+200D U+1F466
#define D_EMOJI_FAMILY_WOMAN_BOY_BOY             "\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA6\xE2\x80\x8D\xF0\x9F\x91\xA6"  // U+1F469 U+200D U+1F466 U+200D U+1F466
#define D_EMOJI_FAMILY_WOMAN_GIRL_GIRL           "\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7\xE2\x80\x8D\xF0\x9F\x91\xA7"  // U+1F469 U+200D U+1F467 U+200D U+1F467
// couples
#define D_EMOJI_COUPLE_KISS                      "\xF0\x9F\x92\x8F"  // U+1F48F
#define D_EMOJI_COUPLE_WITH_HEART                "\xF0\x9F\x92\x91"  // U+1F491

// ============================================================================
// ANIMAL EMOJIS
// ============================================================================
// mammals (land)
#define D_EMOJI_ANIMAL_MONKEY_FACE               "\xF0\x9F\x90\xB5"  // U+1F435
#define D_EMOJI_ANIMAL_MONKEY                    "\xF0\x9F\x90\x92"  // U+1F412
#define D_EMOJI_ANIMAL_GORILLA                   "\xF0\x9F\xA6\x8D"  // U+1F98D
#define D_EMOJI_ANIMAL_ORANGUTAN                 "\xF0\x9F\xA6\xA7"  // U+1F9A7
#define D_EMOJI_ANIMAL_DOG_FACE                  "\xF0\x9F\x90\xB6"  // U+1F436
#define D_EMOJI_ANIMAL_DOG                       "\xF0\x9F\x90\x95"  // U+1F415
#define D_EMOJI_ANIMAL_GUIDE_DOG                 "\xF0\x9F\xA6\xAE"  // U+1F9AE
#define D_EMOJI_ANIMAL_SERVICE_DOG               "\xF0\x9F\x90\x95\xE2\x80\x8D\xF0\x9F\xA6\xBA"  // U+1F415 U+200D U+1F9BA
#define D_EMOJI_ANIMAL_POODLE                    "\xF0\x9F\x90\xA9"  // U+1F429
#define D_EMOJI_ANIMAL_WOLF                      "\xF0\x9F\x90\xBA"  // U+1F43A
#define D_EMOJI_ANIMAL_FOX_FACE                  "\xF0\x9F\xA6\x8A"  // U+1F98A
#define D_EMOJI_ANIMAL_RACCOON                   "\xF0\x9F\xA6\x9D"  // U+1F99D
#define D_EMOJI_ANIMAL_CAT_FACE                  "\xF0\x9F\x90\xB1"  // U+1F431
#define D_EMOJI_ANIMAL_CAT                       "\xF0\x9F\x90\x88"  // U+1F408
#define D_EMOJI_ANIMAL_BLACK_CAT                 "\xF0\x9F\x90\x88\xE2\x80\x8D\xE2\xAC\x9B"  // U+1F408 U+200D U+2B1B
#define D_EMOJI_ANIMAL_LION                      "\xF0\x9F\xA6\x81"  // U+1F981
#define D_EMOJI_ANIMAL_TIGER_FACE                "\xF0\x9F\x90\xAF"  // U+1F42F
#define D_EMOJI_ANIMAL_TIGER                     "\xF0\x9F\x90\x85"  // U+1F405
#define D_EMOJI_ANIMAL_LEOPARD                   "\xF0\x9F\x90\x86"  // U+1F406
#define D_EMOJI_ANIMAL_HORSE_FACE                "\xF0\x9F\x90\xB4"  // U+1F434
#define D_EMOJI_ANIMAL_HORSE                     "\xF0\x9F\x90\x8E"  // U+1F40E
#define D_EMOJI_ANIMAL_UNICORN                   "\xF0\x9F\xA6\x84"  // U+1F984
#define D_EMOJI_ANIMAL_ZEBRA                     "\xF0\x9F\xA6\x93"  // U+1F993
#define D_EMOJI_ANIMAL_DEER                      "\xF0\x9F\xA6\x8C"  // U+1F98C
#define D_EMOJI_ANIMAL_BISON                     "\xF0\x9F\xA6\xAC"  // U+1F9AC
#define D_EMOJI_ANIMAL_COW_FACE                  "\xF0\x9F\x90\xAE"  // U+1F42E
#define D_EMOJI_ANIMAL_OX                        "\xF0\x9F\x90\x82"  // U+1F402
#define D_EMOJI_ANIMAL_WATER_BUFFALO             "\xF0\x9F\x90\x83"  // U+1F403
#define D_EMOJI_ANIMAL_COW                       "\xF0\x9F\x90\x84"  // U+1F404
#define D_EMOJI_ANIMAL_PIG_FACE                  "\xF0\x9F\x90\xB7"  // U+1F437
#define D_EMOJI_ANIMAL_PIG                       "\xF0\x9F\x90\x96"  // U+1F416
#define D_EMOJI_ANIMAL_BOAR                      "\xF0\x9F\x90\x97"  // U+1F417
#define D_EMOJI_ANIMAL_PIG_NOSE                  "\xF0\x9F\x90\xBD"  // U+1F43D
#define D_EMOJI_ANIMAL_RAM                       "\xF0\x9F\x90\x8F"  // U+1F40F
#define D_EMOJI_ANIMAL_EWE                       "\xF0\x9F\x90\x91"  // U+1F411
#define D_EMOJI_ANIMAL_GOAT                      "\xF0\x9F\x90\x90"  // U+1F410
#define D_EMOJI_ANIMAL_CAMEL                     "\xF0\x9F\x90\xAA"  // U+1F42A
#define D_EMOJI_ANIMAL_TWO_HUMP_CAMEL            "\xF0\x9F\x90\xAB"  // U+1F42B
#define D_EMOJI_ANIMAL_LLAMA                     "\xF0\x9F\xA6\x99"  // U+1F999
#define D_EMOJI_ANIMAL_GIRAFFE                   "\xF0\x9F\xA6\x92"  // U+1F992
#define D_EMOJI_ANIMAL_ELEPHANT                  "\xF0\x9F\x90\x98"  // U+1F418
#define D_EMOJI_ANIMAL_MAMMOTH                   "\xF0\x9F\xA6\xA3"  // U+1F9A3
#define D_EMOJI_ANIMAL_RHINOCEROS                "\xF0\x9F\xA6\x8F"  // U+1F98F
#define D_EMOJI_ANIMAL_HIPPOPOTAMUS              "\xF0\x9F\xA6\x9B"  // U+1F99B
#define D_EMOJI_ANIMAL_MOUSE_FACE                "\xF0\x9F\x90\xAD"  // U+1F42D
#define D_EMOJI_ANIMAL_MOUSE                     "\xF0\x9F\x90\x81"  // U+1F401
#define D_EMOJI_ANIMAL_RAT                       "\xF0\x9F\x90\x80"  // U+1F400
#define D_EMOJI_ANIMAL_HAMSTER                   "\xF0\x9F\x90\xB9"  // U+1F439
#define D_EMOJI_ANIMAL_RABBIT_FACE               "\xF0\x9F\x90\xB0"  // U+1F430
#define D_EMOJI_ANIMAL_RABBIT                    "\xF0\x9F\x90\x87"  // U+1F407
#define D_EMOJI_ANIMAL_CHIPMUNK                  "\xF0\x9F\x90\xBF"  // U+1F43F
#define D_EMOJI_ANIMAL_BEAVER                    "\xF0\x9F\xA6\xAB"  // U+1F9AB
#define D_EMOJI_ANIMAL_HEDGEHOG                  "\xF0\x9F\xA6\x94"  // U+1F994
#define D_EMOJI_ANIMAL_BAT                       "\xF0\x9F\xA6\x87"  // U+1F987
#define D_EMOJI_ANIMAL_BEAR                      "\xF0\x9F\x90\xBB"  // U+1F43B
#define D_EMOJI_ANIMAL_POLAR_BEAR                "\xF0\x9F\x90\xBB\xE2\x80\x8D\xE2\x9D\x84\xEF\xB8\x8F"  // U+1F43B U+200D U+2744 U+FE0F
#define D_EMOJI_ANIMAL_KOALA                     "\xF0\x9F\x90\xA8"  // U+1F428
#define D_EMOJI_ANIMAL_PANDA                     "\xF0\x9F\x90\xBC"  // U+1F43C
#define D_EMOJI_ANIMAL_SLOTH                     "\xF0\x9F\xA6\xA5"  // U+1F9A5
#define D_EMOJI_ANIMAL_OTTER                     "\xF0\x9F\xA6\xA6"  // U+1F9A6
#define D_EMOJI_ANIMAL_SKUNK                     "\xF0\x9F\xA6\xA8"  // U+1F9A8
#define D_EMOJI_ANIMAL_KANGAROO                  "\xF0\x9F\xA6\x98"  // U+1F998
#define D_EMOJI_ANIMAL_BADGER                    "\xF0\x9F\xA6\xA1"  // U+1F9A1
// mammals (primates)                            
#define D_EMOJI_MONKEY_SEE_NO_EVIL               "\xF0\x9F\x99\x88"  // U+1F648
#define D_EMOJI_MONKEY_HEAR_NO_EVIL              "\xF0\x9F\x99\x89"  // U+1F649
#define D_EMOJI_MONKEY_SPEAK_NO_EVIL             "\xF0\x9F\x99\x8A"  // U+1F64A
// aquatic animals                               
#define D_EMOJI_ANIMAL_WHALE                     "\xF0\x9F\x90\xB3"  // U+1F433
#define D_EMOJI_ANIMAL_SPERM_WHALE               "\xF0\x9F\x90\x8B"  // U+1F40B
#define D_EMOJI_ANIMAL_DOLPHIN                   "\xF0\x9F\x90\xAC"  // U+1F42C
#define D_EMOJI_ANIMAL_ORCA                      "\xF0\x9F\xAB\x8D"  // U+1FACD
#define D_EMOJI_ANIMAL_SEAL                      "\xF0\x9F\xA6\xAD"  // U+1F9AD
#define D_EMOJI_ANIMAL_FISH                      "\xF0\x9F\x90\x9F"  // U+1F41F
#define D_EMOJI_ANIMAL_TROPICAL_FISH             "\xF0\x9F\x90\xA0"  // U+1F420
#define D_EMOJI_ANIMAL_BLOWFISH                  "\xF0\x9F\x90\xA1"  // U+1F421
#define D_EMOJI_ANIMAL_SHARK                     "\xF0\x9F\xA6\x88"  // U+1F988
#define D_EMOJI_ANIMAL_OCTOPUS                   "\xF0\x9F\x90\x99"  // U+1F419
#define D_EMOJI_ANIMAL_SPIRAL_SHELL              "\xF0\x9F\x90\x9A"  // U+1F41A
#define D_EMOJI_ANIMAL_SNAIL                     "\xF0\x9F\x90\x8C"  // U+1F40C
#define D_EMOJI_ANIMAL_BUTTERFLY                 "\xF0\x9F\xA6\x8B"  // U+1F98B
#define D_EMOJI_ANIMAL_BUG                       "\xF0\x9F\x90\x9B"  // U+1F41B
#define D_EMOJI_ANIMAL_ANT                       "\xF0\x9F\x90\x9C"  // U+1F41C
#define D_EMOJI_ANIMAL_HONEYBEE                  "\xF0\x9F\x90\x9D"  // U+1F41D
#define D_EMOJI_ANIMAL_BEETLE                    "\xF0\x9F\x90\x9E"  // U+1F41E
#define D_EMOJI_ANIMAL_CRICKET                   "\xF0\x9F\xA6\x97"  // U+1F997
#define D_EMOJI_ANIMAL_COCKROACH                 "\xF0\x9F\xAA\xB3"  // U+1FAB3
#define D_EMOJI_ANIMAL_SPIDER                    "\xF0\x9F\x95\xB7"  // U+1F577
#define D_EMOJI_ANIMAL_SPIDER_WEB                "\xF0\x9F\x95\xB8"  // U+1F578
#define D_EMOJI_ANIMAL_SCORPION                  "\xF0\x9F\xA6\x82"  // U+1F982
#define D_EMOJI_ANIMAL_MOSQUITO                  "\xF0\x9F\xA6\x9F"  // U+1F99F
#define D_EMOJI_ANIMAL_FLY                       "\xF0\x9F\xAA\xB0"  // U+1FAB0
#define D_EMOJI_ANIMAL_WORM                      "\xF0\x9F\xAA\xB1"  // U+1FAB1
#define D_EMOJI_ANIMAL_MICROBE                   "\xF0\x9F\xA6\xA0"  // U+1F9A0
// marine life                                   
#define D_EMOJI_ANIMAL_SHRIMP                    "\xF0\x9F\xA6\x90"  // U+1F990
#define D_EMOJI_ANIMAL_SQUID                     "\xF0\x9F\xA6\x91"  // U+1F991
#define D_EMOJI_ANIMAL_OYSTER                    "\xF0\x9F\xA6\xAA"  // U+1F9AA
#define D_EMOJI_ANIMAL_CRAB                      "\xF0\x9F\xA6\x80"  // U+1F980
#define D_EMOJI_ANIMAL_LOBSTER                   "\xF0\x9F\xA6\x9E"  // U+1F99E
#define D_EMOJI_ANIMAL_JELLYFISH                 "\xF0\x9F\xAA\xBC"  // U+1FABC
// birds                                         
#define D_EMOJI_ANIMAL_BIRD                      "\xF0\x9F\x90\xA6"  // U+1F426
#define D_EMOJI_ANIMAL_BLACK_BIRD                "\xF0\x9F\x90\xA6\xE2\x80\x8D\xE2\xAC\x9B"  // U+1F426 U+200D U+2B1B
#define D_EMOJI_ANIMAL_PENGUIN                   "\xF0\x9F\x90\xA7"  // U+1F427
#define D_EMOJI_ANIMAL_DOVE                      "\xF0\x9F\x95\x8A"  // U+1F54A
#define D_EMOJI_ANIMAL_EAGLE                     "\xF0\x9F\xA6\x85"  // U+1F985
#define D_EMOJI_ANIMAL_DUCK                      "\xF0\x9F\xA6\x86"  // U+1F986
#define D_EMOJI_ANIMAL_SWAN                      "\xF0\x9F\xA6\xA2"  // U+1F9A2
#define D_EMOJI_ANIMAL_OWL                       "\xF0\x9F\xA6\x89"  // U+1F989
#define D_EMOJI_ANIMAL_DODO                      "\xF0\x9F\xA6\xA4"  // U+1F9A4
#define D_EMOJI_ANIMAL_FEATHER                   "\xF0\x9F\xAA\xB6"  // U+1FAB6
#define D_EMOJI_ANIMAL_FLAMINGO                  "\xF0\x9F\xA6\xA9"  // U+1F9A9
#define D_EMOJI_ANIMAL_PEACOCK                   "\xF0\x9F\xA6\x9A"  // U+1F99A
#define D_EMOJI_ANIMAL_PARROT                    "\xF0\x9F\xA6\x9C"  // U+1F99C
#define D_EMOJI_ANIMAL_TURKEY                    "\xF0\x9F\xA6\x83"  // U+1F983
#define D_EMOJI_ANIMAL_CHICKEN                   "\xF0\x9F\x90\x94"  // U+1F414
#define D_EMOJI_ANIMAL_ROOSTER                   "\xF0\x9F\x90\x93"  // U+1F413
#define D_EMOJI_ANIMAL_HATCHING_CHICK            "\xF0\x9F\x90\xA3"  // U+1F423
#define D_EMOJI_ANIMAL_BABY_CHICK                "\xF0\x9F\x90\xA4"  // U+1F424
#define D_EMOJI_ANIMAL_FRONT_FACING_CHICK        "\xF0\x9F\x90\xA5"  // U+1F425
// reptiles/amphibians                           
#define D_EMOJI_ANIMAL_TURTLE                    "\xF0\x9F\x90\xA2"  // U+1F422
#define D_EMOJI_ANIMAL_LIZARD                    "\xF0\x9F\xA6\x8E"  // U+1F98E
#define D_EMOJI_ANIMAL_SNAKE                     "\xF0\x9F\x90\x8D"  // U+1F40D
#define D_EMOJI_ANIMAL_DRAGON_FACE               "\xF0\x9F\x90\xB2"  // U+1F432
#define D_EMOJI_ANIMAL_DRAGON                    "\xF0\x9F\x89"      // U+1F409
#define D_EMOJI_ANIMAL_SAUROPOD                  "\xF0\x9F\xA6\x95"  // U+1F995
#define D_EMOJI_ANIMAL_T_REX                     "\xF0\x9F\xA6\x96"  // U+1F996
#define D_EMOJI_ANIMAL_FROG                      "\xF0\x9F\x90\xB8"  // U+1F438
#define D_EMOJI_ANIMAL_CROCODILE                 "\xF0\x9F\x90\x8A"  // U+1F40A
// mythical/extinct                              
#define D_EMOJI_ANIMAL_DRAGON_MYTHICAL           "\xF0\x9F\x89"      // U+1F409 (same as dragon, but in mythical context)
// paw prints                                    
#define D_EMOJI_ANIMAL_PAW_PRINTS                "\xF0\x9F\x90\xBE"  // U+1F43E

// ============================================================================
// NATURE EMOJIS
// ============================================================================
// weather/sky
#define D_EMOJI_NATURE_SUN                       "\xE2\x98\x80"      // U+2600 
#define D_EMOJI_NATURE_CLOUD                     "\xE2\x98\x81"      // U+2601 
#define D_EMOJI_NATURE_UMBRELLA                  "\xE2\x98\x82"      // U+2602 
#define D_EMOJI_NATURE_SNOWMAN                   "\xE2\x98\x83"      // U+2603 
#define D_EMOJI_NATURE_COMET                     "\xE2\x98\x84"      // U+2604 
#define D_EMOJI_NATURE_UMBRELLA_RAIN             "\xE2\x98\x94"      // U+2614 
#define D_EMOJI_NATURE_HIGH_VOLTAGE              "\xE2\x9A\xA1"      // U+26A1 
#define D_EMOJI_NATURE_SNOWFLAKE                 "\xE2\x9D\x84"      // U+2744 
#define D_EMOJI_NATURE_SNOWMAN_NO_SNOW           "\xE2\x9B\x84"      // U+26C4 
#define D_EMOJI_NATURE_SUN_BEHIND_CLOUD          "\xE2\x9B\x85"      // U+26C5 
#define D_EMOJI_NATURE_THUNDER_CLOUD_RAIN        "\xE2\x9B\x88"      // U+26C8 
#define D_EMOJI_NATURE_SUN_WITH_FACE             "\xF0\x9F\x8C\x9E"  // U+1F31E
#define D_EMOJI_NATURE_CRESCENT_MOON             "\xF0\x9F\x8C\x99"  // U+1F319
#define D_EMOJI_NATURE_FIRST_QUARTER_MOON        "\xF0\x9F\x8C\x9B"  // U+1F31B
#define D_EMOJI_NATURE_LAST_QUARTER_MOON         "\xF0\x9F\x8C\x9C"  // U+1F31C
#define D_EMOJI_NATURE_NEW_MOON_FACE             "\xF0\x9F\x8C\x9A"  // U+1F31A
#define D_EMOJI_NATURE_FULL_MOON                 "\xF0\x9F\x8C\x95"  // U+1F315
#define D_EMOJI_NATURE_WANING_GIBBOUS_MOON       "\xF0\x9F\x8C\x96"  // U+1F316
#define D_EMOJI_NATURE_LAST_QUARTER_MOON2        "\xF0\x9F\x8C\x97"  // U+1F317
#define D_EMOJI_NATURE_WANING_CRESCENT_MOON      "\xF0\x9F\x8C\x98"  // U+1F318
#define D_EMOJI_NATURE_NEW_MOON                  "\xF0\x9F\x8C\x91"  // U+1F311
#define D_EMOJI_NATURE_WAXING_CRESCENT_MOON      "\xF0\x9F\x8C\x92"  // U+1F312
#define D_EMOJI_NATURE_FIRST_QUARTER_MOON2       "\xF0\x9F\x8C\x93"  // U+1F313
#define D_EMOJI_NATURE_WAXING_GIBBOUS_MOON       "\xF0\x9F\x8C\x94"  // U+1F314
#define D_EMOJI_NATURE_STAR                      "\xE2\xAD\x90"      // U+2B50 
#define D_EMOJI_NATURE_GLOWING_STAR              "\xF0\x9F\x8C\x9F"  // U+1F31F
#define D_EMOJI_NATURE_DIZZY                     "\xF0\x9F\x92\xAB"  // U+1F4AB
#define D_EMOJI_NATURE_SPARKLES                  "\xE2\x9C\xA8"      // U+2728 
#define D_EMOJI_NATURE_SHOOTING_STAR             "\xF0\x9F\x8C\xA0"  // U+1F320
#define D_EMOJI_NATURE_MILKY_WAY                 "\xF0\x9F\x8C\x8C"  // U+1F30C
#define D_EMOJI_NATURE_PARTLY_SUNNY              "\xF0\x9F\x8C\xA4"  // U+1F324
#define D_EMOJI_NATURE_SUN_SMALL_CLOUD           "\xF0\x9F\x8C\xA5"  // U+1F325
#define D_EMOJI_NATURE_SUN_BEHIND_RAIN_CLOUD     "\xF0\x9F\x8C\xA6"  // U+1F326
#define D_EMOJI_NATURE_CLOUD_WITH_RAIN           "\xF0\x9F\x8C\xA7"  // U+1F327
#define D_EMOJI_NATURE_CLOUD_WITH_SNOW           "\xF0\x9F\x8C\xA8"  // U+1F328
#define D_EMOJI_NATURE_CLOUD_WITH_LIGHTNING      "\xF0\x9F\x8C\xA9"  // U+1F329
#define D_EMOJI_NATURE_TORNADO                   "\xF0\x9F\x8C\xAA"  // U+1F32A
#define D_EMOJI_NATURE_FOG                       "\xF0\x9F\x8C\xAB"  // U+1F32B
#define D_EMOJI_NATURE_WIND_FACE                 "\xF0\x9F\x8C\xAC"  // U+1F32C
#define D_EMOJI_NATURE_CYCLONE                   "\xF0\x9F\x8C\x80"  // U+1F300
#define D_EMOJI_NATURE_RAINBOW                   "\xF0\x9F\x8C\x88"  // U+1F308
#define D_EMOJI_NATURE_CLOSED_UMBRELLA           "\xF0\x9F\x8C\x82"  // U+1F302
#define D_EMOJI_NATURE_DROPLET                   "\xF0\x9F\x92\xA7"  // U+1F4A7
#define D_EMOJI_NATURE_SWEAT_DROPS               "\xF0\x9F\x92\xA6"  // U+1F4A6
// fire/energy                                   
#define D_EMOJI_NATURE_FIRE                      "\xF0\x9F\x94\xA5"  // U+1F525
#define D_EMOJI_NATURE_COLLISION                 "\xF0\x9F\x92\xA5"  // U+1F4A5
#define D_EMOJI_NATURE_LANDSLIDE                 "\xF0\x9F\x9B\x98"  // U+1F6D8
// plants/trees                                  
#define D_EMOJI_NATURE_EVERGREEN_TREE            "\xF0\x9F\x8C\xB2"  // U+1F332
#define D_EMOJI_NATURE_DECIDUOUS_TREE            "\xF0\x9F\x8C\xB3"  // U+1F333
#define D_EMOJI_NATURE_PALM_TREE                 "\xF0\x9F\x8C\xB4"  // U+1F334
#define D_EMOJI_NATURE_CACTUS                    "\xF0\x9F\x8C\xB5"  // U+1F335
#define D_EMOJI_NATURE_HERB                      "\xF0\x9F\x8C\xBF"  // U+1F33F
#define D_EMOJI_NATURE_SHAMROCK                  "\xE2\x98\x98"      // U+2618 
#define D_EMOJI_NATURE_FOUR_LEAF_CLOVER          "\xF0\x9F\x8D\x80"  // U+1F340
#define D_EMOJI_NATURE_MAPLE_LEAF                "\xF0\x9F\x8D\x81"  // U+1F341
#define D_EMOJI_NATURE_FALLEN_LEAVES             "\xF0\x9F\x8D\x82"  // U+1F342
#define D_EMOJI_NATURE_LEAF_FLUTTERING           "\xF0\x9F\x8D\x83"  // U+1F343
#define D_EMOJI_NATURE_MUSHROOM                  "\xF0\x9F\x8D\x84"  // U+1F344
#define D_EMOJI_NATURE_EAR_OF_RICE               "\xF0\x9F\x8C\xBE"  // U+1F33E
#define D_EMOJI_NATURE_SHEAF_OF_RICE             "\xF0\x9F\x8C\xBE"  // U+1F33E
// flowers                                       
#define D_EMOJI_NATURE_CHERRY_BLOSSOM            "\xF0\x9F\x8C\xB8"  // U+1F338
#define D_EMOJI_NATURE_ROSE                      "\xF0\x9F\x8C\xB9"  // U+1F339
#define D_EMOJI_NATURE_HIBISCUS                  "\xF0\x9F\x8C\xBA"  // U+1F33A
#define D_EMOJI_NATURE_SUNFLOWER                 "\xF0\x9F\x8C\xBB"  // U+1F33B
#define D_EMOJI_NATURE_BLOSSOM                   "\xF0\x9F\x8C\xBC"  // U+1F33C
#define D_EMOJI_NATURE_TULIP                     "\xF0\x9F\x8C\xB7"  // U+1F337
#define D_EMOJI_NATURE_BOUQUET                   "\xF0\x9F\x92\x90"  // U+1F490
#define D_EMOJI_NATURE_WILTED_FLOWER             "\xF0\x9F\xA5\x80"  // U+1F940
#define D_EMOJI_NATURE_WHITE_FLOWER              "\xF0\x9F\x92\xAE"  // U+1F4AE
#define D_EMOJI_NATURE_LOTUS_FLOWER              "\xF0\x9F\xAA\xB7"  // U+1FAB7
// landscapes                                    
#define D_EMOJI_NATURE_MOUNTAIN                  "\xE2\x9B\xB0"      // U+26F0 
#define D_EMOJI_NATURE_SNOW_CAPPED_MOUNTAIN      "\xF0\x9F\x8F\x94"  // U+1F3D4
#define D_EMOJI_NATURE_VOLCANO                   "\xF0\x9F\x8C\x8B"  // U+1F30B
#define D_EMOJI_NATURE_MOUNT_FUJI                "\xF0\x9F\x97\xBB"  // U+1F5FB
#define D_EMOJI_NATURE_CAMPING                   "\xF0\x9F\x8F\x95"  // U+1F3D5
#define D_EMOJI_NATURE_BEACH_UMBRELLA            "\xF0\x9F\x8F\x96"  // U+1F3D6
#define D_EMOJI_NATURE_DESERT                    "\xF0\x9F\x8F\x9C"  // U+1F3DC
#define D_EMOJI_NATURE_DESERT_ISLAND             "\xF0\x9F\x8F\x9D"  // U+1F3DD
#define D_EMOJI_NATURE_NATIONAL_PARK             "\xF0\x9F\x8F\x9E"  // U+1F3DE
// water features
#define D_EMOJI_NATURE_WATER_WAVE                "\xF0\x9F\x8C\x8A"  // U+1F30A
#define D_EMOJI_NATURE_OCEAN                     "\xF0\x9F\x8C\x8A"  // U+1F30A

// ============================================================================
// GLOBES
// ============================================================================
// globes
#define D_EMOJI_GLOBE_EUROPE_AFRICA              "\xF0\x9F\x8C\x8D"  // U+1F30D
#define D_EMOJI_GLOBE_AMERICAS                   "\xF0\x9F\x8C\x8E"  // U+1F30E
#define D_EMOJI_GLOBE_ASIA_AUSTRALIA             "\xF0\x9F\x8C\x8F"  // U+1F30F
#define D_EMOJI_GLOBE_WITH_MERIDIANS             "\xF0\x9F\x8C\x90"  // U+1F310

// ============================================================================
// FOOD & DRINKS
// ============================================================================
// fruits
#define D_EMOJI_FOOD_GRAPES                      "\xF0\x9F\x8D\x87"  // U+1F347
#define D_EMOJI_FOOD_MELON                       "\xF0\x9F\x8D\x88"  // U+1F348
#define D_EMOJI_FOOD_WATERMELON                  "\xF0\x9F\x8D\x89"  // U+1F349
#define D_EMOJI_FOOD_ORANGE                      "\xF0\x9F\x8D\x8A"  // U+1F34A
#define D_EMOJI_FOOD_LEMON                       "\xF0\x9F\x8D\x8B"  // U+1F34B
#define D_EMOJI_FOOD_BANANA                      "\xF0\x9F\x8D\x8C"  // U+1F34C
#define D_EMOJI_FOOD_PINEAPPLE                   "\xF0\x9F\x8D\x8D"  // U+1F34D
#define D_EMOJI_FOOD_MANGO                       "\xF0\x9F\xA5\xAD"  // U+1F96D
#define D_EMOJI_FOOD_RED_APPLE                   "\xF0\x9F\x8D\x8E"  // U+1F34E
#define D_EMOJI_FOOD_GREEN_APPLE                 "\xF0\x9F\x8D\x8F"  // U+1F34F
#define D_EMOJI_FOOD_PEAR                        "\xF0\x9F\x8D\x90"  // U+1F350
#define D_EMOJI_FOOD_PEACH                       "\xF0\x9F\x8D\x91"  // U+1F351
#define D_EMOJI_FOOD_CHERRIES                    "\xF0\x9F\x8D\x92"  // U+1F352
#define D_EMOJI_FOOD_STRAWBERRY                  "\xF0\x9F\x8D\x93"  // U+1F353
#define D_EMOJI_FOOD_BLUEBERRIES                 "\xF0\x9F\xAB\x90"  // U+1FAD0
#define D_EMOJI_FOOD_KIWI_FRUIT                  "\xF0\x9F\xA5\x9D"  // U+1F95D
#define D_EMOJI_FOOD_AVOCADO                     "\xF0\x9F\xA5\x91"  // U+1F951
#define D_EMOJI_FOOD_COCONUT                     "\xF0\x9F\xA5\xA5"  // U+1F965
// vegetables                                    
#define D_EMOJI_FOOD_TOMATO                      "\xF0\x9F\x8D\x85"  // U+1F345
#define D_EMOJI_FOOD_EGGPLANT                    "\xF0\x9F\x8D\x86"  // U+1F346
#define D_EMOJI_FOOD_CARROT                      "\xF0\x9F\xA5\x95"  // U+1F955
#define D_EMOJI_FOOD_CORN                        "\xF0\x9F\x8C\xBD"  // U+1F33D
#define D_EMOJI_FOOD_HOT_PEPPER                  "\xF0\x9F\x8C\xB6"  // U+1F336
#define D_EMOJI_FOOD_BELL_PEPPER                 "\xF0\x9F\xAB\x91"  // U+1FAD1
#define D_EMOJI_FOOD_CUCUMBER                    "\xF0\x9F\xA5\x92"  // U+1F952
#define D_EMOJI_FOOD_LEAFY_GREENS                "\xF0\x9F\xA5\xAC"  // U+1F96C
#define D_EMOJI_FOOD_BROCCOLI                    "\xF0\x9F\xA5\xA6"  // U+1F966
#define D_EMOJI_FOOD_GARLIC                      "\xF0\x9F\xA7\x84"  // U+1F9C4
#define D_EMOJI_FOOD_ONION                       "\xF0\x9F\xA7\x85"  // U+1F9C5
#define D_EMOJI_FOOD_POTATO                      "\xF0\x9F\xA5\x94"  // U+1F954
#define D_EMOJI_FOOD_SWEET_POTATO                "\xF0\x9F\x8D\xA0"  // U+1F360
// prepared foods                                
#define D_EMOJI_FOOD_BREAD                       "\xF0\x9F\x8D\x9E"  // U+1F35E
#define D_EMOJI_FOOD_CROISSANT                   "\xF0\x9F\xA5\x90"  // U+1F950
#define D_EMOJI_FOOD_BAGUETTE_BREAD              "\xF0\x9F\xA5\x96"  // U+1F956
#define D_EMOJI_FOOD_FLATBREAD                   "\xF0\x9F\xAB\x93"  // U+1FAD3
#define D_EMOJI_FOOD_PRETZEL                     "\xF0\x9F\xA5\xA8"  // U+1F968
#define D_EMOJI_FOOD_BAGEL                       "\xF0\x9F\xA5\xAF"  // U+1F96F
#define D_EMOJI_FOOD_PANCAKES                    "\xF0\x9F\xA5\x9E"  // U+1F95E
#define D_EMOJI_FOOD_WAFFLE                      "\xF0\x9F\xA7\x87"  // U+1F9C7
#define D_EMOJI_FOOD_CHEESE_WEDGE                "\xF0\x9F\xA7\x80"  // U+1F9C0
#define D_EMOJI_FOOD_MEAT_ON_BONE                "\xF0\x9F\x8D\x96"  // U+1F356
#define D_EMOJI_FOOD_POULTRY_LEG                 "\xF0\x9F\x8D\x97"  // U+1F357
#define D_EMOJI_FOOD_CUT_OF_MEAT                 "\xF0\x9F\xA5\xA9"  // U+1F969
#define D_EMOJI_FOOD_BACON                       "\xF0\x9F\xA5\x93"  // U+1F953
#define D_EMOJI_FOOD_HAMBURGER                   "\xF0\x9F\x8D\x94"  // U+1F354
#define D_EMOJI_FOOD_FRENCH_FRIES                "\xF0\x9F\x8D\x9F"  // U+1F35F
#define D_EMOJI_FOOD_PIZZA                       "\xF0\x9F\x8D\x95"  // U+1F355
#define D_EMOJI_FOOD_HOT_DOG                     "\xF0\x9F\x8C\xAD"  // U+1F32D
#define D_EMOJI_FOOD_SANDWICH                    "\xF0\x9F\xA5\xAA"  // U+1F96A
#define D_EMOJI_FOOD_TACO                        "\xF0\x9F\x8C\xAE"  // U+1F32E
#define D_EMOJI_FOOD_BURRITO                     "\xF0\x9F\x8C\xAF"  // U+1F32F
#define D_EMOJI_FOOD_TAMALE                      "\xF0\x9F\xAB\x94"  // U+1FAD4
#define D_EMOJI_FOOD_STUFFED_FLATBREAD           "\xF0\x9F\xA5\x99"  // U+1F959
#define D_EMOJI_FOOD_FALAFEL                     "\xF0\x9F\xA7\x86"  // U+1F9C6
#define D_EMOJI_FOOD_EGG                         "\xF0\x9F\xA5\x9A"  // U+1F95A
// cooked dishes/rice                            
#define D_EMOJI_FOOD_COOKED_RICE                 "\xF0\x9F\x8D\x9A"  // U+1F35A
#define D_EMOJI_FOOD_RICE_BALL                   "\xF0\x9F\x8D\x99"  // U+1F359
#define D_EMOJI_FOOD_RICE_CRACKER                "\xF0\x9F\x8D\x98"  // U+1F358
#define D_EMOJI_FOOD_CURRY_RICE                  "\xF0\x9F\x8D\x9B"  // U+1F35B
#define D_EMOJI_FOOD_STEAMING_BOWL               "\xF0\x9F\x8D\x9C"  // U+1F35C
#define D_EMOJI_FOOD_SPAGHETTI                   "\xF0\x9F\x8D\x9D"  // U+1F35D
#define D_EMOJI_FOOD_POT_OF_FOOD                 "\xF0\x9F\x8D\xB2"  // U+1F372
// seafood                                       
#define D_EMOJI_FOOD_FRIED_SHRIMP                "\xF0\x9F\x8D\xA4"  // U+1F364
#define D_EMOJI_FOOD_FISH_CAKE                   "\xF0\x9F\x8D\xA5"  // U+1F365
#define D_EMOJI_FOOD_SUSHI                       "\xF0\x9F\x8D\xA3"  // U+1F363
#define D_EMOJI_FOOD_BENTO_BOX                   "\xF0\x9F\x8D\xB1"  // U+1F371
// snacks                                        
#define D_EMOJI_FOOD_POPCORN                     "\xF0\x9F\x8D\xBF"  // U+1F37F
#define D_EMOJI_FOOD_BUTTER                      "\xF0\x9F\xA7\x88"  // U+1F9C8
#define D_EMOJI_FOOD_SALT                        "\xF0\x9F\xA7\x82"  // U+1F9C2
#define D_EMOJI_FOOD_CANNED_FOOD                 "\xF0\x9F\xA5\xAB"  // U+1F96B
// desserts/sweets                               
#define D_EMOJI_FOOD_SOFT_ICE_CREAM              "\xF0\x9F\x8D\xA6"  // U+1F366
#define D_EMOJI_FOOD_SHAVED_ICE                  "\xF0\x9F\x8D\xA7"  // U+1F367
#define D_EMOJI_FOOD_ICE_CREAM                   "\xF0\x9F\x8D\xA8"  // U+1F368
#define D_EMOJI_FOOD_DOUGHNUT                    "\xF0\x9F\x8D\xA9"  // U+1F369
#define D_EMOJI_FOOD_COOKIE                      "\xF0\x9F\x8D\xAA"  // U+1F36A
#define D_EMOJI_FOOD_BIRTHDAY_CAKE               "\xF0\x9F\x8E\x82"  // U+1F382
#define D_EMOJI_FOOD_SHORTCAKE                   "\xF0\x9F\x8D\xB0"  // U+1F370
#define D_EMOJI_FOOD_CUPCAKE                     "\xF0\x9F\xA7\x81"  // U+1F9C1
#define D_EMOJI_FOOD_PIE                         "\xF0\x9F\xA5\xA7"  // U+1F967
#define D_EMOJI_FOOD_CHOCOLATE_BAR               "\xF0\x9F\x8D\xAB"  // U+1F36B
#define D_EMOJI_FOOD_CANDY                       "\xF0\x9F\x8D\xAC"  // U+1F36C
#define D_EMOJI_FOOD_LOLLIPOP                    "\xF0\x9F\x8D\xAD"  // U+1F36D
#define D_EMOJI_FOOD_CUSTARD                     "\xF0\x9F\x8D\xAE"  // U+1F36E
#define D_EMOJI_FOOD_HONEY_POT                   "\xF0\x9F\x8D\xAF"  // U+1F36F
// beverages
#define D_EMOJI_DRINK_BABY_BOTTLE                "\xF0\x9F\x8D\xBC"  // U+1F37C
#define D_EMOJI_DRINK_GLASS_OF_MILK              "\xF0\x9F\xA5\x9B"  // U+1F95B
#define D_EMOJI_DRINK_HOT_BEVERAGE               "\xE2\x98\x95"      // U+2615 
#define D_EMOJI_DRINK_TEAPOT                     "\xF0\x9F\xAB\x96"  // U+1FAD6
#define D_EMOJI_DRINK_TEACUP_WITHOUT_HANDLE      "\xF0\x9F\x8D\xB5"  // U+1F375
#define D_EMOJI_DRINK_SAKE                       "\xF0\x9F\x8D\xB6"  // U+1F376
#define D_EMOJI_DRINK_BOTTLE_WITH_POPPING_CORK   "\xF0\x9F\x8D\xBE"  // U+1F37E
#define D_EMOJI_DRINK_WINE_GLASS                 "\xF0\x9F\x8D\xB7"  // U+1F377
#define D_EMOJI_DRINK_COCKTAIL_GLASS             "\xF0\x9F\x8D\xB8"  // U+1F378
#define D_EMOJI_DRINK_TROPICAL_DRINK             "\xF0\x9F\x8D\xB9"  // U+1F379
#define D_EMOJI_DRINK_BEER_MUG                   "\xF0\x9F\x8D\xBA"  // U+1F37A
#define D_EMOJI_DRINK_CLINKING_BEER_MUGS         "\xF0\x9F\x8D\xBB"  // U+1F37B
#define D_EMOJI_DRINK_CLINKING_GLASSES           "\xF0\x9F\xA5\x82"  // U+1F942
#define D_EMOJI_DRINK_TUMBLER_GLASS              "\xF0\x9F\xA5\x83"  // U+1F943
#define D_EMOJI_DRINK_CUP_WITH_STRAW             "\xF0\x9F\xA5\xA4"  // U+1F964
#define D_EMOJI_DRINK_BUBBLE_TEA                 "\xF0\x9F\xA7\x8B"  // U+1F9CB
#define D_EMOJI_DRINK_BEVERAGE_BOX               "\xF0\x9F\xA7\x83"  // U+1F9C3
#define D_EMOJI_DRINK_MATE                       "\xF0\x9F\xA7\x89"  // U+1F9C9
#define D_EMOJI_DRINK_ICE                        "\xF0\x9F\xA7\x8A"  // U+1F9CA
// utensils
#define D_EMOJI_FOOD_FORK_AND_KNIFE              "\xF0\x9F\x8D\xB4"  // U+1F374
#define D_EMOJI_FOOD_SPOON                       "\xF0\x9F\xA5\x84"  // U+1F944
#define D_EMOJI_FOOD_KITCHEN_KNIFE               "\xF0\x9F\x94\xAA"  // U+1F52A
#define D_EMOJI_FOOD_AMPHORA                     "\xF0\x9F\x8F\xBA"  // U+1F3FA

// ============================================================================
// emoji bytes (ASCII-only)
// ============================================================================
// ball sports
#define D_EMOJI_SPORT_SOCCER_BALL                "\xE2\x9A\xBD"      // U+26BD 
#define D_EMOJI_SPORT_BASKETBALL                 "\xF0\x9F\x8F\x80"  // U+1F3C0
#define D_EMOJI_SPORT_AMERICAN_FOOTBALL          "\xF0\x9F\x8F\x88"  // U+1F3C8
#define D_EMOJI_SPORT_BASEBALL                   "\xE2\x9A\xBE"      // U+26BE 
#define D_EMOJI_SPORT_SOFTBALL                   "\xF0\x9F\xA5\x8E"  // U+1F94E
#define D_EMOJI_SPORT_TENNIS                     "\xF0\x9F\x8E\xBE"  // U+1F3BE
#define D_EMOJI_SPORT_VOLLEYBALL                 "\xF0\x9F\x8F\x90"  // U+1F3D0
#define D_EMOJI_SPORT_RUGBY_FOOTBALL             "\xF0\x9F\x8F\x89"  // U+1F3C9
#define D_EMOJI_SPORT_PING_PONG                  "\xF0\x9F\x8F\x93"  // U+1F3D3
#define D_EMOJI_SPORT_BADMINTON                  "\xF0\x9F\x8F\xB8"  // U+1F3F8
#define D_EMOJI_SPORT_LACROSSE                   "\xF0\x9F\xA5\x8D"  // U+1F94D
#define D_EMOJI_SPORT_FIELD_HOCKEY               "\xF0\x9F\x8F\x91"  // U+1F3D1
#define D_EMOJI_SPORT_ICE_HOCKEY                 "\xF0\x9F\x8F\x92"  // U+1F3D2
#define D_EMOJI_SPORT_CRICKET_GAME               "\xF0\x9F\x8F\x8F"  // U+1F3CF
// water sports                                  
#define D_EMOJI_SPORT_SWIMMING                   "\xF0\x9F\x8F\x8A"  // U+1F3CA
#define D_EMOJI_SPORT_WATER_POLO                 "\xF0\x9F\xA4\xBD"  // U+1F93D
#define D_EMOJI_SPORT_SURFING                    "\xF0\x9F\x8F\x84"  // U+1F3C4
#define D_EMOJI_SPORT_ROWING_BOAT                "\xF0\x9F\x9A\xA3"  // U+1F6A3
// track and field                               
#define D_EMOJI_SPORT_RUNNING                    "\xF0\x9F\x8F\x83"  // U+1F3C3
#define D_EMOJI_SPORT_WALKING                    "\xF0\x9F\x9A\xB6"  // U+1F6B6
#define D_EMOJI_SPORT_RACING_CAR                 "\xF0\x9F\x8F\x8E"  // U+1F3CE
#define D_EMOJI_SPORT_MOTORCYCLE                 "\xF0\x9F\x8F\x8D"  // U+1F3CD
#define D_EMOJI_SPORT_BICYCLE                    "\xF0\x9F\x9A\xB2"  // U+1F6B2
#define D_EMOJI_SPORT_MOUNTAIN_BICYCLING         "\xF0\x9F\x9A\xB5"  // U+1F6B5
#define D_EMOJI_SPORT_PERSON_BIKING              "\xF0\x9F\x9A\xB4"  // U+1F6B4
// winter sports                                 
#define D_EMOJI_SPORT_SKIER                      "\xE2\x9B\xB7"      // U+26F7 
#define D_EMOJI_SPORT_SNOWBOARDER                "\xF0\x9F\x8F\x82"  // U+1F3C2
#define D_EMOJI_SPORT_ICE_SKATE                  "\xE2\x9B\xB8"      // U+26F8 
#define D_EMOJI_SPORT_CURLING_STONE              "\xF0\x9F\xA5\x8C"  // U+1F94C
// combat sports                                 
#define D_EMOJI_SPORT_BOXING_GLOVE               "\xF0\x9F\xA5\x8A"  // U+1F94A
#define D_EMOJI_SPORT_MARTIAL_ARTS_UNIFORM       "\xF0\x9F\xA5\x8B"  // U+1F94B
#define D_EMOJI_SPORT_FENCING                    "\xF0\x9F\xA4\xBA"  // U+1F93A
#define D_EMOJI_SPORT_WRESTLING                  "\xF0\x9F\xA4\xBC"  // U+1F93C
// gymnastics and acrobatics                     
#define D_EMOJI_SPORT_GYMNASTICS                 "\xF0\x9F\xA4\xB8"  // U+1F938
#define D_EMOJI_SPORT_JUGGLING                   "\xF0\x9F\xA4\xB9"  // U+1F939
// target sports                                 
#define D_EMOJI_SPORT_BOW_AND_ARROW              "\xF0\x9F\x8F\xB9"  // U+1F3F9
#define D_EMOJI_SPORT_DIRECT_HIT                 "\xF0\x9F\x8E\xAF"  // U+1F3AF
// climbing                                      
#define D_EMOJI_SPORT_CLIMBING                   "\xF0\x9F\xA7\x97"  // U+1F9D7
// golf                                          
#define D_EMOJI_SPORT_GOLF                       "\xE2\x9B\xB3"      // U+26F3 
#define D_EMOJI_SPORT_FLAG_IN_HOLE               "\xE2\x9B\xB3"      // U+26F3 
// weightlifting                                 
#define D_EMOJI_SPORT_WEIGHT_LIFTING             "\xF0\x9F\x8F\x8B"  // U+1F3CB
// racing                                        
#define D_EMOJI_SPORT_AUTO_RACING                "\xF0\x9F\x8F\x81"  // U+1F3C1
#define D_EMOJI_SPORT_HORSE_RACING               "\xF0\x9F\x8F\x87"  // U+1F3C7
// entertainment sports                          
#define D_EMOJI_SPORT_BOWLING                    "\xF0\x9F\x8E\xB3"  // U+1F3B3
#define D_EMOJI_SPORT_FLYING_DISC                "\xF0\x9F\xA5\x8F"  // U+1F94F
#define D_EMOJI_SPORT_KITE                       "\xF0\x9F\xAA\x81"  // U+1FA81
#define D_EMOJI_SPORT_YO_YO                      "\xF0\x9F\xAA\x80"  // U+1FA80
// sports equipment                              
#define D_EMOJI_SPORT_FISHING_POLE               "\xF0\x9F\x8E\xA3"  // U+1F3A3
#define D_EMOJI_SPORT_SLED                       "\xF0\x9F\x9B\xB7"  // U+1F6F7
#define D_EMOJI_SPORT_PARACHUTE                  "\xF0\x9F\xAA\x82"  // U+1FA82
// awards and achievements                       
#define D_EMOJI_SPORT_TROPHY                     "\xF0\x9F\x8F\x86"  // U+1F3C6
#define D_EMOJI_SPORT_MEDAL                      "\xF0\x9F\x8F\x85"  // U+1F3C5
#define D_EMOJI_SPORT_FIRST_PLACE_MEDAL          "\xF0\x9F\xA5\x87"  // U+1F947
#define D_EMOJI_SPORT_SECOND_PLACE_MEDAL         "\xF0\x9F\xA5\x88"  // U+1F948
#define D_EMOJI_SPORT_THIRD_PLACE_MEDAL          "\xF0\x9F\xA5\x89"  // U+1F949
// stadium and venues                            
#define D_EMOJI_SPORT_STADIUM                    "\xF0\x9F\x8F\x9F"  // U+1F3DF
#define D_EMOJI_SPORT_COLOSSEUM                  "\xF0\x9F\x8F\x9B"  // U+1F3DB
// fan items                                     
#define D_EMOJI_TICKET                           "\xF0\x9F\x8E\xAB"  // U+1F3AB

// ============================================================================
// MUSICAL EMOJIS
// ============================================================================
// string instruments
#define D_EMOJI_MUSIC_GUITAR                     "\xF0\x9F\x8E\xB8"  // U+1F3B8
#define D_EMOJI_MUSIC_VIOLIN                     "\xF0\x9F\x8E\xBB"  // U+1F3BB
#define D_EMOJI_MUSIC_BANJO                      "\xF0\x9F\xAA\x95"  // U+1FA95
#define D_EMOJI_MUSIC_LONG_DRUM                  "\xF0\x9F\xAA\x98"  // U+1FA98
// wind instruments                              
#define D_EMOJI_MUSIC_TRUMPET                    "\xF0\x9F\x8E\xBA"  // U+1F3BA
#define D_EMOJI_MUSIC_SAXOPHONE                  "\xF0\x9F\x8E\xB7"  // U+1F3B7
#define D_EMOJI_MUSIC_FLUTE                      "\xF0\x9F\xAA\x88"  // U+1FA88
#define D_EMOJI_MUSIC_ACCORDION                  "\xF0\x9F\xAA\x97"  // U+1FA97
#define D_EMOJI_MUSIC_TROMBONE                   "\xF0\x9F\xAA\x8A"  // U+1FA8A
// percussion                                    
#define D_EMOJI_MUSIC_DRUM                       "\xF0\x9F\x8E\xA5"  // U+1F3A5
#define D_EMOJI_MUSIC_MUSICAL_KEYBOARD           "\xF0\x9F\x8E\xB9"  // U+1F3B9
// audio equipment                               
#define D_EMOJI_MUSIC_MICROPHONE                 "\xF0\x9F\x8E\xA4"  // U+1F3A4
#define D_EMOJI_MUSIC_HEADPHONE                  "\xF0\x9F\x8E\xA7"  // U+1F3A7
#define D_EMOJI_MUSIC_RADIO                      "\xF0\x9F\x93\xBB"  // U+1F4FB
#define D_EMOJI_MUSIC_SPEAKER_HIGH_VOLUME        "\xF0\x9F\x94\x8A"  // U+1F50A
#define D_EMOJI_MUSIC_SPEAKER_MEDIUM_VOLUME      "\xF0\x9F\x94\x89"  // U+1F509
#define D_EMOJI_MUSIC_SPEAKER_LOW_VOLUME         "\xF0\x9F\x94\x88"  // U+1F508
#define D_EMOJI_MUSIC_MUTED_SPEAKER              "\xF0\x9F\x94\x87"  // U+1F507
#define D_EMOJI_MUSIC_POSTAL_HORN                "\xF0\x9F\x93\xAF"  // U+1F4EF
// musical notation                              
#define D_EMOJI_MUSIC_MUSICAL_NOTE               "\xF0\x9F\x8E\xB5"  // U+1F3B5
#define D_EMOJI_MUSIC_MUSICAL_NOTES              "\xF0\x9F\x8E\xB6"  // U+1F3B6
#define D_EMOJI_MUSIC_MUSICAL_SCORE              "\xF0\x9F\x8E\xBC"  // U+1F3BC
// entertainment                                 
#define D_EMOJI_MUSIC_CLAPPER_BOARD              "\xF0\x9F\x8E\xAC"  // U+1F3AC
#define D_EMOJI_MUSIC_PERFORMING_ARTS            "\xF0\x9F\x8E\xAD"  // U+1F3AD

// ============================================================================
// TRANSPORTATION EMOJIS
// ============================================================================
// cars/road vehicles
#define D_EMOJI_TRANSPORT_AUTOMOBILE             "\xF0\x9F\x9A\x97"  // U+1F697
#define D_EMOJI_TRANSPORT_TAXI                   "\xF0\x9F\x9A\x95"  // U+1F695
#define D_EMOJI_TRANSPORT_BUS                    "\xF0\x9F\x9A\x8C"  // U+1F68C
#define D_EMOJI_TRANSPORT_TROLLEYBUS             "\xF0\x9F\x9A\x8E"  // U+1F68E
#define D_EMOJI_TRANSPORT_MINIBUS                "\xF0\x9F\x9A\x90"  // U+1F690
#define D_EMOJI_TRANSPORT_DELIVERY_TRUCK         "\xF0\x9F\x9A\x9A"  // U+1F69A
#define D_EMOJI_TRANSPORT_ARTICULATED_LORRY      "\xF0\x9F\x9A\x9B"  // U+1F69B
#define D_EMOJI_TRANSPORT_PICKUP_TRUCK           "\xF0\x9F\x9B\xBB"  // U+1F6FB
#define D_EMOJI_TRANSPORT_SPORT_UTILITY_VEHICLE  "\xF0\x9F\x9A\x99"  // U+1F699
// emergency vehicles                            
#define D_EMOJI_TRANSPORT_FIRE_ENGINE            "\xF0\x9F\x9A\x92"  // U+1F692
#define D_EMOJI_TRANSPORT_POLICE_CAR             "\xF0\x9F\x9A\x93"  // U+1F693
#define D_EMOJI_TRANSPORT_AMBULANCE              "\xF0\x9F\x9A\x91"  // U+1F691
// construction vehicles                         
#define D_EMOJI_TRANSPORT_TRACTOR                "\xF0\x9F\x9A\x9C"  // U+1F69C
#define D_EMOJI_TRANSPORT_EXCAVATOR              "\xF0\x9F\x9A\x9C"  // U+1F69C
// two-wheeled vehicles                          
#define D_EMOJI_TRANSPORT_MOTOR_SCOOTER          "\xF0\x9F\x9B\xB5"  // U+1F6F5
#define D_EMOJI_TRANSPORT_MANUAL_WHEELCHAIR      "\xF0\x9F\xA6\xBD"  // U+1F9BD
#define D_EMOJI_TRANSPORT_MOTORIZED_WHEELCHAIR   "\xF0\x9F\xA6\xBC"  // U+1F9BC
// rail transport                                
#define D_EMOJI_TRANSPORT_TRAIN                  "\xF0\x9F\x9A\x8B"  // U+1F68B
#define D_EMOJI_TRANSPORT_METRO                  "\xF0\x9F\x9A\x87"  // U+1F687
#define D_EMOJI_TRANSPORT_LIGHT_RAIL             "\xF0\x9F\x9A\x88"  // U+1F688
#define D_EMOJI_TRANSPORT_STATION                "\xF0\x9F\x9A\x89"  // U+1F689
#define D_EMOJI_TRANSPORT_TRAM                   "\xF0\x9F\x9A\x8A"  // U+1F68A
#define D_EMOJI_TRANSPORT_MONORAIL               "\xF0\x9F\x9A\x9D"  // U+1F69D
#define D_EMOJI_TRANSPORT_MOUNTAIN_RAILWAY       "\xF0\x9F\x9A\x9E"  // U+1F69E
#define D_EMOJI_TRANSPORT_LOCOMOTIVE             "\xF0\x9F\x9A\x82"  // U+1F682
#define D_EMOJI_TRANSPORT_RAILWAY_CAR            "\xF0\x9F\x9A\x83"  // U+1F683
#define D_EMOJI_TRANSPORT_HIGH_SPEED_TRAIN       "\xF0\x9F\x9A\x84"  // U+1F684
#define D_EMOJI_TRANSPORT_BULLET_TRAIN           "\xF0\x9F\x9A\x85"  // U+1F685
// air transport                                 
#define D_EMOJI_TRANSPORT_AIRPLANE               "\xE2\x9C\x88"      // U+2708 
#define D_EMOJI_TRANSPORT_SMALL_AIRPLANE         "\xF0\x9F\x9B\xA9"  // U+1F6E9
#define D_EMOJI_TRANSPORT_AIRPLANE_DEPARTURE     "\xF0\x9F\x9B\xAB"  // U+1F6EB
#define D_EMOJI_TRANSPORT_AIRPLANE_ARRIVAL       "\xF0\x9F\x9B\xAC"  // U+1F6EC
#define D_EMOJI_TRANSPORT_HELICOPTER             "\xF0\x9F\x9A\x81"  // U+1F681
#define D_EMOJI_TRANSPORT_SEAT                   "\xF0\x9F\x92\xBA"  // U+1F4BA
// water transport                               
#define D_EMOJI_TRANSPORT_SHIP                   "\xF0\x9F\x9A\xA2"  // U+1F6A2
#define D_EMOJI_TRANSPORT_SPEEDBOAT              "\xF0\x9F\x9A\xA4"  // U+1F6A4
#define D_EMOJI_TRANSPORT_PASSENGER_SHIP         "\xF0\x9F\x9B\xB3"  // U+1F6F3
#define D_EMOJI_TRANSPORT_FERRY                  "\xE2\x9B\xB4"      // U+26F4 
#define D_EMOJI_TRANSPORT_MOTOR_BOAT             "\xF0\x9F\x9B\xA5"  // U+1F6E5
#define D_EMOJI_TRANSPORT_SAILBOAT               "\xE2\x9B\xB5"      // U+26F5 
#define D_EMOJI_TRANSPORT_CANOE                  "\xF0\x9F\x9B\xB6"  // U+1F6F6
// infrastructure
#define D_EMOJI_TRANSPORT_FUEL_PUMP              "\xE2\x9B\xBD"      // U+26FD 
#define D_EMOJI_TRANSPORT_BUS_STOP               "\xF0\x9F\x9A\x8F"  // U+1F68F
#define D_EMOJI_TRANSPORT_TRAFFIC_LIGHT_VERT     "\xF0\x9F\x9A\xA6"  // U+1F6A6
#define D_EMOJI_TRANSPORT_TRAFFIC_LIGHT_HORIZ    "\xF0\x9F\x9A\xA5"  // U+1F6A5
#define D_EMOJI_TRANSPORT_CONSTRUCTION           "\xF0\x9F\x9A\xA7"  // U+1F6A7
#define D_EMOJI_TRANSPORT_ANCHOR                 "\xE2\x9A\x93"      // U+2693 
#define D_EMOJI_TRANSPORT_BRIDGE_AT_NIGHT        "\xF0\x9F\x8C\x89"  // U+1F309
// space transportation                          
#define D_EMOJI_TRANSPORT_ROCKET                 "\xF0\x9F\x9A\x80"  // U+1F680
#define D_EMOJI_TRANSPORT_FLYING_SAUCER          "\xF0\x9F\x9B\xB8"  // U+1F6F8
#define D_EMOJI_TRANSPORT_SATELLITE              "\xF0\x9F\x9B\xB0"  // U+1F6F0
// other transportation                          
#define D_EMOJI_TRANSPORT_AUTO_RICKSHAW          "\xF0\x9F\x9B\xBA"  // U+1F6FA
#define D_EMOJI_TRANSPORT_KICK_SCOOTER           "\xF0\x9F\x9B\xB4"  // U+1F6F4

// ============================================================================
// TRAVEL & PLACES EMOJIS
// ============================================================================
// buildings and homes
#define D_EMOJI_PLACE_HOUSE                      "\xF0\x9F\x8F\xA0"  // U+1F3E0
#define D_EMOJI_PLACE_HOUSE_WITH_GARDEN          "\xF0\x9F\x8F\xA1"  // U+1F3E1
#define D_EMOJI_PLACE_OFFICE_BUILDING            "\xF0\x9F\x8F\xA2"  // U+1F3E2
#define D_EMOJI_PLACE_JAPANESE_POST_OFFICE       "\xF0\x9F\x8F\xA3"  // U+1F3E3
#define D_EMOJI_PLACE_POST_OFFICE                "\xF0\x9F\x8F\xA4"  // U+1F3E4
#define D_EMOJI_PLACE_HOSPITAL                   "\xF0\x9F\x8F\xA5"  // U+1F3E5
#define D_EMOJI_PLACE_BANK                       "\xF0\x9F\x8F\xA6"  // U+1F3E6
#define D_EMOJI_PLACE_ATM                        "\xF0\x9F\x8F\xA7"  // U+1F3E7
#define D_EMOJI_PLACE_HOTEL                      "\xF0\x9F\x8F\xA8"  // U+1F3E8
#define D_EMOJI_PLACE_LOVE_HOTEL                 "\xF0\x9F\x8F\xA9"  // U+1F3E9
#define D_EMOJI_PLACE_CONVENIENCE_STORE          "\xF0\x9F\x8F\xAA"  // U+1F3EA
#define D_EMOJI_PLACE_SCHOOL                     "\xF0\x9F\x8F\xAB"  // U+1F3EB
#define D_EMOJI_PLACE_DEPARTMENT_STORE           "\xF0\x9F\x8F\xAC"  // U+1F3EC
#define D_EMOJI_PLACE_FACTORY                    "\xF0\x9F\x8F\xAD"  // U+1F3ED
#define D_EMOJI_PLACE_JAPANESE_CASTLE            "\xF0\x9F\x8F\xAF"  // U+1F3EF
#define D_EMOJI_PLACE_EUROPEAN_CASTLE            "\xF0\x9F\x8F\xB0"  // U+1F3F0
#define D_EMOJI_PLACE_WEDDING                    "\xF0\x9F\x92\x92"  // U+1F492
#define D_EMOJI_PLACE_TOKYO_TOWER                "\xF0\x9F\x97\xBC"  // U+1F5FC
#define D_EMOJI_PLACE_STATUE_OF_LIBERTY          "\xF0\x9F\x97\xBD"  // U+1F5FD
// religious buildings
#define D_EMOJI_CHURCH                           "\xE2\x9B\xAA"      // U+26EA 
#define D_EMOJI_MOSQUE                           "\xF0\x9F\x95\x8C"  // U+1F54C
#define D_EMOJI_HINDU_TEMPLE                     "\xF0\x9F\x9B\x95"  // U+1F6D5
#define D_EMOJI_SYNAGOGUE                        "\xF0\x9F\x95\x8D"  // U+1F54D
// geographic features
#define D_EMOJI_CITYSCAPE                        "\xF0\x9F\x8F\x99"  // U+1F3D9
#define D_EMOJI_CITYSCAPE_AT_DUSK                "\xF0\x9F\x8C\x86"  // U+1F306
#define D_EMOJI_SUNRISE_OVER_MOUNTAINS           "\xF0\x9F\x8C\x84"  // U+1F304
#define D_EMOJI_SUNRISE                          "\xF0\x9F\x8C\x85"  // U+1F305
#define D_EMOJI_NIGHT_WITH_STARS                 "\xF0\x9F\x8C\x83"  // U+1F303
#define D_EMOJI_SUNSET                           "\xF0\x9F\x8C\x87"  // U+1F307
// entertainment venues                          
#define D_EMOJI_CIRCUS_TENT                      "\xF0\x9F\x8E\xAA"  // U+1F3AA
#define D_EMOJI_FERRIS_WHEEL                     "\xF0\x9F\x8E\xA1"  // U+1F3A1
#define D_EMOJI_ROLLER_COASTER                   "\xF0\x9F\x8E\xA2"  // U+1F3A2
#define D_EMOJI_CAROUSEL_HORSE                   "\xF0\x9F\x8E\xA0"  // U+1F3A0
// transportation hubs                           
#define D_EMOJI_RAILWAY_TRACK                    "\xF0\x9F\x9B\xA4"  // U+1F6E4
#define D_EMOJI_MOTORWAY                         "\xF0\x9F\x9B\xA3"  // U+1F6E3
#define D_EMOJI_AIRPORT                          "\xF0\x9F\x9B\xAC"  // U+1F6EC
// outdoor/camping                               
#define D_EMOJI_TENT                             "\xE2\x9B\xBA"      // U+26FA 
#define D_EMOJI_HUT                              "\xF0\x9F\x9B\x96"  // U+1F6D6
// maps/navigation                               
#define D_EMOJI_WORLD_MAP                        "\xF0\x9F\x97\xBA"  // U+1F5FA
#define D_EMOJI_JAPAN_MAP                        "\xF0\x9F\x97\xBE"  // U+1F5FE
#define D_EMOJI_COMPASS                          "\xF0\x9F\xA7\xAD"  // U+1F9ED
// signage/markers                               
#define D_EMOJI_ROUND_PUSHPIN                    "\xF0\x9F\x93\x8D"  // U+1F4CD
#define D_EMOJI_PUSHPIN                          "\xF0\x9F\x93\x8C"  // U+1F4CC
#define D_EMOJI_TRIANGULAR_FLAG                  "\xF0\x9F\x9A\xA9"  // U+1F6A9
#define D_EMOJI_CROSSED_FLAGS                    "\xF0\x9F\x8E\x8C"  // U+1F38C
#define D_EMOJI_BLACK_FLAG                       "\xF0\x9F\x8F\xB4"  // U+1F3F4
#define D_EMOJI_WHITE_FLAG                       "\xF0\x9F\x8F\xB3"  // U+1F3F3
// time/schedules                                
#define D_EMOJI_CLOCK_TWELVE                     "\xF0\x9F\x95\x9B"  // U+1F55B
#define D_EMOJI_CLOCK_ONE                        "\xF0\x9F\x95\x90"  // U+1F550
#define D_EMOJI_CLOCK_TWO                        "\xF0\x9F\x95\x91"  // U+1F551
#define D_EMOJI_CLOCK_THREE                      "\xF0\x9F\x95\x92"  // U+1F552
#define D_EMOJI_CLOCK_FOUR                       "\xF0\x9F\x95\x93"  // U+1F553
#define D_EMOJI_CLOCK_FIVE                       "\xF0\x9F\x95\x94"  // U+1F554
#define D_EMOJI_CLOCK_SIX                        "\xF0\x9F\x95\x95"  // U+1F555
#define D_EMOJI_CLOCK_SEVEN                      "\xF0\x9F\x95\x96"  // U+1F556
#define D_EMOJI_CLOCK_EIGHT                      "\xF0\x9F\x95\x97"  // U+1F557
#define D_EMOJI_CLOCK_NINE                       "\xF0\x9F\x95\x98"  // U+1F558
#define D_EMOJI_CLOCK_TEN                        "\xF0\x9F\x95\x99"  // U+1F559
#define D_EMOJI_CLOCK_ELEVEN                     "\xF0\x9F\x95\x9A"  // U+1F55A
#define D_EMOJI_CLOCK_TWELVE_THIRTY              "\xF0\x9F\x95\xA7"  // U+1F567
#define D_EMOJI_CLOCK_ONE_THIRTY                 "\xF0\x9F\x95\x9C"  // U+1F55C 
#define D_EMOJI_CLOCK_TWO_THIRTY                 "\xF0\x9F\x95\x9D"  // U+1F55D 
#define D_EMOJI_CLOCK_THREE_THIRTY               "\xF0\x9F\x95\x9E"  // U+1F55E
#define D_EMOJI_CLOCK_FOUR_THIRTY                "\xF0\x9F\x95\x9F"  // U+1F55F
#define D_EMOJI_CLOCK_FIVE_THIRTY                "\xF0\x9F\x95\xA0"  // U+1F560
#define D_EMOJI_CLOCK_SIX_THIRTY                 "\xF0\x9F\x95\xA1"  // U+1F561
#define D_EMOJI_CLOCK_SEVEN_THIRTY               "\xF0\x9F\x95\xA2"  // U+1F562
#define D_EMOJI_CLOCK_EIGHT_THIRTY               "\xF0\x9F\x95\xA3"  // U+1F563
#define D_EMOJI_CLOCK_NINE_THIRTY                "\xF0\x9F\x95\xA4"  // U+1F564
#define D_EMOJI_CLOCK_TEN_THIRTY                 "\xF0\x9F\x95\xA5"  // U+1F565
#define D_EMOJI_CLOCK_ELEVEN_THIRTY              "\xF0\x9F\x95\xA6"  // U+1F566
// weather symbols
#define D_EMOJI_THERMOMETER                      "\xF0\x9F\x8C\xA1"  // U+1F321
#define D_EMOJI_BEACH_WITH_UMBRELLA              "\xF0\x9F\x8F\x96"  // U+1F3D6
// international/cultural                                                  
#define D_EMOJI_KAABA                            "\xF0\x9F\x95\x8B"  // U+1F54B
#define D_EMOJI_FOUNTAIN                         "\xE2\x9B\xB2"      // U+26F2 
#define D_EMOJI_SHINTO_SHRINE                    "\xE2\x9B\xA9"      // U+26E9 
// travel documents/currency                                               
#define D_EMOJI_PASSPORT_CONTROL                 "\xF0\x9F\x9B\x82"  // U+1F6C2
#define D_EMOJI_CUSTOMS                          "\xF0\x9F\x9B\x83"  // U+1F6C3
#define D_EMOJI_BAGGAGE_CLAIM                    "\xF0\x9F\x9B\x84"  // U+1F6C4
#define D_EMOJI_LEFT_LUGGAGE                     "\xF0\x9F\x9B\x85"  // U+1F6C5
// accommodation                                                               
#define D_EMOJI_CLASSICAL_BUILDING               "\xF0\x9F\x8F\x9B"  // U+1F3DB
#define D_EMOJI_DERELICT_HOUSE                   "\xF0\x9F\x8F\x9A"  // U+1F3DA
// modern buildings                                                            
#define D_EMOJI_BUILDING_CONSTRUCTION            "\xF0\x9F\x8F\x97"  // U+1F3D7
#define D_EMOJI_HOUSES                           "\xF0\x9F\x8F\x98"  // U+1F3D8

// ============================================================================
// OBJECT EMOJIS
// ============================================================================
// household items
#define D_EMOJI_OBJECT_BATHTUB                   "\xF0\x9F\x9B\x81"  // U+1F6C1
#define D_EMOJI_OBJECT_BED                       "\xF0\x9F\x9B\x8F"  // U+1F6CF
#define D_EMOJI_OBJECT_COUCH_AND_LAMP            "\xF0\x9F\x9B\x8B"  // U+1F6CB
#define D_EMOJI_OBJECT_CHAIR                     "\xF0\x9F\xAA\x91"  // U+1FA91
#define D_EMOJI_OBJECT_TOILET                    "\xF0\x9F\x9A\xBD"  // U+1F6BD
#define D_EMOJI_OBJECT_SHOWER                    "\xF0\x9F\x9A\xBF"  // U+1F6BF
#define D_EMOJI_OBJECT_DOOR                      "\xF0\x9F\x9A\xAA"  // U+1F6AA
#define D_EMOJI_OBJECT_WINDOW                    "\xF0\x9F\xAA\x9F"  // U+1FA9F
#define D_EMOJI_OBJECT_MIRROR                    "\xF0\x9F\xAA\x9E"  // U+1FA9E
#define D_EMOJI_OBJECT_BASKET                    "\xF0\x9F\xA7\xBA"  // U+1F9FA
#define D_EMOJI_OBJECT_BROOM                     "\xF0\x9F\xA7\xB9"  // U+1F9F9
#define D_EMOJI_OBJECT_SOAP                      "\xF0\x9F\xA7\xBC"  // U+1F9FC
#define D_EMOJI_OBJECT_SPONGE                    "\xF0\x9F\xA7\xBD"  // U+1F9FD
// tools/hardware
#define D_EMOJI_OBJECT_HAMMER                    "\xF0\x9F\x94\x88"  // U+1F528
#define D_EMOJI_OBJECT_PICK                      "\xE2\x9B\x8F"      // U+26CF 
#define D_EMOJI_OBJECT_HAMMER_AND_PICK           "\xE2\x9A\x92"      // U+2692 
#define D_EMOJI_OBJECT_HAMMER_AND_WRENCH         "\xF0\x9F\x9B\xA0"  // U+1F6E0
#define D_EMOJI_OBJECT_DAGGER                    "\xF0\x9F\x97\xA1"  // U+1F5E1
#define D_EMOJI_OBJECT_CROSSED_SWORDS            "\xE2\x9A\x94"      // U+2694 
#define D_EMOJI_OBJECT_PISTOL                    "\xF0\x9F\x94\xAB"  // U+1F52B
#define D_EMOJI_OBJECT_BOW_AND_ARROW_TOOL        "\xF0\x9F\x8F\xB9"  // U+1F3F9
#define D_EMOJI_OBJECT_SHIELD                    "\xF0\x9F\x9B\xA1"  // U+1F6E1
#define D_EMOJI_OBJECT_WRENCH                    "\xF0\x9F\x94\xA7"  // U+1F527
#define D_EMOJI_OBJECT_NUT_AND_BOLT              "\xF0\x9F\x94\xA9"  // U+1F529
#define D_EMOJI_OBJECT_GEAR                      "\xE2\x9A\x99"      // U+2699 
#define D_EMOJI_OBJECT_CLAMP                     "\xF0\x9F\x97\x9C"  // U+1F5DC
#define D_EMOJI_OBJECT_BALANCE_SCALE             "\xE2\x9A\x96"      // U+2696 
#define D_EMOJI_OBJECT_TREASURE_CHEST            "\xF0\x9F\xAA\x8E"  // U+1FA8E
// technology/electronics
#define D_EMOJI_OBJECT_MOBILE_PHONE              "\xF0\x9F\x93\xB1"  // U+1F4F1
#define D_EMOJI_OBJECT_TELEPHONE                 "\xE2\x98\x8E"      // U+260E 
#define D_EMOJI_OBJECT_TELEPHONE_RECEIVER        "\xF0\x9F\x93\x9E"  // U+1F4DE
#define D_EMOJI_OBJECT_PAGER                     "\xF0\x9F\x93\x9F"  // U+1F4DF
#define D_EMOJI_OBJECT_FAX_MACHINE               "\xF0\x9F\x93\xA0"  // U+1F4E0
#define D_EMOJI_OBJECT_BATTERY                   "\xF0\x9F\x94\x8B"  // U+1F50B
#define D_EMOJI_OBJECT_ELECTRIC_PLUG             "\xF0\x9F\x94\x8C"  // U+1F50C
#define D_EMOJI_OBJECT_LAPTOP_COMPUTER           "\xF0\x9F\x92\xBB"  // U+1F4BB
#define D_EMOJI_OBJECT_DESKTOP_COMPUTER          "\xF0\x9F\x96\xA5"  // U+1F5A5
#define D_EMOJI_OBJECT_PRINTER                   "\xF0\x9F\x96\xA8"  // U+1F5A8
#define D_EMOJI_OBJECT_KEYBOARD                  "\xE2\x8C\xA8"      // U+2328 
#define D_EMOJI_OBJECT_COMPUTER_MOUSE            "\xF0\x9F\x96\xB1"  // U+1F5B1
#define D_EMOJI_OBJECT_TRACKBALL                 "\xF0\x9F\x96\xB2"  // U+1F5B2
#define D_EMOJI_OBJECT_JOYSTICK                  "\xF0\x9F\x95\xB9"  // U+1F579
#define D_EMOJI_OBJECT_COMPUTER_DISK             "\xF0\x9F\x92\xBD"  // U+1F4BD
#define D_EMOJI_OBJECT_FLOPPY_DISK               "\xF0\x9F\x92\xBE"  // U+1F4BE
#define D_EMOJI_OBJECT_OPTICAL_DISK              "\xF0\x9F\x92\xBF"  // U+1F4BF
#define D_EMOJI_OBJECT_DVD                       "\xF0\x9F\x93\x80"  // U+1F4C0
#define D_EMOJI_OBJECT_CAMERA                    "\xF0\x9F\x93\xB7"  // U+1F4F7
#define D_EMOJI_OBJECT_CAMERA_WITH_FLASH         "\xF0\x9F\x93\xB8"  // U+1F4F8
#define D_EMOJI_OBJECT_VIDEO_CAMERA              "\xF0\x9F\x93\xB9"  // U+1F4F9
#define D_EMOJI_OBJECT_MOVIE_CAMERA              "\xF0\x9F\x8E\xA5"  // U+1F3A5
#define D_EMOJI_OBJECT_FILM_PROJECTOR            "\xF0\x9F\x93\xBD"  // U+1F4FD
#define D_EMOJI_OBJECT_TELEVISION                "\xF0\x9F\x93\xBA"  // U+1F4FA
// office supplies
#define D_EMOJI_OBJECT_MEMO                      "\xF0\x9F\x93\x9D"  // U+1F4DD
#define D_EMOJI_OBJECT_PENCIL                    "\xE2\x9C\x8F"      // U+270F 
#define D_EMOJI_OBJECT_BLACK_NIB                 "\xE2\x9C\x92"      // U+2712 
#define D_EMOJI_OBJECT_FOUNTAIN_PEN              "\xF0\x9F\x96\x8B"  // U+1F58B
#define D_EMOJI_OBJECT_PEN                       "\xF0\x9F\x96\x8A"  // U+1F58A
#define D_EMOJI_OBJECT_PAINTBRUSH                "\xF0\x9F\x96\x8C"  // U+1F58C
#define D_EMOJI_OBJECT_CRAYON                    "\xF0\x9F\x96\x8D"  // U+1F58D
#define D_EMOJI_OBJECT_BRIEFCASE                 "\xF0\x9F\x92\xBC"  // U+1F4BC
#define D_EMOJI_OBJECT_FILE_CABINET              "\xF0\x9F\x97\x84"  // U+1F5C4
#define D_EMOJI_OBJECT_WASTEBASKET               "\xF0\x9F\x97\x91"  // U+1F5D1
#define D_EMOJI_OBJECT_LOCKED                    "\xF0\x9F\x94\x92"  // U+1F512
#define D_EMOJI_OBJECT_UNLOCKED                  "\xF0\x9F\x94\x93"  // U+1F513
#define D_EMOJI_OBJECT_LOCKED_WITH_PEN           "\xF0\x9F\x94\x8F"  // U+1F50F
#define D_EMOJI_OBJECT_LOCKED_WITH_KEY           "\xF0\x9F\x94\x90"  // U+1F510
#define D_EMOJI_OBJECT_KEY                       "\xF0\x9F\x94\x91"  // U+1F511
#define D_EMOJI_OBJECT_OLD_KEY                   "\xF0\x9F\x97\x9D"  // U+1F5DD
// personal items/accessories
#define D_EMOJI_OBJECT_EYEGLASSES                "\xF0\x9F\x91\x93"  // U+1F453
#define D_EMOJI_OBJECT_DARK_SUNGLASSES           "\xF0\x9F\x95\xB6"  // U+1F576
#define D_EMOJI_OBJECT_NECKTIE                   "\xF0\x9F\x91\x94"  // U+1F454
#define D_EMOJI_OBJECT_T_SHIRT                   "\xF0\x9F\x91\x95"  // U+1F455
#define D_EMOJI_OBJECT_JEANS                     "\xF0\x9F\x91\x96"  // U+1F456
#define D_EMOJI_OBJECT_DRESS                     "\xF0\x9F\x91\x97"  // U+1F457
#define D_EMOJI_OBJECT_KIMONO                    "\xF0\x9F\x91\x98"  // U+1F458
#define D_EMOJI_OBJECT_BIKINI                    "\xF0\x9F\x91\x99"  // U+1F459
#define D_EMOJI_OBJECT_WOMANS_CLOTHES            "\xF0\x9F\x91\x9A"  // U+1F45A
#define D_EMOJI_OBJECT_PURSE                     "\xF0\x9F\x91\x9B"  // U+1F45B
#define D_EMOJI_OBJECT_HANDBAG                   "\xF0\x9F\x91\x9C"  // U+1F45C
#define D_EMOJI_OBJECT_POUCH                     "\xF0\x9F\x91\x9D"  // U+1F45D
#define D_EMOJI_OBJECT_SHOPPING_BAGS             "\xF0\x9F\x9B\x8D"  // U+1F6CD
#define D_EMOJI_OBJECT_SCHOOL_BACKPACK           "\xF0\x9F\x8E\x92"  // U+1F392
#define D_EMOJI_OBJECT_MANS_SHOE                 "\xF0\x9F\x91\x9E"  // U+1F45E
#define D_EMOJI_OBJECT_ATHLETIC_SHOE             "\xF0\x9F\x91\x9F"  // U+1F45F
#define D_EMOJI_OBJECT_HIGH_HEELED_SHOE          "\xF0\x9F\x91\xA0"  // U+1F460
#define D_EMOJI_OBJECT_WOMANS_SANDAL             "\xF0\x9F\x91\xA1"  // U+1F461
#define D_EMOJI_OBJECT_WOMANS_BOOT               "\xF0\x9F\x91\xA2"  // U+1F462
#define D_EMOJI_OBJECT_CROWN                     "\xF0\x9F\x91\x91"  // U+1F451
#define D_EMOJI_OBJECT_WOMANS_HAT                "\xF0\x9F\x91\x92"  // U+1F452
#define D_EMOJI_OBJECT_TOP_HAT                   "\xF0\x9F\x8E\xA9"  // U+1F3A9
#define D_EMOJI_OBJECT_GRADUATION_CAP            "\xF0\x9F\x8E\x93"  // U+1F393
//    (needed for ballet dancer sequence)
#define D_EMOJI_OBJECT_BALLET_SHOES              "\xF0\x9F\xA9\xB0"  // U+1FA70
// medical/healthcare 
#define D_EMOJI_OBJECT_PILL                      "\xF0\x9F\x92\x8A"  // U+1F48A
#define D_EMOJI_OBJECT_SYRINGE                   "\xF0\x9F\x92\x89"  // U+1F489
#define D_EMOJI_OBJECT_THERMOMETER_MEDICAL       "\xF0\x9F\x8C\xA1"  // U+1F321
#define D_EMOJI_OBJECT_ADHESIVE_BANDAGE          "\xF0\x9F\xA9\xB9"  // U+1FA79
#define D_EMOJI_OBJECT_STETHOSCOPE               "\xF0\x9F\xA9\xBA"  // U+1FA7A
// lighting                                      
#define D_EMOJI_OBJECT_ELECTRIC_LIGHT_BULB       "\xF0\x9F\x92\xA1"  // U+1F4A1
#define D_EMOJI_OBJECT_FLASHLIGHT                "\xF0\x9F\x94\x86"  // U+1F526
#define D_EMOJI_OBJECT_CANDLE                    "\xF0\x9F\x95\xAF"  // U+1F56F
#define D_EMOJI_OBJECT_FIRE_EXTINGUISHER         "\xF0\x9F\xA7\xAF"  // U+1F9EF
// books/media
#define D_EMOJI_OBJECT_BOOKMARK_TABS             "\xF0\x9F\x93\x91"  // U+1F4D1
#define D_EMOJI_OBJECT_BOOK_LEDGER               "\xF0\x9F\x93\x92"  // U+1F4D2
#define D_EMOJI_OBJECT_NOTEBOOK                  "\xF0\x9F\x93\x93"  // U+1F4D3
#define D_EMOJI_OBJECT_NOTEBOOK_DECORATIVE_COVER "\xF0\x9F\x93\x94"  // U+1F4D4
#define D_EMOJI_OBJECT_BOOK_CLOSED               "\xF0\x9F\x93\x95"  // U+1F4D5
#define D_EMOJI_OBJECT_BOOK                      "\xF0\x9F\x93\x96"  // U+1F4D6
#define D_EMOJI_OBJECT_BOOK_GREEN                "\xF0\x9F\x93\x97"  // U+1F4D7
#define D_EMOJI_OBJECT_BOOK_BLUE                 "\xF0\x9F\x93\x98"  // U+1F4D8
#define D_EMOJI_OBJECT_BOOK_ORANGE               "\xF0\x9F\x93\x99"  // U+1F4D9
#define D_EMOJI_OBJECT_BOOKS                     "\xF0\x9F\x93\x9A"  // U+1F4DA
#define D_EMOJI_OBJECT_BOOKS                     "\xF0\x9F\x93\x9B"  // U+1F4DA
#define D_EMOJI_OBJECT_NEWSPAPER                 "\xF0\x9F\x93\xB0"  // U+1F4F0
#define D_EMOJI_OBJECT_ROLLED_UP_NEWSPAPER       "\xF0\x9F\x97\x9E"  // U+1F5DE
#define D_EMOJI_OBJECT_BOOKMARK                  "\xF0\x9F\x94\x96"  // U+1F516
#define D_EMOJI_OBJECT_SCROLL                    "\xF0\x9F\x93\x9C"  // U+1F4DC
// games/toys                                    
#define D_EMOJI_OBJECT_GAME_DIE                  "\xF0\x9F\x8E\xB2"  // U+1F3B2
#define D_EMOJI_OBJECT_JIGSAW                    "\xF0\x9F\xA7\xA9"  // U+1F9E9
#define D_EMOJI_OBJECT_TEDDY_BEAR                "\xF0\x9F\xA7\xB8"  // U+1F9F8
#define D_EMOJI_OBJECT_PLAYING_CARD_BLACK_JOKER  "\xF0\x9F\x83\x8F"  // U+1F0CF
#define D_EMOJI_OBJECT_MAHJONG_TILE_RED_DRAGON   "\xF0\x9F\x80\x84"  // U+1F004
#define D_EMOJI_OBJECT_CHESS_PAWN                "\xE2\x99\x9F"      // U+265F
// money/finance                                 
#define D_EMOJI_OBJECT_MONEY_BAG                 "\xF0\x9F\x92\xB0"  // U+1F4B0
#define D_EMOJI_OBJECT_COIN                      "\xF0\x9F\xAA\x99"  // U+1FA99
#define D_EMOJI_OBJECT_BANKNOTE_DOLLAR_SIGN      "\xF0\x9F\x92\xB5"  // U+1F4B5
#define D_EMOJI_OBJECT_BANKNOTE_YEN_SIGN         "\xF0\x9F\x92\xB4"  // U+1F4B4
#define D_EMOJI_OBJECT_BANKNOTE_EURO_SIGN        "\xF0\x9F\x92\xB6"  // U+1F4B6
#define D_EMOJI_OBJECT_BANKNOTE_POUND_SIGN       "\xF0\x9F\x92\xB7"  // U+1F4B7
#define D_EMOJI_OBJECT_CREDIT_CARD               "\xF0\x9F\x92\xB3"  // U+1F4B3 
#define D_EMOJI_OBJECT_CHART_UPWARDS_TREND       "\xF0\x9F\x93\x88"  // U+1F4C8 
#define D_EMOJI_OBJECT_CHART_DOWNWARDS_TREND     "\xF0\x9F\x93\x89" // U+1F4C9 
// containers/storage
#define D_EMOJI_OBJECT_TOOLBOX                   "\xF0\x9F\xA7\xB0"  // U+1F9F0
#define D_EMOJI_OBJECT_CARDBOARD_BOX             "\xF0\x9F\x93\xA6"  // U+1F4E6
// arts/crafts                                   
#define D_EMOJI_OBJECT_ARTIST_PALETTE            "\xF0\x9F\x8E\xA8"  // U+1F3A8
#define D_EMOJI_OBJECT_THREAD                    "\xF0\x9F\xA7\xB5"  // U+1F9F5
#define D_EMOJI_OBJECT_YARN                      "\xF0\x9F\xA7\xB6"  // U+1F9F6
// 
#define D_EMOJI_PAGE_FACING_UP                   "\xF0\x9F\x93\x84"  // U+1F4C4
#define D_EMOJI_FILE_FOLDER                      "\xF0\x9F\x93\x81"  // U+1F4C1
#define D_EMOJI_PARTY_POPPER                     "\xF0\x9F\x8E\x89"  // U+1F389
#define D_EMOJI_CLIPBOARD                        "\xF0\x9F\x93\x8B"

// ============================================================================
// SYMBOL EMOJIS
// ============================================================================
// hearts/love
#define D_EMOJI_SYMBOL_HEART_RED                 "\xE2\x9D\xA4"      // U+2764 
#define D_EMOJI_SYMBOL_HEART_ORANGE              "\xF0\x9F\xA7\xA1"  // U+1F9E1
#define D_EMOJI_SYMBOL_HEART_YELLOW              "\xF0\x9F\x92\x9B"  // U+1F49B
#define D_EMOJI_SYMBOL_HEART_GREEN               "\xF0\x9F\x92\x9A"  // U+1F49A
#define D_EMOJI_SYMBOL_HEART_BLUE                "\xF0\x9F\x92\x99"  // U+1F499
#define D_EMOJI_SYMBOL_HEART_PURPLE              "\xF0\x9F\x92\x9C"  // U+1F49C
#define D_EMOJI_SYMBOL_HEART_BLACK               "\xF0\x9F\x96\xA4"  // U+1F5A4
#define D_EMOJI_SYMBOL_HEART_WHITE               "\xF0\x9F\xA4\x8D"  // U+1F90D
#define D_EMOJI_SYMBOL_HEART_BROWN               "\xF0\x9F\xA4\x8E"  // U+1F90E
#define D_EMOJI_SYMBOL_HEART_BROKEN              "\xF0\x9F\x92\x94"  // U+1F494
#define D_EMOJI_SYMBOL_HEART_EXCLAMATION         "\xE2\x9D\xA3"      // U+2763 
#define D_EMOJI_SYMBOL_HEARTS_DOUBLE             "\xF0\x9F\x92\x95"  // U+1F495
#define D_EMOJI_SYMBOL_HEART_REVOLVING           "\xF0\x9F\x92\x9E"  // U+1F49E
#define D_EMOJI_SYMBOL_HEART_BEATING             "\xF0\x9F\x92\x93"  // U+1F493
#define D_EMOJI_SYMBOL_HEART_GROWING             "\xF0\x9F\x92\x97"  // U+1F497
#define D_EMOJI_SYMBOL_HEART_SPARKLING           "\xF0\x9F\x92\x96"  // U+1F496
#define D_EMOJI_SYMBOL_HEART_WITH_ARROW          "\xF0\x9F\x92\x98"  // U+1F498
#define D_EMOJI_SYMBOL_HEART_WITH_RIBBON         "\xF0\x9F\x92\x9D"  // U+1F49D
#define D_EMOJI_SYMBOL_HEART_DECORATION          "\xF0\x9F\x92\x9F"  // U+1F49F
// arrows                                        
#define D_EMOJI_SYMBOL_ARROW_LEFT                "\xE2\xAC\x85"      // U+2B05
#define D_EMOJI_SYMBOL_ARROW_UP                  "\xE2\xAC\x86"      // U+2B06
#define D_EMOJI_SYMBOL_ARROW_RIGHT               "\xE2\x9E\xA1"      // U+27A1
#define D_EMOJI_SYMBOL_ARROW_DOWN                "\xE2\xAC\x87"      // U+2B07
#define D_EMOJI_SYMBOL_ARROW_UP_LEFT             "\xE2\x86\x96"      // U+2196
#define D_EMOJI_SYMBOL_ARROW_UP_RIGHT            "\xE2\x86\x97"      // U+2197
#define D_EMOJI_SYMBOL_ARROW_DOWN_RIGHT          "\xE2\x86\x98"      // U+2198
#define D_EMOJI_SYMBOL_ARROW_DOWN_LEFT           "\xE2\x86\x99"      // U+2199
#define D_EMOJI_SYMBOL_ARROW_UP_DOWN             "\xE2\x86\x95"      // U+2195
#define D_EMOJI_SYMBOL_ARROW_LEFT_RIGHT          "\xE2\x86\x94"      // U+2194
#define D_EMOJI_SYMBOL_ARROW_CLOCKWISE           "\xF0\x9F\x94\x83"  // U+1F503
#define D_EMOJI_SYMBOL_ARROW_COUNTERCLOCKWISE    "\xF0\x9F\x94\x84"  // U+1F504
#define D_EMOJI_SYMBOL_ARROW_BACK                "\xF0\x9F\x94\x99"  // U+1F519
#define D_EMOJI_SYMBOL_ARROW_END                 "\xF0\x9F\x94\x9A"  // U+1F51A
#define D_EMOJI_SYMBOL_ARROW_ON                  "\xF0\x9F\x94\x9B"  // U+1F51B
#define D_EMOJI_SYMBOL_ARROW_SOON                "\xF0\x9F\x94\x9C"  // U+1F51C
#define D_EMOJI_SYMBOL_ARROW_TOP                 "\xF0\x9F\x94\x9D"  // U+1F51D
// shapes
#define D_EMOJI_SHAPE_CIRCLE_RED                 "\xF0\x9F\x94\xB4"  // U+1F534
#define D_EMOJI_SHAPE_CIRCLE_ORANGE              "\xF0\x9F\x9F\xA0"  // U+1F7E0
#define D_EMOJI_SHAPE_CIRCLE_YELLOW              "\xF0\x9F\x9F\xA1"  // U+1F7E1
#define D_EMOJI_SHAPE_CIRCLE_GREEN               "\xF0\x9F\x9F\xA2"  // U+1F7E2
#define D_EMOJI_SHAPE_CIRCLE_BLUE                "\xF0\x9F\x94\xB5"  // U+1F535
#define D_EMOJI_SHAPE_CIRCLE_PURPLE              "\xF0\x9F\x9F\xA3"  // U+1F7E3
#define D_EMOJI_SHAPE_CIRCLE_BROWN               "\xF0\x9F\x9F\xA4"  // U+1F7E4
#define D_EMOJI_SHAPE_CIRCLE_BLACK               "\xE2\x9A\xAB"      // U+26AB 
#define D_EMOJI_SHAPE_CIRCLE_WHITE               "\xE2\x9A\xAA"      // U+26AA 
#define D_EMOJI_SHAPE_SQUARE_RED                 "\xF0\x9F\x9F\xA5"  // U+1F7E5
#define D_EMOJI_SHAPE_SQUARE_ORANGE              "\xF0\x9F\x9F\xA7"  // U+1F7E7
#define D_EMOJI_SHAPE_SQUARE_YELLOW              "\xF0\x9F\x9F\xA8"  // U+1F7E8
#define D_EMOJI_SHAPE_SQUARE_GREEN               "\xF0\x9F\x9F\xA9"  // U+1F7E9
#define D_EMOJI_SHAPE_SQUARE_BLUE                "\xF0\x9F\x9F\xA6"  // U+1F7E6
#define D_EMOJI_SHAPE_SQUARE_PURPLE              "\xF0\x9F\x9F\xAA"  // U+1F7EA
#define D_EMOJI_SHAPE_SQUARE_BROWN               "\xF0\x9F\x9F\xAB"  // U+1F7EB
#define D_EMOJI_SHAPE_SQUARE_BLACK_LARGE         "\xE2\xAC\x9B"      // U+2B1B 
#define D_EMOJI_SHAPE_SQUARE_WHITE_LARGE         "\xE2\xAC\x9C"      // U+2B1C 
#define D_EMOJI_SHAPE_SQUARE_BLACK_MEDIUM        "\xE2\x97\xBC"      // U+25FC 
#define D_EMOJI_SHAPE_SQUARE_WHITE_MEDIUM        "\xE2\x97\xBB"      // U+25FB 
#define D_EMOJI_SHAPE_SQUARE_BLACK_SMALL         "\xE2\x96\xAA"      // U+25AA 
#define D_EMOJI_SHAPE_SQUARE_WHITE_SMALL         "\xE2\x96\xAB"      // U+25AB 
#define D_EMOJI_SHAPE_DIAMOND_WITH_A_DOT         "\xF0\x9F\x92\xA0"  // U+1F4A0
#define D_EMOJI_SHAPE_BUTTON_RADIO               "\xF0\x9F\x94\x98"  // U+1F518
#define D_EMOJI_SHAPE_BUTTON_WHITE_SQUARE        "\xF0\x9F\x94\xB3"  // U+1F533
#define D_EMOJI_SHAPE_BUTTON_BLACK_SQUARE        "\xF0\x9F\x94\xB2"  // U+1F532
// mathematical                                  
#define D_EMOJI_MATH_HEAVY_PLUS_SIGN             "\xE2\x9E\x95"      // U+2795
#define D_EMOJI_MATH_HEAVY_MINUS_SIGN            "\xE2\x9E\x96"      // U+2796
#define D_EMOJI_MATH_HEAVY_MULTIPLICATION_X      "\xE2\x9C\x96"      // U+2716
#define D_EMOJI_MATH_HEAVY_DIVISION_SIGN         "\xE2\x9E\x97"      // U+2797
#define D_EMOJI_MATH_INFINITY                    "\xE2\x99\xBE"      // U+267E
// punctuation/symbols
#define D_EMOJI_PUNCTUATION_BANGBANG             "\xE2\x80\xBC"      // U+203C
#define D_EMOJI_PUNCTUATION_INTERROBANG          "\xE2\x81\x89"      // U+2049
#define D_EMOJI_PUNCTUATION_QUESTION             "\xE2\x9D\x93"      // U+2753
#define D_EMOJI_PUNCTUATION_GREY_QUESTION        "\xE2\x9D\x94"      // U+2754
#define D_EMOJI_PUNCTUATION_GREY_EXCLAMATION     "\xE2\x9D\x95"      // U+2755
#define D_EMOJI_PUNCTUATION_EXCLAMATION          "\xE2\x9D\x97"      // U+2757
#define D_EMOJI_PUNCTUATION_WAVY_DASH            "\xE3\x80\xB0"      // U+3030
#define D_EMOJI_PUNCTUATION_CURLY_LOOP           "\xE2\x9E\xB0"      // U+27B0
#define D_EMOJI_PUNCTUATION_LOOP                 "\xE2\x9E\xBF"      // U+27BF
// currency
#define D_EMOJI_DOLLAR_SIGN                      "\xF0\x9F\x92\xB2"  // U+1F4B2
#define D_EMOJI_CURRENCY_EXCHANGE                "\xF0\x9F\x92\xB1"  // U+1F4B1
// zodiac Signs
#define D_EMOJI_ZODIAC_ARIES                     "\xE2\x99\x88"      // U+2648
#define D_EMOJI_ZODIAC_TAURUS                    "\xE2\x99\x89"      // U+2649
#define D_EMOJI_ZODIAC_GEMINI                    "\xE2\x99\x8A"      // U+264A
#define D_EMOJI_ZODIAC_CANCER                    "\xE2\x99\x8B"      // U+264B
#define D_EMOJI_ZODIAC_LEO                       "\xE2\x99\x8C"      // U+264C
#define D_EMOJI_ZODIAC_VIRGO                     "\xE2\x99\x8D"      // U+264D
#define D_EMOJI_ZODIAC_LIBRA                     "\xE2\x99\x8E"      // U+264E
#define D_EMOJI_ZODIAC_SCORPIUS                  "\xE2\x99\x8F"      // U+264F
#define D_EMOJI_ZODIAC_SAGITTARIUS               "\xE2\x99\x90"      // U+2650
#define D_EMOJI_ZODIAC_CAPRICORN                 "\xE2\x99\x91"      // U+2651
#define D_EMOJI_ZODIAC_AQUARIUS                  "\xE2\x99\x92"      // U+2652
#define D_EMOJI_ZODIAC_PISCES                    "\xE2\x99\x93"      // U+2653
#define D_EMOJI_ZODIAC_OPHIUCHUS                 "\xE2\x9B\x8E"      // U+26CE
// religious/spiritual
#define D_EMOJI_RELIGION_LATIN_CROSS             "\xE2\x9C\x9D"      // U+271D
#define D_EMOJI_RELIGION_ORTHODOX_CROSS          "\xE2\x98\xA6"      // U+2626
#define D_EMOJI_RELIGION_STAR_OF_DAVID           "\xE2\x9C\xA1"      // U+2721
#define D_EMOJI_RELIGION_WHEEL_OF_DHARMA         "\xE2\x98\xB8"      // U+2638
#define D_EMOJI_RELIGION_YIN_YANG                "\xE2\x98\xAF"      // U+262F
#define D_EMOJI_RELIGION_PEACE_SYMBOL            "\xE2\x98\xAE"      // U+262E
#define D_EMOJI_RELIGION_MENORAH                 "\xF0\x9F\x95\x8E"  // U+1F54E
#define D_EMOJI_RELIGION_DOTTED_SIX_POINTED_STAR "\xF0\x9F\x94\xAF"  // U+1F52F
// gender/sexuality
#define D_EMOJI_GENDER_FEMALE_SIGN               "\xE2\x99\x80"      // U+2640 
#define D_EMOJI_GENDER_MALE_SIGN                 "\xE2\x99\x82"      // U+2642 
#define D_EMOJI_GENDER_TRANSGENDER_SYMBOL        "\xE2\x9A\xA7"      // U+26A7 
// warning/hazard
#define D_EMOJI_SIGN_WARNING_SIGN                "\xE2\x9A\xA0"      // U+26A0 
#define D_EMOJI_SIGN_CHILDREN_CROSSING           "\xF0\x9F\x9A\xB8"  // U+1F6B8
#define D_EMOJI_SIGN_NO_ENTRY                    "\xE2\x9B\x94"      // U+26D4 
#define D_EMOJI_SIGN_NO_ENTRY_SIGN               "\xF0\x9F\x9A\xAB"  // U+1F6AB
#define D_EMOJI_SIGN_NAME_BADGE                  "\xF0\x9F\x93\x9B"  // U+1F4DB
#define D_EMOJI_SIGN_NO_BICYCLES                 "\xF0\x9F\x9A\xB3"  // U+1F6B3
#define D_EMOJI_SIGN_NO_SMOKING                  "\xF0\x9F\x9A\xAD"  // U+1F6AD
#define D_EMOJI_SIGN_NO_LITTERING                "\xF0\x9F\x9A\xAF"  // U+1F6AF
#define D_EMOJI_SIGN_NON_POTABLE_WATER           "\xF0\x9F\x9A\xB1"  // U+1F6B1
#define D_EMOJI_SIGN_NO_PEDESTRIANS              "\xF0\x9F\x9A\xB7"  // U+1F6B7
#define D_EMOJI_SIGN_NO_MOBILE_PHONES            "\xF0\x9F\x93\xB5"  // U+1F4F5
#define D_EMOJI_SIGN_NO_ONE_UNDER_EIGHTEEN       "\xF0\x9F\x94\x9E"  // U+1F51E
#define D_EMOJI_SIGN_RADIOACTIVE                 "\xE2\x98\xA2"      // U+2622 
#define D_EMOJI_SIGN_BIOHAZARD                   "\xE2\x98\xA3"      // U+2623 
// recycling/environmental
#define D_EMOJI_SYMBOL_RECYCLING_SYMBOL          "\xE2\x99\xBB"      // U+267B 
#define D_EMOJI_SYMBOL_TRIDENT_EMBLEM            "\xF0\x9F\x94\xB1"  // U+1F531
// music controls
#define D_EMOJI_CONTROL_BUTTON_PLAY              "\xE2\x96\xB6"      // U+25B6
#define D_EMOJI_CONTROL_BUTTON_PAUSE             "\xE2\x8F\xB8"      // U+23F8
#define D_EMOJI_CONTROL_BUTTON_STOP              "\xE2\x8F\xB9"      // U+23F9
#define D_EMOJI_CONTROL_BUTTON_RECORD            "\xE2\x8F\xBA"      // U+23FA
#define D_EMOJI_CONTROL_BUTTON_NEXT_TRACK        "\xE2\x8F\xAD"      // U+23ED
#define D_EMOJI_CONTROL_BUTTON_PREVIOUS_TRACK    "\xE2\x8F\xAE"      // U+23EE
#define D_EMOJI_CONTROL_BUTTON_FAST_FORWARD      "\xE2\x8F\xA9"      // U+23E9
#define D_EMOJI_CONTROL_BUTTON_FAST_REVERSE      "\xE2\x8F\xAA"      // U+23EA
#define D_EMOJI_CONTROL_BUTTON_FAST_UP           "\xE2\x8F\xAB"      // U+23EB
#define D_EMOJI_CONTROL_BUTTON_FAST_DOWN         "\xE2\x8F\xAC"      // U+23EC
// checkmarks/crosses
#define D_EMOJI_SYMBOL_CHECK_MARK_BUTTON         "\xE2\x9C\x85"      // U+2705
#define D_EMOJI_SYMBOL_HEAVY_CHECK_MARK          "\xE2\x9C\x94"      // U+2714
#define D_EMOJI_SYMBOL_X                         "\xE2\x9D\x8C"      // U+274C
#define D_EMOJI_SYMBOL_CROSS_MARK_HEAVY          "\xE2\x9D\x8E"      // U+274E
#define D_EMOJI_INFORMATION_SOURCE               "\xE2\x84\xB9\xEF\xB8\x8F"  // U+2139 + FE0F
// other symbols
#define D_EMOJI_SYMBOL_ATOM_SYMBOL               "\xE2\x9A\x9B"      // U+269B
#define D_EMOJI_SYMBOL_FLEUR_DE_LIS              "\xE2\x9A\x9C"      // U+269C
#define D_EMOJI_SYMBOL_PART_ALTERNATION_MARK     "\xE3\x80\xBD"      // U+303D
#define D_EMOJI_SYMBOL_EIGHT_SPOKED_ASTERISK     "\xE2\x9C\xB3"      // U+2733
#define D_EMOJI_SYMBOL_EIGHT_POINTED_STAR        "\xE2\x9C\xB4"      // U+2734
#define D_EMOJI_SYMBOL_SPARKLE                   "\xE2\x9D\x87"      // U+2747
#define D_EMOJI_SYMBOL_DOUBLE_EXCLAMATION_MARK   "\xE2\x80\xBC"      // U+203C
#define D_EMOJI_SYMBOL_FIGHT_CLOUD               "\xF0\x9F\xAB\xAF"  // U+1FAEF

// ============================================================================
// emoji bytes (ASCII-only)
// ============================================================================
// keycap pieces + semantic builder
#define D_EMOJI_VS16            D_UNICODE_VS16                       // U+FE0F
#define D_EMOJI_KEYCAP_SUFFIX   D_UNICODE_KEYCAP_SUFFIX              // U+20E3
#define D_EMOJI_ZWJ             D_UNICODE_ZWJ                        // U+200D

// keycap base codepoints (ASCII digits, #, *)
#define D_U0023                  "\x23"
#define D_U002A                  "\x2A"
#define D_U0030                  "\x30"
#define D_U0031                  "\x31"
#define D_U0032                  "\x32"
#define D_U0033                  "\x33"
#define D_U0034                  "\x34"
#define D_U0035                  "\x35"
#define D_U0036                  "\x36"
#define D_U0037                  "\x37"
#define D_U0038                  "\x38"
#define D_U0039                  "\x39"

// D_EMOJI_KEYCAP
//   macro: 
#define D_EMOJI_KEYCAP(base)                    \
    (base D_EMOJI_VS16 D_EMOJI_KEYCAP_SUFFIX)

// keycap emojis (semantic use of builder)
#define D_EMOJI_KEYCAP_HASH         \
    D_EMOJI_KEYCAP(D_U0023)

#define D_EMOJI_KEYCAP_STAR         \
    D_EMOJI_KEYCAP(D_U002A)

#define D_EMOJI_KEYCAP_0            \
    D_EMOJI_KEYCAP(D_U0030)

#define D_EMOJI_KEYCAP_1            \
    D_EMOJI_KEYCAP(D_U0031)

#define D_EMOJI_KEYCAP_2            \
    D_EMOJI_KEYCAP(D_U0032)

#define D_EMOJI_KEYCAP_3            \
    D_EMOJI_KEYCAP(D_U0033)

#define D_EMOJI_KEYCAP_4            \
    D_EMOJI_KEYCAP(D_U0034)

#define D_EMOJI_KEYCAP_5            \
    D_EMOJI_KEYCAP(D_U0035)

#define D_EMOJI_KEYCAP_6            \
    D_EMOJI_KEYCAP(D_U0036)

#define D_EMOJI_KEYCAP_7            \
    D_EMOJI_KEYCAP(D_U0037)

#define D_EMOJI_KEYCAP_8            \
    D_EMOJI_KEYCAP(D_U0038)

#define D_EMOJI_KEYCAP_9            \
    D_EMOJI_KEYCAP(D_U0039)

// single codepoint symbols
#define D_EMOJI_COPYRIGHT          "\xC2\xA9"            // U+00A9 
#define D_EMOJI_REGISTERED         "\xC2\xAE"            // U+00AE 
#define D_EMOJI_MAHJONG_RED_DRAGON "\xF0\x9F\x80\x84"    // U+1F004
#define D_EMOJI_BLACK_JOKER        "\xF0\x9F\x83\x8F"    // U+1F0CF

// negative squared letters/word buttons
#define D_EMOJI_NEG_SQUARED_A      "\xF0\x9F\x85\xB0"    // U+1F170
#define D_EMOJI_NEG_SQUARED_B      "\xF0\x9F\x85\xB1"    // U+1F171
#define D_EMOJI_NEG_SQUARED_O      "\xF0\x9F\x85\xBE"    // U+1F17E
#define D_EMOJI_NEG_SQUARED_P      "\xF0\x9F\x85\xBF"    // U+1F17F
#define D_EMOJI_NEG_SQUARED_AB     "\xF0\x9F\x86\x8E"    // U+1F18E
                                   
#define D_EMOJI_SQUARED_CL         "\xF0\x9F\x86\x91"    // U+1F191
#define D_EMOJI_SQUARED_COOL       "\xF0\x9F\x86\x92"    // U+1F192
#define D_EMOJI_SQUARED_FREE       "\xF0\x9F\x86\x93"    // U+1F193
#define D_EMOJI_SQUARED_ID         "\xF0\x9F\x86\x94"    // U+1F194
#define D_EMOJI_SQUARED_NEW        "\xF0\x9F\x86\x95"    // U+1F195
#define D_EMOJI_SQUARED_NG         "\xF0\x9F\x86\x96"    // U+1F196
#define D_EMOJI_SQUARED_OK         "\xF0\x9F\x86\x97"    // U+1F197
#define D_EMOJI_SQUARED_SOS        "\xF0\x9F\x86\x98"    // U+1F198
#define D_EMOJI_SQUARED_UP_EXCL    "\xF0\x9F\x86\x99"    // U+1F199
#define D_EMOJI_SQUARED_VS         "\xF0\x9F\x86\x9A"    // U+1F19A


// ============================================================================
// FLAGS
// ============================================================================

// D_EMOJI_FLAG
//   macro: 
#define D_EMOJI_FLAG(L1, L2)  \
    (L1 L2)

// flags (pairs exactly as listed)
// A
//
//
#define D_EMOJI_FLAG_ASCENSION_ISLAND                   \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_C)  // AC

//
//
#define D_EMOJI_FLAG_ANDORRA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_D)  // AD

//
//
#define D_EMOJI_FLAG_UNITED_ARAB_EMIRATES               \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_E)  // AE

//
//
#define D_EMOJI_FLAG_AFGHANISTAN                        \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_F)  // AF

//
//
#define D_EMOJI_FLAG_ANTIGUA_AND_BARBUDA                \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_G)  // AG

//
//
#define D_EMOJI_FLAG_ANGUILLA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_I)  // AI

//
//
#define D_EMOJI_FLAG_ALBANIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_L)  // AL

//
//
#define D_EMOJI_FLAG_ARMENIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_M)  // AM

//
//
#define D_EMOJI_FLAG_ANGOLA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_O)  // AO

//
//
#define D_EMOJI_FLAG_ANTARCTICA                         \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_Q)  // AQ

//
//
#define D_EMOJI_FLAG_ARGENTINA                          \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_R)  // AR

//
//
#define D_EMOJI_FLAG_AMERICAN_SAMOA                     \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_S)  // AS

//
//
#define D_EMOJI_FLAG_AUSTRIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_T)  // AT

//
//
#define D_EMOJI_FLAG_AUSTRALIA                          \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_U)  // AU

//
//
#define D_EMOJI_FLAG_ARUBA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_W)  // AW

//
//
#define D_EMOJI_FLAG_ALAND_ISLANDS                      \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_X)  // AX

//
//
#define D_EMOJI_FLAG_AZERBAIJAN                         \
    D_EMOJI_FLAG(D_UNICODE_RI_A, D_UNICODE_RI_Z)  // AZ

// B
//
//
#define D_EMOJI_FLAG_BOSNIA_AND_HERZEGOVINA             \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_A)  // BA

//
//
#define D_EMOJI_FLAG_BARBADOS                           \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_B)  // BB

//
//
#define D_EMOJI_FLAG_BANGLADESH                         \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_D)  // BD

//
//
#define D_EMOJI_FLAG_BELGIUM                            \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_E)  // BE

//
//
#define D_EMOJI_FLAG_BURKINA_FASO                       \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_F)  // BF

//
//
#define D_EMOJI_FLAG_BULGARIA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_G)  // BG

//
//
#define D_EMOJI_FLAG_BAHRAIN                            \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_H)  // BH

//
//
#define D_EMOJI_FLAG_BURUNDI                            \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_I)  // BI

//
//
#define D_EMOJI_FLAG_BENIN                              \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_J)  // BJ

//
//
#define D_EMOJI_FLAG_SAINT_BARTHELEMY                   \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_L)  // BL

//
//
#define D_EMOJI_FLAG_BERMUDA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_M)  // BM

//
//
#define D_EMOJI_FLAG_BRUNEI                             \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_N)  // BN

//
//
#define D_EMOJI_FLAG_BOLIVIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_O)  // BO

//
//
#define D_EMOJI_FLAG_CARIBBEAN_NETHERLANDS              \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_Q)  // BQ

//
//
#define D_EMOJI_FLAG_BRAZIL                             \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_R)  // BR

//
//
#define D_EMOJI_FLAG_BAHAMAS                            \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_S)  // BS

//
//
#define D_EMOJI_FLAG_BHUTAN                             \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_T)  // BT

//
//
#define D_EMOJI_FLAG_BOUVET_ISLAND                      \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_V)  // BV

//
//
#define D_EMOJI_FLAG_BOTSWANA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_W)  // BW

//
//
#define D_EMOJI_FLAG_BELARUS                            \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_Y)  // BY

//
//
#define D_EMOJI_FLAG_BELIZE                             \
    D_EMOJI_FLAG(D_UNICODE_RI_B, D_UNICODE_RI_Z)  // BZ

// C                                        
//
//
#define D_EMOJI_FLAG_CANADA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_A)  // CA

//
//
#define D_EMOJI_FLAG_COCOS_ISLANDS                      \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_C)  // CC

//
//
#define D_EMOJI_FLAG_CONGO_KINSHASA                     \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_D)  // CD

//
//
#define D_EMOJI_FLAG_CENTRAL_AFRICAN_REPUBLIC           \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_F)  // CF

//
//
#define D_EMOJI_FLAG_CONGO_BRAZZAVILLE                  \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_G)  // CG

//
//
#define D_EMOJI_FLAG_SWITZERLAND                        \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_H)  // CH

//
//
#define D_EMOJI_FLAG_COTE_DIVOIRE                       \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_I)  // CI

//
//
#define D_EMOJI_FLAG_COOK_ISLANDS                       \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_K)  // CK

//
//
#define D_EMOJI_FLAG_CHILE                              \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_L)  // CL

//
//
#define D_EMOJI_FLAG_CAMEROON                           \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_M)  // CM

//
//
#define D_EMOJI_FLAG_CHINA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_N)  // CN

//
//
#define D_EMOJI_FLAG_COLOMBIA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_O)  // CO

//
//
#define D_EMOJI_FLAG_CLIPPERTON_ISLAND                  \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_P)  // CP

//
//
#define D_EMOJI_FLAG_COSTA_RICA                         \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_R)  // CR

//
//
#define D_EMOJI_FLAG_CUBA                               \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_U)  // CU

//
//
#define D_EMOJI_FLAG_CAPE_VERDE                         \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_V)  // CV

//
//
#define D_EMOJI_FLAG_CURACAO                            \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_W)  // CW

//
//
#define D_EMOJI_FLAG_CHRISTMAS_ISLAND                   \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_X)  // CX

//
//
#define D_EMOJI_FLAG_CYPRUS                             \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_Y)  // CY

//
//
#define D_EMOJI_FLAG_CZECHIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_C, D_UNICODE_RI_Z)  // CZ

// D

//
//
#define D_EMOJI_FLAG_GERMANY                            \
    D_EMOJI_FLAG(D_UNICODE_RI_D, D_UNICODE_RI_E)  // DE

//
//
#define D_EMOJI_FLAG_DIEGO_GARCIA                       \
    D_EMOJI_FLAG(D_UNICODE_RI_D, D_UNICODE_RI_G)  // DG

//
//
#define D_EMOJI_FLAG_DJIBOUTI                           \
    D_EMOJI_FLAG(D_UNICODE_RI_D, D_UNICODE_RI_J)  // DJ

//
//
#define D_EMOJI_FLAG_DENMARK                            \
    D_EMOJI_FLAG(D_UNICODE_RI_D, D_UNICODE_RI_K)  // DK

//
//
#define D_EMOJI_FLAG_DOMINICA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_D, D_UNICODE_RI_M)  // DM

//
//
#define D_EMOJI_FLAG_DOMINICAN_REPUBLIC                 \
    D_EMOJI_FLAG(D_UNICODE_RI_D, D_UNICODE_RI_O)  // DO

//
//
#define D_EMOJI_FLAG_ALGERIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_D, D_UNICODE_RI_Z)  // DZ

// E                                        
//
//
#define D_EMOJI_FLAG_CEUTA_AND_MELILLA                  \
    D_EMOJI_FLAG(D_UNICODE_RI_E, D_UNICODE_RI_A)  // EA

//
//
#define D_EMOJI_FLAG_ECUADOR                            \
    D_EMOJI_FLAG(D_UNICODE_RI_E, D_UNICODE_RI_C)  // EC

//
//
#define D_EMOJI_FLAG_ESTONIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_E, D_UNICODE_RI_E)  // EE

//
//
#define D_EMOJI_FLAG_EGYPT                              \
    D_EMOJI_FLAG(D_UNICODE_RI_E, D_UNICODE_RI_G)  // EG

//
//
#define D_EMOJI_FLAG_WESTERN_SAHARA                     \
    D_EMOJI_FLAG(D_UNICODE_RI_E, D_UNICODE_RI_H)  // EH

//
//
#define D_EMOJI_FLAG_ERITREA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_E, D_UNICODE_RI_R)  // ER

//
//
#define D_EMOJI_FLAG_SPAIN                              \
    D_EMOJI_FLAG(D_UNICODE_RI_E, D_UNICODE_RI_S)  // ES

//
//
#define D_EMOJI_FLAG_ETHIOPIA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_E, D_UNICODE_RI_T)  // ET

//
//
#define D_EMOJI_FLAG_EUROPEAN_UNION                     \
    D_EMOJI_FLAG(D_UNICODE_RI_E, D_UNICODE_RI_U)  // EU

// F                                        
//
//
#define D_EMOJI_FLAG_FINLAND                            \
    D_EMOJI_FLAG(D_UNICODE_RI_F, D_UNICODE_RI_I)  // FI

//
//
#define D_EMOJI_FLAG_FIJI                               \
    D_EMOJI_FLAG(D_UNICODE_RI_F, D_UNICODE_RI_J)  // FJ

//
//
#define D_EMOJI_FLAG_FALKLAND_ISLANDS                   \
    D_EMOJI_FLAG(D_UNICODE_RI_F, D_UNICODE_RI_K)  // FK

//
//
#define D_EMOJI_FLAG_MICRONESIA                         \
    D_EMOJI_FLAG(D_UNICODE_RI_F, D_UNICODE_RI_M)  // FM

//
//
#define D_EMOJI_FLAG_FAROE_ISLANDS                      \
    D_EMOJI_FLAG(D_UNICODE_RI_F, D_UNICODE_RI_O)  // FO

//
//
#define D_EMOJI_FLAG_FRANCE                             \
    D_EMOJI_FLAG(D_UNICODE_RI_F, D_UNICODE_RI_R)  // FR

// G                                        
//
//
#define D_EMOJI_FLAG_GABON                              \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_A)  // GA

//
//
#define D_EMOJI_FLAG_UNITED_KINGDOM                     \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_B)  // GB

//
//
#define D_EMOJI_FLAG_GRENADA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_D)  // GD

//
//
#define D_EMOJI_FLAG_GEORGIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_E)  // GE

//
//
#define D_EMOJI_FLAG_FRENCH_GUIANA                      \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_F)  // GF

//
//
#define D_EMOJI_FLAG_GUERNSEY                           \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_G)  // GG

//
//
#define D_EMOJI_FLAG_GHANA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_H)  // GH

//
//
#define D_EMOJI_FLAG_GIBRALTAR                          \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_I)  // GI

//
//
#define D_EMOJI_FLAG_GREENLAND                          \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_L)  // GL

//
//
#define D_EMOJI_FLAG_GAMBIA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_M)  // GM

//
//
#define D_EMOJI_FLAG_GUINEA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_N)  // GN

//
//
#define D_EMOJI_FLAG_GUADELOUPE                         \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_P)  // GP

//
//
#define D_EMOJI_FLAG_EQUATORIAL_GUINEA                  \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_Q)  // GQ

//
//
#define D_EMOJI_FLAG_GREECE                             \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_R)  // GR

//
//
#define D_EMOJI_FLAG_SOUTH_GEORGIA_SANDWICH_ISLANDS     \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_S)  // GS

//
//
#define D_EMOJI_FLAG_GUATEMALA                          \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_T)  // GT

//
//
#define D_EMOJI_FLAG_GUAM                               \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_U)  // GU

//
//
#define D_EMOJI_FLAG_GUINEA_BISSAU                      \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_W)  // GW

//
//
#define D_EMOJI_FLAG_GUYANA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_G, D_UNICODE_RI_Y)  // GY

// H
//
//
#define D_EMOJI_FLAG_HONG_KONG                          \
    D_EMOJI_FLAG(D_UNICODE_RI_H, D_UNICODE_RI_K)  // HK

//
//
#define D_EMOJI_FLAG_HEARD_MCDONALD_ISLANDS             \
    D_EMOJI_FLAG(D_UNICODE_RI_H, D_UNICODE_RI_M)  // HM

//
//
#define D_EMOJI_FLAG_HONDURAS                           \
    D_EMOJI_FLAG(D_UNICODE_RI_H, D_UNICODE_RI_N)  // HN

//
//
#define D_EMOJI_FLAG_CROATIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_H, D_UNICODE_RI_R)  // HR

//
//
#define D_EMOJI_FLAG_HAITI                              \
    D_EMOJI_FLAG(D_UNICODE_RI_H, D_UNICODE_RI_T)  // HT

//
//
#define D_EMOJI_FLAG_HUNGARY                            \
    D_EMOJI_FLAG(D_UNICODE_RI_H, D_UNICODE_RI_U)  // HU

// I
//
//
#define D_EMOJI_FLAG_CANARY_ISLANDS                     \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_C)  // IC

//
//
#define D_EMOJI_FLAG_INDONESIA                          \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_D)  // ID

//
//
#define D_EMOJI_FLAG_IRELAND                            \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_E)  // IE

//
//
#define D_EMOJI_FLAG_ISRAEL                             \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_L)  // IL

//
//
#define D_EMOJI_FLAG_ISLE_OF_MAN                        \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_M)  // IM

//
//
#define D_EMOJI_FLAG_INDIA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_N)  // IN

//
//
#define D_EMOJI_FLAG_BRITISH_INDIAN_OCEAN_TERRITORY     \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_O)  // IO

//
//
#define D_EMOJI_FLAG_IRAQ                               \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_Q)  // IQ

//
//
#define D_EMOJI_FLAG_IRAN                               \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_R)  // IR

//
//
#define D_EMOJI_FLAG_ICELAND                            \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_S)  // IS

//
//
#define D_EMOJI_FLAG_ITALY                              \
    D_EMOJI_FLAG(D_UNICODE_RI_I, D_UNICODE_RI_T)  // IT

// J                                        
//
//
#define D_EMOJI_FLAG_JERSEY                             \
    D_EMOJI_FLAG(D_UNICODE_RI_J, D_UNICODE_RI_E)  // JE

//
//
#define D_EMOJI_FLAG_JAMAICA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_J, D_UNICODE_RI_M)  // JM

//
//
#define D_EMOJI_FLAG_JORDAN                             \
    D_EMOJI_FLAG(D_UNICODE_RI_J, D_UNICODE_RI_O)  // JO

//
//
#define D_EMOJI_FLAG_JAPAN                              \
    D_EMOJI_FLAG(D_UNICODE_RI_J, D_UNICODE_RI_P)  // JP

// K                                        
//
//
#define D_EMOJI_FLAG_KENYA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_E)  // KE

//
//
#define D_EMOJI_FLAG_KYRGYZSTAN                         \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_G)  // KG

//
//
#define D_EMOJI_FLAG_CAMBODIA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_H)  // KH

//
//
#define D_EMOJI_FLAG_KIRIBATI                           \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_I)  // KI

//
//
#define D_EMOJI_FLAG_COMOROS                            \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_M)  // KM

//
//
#define D_EMOJI_FLAG_SAINT_KITTS_AND_NEVIS              \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_N)  // KN

//
//
#define D_EMOJI_FLAG_NORTH_KOREA                        \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_P)  // KP

//
//
#define D_EMOJI_FLAG_SOUTH_KOREA                        \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_R)  // KR

//
//
#define D_EMOJI_FLAG_KUWAIT                             \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_W)  // KW

//
//
#define D_EMOJI_FLAG_CAYMAN_ISLANDS                     \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_Y)  // KY

//
//
#define D_EMOJI_FLAG_KAZAKHSTAN                         \
    D_EMOJI_FLAG(D_UNICODE_RI_K, D_UNICODE_RI_Z)  // KZ

// L
//
//
#define D_EMOJI_FLAG_LEBANON                            \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_B)  // LB

//
//
#define D_EMOJI_FLAG_SAINT_LUCIA                        \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_C)  // LC

//
//
#define D_EMOJI_FLAG_LIECHTENSTEIN                      \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_I)  // LI

//
//
#define D_EMOJI_FLAG_SRI_LANKA                          \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_K)  // LK

//
//
#define D_EMOJI_FLAG_LIBERIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_R)  // LR

//
//
#define D_EMOJI_FLAG_LESOTHO                            \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_S)  // LS

//
//
#define D_EMOJI_FLAG_LITHUANIA                          \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_T)  // LT

//
//
#define D_EMOJI_FLAG_LUXEMBOURG                         \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_U)  // LU

//
//
#define D_EMOJI_FLAG_LATVIA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_V)  // LV

//
//
#define D_EMOJI_FLAG_LIBYA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_L, D_UNICODE_RI_Y)  // LY

// M
//
//
#define D_EMOJI_FLAG_MOROCCO                            \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_A)  // MA

//
//
#define D_EMOJI_FLAG_MONACO                             \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_C)  // MC

//
//
#define D_EMOJI_FLAG_MOLDOVA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_D)  // MD

//
//
#define D_EMOJI_FLAG_MONTENEGRO                         \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_E)  // ME

//
//
#define D_EMOJI_FLAG_SAINT_MARTIN                       \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_F)  // MF

//
//
#define D_EMOJI_FLAG_MADAGASCAR                         \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_G)  // MG

//
//
#define D_EMOJI_FLAG_MARSHALL_ISLANDS                   \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_H)  // MH

//
//
#define D_EMOJI_FLAG_NORTH_MACEDONIA                    \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_K)  // MK

//
//
#define D_EMOJI_FLAG_MALI                               \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_L)  // ML

//
//
#define D_EMOJI_FLAG_MYANMAR                            \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_M)  // MM

//
//
#define D_EMOJI_FLAG_MONGOLIA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_N)  // MN

//
//
#define D_EMOJI_FLAG_MACAO                              \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_O)  // MO

//
//
#define D_EMOJI_FLAG_NORTHERN_MARIANA_ISLANDS           \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_P)  // MP

//
//
#define D_EMOJI_FLAG_MARTINIQUE                         \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_Q)  // MQ

//
//
#define D_EMOJI_FLAG_MAURITANIA                         \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_R)  // MR

//
//
#define D_EMOJI_FLAG_MONTSERRAT                         \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_S)  // MS

//
//
#define D_EMOJI_FLAG_MALTA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_T)  // MT

//
//
#define D_EMOJI_FLAG_MAURITIUS                          \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_U)  // MU

//
//
#define D_EMOJI_FLAG_MALDIVES                           \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_V)  // MV

//
//
#define D_EMOJI_FLAG_MALAWI                             \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_W)  // MW

//
//
#define D_EMOJI_FLAG_MEXICO                             \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_X)  // MX

//
//
#define D_EMOJI_FLAG_MALAYSIA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_Y)  // MY

//
//
#define D_EMOJI_FLAG_MOZAMBIQUE                         \
    D_EMOJI_FLAG(D_UNICODE_RI_M, D_UNICODE_RI_Z)  // MZ

// N                                        
//
//
#define D_EMOJI_FLAG_NEW_CALEDONIA                      \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_C)  // NC

//
//
#define D_EMOJI_FLAG_NIGER                              \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_E)  // NE

//
//
#define D_EMOJI_FLAG_NORFOLK_ISLAND                     \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_F)  // NF

//
//
#define D_EMOJI_FLAG_NIGERIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_G)  // NG

//
//
#define D_EMOJI_FLAG_NICARAGUA                          \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_I)  // NI

//
//
#define D_EMOJI_FLAG_NETHERLANDS                        \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_L)  // NL

//
//
#define D_EMOJI_FLAG_NORWAY                             \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_O)  // NO

//
//
#define D_EMOJI_FLAG_NEPAL                              \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_P)  // NP

//
//
#define D_EMOJI_FLAG_NAURU                              \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_R)  // NR

//
//
#define D_EMOJI_FLAG_NIUE                               \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_U)  // NU

//
//
#define D_EMOJI_FLAG_NEW_ZEALAND                        \
    D_EMOJI_FLAG(D_UNICODE_RI_N, D_UNICODE_RI_Z)  // NZ

// O
//
//
#define D_EMOJI_FLAG_OMAN                               \
    D_EMOJI_FLAG(D_UNICODE_RI_O, D_UNICODE_RI_M)  // OM

// P
//
//
#define D_EMOJI_FLAG_PANAMA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_A)  // PA

//
//
#define D_EMOJI_FLAG_PERU                               \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_E)  // PE

//
//
#define D_EMOJI_FLAG_FRENCH_POLYNESIA                   \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_F)  // PF

//
//
#define D_EMOJI_FLAG_PAPUA_NEW_GUINEA                   \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_G)  // PG

//
//
#define D_EMOJI_FLAG_PHILIPPINES                        \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_H)  // PH

//
//
#define D_EMOJI_FLAG_PAKISTAN                           \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_K)  // PK

//
//
#define D_EMOJI_FLAG_POLAND                             \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_L)  // PL

//
//
#define D_EMOJI_FLAG_SAINT_PIERRE_MIQUELON              \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_M)  // PM

//
//
#define D_EMOJI_FLAG_PITCAIRN_ISLANDS                   \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_N)  // PN

//
//
#define D_EMOJI_FLAG_PUERTO_RICO                        \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_R)  // PR

//
//
#define D_EMOJI_FLAG_PALESTINIAN_TERRITORIES            \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_S)  // PS

//
//
#define D_EMOJI_FLAG_PORTUGAL                           \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_T)  // PT

//
//
#define D_EMOJI_FLAG_PALAU                              \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_W)  // PW

//
//
#define D_EMOJI_FLAG_PARAGUAY                           \
    D_EMOJI_FLAG(D_UNICODE_RI_P, D_UNICODE_RI_Y)  // PY

// Q
//
//
#define D_EMOJI_FLAG_QATAR                              \
    D_EMOJI_FLAG(D_UNICODE_RI_Q, D_UNICODE_RI_A)  // QA

// R
//
//
#define D_EMOJI_FLAG_REUNION                            \
    D_EMOJI_FLAG(D_UNICODE_RI_R, D_UNICODE_RI_E)  // RE

//
//
#define D_EMOJI_FLAG_ROMANIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_R, D_UNICODE_RI_O)  // RO

//
//
#define D_EMOJI_FLAG_SERBIA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_R, D_UNICODE_RI_S)  // RS

//
//
#define D_EMOJI_FLAG_RUSSIA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_R, D_UNICODE_RI_U)  // RU

//
//
#define D_EMOJI_FLAG_RWANDA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_R, D_UNICODE_RI_W)  // RW

// S                                        
//
//
#define D_EMOJI_FLAG_SAUDI_ARABIA                       \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_A)  // SA

//
//
#define D_EMOJI_FLAG_SOLOMON_ISLANDS                    \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_B)  // SB

//
//
#define D_EMOJI_FLAG_SEYCHELLES                         \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_C)  // SC

//
//
#define D_EMOJI_FLAG_SUDAN                              \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_D)  // SD

//
//
#define D_EMOJI_FLAG_SWEDEN                             \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_E)  // SE

//
//
#define D_EMOJI_FLAG_SINGAPORE                          \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_G)  // SG

//
//
#define D_EMOJI_FLAG_SAINT_HELENA                       \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_H)  // SH

//
//
#define D_EMOJI_FLAG_SLOVENIA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_I)  // SI

//
//
#define D_EMOJI_FLAG_SVALBARD_JAN_MAYEN                 \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_J)  // SJ

//
//
#define D_EMOJI_FLAG_SLOVAKIA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_K)  // SK

//
//
#define D_EMOJI_FLAG_SIERRA_LEONE                       \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_L)  // SL

//
//
#define D_EMOJI_FLAG_SAN_MARINO                         \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_M)  // SM

//
//
#define D_EMOJI_FLAG_SENEGAL                            \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_N)  // SN

//
//
#define D_EMOJI_FLAG_SOMALIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_O)  // SO

//
//
#define D_EMOJI_FLAG_SURINAME                           \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_R)  // SR

//
//
#define D_EMOJI_FLAG_SOUTH_SUDAN                        \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_S)  // SS

//
//
#define D_EMOJI_FLAG_SAO_TOME_PRINCIPE                  \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_T)  // ST

//
//
#define D_EMOJI_FLAG_EL_SALVADOR                        \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_V)  // SV

//
//
#define D_EMOJI_FLAG_SINT_MAARTEN                       \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_X)  // SX

//
//
#define D_EMOJI_FLAG_SYRIA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_Y)  // SY

//
//
#define D_EMOJI_FLAG_ESWATINI                           \
    D_EMOJI_FLAG(D_UNICODE_RI_S, D_UNICODE_RI_Z)  // SZ

// T                                        
//
//
#define D_EMOJI_FLAG_TRISTAN_DA_CUNHA                   \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_A)  // TA

//
//
#define D_EMOJI_FLAG_TURKS_CAICOS_ISLANDS               \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_C)  // TC

//
//
#define D_EMOJI_FLAG_CHAD                               \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_D)  // TD

//
//
#define D_EMOJI_FLAG_FRENCH_SOUTHERN_TERRITORIES        \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_F)  // TF

//
//
#define D_EMOJI_FLAG_TOGO                               \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_G)  // TG

//
//
#define D_EMOJI_FLAG_THAILAND                           \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_H)  // TH

//
//
#define D_EMOJI_FLAG_TAJIKISTAN                         \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_J)  // TJ

//
//
#define D_EMOJI_FLAG_TOKELAU                            \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_K)  // TK

//
//
#define D_EMOJI_FLAG_TIMOR_LESTE                        \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_L)  // TL

//
//
#define D_EMOJI_FLAG_TURKMENISTAN                       \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_M)  // TM

//
//
#define D_EMOJI_FLAG_TUNISIA                            \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_N)  // TN

//
//
#define D_EMOJI_FLAG_TONGA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_O)  // TO

//
//
#define D_EMOJI_FLAG_TURKEY                             \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_R)  // TR

//
//
#define D_EMOJI_FLAG_TRINIDAD_AND_TOBAGO                \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_T)  // TT

//
//
#define D_EMOJI_FLAG_TUVALU                             \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_V)  // TV

//
//
#define D_EMOJI_FLAG_TAIWAN                             \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_W)  // TW

//
//
#define D_EMOJI_FLAG_TANZANIA                           \
    D_EMOJI_FLAG(D_UNICODE_RI_T, D_UNICODE_RI_Z)  // TZ

// U
//
//
#define D_EMOJI_FLAG_UKRAINE                            \
    D_EMOJI_FLAG(D_UNICODE_RI_U, D_UNICODE_RI_A)  // UA

//
//
#define D_EMOJI_FLAG_UGANDA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_U, D_UNICODE_RI_G)  // UG

//
//
#define D_EMOJI_FLAG_US_OUTLYING_ISLANDS                \
    D_EMOJI_FLAG(D_UNICODE_RI_U, D_UNICODE_RI_M)  // UM

//
//
#define D_EMOJI_FLAG_UNITED_NATIONS                     \
    D_EMOJI_FLAG(D_UNICODE_RI_U, D_UNICODE_RI_N)  // UN

//
//
#define D_EMOJI_FLAG_UNITED_STATES                      \
    D_EMOJI_FLAG(D_UNICODE_RI_U, D_UNICODE_RI_S)  // US

//
//
#define D_EMOJI_FLAG_URUGUAY                            \
    D_EMOJI_FLAG(D_UNICODE_RI_U, D_UNICODE_RI_Y)  // UY

//
//
#define D_EMOJI_FLAG_UZBEKISTAN                         \
    D_EMOJI_FLAG(D_UNICODE_RI_U, D_UNICODE_RI_Z)  // UZ

// V
//
//
#define D_EMOJI_FLAG_VATICAN_CITY                       \
    D_EMOJI_FLAG(D_UNICODE_RI_V, D_UNICODE_RI_A)  // VA

//
//
#define D_EMOJI_FLAG_SAINT_VINCENT_GRENADINES           \
    D_EMOJI_FLAG(D_UNICODE_RI_V, D_UNICODE_RI_C)  // VC

//
//
#define D_EMOJI_FLAG_VENEZUELA                          \
    D_EMOJI_FLAG(D_UNICODE_RI_V, D_UNICODE_RI_E)  // VE

//
//
#define D_EMOJI_FLAG_BRITISH_VIRGIN_ISLANDS             \
    D_EMOJI_FLAG(D_UNICODE_RI_V, D_UNICODE_RI_G)  // VG

//
//
#define D_EMOJI_FLAG_US_VIRGIN_ISLANDS                  \
    D_EMOJI_FLAG(D_UNICODE_RI_V, D_UNICODE_RI_I)  // VI

//
//
#define D_EMOJI_FLAG_VIETNAM                            \
    D_EMOJI_FLAG(D_UNICODE_RI_V, D_UNICODE_RI_N)  // VN

//
//
#define D_EMOJI_FLAG_VANUATU                            \
    D_EMOJI_FLAG(D_UNICODE_RI_V, D_UNICODE_RI_U)  // VU

// W
//
//
#define D_EMOJI_FLAG_WALLIS_AND_FUTUNA                  \
    D_EMOJI_FLAG(D_UNICODE_RI_W, D_UNICODE_RI_F)  // WF

//
//
#define D_EMOJI_FLAG_SAMOA                              \
    D_EMOJI_FLAG(D_UNICODE_RI_W, D_UNICODE_RI_S)  // WS

// X
//
//
#define D_EMOJI_FLAG_KOSOVO                             \
    D_EMOJI_FLAG(D_UNICODE_RI_X, D_UNICODE_RI_K)  // XK

// Y                                        
//
//
#define D_EMOJI_FLAG_YEMEN                              \
    D_EMOJI_FLAG(D_UNICODE_RI_Y, D_UNICODE_RI_E)  // YE

//
//
#define D_EMOJI_FLAG_MAYOTTE                            \
    D_EMOJI_FLAG(D_UNICODE_RI_Y, D_UNICODE_RI_T)  // YT

// Z

//
//
#define D_EMOJI_FLAG_SOUTH_AFRICA                       \
    D_EMOJI_FLAG(D_UNICODE_RI_Z, D_UNICODE_RI_A)  // ZA

//
//
#define D_EMOJI_FLAG_ZAMBIA                             \
    D_EMOJI_FLAG(D_UNICODE_RI_Z, D_UNICODE_RI_M)  // ZM

//
//
#define D_EMOJI_FLAG_ZIMBABWE                           \
    D_EMOJI_FLAG(D_UNICODE_RI_Z, D_UNICODE_RI_W)  // ZW

// D_EMOJI_UTF8
//   macro: convenience macros for printf-friendly use
#define D_EMOJI_UTF8(cp_)  \
    (d_text_emoji_utf8((uint32_t)(cp_)))

// D_EMOJI_SEQ
//   macro: 
#define D_EMOJI_SEQ(...)  \
    ( d_text_emoji_utf8_seq((const uint32_t[]){ __VA_ARGS__ },  \
     ( sizeof((const uint32_t[]){ __VA_ARGS__ }) / sizeof(uint32_t) ) ) )


size_t      d_text_emoji_utf8_encode(uint32_t _cp, char* _out);
const char* d_text_emoji_utf8(uint32_t _cp);
const char* d_text_emoji_utf8_seq(const uint32_t* _cps, size_t _length);


#endif	// DJINTERP_TEXT_SYMBOL_EMOJI_