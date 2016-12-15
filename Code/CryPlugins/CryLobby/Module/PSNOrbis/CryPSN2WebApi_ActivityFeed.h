#ifndef __CRYPSN2_WEBAPI_ACTIVITYFEED_H__
#define __CRYPSN2_WEBAPI_ACTIVITYFEED_H__
#pragma once

#if CRY_PLATFORM_ORBIS
	#if USE_PSN

//////////////////////////////////////////////////////////////////////////////////////////////

		#define CRY_WEBAPI_ACTIVITYFEED_HTTP_METHOD  SCE_NP_WEBAPI_HTTP_METHOD_POST
		#define CRY_WEBAPI_ACTIVITYFEED_CONTENT_TYPE SCE_NP_WEBAPI_CONTENT_TYPE_APPLICATION_JSON_UTF8
		#define CRY_WEBAPI_ACTIVITYFEED_API_GROUP    "activityFeed"
		#define CRY_WEBAPI_ACTIVITYFEED_REQUEST_PATH "/v1/users/me/feed"
// This is an example for activity feed JSON document formatting.
		#define CRY_WEBAPI_ACTIVITYFEED_INGAME_POST_BODY_SAMPLE                                             \
		  "{"                                                                                               \
		  "\"captions\":{"                                                                                  \
		  "\"en\":\"$USER_NAME_OR_ID has posted a $TITLE_NAME activity.\""                                  \
		  "},"                                                                                              \
		  "\"condensedCaptions\":{"                                                                         \
		  "\"en\": \"$STORY_COUNT $TITLE_NAME stories.\""                                                   \
		  "},"                                                                                              \
		  "\"source\":{"                                                                                    \
		  "\"meta\":\"%s\","                                                                                \
		  "\"type\":\"ONLINE_ID\""                                                                          \
		  "},"                                                                                              \
		  "\"storyType\":\"IN_GAME_POST\","                                                                 \
		  "\"subType\":1,"                                                                                  \
		  "\"actions\":["                                                                                   \
		  "{"                                                                                               \
		  "\"type\":\"url\","                                                                               \
		  "\"uri\":\"http://www.gface.com\","                                                               \
		  "\"platform\":\"PS4\","                                                                           \
		  "\"buttonCaptions\":{"                                                                            \
		  "\"en\":\"URL Test\""                                                                             \
		  "}"                                                                                               \
		  "}"                                                                                               \
		  "],"                                                                                              \
		  "\"targets\":["                                                                                   \
		  "{"                                                                                               \
		  "\"meta\":\"%s\","                                                                                \
		  "\"type\":\"TITLE_ID\""                                                                           \
		  "},"                                                                                              \
		  "{"                                                                                               \
		  "\"meta\":\"https://ukps4lts01.intern.crytek.de/Warface/activityfeed/feed_small470x470_1.png\","  \
		  "\"type\":\"SMALL_IMAGE_URL\","                                                                   \
		  "\"aspectRatio\":\"1:1\""                                                                         \
		  "},"                                                                                              \
		  "{"                                                                                               \
		  "\"meta\":\"https://ukps4lts01.intern.crytek.de/Warface/activityfeed/feed_large1090x613_1.png\"," \
		  "\"type\":\"LARGE_IMAGE_URL\""                                                                    \
		  "}"                                                                                               \
		  "]"                                                                                               \
		  "}"

		#define CRY_WEBAPI_ACTIVITYFEED_MAX_SIZE (4096)         // can change this if needed

struct SCryPSNOrbisWebApiAddActivityFeedInput
{
	char feed[CRY_WEBAPI_ACTIVITYFEED_MAX_SIZE];
};

//////////////////////////////////////////////////////////////////////////////////////////////

	#endif // USE_PSN
#endif   // CRY_PLATFORM_ORBIS

#endif // __CRYPSN2_WEBAPI_ACTIVITYFEED_H__
