/*******************************************************************************
* djinterp [env]                                           env_dynamodb_config.h
*
* djinterp Amazon DynamoDB detection configuration.
*   The settings env_dynamodb.h reads. DynamoDB is a managed AWS service with
* no installable server and no server version, so they cover the AWS SDK for
* C++ (the client): whether its DynamoDB client header is included and where
* to find it, and whether automatic SDK detection is bypassed; and the
* deployment target, the cloud service or DynamoDB Local.
*   Every setting is #ifndef-guarded: define it before this header, for
* instance with -D or in a project-wide configuration header, to override it.
*   In manual mode (D_CFG_ENV_DYNAMODB_CUSTOM set to 1), env_dynamodb.h takes
* the SDK version from D_ENV_DYNAMODB_DETECTED_SDK_VERSION. Capabilities that
* depend on the region, the account or a table's settings are declared with
* env_dynamodb.h's D_ENV_DYNAMODB_NO_* opt-outs, not by version.
*
*
* path:      /inc/djinterp/env/db/dynamodb/env_dynamodb_config.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.06.15
*                                                            revised: 2026.09.27
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VENDOR HEADER INCLUSION CONTROL
    -------------------------------
    1.  SDK header
         1.  D_CFG_ENV_USING_DYNAMODB
         2.  D_CFG_ENV_DYNAMODB_CPP_PATH
2.  DETECTION MODE
    --------------
    1.  SDK detection
         1.  D_CFG_ENV_DYNAMODB_CUSTOM
3.  DEPLOYMENT TARGET
    -----------------
    1.  Deployment target
         1.  D_CFG_ENV_DYNAMODB_TARGET_<TARGET>
         2.  D_CFG_ENV_DYNAMODB_TARGET
*/

#ifndef DJINTERP_ENV_DB_DYNAMODB_ENV_DYNAMODB_CONFIG_H
#define DJINTERP_ENV_DB_DYNAMODB_ENV_DYNAMODB_CONFIG_H 1


//==============================================================================
// 1.  VENDOR HEADER INCLUSION CONTROL
//==============================================================================


// 1.1    SDK header
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_ENV_USING_DYNAMODB
//   configuration: 1 to include the AWS SDK for C++'s DynamoDB client header
// and detect the SDK at compile time; C++ only. When 0, no AWS SDK symbol is
// referenced and the SDK reads as absent.
#ifndef D_CFG_ENV_USING_DYNAMODB
    #define D_CFG_ENV_USING_DYNAMODB 0
#endif  // D_CFG_ENV_USING_DYNAMODB

// 1.1.2
// D_CFG_ENV_DYNAMODB_CPP_PATH
//   configuration: the include path of the SDK's DynamoDB client header. It is
// tried first, and included unconditionally where __has_include is
// unavailable. With a standard SDK install, leave the default and rely on
// env_dynamodb.h's <aws/dynamodb/DynamoDBClient.h> fallback.
#ifndef D_CFG_ENV_DYNAMODB_CPP_PATH
    #define D_CFG_ENV_DYNAMODB_CPP_PATH <aws/dynamodb/DynamoDBClient.h>
#endif  // D_CFG_ENV_DYNAMODB_CPP_PATH


//==============================================================================
// 2.  DETECTION MODE
//==============================================================================


// 2.1    SDK detection
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_ENV_DYNAMODB_CUSTOM
//   configuration: 0 (the default) for automatic SDK detection from the AWS
// SDK version macros; 1 to skip it and use a pre-defined
// D_ENV_DYNAMODB_DETECTED_SDK_VERSION instead.
#ifndef D_CFG_ENV_DYNAMODB_CUSTOM
    #define D_CFG_ENV_DYNAMODB_CUSTOM 0
#endif  // D_CFG_ENV_DYNAMODB_CUSTOM


//==============================================================================
// 3.  DEPLOYMENT TARGET
//==============================================================================
// DynamoDB runs as the managed AWS service or as DynamoDB Local, a
// downloadable build for development and testing. Some capabilities, such as
// global tables, point-in-time recovery and true streams, are missing or
// emulated on Local, so the target must be declared for feature gating to
// branch.


// 3.1    Deployment target
//------------------------------------------------------------------------------
// 3.1.1
// D_CFG_ENV_DYNAMODB_TARGET_<TARGET>
//   constant: the target values: D_CFG_ENV_DYNAMODB_TARGET_CLOUD (0) and
// D_CFG_ENV_DYNAMODB_TARGET_LOCAL (1).
#define D_CFG_ENV_DYNAMODB_TARGET_CLOUD   0
#define D_CFG_ENV_DYNAMODB_TARGET_LOCAL   1

// 3.1.2
// D_CFG_ENV_DYNAMODB_TARGET
//   configuration: the deployment target, one of the values above: the cloud
// service by default, or Local when D_ENV_DYNAMODB_DETECTED_LOCAL is defined.
#ifndef D_CFG_ENV_DYNAMODB_TARGET
    #if defined(D_ENV_DYNAMODB_DETECTED_LOCAL)
        #define D_CFG_ENV_DYNAMODB_TARGET D_CFG_ENV_DYNAMODB_TARGET_LOCAL
    #else
        #define D_CFG_ENV_DYNAMODB_TARGET D_CFG_ENV_DYNAMODB_TARGET_CLOUD
    #endif
#endif  // D_CFG_ENV_DYNAMODB_TARGET


#endif  // DJINTERP_ENV_DB_DYNAMODB_ENV_DYNAMODB_CONFIG_H
