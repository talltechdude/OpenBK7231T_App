#include "../obk_config.h"

#include "../new_common.h"
#include "../logging/logging.h"
#include "../httpserver/new_http.h"
#include "../new_pins.h"
#include "../jsmn/jsmn_h.h"
#include "../ota/ota.h"
#include "../hal/hal_wifi.h"
#include "../hal/hal_flashVars.h"
#ifdef BK_LITTLEFS
#include "../littlefs/our_lfs.h"
#endif
#include "lwip/sockets.h"
#if PLATFORM_XR809
    #include <image/flash.h>
#elif PLATFORM_BL602

#else
#endif
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"

#if PLATFORM_XR809
uint32_t flash_read(uint32_t flash, uint32_t addr,void *buf, uint32_t size);
#define FLASH_INDEX_XR809 0
#elif PLATFORM_BL602
#else
extern UINT32 flash_read(char *user_buf, UINT32 count, UINT32 address);
#endif

static int http_rest_error(http_request_t *request, int code, char *msg);

static int http_rest_get(http_request_t *request);
static int http_rest_post(http_request_t *request);
static int http_rest_app(http_request_t *request);

static int http_rest_post_pins(http_request_t *request);
static int http_rest_get_pins(http_request_t *request);

static int http_rest_get_seriallog(http_request_t *request);

static int http_rest_post_logconfig(http_request_t *request);
static int http_rest_get_logconfig(http_request_t *request);

#ifdef BK_LITTLEFS
static int http_rest_get_lfs_file(http_request_t *request);
static int http_rest_post_lfs_file(http_request_t *request);
#endif
static int http_favicon(http_request_t *request);

static int http_rest_post_reboot(http_request_t *request);
static int http_rest_post_flash(http_request_t *request, int startaddr);
static int http_rest_get_flash(http_request_t *request, int startaddr, int len);
static int http_rest_get_flash_advanced(http_request_t *request);
static int http_rest_post_flash_advanced(http_request_t *request);

static int http_rest_get_info(http_request_t *request);

static int http_rest_get_dumpconfig(http_request_t *request);
static int http_rest_get_testconfig(http_request_t *request);

static int http_rest_post_channels(http_request_t *request);
static int http_rest_get_channels(http_request_t *request);

static int http_rest_get_flash_vars_test(http_request_t *request);

static int http_rest_post_cmd(http_request_t *request);


void init_rest(){
    HTTP_RegisterCallback( "/api/", HTTP_GET, http_rest_get);
    HTTP_RegisterCallback( "/api/", HTTP_POST, http_rest_post);
    HTTP_RegisterCallback( "/app", HTTP_GET, http_rest_app);
    HTTP_RegisterCallback( "/favicon.ico", HTTP_GET, http_favicon);
}

const char *apppage1 = 
"<!DOCTYPE html>"
"<html>"
"    <head>"
"        <script>"
"            var root = '";
#if WINDOWS
const char *obktype = "windows";
const char * apppage2 = "';"
"            var obktype = 'windows';"
"            var device = 'http://";
#elif PLATFORM_XR809
const char *obktype = "XR809";
const char * apppage2 = "';"
"            var obktype = 'XR809';"
"            var device = 'http://";
#elif PLATFORM_BL602
const char *obktype = "BL602";
const char * apppage2 = "';"
"            var obktype = 'BL602';"
"            var device = 'http://";
#else
const char *obktype = "beken";
const char * apppage2 = "';"
"            var obktype = 'beken';"
"            var device = 'http://";
#endif

const char * apppage3 = "';"
"        </script>"
"        <script src=\"";
const char * apppage4 = "startup.js\"></script>"
"    </head>"
"<body>"
"</body>"
"</html>";


static int http_rest_get(http_request_t *request){
    ADDLOG_DEBUG(LOG_FEATURE_API, "GET of %s", request->url);
    
    if (!strcmp(request->url, "api/channels")){
        return http_rest_get_channels(request);
    }

    if (!strcmp(request->url, "api/pins")){
        return http_rest_get_pins(request);
    }
    if (!strcmp(request->url, "api/logconfig")){
        return http_rest_get_logconfig(request);
    }

    if (!strncmp(request->url, "api/seriallog", 13)){
        return http_rest_get_seriallog(request);
    }

#ifdef BK_LITTLEFS
    if (!strcmp(request->url, "api/fsblock")){
        return http_rest_get_flash(request, LFS_BLOCKS_START, LFS_BLOCKS_LEN);
    }
#endif

#ifdef BK_LITTLEFS
    if (!strncmp(request->url, "api/lfs/", 8)){
        return http_rest_get_lfs_file(request);
    }
#endif

    if (!strcmp(request->url, "api/info")){
        return http_rest_get_info(request);
    }

    if (!strncmp(request->url, "api/flash/", 10)){
        return http_rest_get_flash_advanced(request);
    }


    if (!strcmp(request->url, "api/dumpconfig")){
        return http_rest_get_dumpconfig(request);
    }

    if (!strcmp(request->url, "api/testconfig")){
        return http_rest_get_testconfig(request);
    }

    if (!strncmp(request->url, "api/testflashvars", 17)){
        return http_rest_get_flash_vars_test(request);
    }
    
    
    

    http_setup(request, httpMimeTypeHTML);
    poststr(request, "GET of ");
    poststr(request, request->url);
    poststr(request, htmlEnd);
    poststr(request,NULL);
    return 0;
}

static int http_rest_post(http_request_t *request){
    char tmp[20];
    ADDLOG_DEBUG(LOG_FEATURE_API, "POST to %s", request->url);

    if (!strcmp(request->url, "api/channels")){
        return http_rest_post_channels(request);
    }

    if (!strcmp(request->url, "api/pins")){
        return http_rest_post_pins(request);
    }
    if (!strcmp(request->url, "api/logconfig")){
        return http_rest_post_logconfig(request);
    }

    if (!strcmp(request->url, "api/reboot")){
        return http_rest_post_reboot(request);
    }
    if (!strcmp(request->url, "api/ota")){
#if PLATFORM_BK7231T
        return http_rest_post_flash(request, START_ADR_OF_BK_PARTITION_OTA);
#elif PLATFORM_BK7231N
        return http_rest_post_flash(request, START_ADR_OF_BK_PARTITION_OTA);
#else
		// TODO
#endif
    }
    if (!strncmp(request->url, "api/flash/", 10)){
        return http_rest_post_flash_advanced(request);
    }

    if (!strcmp(request->url, "api/cmnd")){
        return http_rest_post_cmd(request);
    }
    

#ifdef BK_LITTLEFS
    if (!strcmp(request->url, "api/fsblock")){
        if (lfs_present()){
            release_lfs();
        }
        // we are writing the lfs block
        int res = http_rest_post_flash(request, LFS_BLOCKS_START);
        // initialise the filesystem, it should be there now.
        // don't create if it does not mount
        init_lfs(0);
        return res;
    }
    if (!strncmp(request->url, "api/lfs/", 8)){
        return http_rest_post_lfs_file(request);
    }
#endif

    http_setup(request, httpMimeTypeHTML);
    poststr(request, "POST to ");
    poststr(request, request->url);
    poststr(request, "<br/>Content Length:");
    sprintf(tmp, "%d", request->contentLength);
    poststr(request, tmp);
    poststr(request, "<br/>Content:[");
    poststr(request, request->bodystart);
    poststr(request, "]<br/>");
    poststr(request, htmlEnd);
    poststr(request,NULL);
    return 0;
}






static int http_rest_app(http_request_t *request){
    const char *webhost = CFG_GetWebappRoot();
    const char *ourip = HAL_GetMyIPString(); //CFG_GetOurIP();
    http_setup(request, httpMimeTypeHTML);
    if (webhost && ourip){
        poststr(request, apppage1);
        poststr(request, webhost);
        poststr(request, apppage2);
        poststr(request, ourip);
        poststr(request, apppage3);
        poststr(request, webhost);
        poststr(request, apppage4);
    } else {
        poststr(request,htmlHeader);
        poststr(request,htmlReturnToMenu);
        poststr(request,"no APP available<br/>");
    }
    poststr(request,NULL);
    return 0;
}

#ifdef BK_LITTLEFS

int EndsWith(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

static int http_rest_get_lfs_file(http_request_t *request){
    char *fpath;
    char *buff;
    int len;
    int lfsres;
    int total = 0;
    lfs_file_t *file;

    // don't start LFS just because we're trying to read a file -
    // it won't exist anyway
    if (!lfs_present()){
        request->responseCode = HTTP_RESPONSE_NOT_FOUND;
        http_setup(request, httpMimeTypeText);
        poststr(request,NULL);
        return 0;
    }

    fpath = os_malloc(strlen(request->url) - strlen("api/lfs/") + 1);

    buff = os_malloc(1024);
    file = os_malloc(sizeof(lfs_file_t));
    memset(file, 0, sizeof(lfs_file_t));

    strcpy(fpath, request->url + strlen("api/lfs/"));
    
    ADDLOG_DEBUG(LOG_FEATURE_API, "LFS read of %s", fpath);
    lfsres = lfs_file_open(&lfs, file, fpath, LFS_O_RDONLY);

    if (lfsres == -21){
        lfs_dir_t *dir;
        ADDLOG_DEBUG(LOG_FEATURE_API, "%s is a folder", fpath);
        dir = os_malloc(sizeof(lfs_dir_t));
        os_memset(dir, 0, sizeof(*dir));
        // if the thing is a folder.
        lfsres = lfs_dir_open(&lfs, dir, fpath);

        if (lfsres >= 0){
            // this is needed during iteration...?
            struct lfs_info info;
            int count = 0;
            http_setup(request, httpMimeTypeJson);
            ADDLOG_DEBUG(LOG_FEATURE_API, "opened folder %s lfs result %d", fpath, lfsres);
            hprintf128(request, "{\"dir\":\"%s\",\"content\":[", fpath);
            do {
                // Read an entry in the directory
                //
                // Fills out the info structure, based on the specified file or directory.
                // Returns a positive value on success, 0 at the end of directory,
                // or a negative error code on failure.
                lfsres = lfs_dir_read(&lfs, dir, &info);
                if (lfsres > 0){
                    if (count) poststr(request, ",");
                    hprintf128(request, "{\"name\":\"%s\",\"type\":%d,\"size\":%d}", 
                        info.name, info.type, info.size);
                } else {
                    if (lfsres < 0){
                        if (count) poststr(request, ",");
                        hprintf128(request, "{\"error\":%d}", lfsres); 
                    }
                }
                count++;
            } while (lfsres > 0);

            hprintf128(request, "]}");

            lfs_dir_close(&lfs, dir);
            if (dir) os_free(dir);
            dir = NULL;
        } else {
            if (dir) os_free(dir);
            dir = NULL;
            request->responseCode = HTTP_RESPONSE_NOT_FOUND;
            http_setup(request, httpMimeTypeJson);
            ADDLOG_DEBUG(LOG_FEATURE_API, "failed to open %s lfs result %d", fpath, lfsres);
            hprintf128(request, "{\"fname\":\"%s\",\"error\":%d}", fpath, lfsres);
        }
    } else {
        ADDLOG_DEBUG(LOG_FEATURE_API, "LFS open [%s] gives %d", fpath, lfsres);
        if (lfsres >= 0){
            const char *mimetype = httpMimeTypeBinary;
            do {
                if (EndsWith(fpath, ".ico")){
                    mimetype = "image/x-icon";
                    break;
                }
                if (EndsWith(fpath, ".js")){
                    mimetype = "text/javascript";
                    break;
                }
                if (EndsWith(fpath, ".json")){
                    mimetype = httpMimeTypeJson;
                    break;
                }
                if (EndsWith(fpath, ".html")){
                    mimetype = "text/html";
                    break;
                }
                if (EndsWith(fpath, ".vue")){
                    mimetype = "application/javascript";
                    break;
                }
                break;
            } while (0);

            http_setup(request, mimetype);
            do {
                len = lfs_file_read(&lfs, file, buff, 1024);
                total += len;
                if (len){
                    //ADDLOG_DEBUG(LOG_FEATURE_API, "%d bytes read", len);
                    postany(request, buff, len);
                }
            } while (len > 0);
            lfs_file_close(&lfs, file);
            ADDLOG_DEBUG(LOG_FEATURE_API, "%d total bytes read", total);
        } else {
            request->responseCode = HTTP_RESPONSE_NOT_FOUND;
            http_setup(request, httpMimeTypeJson);
            ADDLOG_DEBUG(LOG_FEATURE_API, "failed to open %s lfs result %d", fpath, lfsres);
            hprintf128(request, "{\"fname\":\"%s\",\"error\":%d}", fpath, lfsres);
        }
    }
    poststr(request,NULL);
    if (fpath) os_free(fpath);
    if (file) os_free(file);
    if (buff) os_free(buff);
    return 0;
}

static int http_rest_post_lfs_file(http_request_t *request){
    int len;
    int lfsres;
    int total = 0;

    // allocated variables
    lfs_file_t *file;
    char *fpath;
    char *folder;

    // create if it does not exist
    init_lfs(1);

    fpath = os_malloc(strlen(request->url) - strlen("api/lfs/") + 1);
    file = os_malloc(sizeof(lfs_file_t));
    memset(file, 0, sizeof(lfs_file_t));

    strcpy(fpath, request->url + strlen("api/lfs/"));
    ADDLOG_DEBUG(LOG_FEATURE_API, "LFS write of %s len %d", fpath, request->contentLength);

    folder = strchr(fpath, '/');
    if (folder){
        int folderlen = folder - fpath;
        folder = os_malloc(folderlen+1);
        strncpy(folder, fpath, folderlen);
        folder[folderlen] = 0;
        ADDLOG_DEBUG(LOG_FEATURE_API, "file is in folder %s try to create", folder);
        lfsres = lfs_mkdir(&lfs, folder);
        if (lfsres < 0){
            ADDLOG_DEBUG(LOG_FEATURE_API, "mkdir error %d", lfsres);
        }
    }

    //ADDLOG_DEBUG(LOG_FEATURE_API, "LFS write of %s len %d", fpath, request->contentLength);

    lfsres = lfs_file_open(&lfs, file, fpath, LFS_O_RDWR | LFS_O_CREAT);
    if (lfsres >= 0){
        //ADDLOG_DEBUG(LOG_FEATURE_API, "opened %s");
        int towrite = request->bodylen;
        char *writebuf = request->bodystart;
        int writelen = request->bodylen;
        if (request->contentLength >= 0){
            towrite = request->contentLength;
        }
        //ADDLOG_DEBUG(LOG_FEATURE_API, "bodylen %d, contentlen %d", request->bodylen, request->contentLength);
        
        if (writelen < 0){
            ADDLOG_DEBUG(LOG_FEATURE_API, "ABORTED: %d bytes to write", writelen);
            lfs_file_close(&lfs, file);
            request->responseCode = HTTP_RESPONSE_SERVER_ERROR;
            http_setup(request, httpMimeTypeJson);
            hprintf128(request, "{\"fname\":\"%s\",\"error\":%d}", fpath, -20);
            goto exit;
        }

        do {
            //ADDLOG_DEBUG(LOG_FEATURE_API, "%d bytes to write", writelen);
            len = lfs_file_write(&lfs, file, writebuf, writelen);
            total += len;
            if (len > 0){
                //ADDLOG_DEBUG(LOG_FEATURE_API, "%d bytes written", len);
            }
            towrite -= len;
            if (towrite > 0){
                writebuf = request->received;
                writelen = recv(request->fd, writebuf, request->receivedLenmax, 0);
                if (writelen < 0){
                    ADDLOG_DEBUG(LOG_FEATURE_API, "recv returned %d - end of data - remaining %d", writelen, towrite);
                }
            }
        } while ((towrite > 0) && (writelen >= 0));
        //ADDLOG_DEBUG(LOG_FEATURE_API, "closing %s", fpath);
        lfs_file_close(&lfs, file);
        ADDLOG_DEBUG(LOG_FEATURE_API, "%d total bytes written", total);
        http_setup(request, httpMimeTypeJson);
        hprintf128(request, "{\"fname\":\"%s\",\"size\":%d}", fpath, total);
    } else {
        request->responseCode = HTTP_RESPONSE_SERVER_ERROR;
        http_setup(request, httpMimeTypeJson);
        ADDLOG_DEBUG(LOG_FEATURE_API, "failed to open %s err %d", fpath, lfsres);
        hprintf128(request, "{\"fname\":\"%s\",\"error\":%d}", fpath, lfsres);
    }
exit:
    poststr(request,NULL);
    if (folder) os_free(folder);
    if (file) os_free(file);
    if (fpath) os_free(fpath);
    return 0;
}

static int http_favicon(http_request_t *request){
    request->url = "api/lfs/favicon.ico";
    return http_rest_get_lfs_file(request);
}

#else
static int http_favicon(http_request_t *request){
    request->responseCode = HTTP_RESPONSE_NOT_FOUND;
    http_setup(request, httpMimeTypeHTML);
    poststr(request,NULL);
    return 0;
}
#endif



static int http_rest_get_seriallog(http_request_t *request){
    if (request->url[strlen(request->url)-1] == '1'){
        direct_serial_log = 1;
    } else {
        direct_serial_log = 0;
    }
    http_setup(request, httpMimeTypeJson);
    hprintf128(request, "Direct serial logging set to %d", direct_serial_log);
    poststr(request, NULL);
    return 0;
}



static int http_rest_get_pins(http_request_t *request){
    int i;
    http_setup(request, httpMimeTypeJson);
    poststr(request, "{\"rolenames\":[");
    for (i = 0; i < IOR_Total_Options; i++){
        if (i){
            hprintf128(request, ",\"%s\"", htmlPinRoleNames[i]);
        } else {
            hprintf128(request, "\"%s\"", htmlPinRoleNames[i]);
        }
    }
    poststr(request, "],\"roles\":[");

    for (i = 0; i < 32; i++){
        if (i){
			hprintf128(request, ",%d", g_cfg.pins.roles[i]);
        } else {
            hprintf128(request, "%d", g_cfg.pins.roles[i]);
        }
    }
    poststr(request, "],\"channels\":[");
    for (i = 0; i < 32; i++){
        if (i){
            hprintf128(request, ",%d", g_cfg.pins.channels[i]);
        } else {
            hprintf128(request, "%d", g_cfg.pins.channels[i]);
        }
    }
    poststr(request, "]}");
    poststr(request, NULL);
    return 0;
}



////////////////////////////
// log config
static int http_rest_get_logconfig(http_request_t *request){
    int i;
    http_setup(request, httpMimeTypeJson);
    hprintf128(request, "{\"level\":%d,", loglevel);
    hprintf128(request, "\"features\":%d,", logfeatures);
    poststr(request, "\"levelnames\":[");
    for (i = 0; i < LOG_MAX; i++){
        if (i){
            hprintf128(request, ",\"%s\"", loglevelnames[i]);
        } else {
            hprintf128(request, "\"%s\"", loglevelnames[i]);
        }
    }
    poststr(request, "],\"featurenames\":[");
    for (i = 0; i < LOG_FEATURE_MAX; i++){
        if (i){
            hprintf128(request, ",\"%s\"", logfeaturenames[i]);
        } else {
            hprintf128(request, "\"%s\"", logfeaturenames[i]);
        }
    }
    poststr(request, "]}");
    poststr(request, NULL);
    return 0;
}

static int http_rest_post_logconfig(http_request_t *request){
    int i;
    int r;
    char tmp[64];

    //https://github.com/zserge/jsmn/blob/master/example/simple.c
    //jsmn_parser p;
    jsmn_parser *p = os_malloc(sizeof(jsmn_parser));
    //jsmntok_t t[128]; /* We expect no more than 128 tokens */
#define TOKEN_COUNT 128
    jsmntok_t *t = os_malloc(sizeof(jsmntok_t)*TOKEN_COUNT);
    char *json_str = request->bodystart;
    int json_len = strlen(json_str);

	http_setup(request, httpMimeTypeText);
	memset(p, 0, sizeof(jsmn_parser));
	memset(t, 0, sizeof(jsmntok_t)*128);

    jsmn_init(p);
    r = jsmn_parse(p, json_str, json_len, t, TOKEN_COUNT);
    if (r < 0) {
        ADDLOG_ERROR(LOG_FEATURE_API, "Failed to parse JSON: %d", r);
        poststr(request, NULL);
        os_free(p);
        os_free(t);
        return 0;
    }

    /* Assume the top-level element is an object */
    if (r < 1 || t[0].type != JSMN_OBJECT) {
        ADDLOG_ERROR(LOG_FEATURE_API, "Object expected", r);
        poststr(request, NULL);
        os_free(p);
        os_free(t);
        return 0;
    }

    //sprintf(tmp,"parsed JSON: %s\n", json_str);
    //poststr(request, tmp);
    //poststr(request, NULL);

        /* Loop over all keys of the root object */
    for (i = 1; i < r; i++) {
        if (jsoneq(json_str, &t[i], "level") == 0) {
            if (t[i + 1].type != JSMN_PRIMITIVE) {
                continue; /* We expect groups to be an array of strings */
            }
            loglevel = atoi(json_str + t[i + 1].start);
            i += t[i + 1].size + 1;
        } else if (jsoneq(json_str, &t[i], "features") == 0) {
            if (t[i + 1].type != JSMN_PRIMITIVE) {
                continue; /* We expect groups to be an array of strings */
            }
            logfeatures = atoi(json_str + t[i + 1].start);;
            i += t[i + 1].size + 1;
        } else {
            ADDLOG_ERROR(LOG_FEATURE_API, "Unexpected key: %.*s", t[i].end - t[i].start,
                json_str + t[i].start);
            sprintf(tmp,"Unexpected key: %.*s\n", t[i].end - t[i].start,
                json_str + t[i].start);
            poststr(request, tmp);
        }
    }

    poststr(request, NULL);
    os_free(p);
    os_free(t);
    return 0;
}

/////////////////////////////////////////////////


static int http_rest_get_info(http_request_t *request){
    char macstr[3*6+1];
    http_setup(request, httpMimeTypeJson);
    hprintf128(request, "{\"uptime_s\":%d,", Time_getUpTimeSeconds());
    hprintf128(request, "\"build\":\"%s\",", g_build_str);
    hprintf128(request, "\"sys\":\"%s\",", obktype);
    hprintf128(request, "\"ip\":\"%s\",", HAL_GetMyIPString());
    hprintf128(request, "\"mac\":\"%s\",", HAL_GetMACStr(macstr));
    hprintf128(request, "\"mqtthost\":\"%s:%d\",", CFG_GetMQTTHost(), CFG_GetMQTTPort());
    hprintf128(request, "\"mqtttopic\":\"%s\",", CFG_GetShortDeviceName());
    hprintf128(request, "\"webapp\":\"%s\"}", CFG_GetWebappRoot());

    poststr(request, NULL);
    return 0;
}


static int http_rest_post_pins(http_request_t *request){
    int i;
    int r;
    char tmp[64];
    int iChanged = 0;

    //https://github.com/zserge/jsmn/blob/master/example/simple.c
    //jsmn_parser p;
    jsmn_parser *p = os_malloc(sizeof(jsmn_parser));
    //jsmntok_t t[128]; /* We expect no more than 128 tokens */
#define TOKEN_COUNT 128
    jsmntok_t *t = os_malloc(sizeof(jsmntok_t)*TOKEN_COUNT);
    char *json_str = request->bodystart;
    int json_len = strlen(json_str);

	memset(p, 0, sizeof(jsmn_parser));
	memset(t, 0, sizeof(jsmntok_t)*128);

    jsmn_init(p);
    r = jsmn_parse(p, json_str, json_len, t, TOKEN_COUNT);
    if (r < 0) {
        ADDLOG_ERROR(LOG_FEATURE_API, "Failed to parse JSON: %d", r);
        sprintf(tmp,"Failed to parse JSON: %d\n", r);
        os_free(p);
        os_free(t);
        return http_rest_error(request, 400, tmp);
    }

    /* Assume the top-level element is an object */
    if (r < 1 || t[0].type != JSMN_OBJECT) {
        ADDLOG_ERROR(LOG_FEATURE_API, "Object expected", r);
        sprintf(tmp,"Object expected\n");
        os_free(p);
        os_free(t);
        return http_rest_error(request, 400, tmp);
    }

    /* Loop over all keys of the root object */
    for (i = 1; i < r; i++) {
        if (jsoneq(json_str, &t[i], "roles") == 0) {
            int j;
            if (t[i + 1].type != JSMN_ARRAY) {
                continue; /* We expect groups to be an array of strings */
            }
            for (j = 0; j < t[i + 1].size; j++) {
                int roleval, pr;
                jsmntok_t *g = &t[i + j + 2];
                roleval = atoi(json_str + g->start);
				pr = PIN_GetPinRoleForPinIndex(j);
				if(pr != roleval) {
					PIN_SetPinRoleForPinIndex(j,roleval);
					iChanged++;
				}
            }
            i += t[i + 1].size + 1;
        } else if (jsoneq(json_str, &t[i], "channels") == 0) {
            int j;
            if (t[i + 1].type != JSMN_ARRAY) {
                continue; /* We expect groups to be an array of strings */
            }
            for (j = 0; j < t[i + 1].size; j++) {
                int chanval, pr;
                jsmntok_t *g = &t[i + j + 2];
                chanval = atoi(json_str + g->start);
				pr = PIN_GetPinChannelForPinIndex(j);
				if(pr != chanval) {
					PIN_SetPinChannelForPinIndex(j,chanval);
					iChanged++;
				}
            }
            i += t[i + 1].size + 1;
        } else {
            ADDLOG_ERROR(LOG_FEATURE_API, "Unexpected key: %.*s", t[i].end - t[i].start,
                json_str + t[i].start);
        }
    }
    if (iChanged){
		CFG_Save_SetupTimer();
        ADDLOG_DEBUG(LOG_FEATURE_API, "Changed %d - saved to flash", iChanged);
    }

    os_free(p);
    os_free(t);
    return http_rest_error(request, 200, "OK");
    return 0;
}

static int http_rest_error(http_request_t *request, int code, char *msg){
    request->responseCode = code;
    http_setup(request, httpMimeTypeJson);
    if (code != 200){
        hprintf128(request, "{\"error\":%d, \"msg\":\"%s\"}", code, msg);
    } else {
        hprintf128(request, "{\"success\":%d, \"msg\":\"%s\"}", code, msg);
    }
    poststr(request,NULL);
    return 0;
}


static int http_rest_post_flash(http_request_t *request, int startaddr){
#if PLATFORM_XR809

#elif PLATFORM_BL602

#else
    int total = 0;
    int towrite;
    char *writebuf;
    int writelen;

    ADDLOG_DEBUG(LOG_FEATURE_API, "OTA post len %d", request->contentLength);

    init_ota(startaddr);

    towrite = request->bodylen;
    writebuf = request->bodystart;
    writelen = request->bodylen;
    if (request->contentLength >= 0){
        towrite = request->contentLength;
    }
        
    if (writelen < 0 || (startaddr + writelen > 0x200000)){
        ADDLOG_DEBUG(LOG_FEATURE_API, "ABORTED: %d bytes to write", writelen);
        return http_rest_error(request, -20, "writelen < 0 or end > 0x200000");
    }

    do {
        //ADDLOG_DEBUG(LOG_FEATURE_API, "%d bytes to write", writelen);
        add_otadata((unsigned char *)writebuf, writelen);
        total += writelen;
        towrite -= writelen;
        if (towrite > 0){
            writebuf = request->received;
            writelen = recv(request->fd, writebuf, request->receivedLenmax, 0);
            if (writelen < 0){
                ADDLOG_DEBUG(LOG_FEATURE_API, "recv returned %d - end of data - remaining %d", writelen, towrite);
            }
        }
    } while ((towrite > 0) && (writelen >= 0));
    ADDLOG_DEBUG(LOG_FEATURE_API, "%d total bytes written", total);
    http_setup(request, httpMimeTypeJson);
    hprintf128(request, "{\"size\":%d}", total);
    close_ota();

    poststr(request,NULL);
#endif
    return 0;
}

static int http_rest_post_reboot(http_request_t *request){
    http_setup(request, httpMimeTypeJson);
    hprintf128(request, "{\"reboot\":%d}", 3);
    ADDLOG_DEBUG(LOG_FEATURE_API, "Rebooting in 3 seconds...");
	RESET_ScheduleModuleReset(3);
    poststr(request,NULL);
    return 0;
}

static int http_rest_get_flash_advanced(http_request_t *request){
    char *params = request->url + 10;
    int startaddr = 0; 
    int len = 0;
    int sres;
    sres = sscanf(params, "%x-%x", &startaddr, &len);
    if (sres == 2) {
        return http_rest_get_flash(request, startaddr, len);
    }
    return http_rest_error(request, -1, "invalid url");
}

static int http_rest_post_flash_advanced(http_request_t *request){
    char *params = request->url + 10;
    int startaddr = 0; 
    int sres;
    sres = sscanf(params, "%x", &startaddr);
    if (sres == 1 && startaddr >= START_ADR_OF_BK_PARTITION_OTA){
        return http_rest_post_flash(request, startaddr);
    }
    return http_rest_error(request, -1, "invalid url");
}

static int http_rest_get_flash(http_request_t *request, int startaddr, int len){
    char *buffer;
    int res;

    if (startaddr < 0 || (startaddr + len > 0x200000)){
        return http_rest_error(request, -1, "requested flash read out of range");
    }

    buffer = os_malloc(1024);

    http_setup(request, httpMimeTypeBinary);
    while(len){
        int readlen = len;
        if (readlen > 1024){
            readlen = 1024;
        }
#if PLATFORM_XR809
  //uint32_t flash_read(uint32_t flash, uint32_t addr,void *buf, uint32_t size)
 #define FLASH_INDEX_XR809 0
        res = flash_read(FLASH_INDEX_XR809, startaddr, buffer, readlen);
#elif PLATFORM_BL602
		res = 0;
#else
        res = flash_read((char *)buffer, readlen, startaddr);
#endif
        startaddr += readlen;
        len -= readlen;
        postany(request, buffer, readlen);
    }
    poststr(request, NULL);
    return 0;
}


static int http_rest_get_dumpconfig(http_request_t *request){



    http_setup(request, httpMimeTypeText);
    poststr(request, NULL);
    return 0;
}



#ifdef TESTCONFIG_ENABLE    
// added for OpenBK7231T
typedef struct item_new_test_config
{
	INFO_ITEM_ST head;
	char somename[64];
}ITEM_NEW_TEST_CONFIG,*ITEM_NEW_TEST_CONFIG_PTR;

ITEM_NEW_TEST_CONFIG testconfig;
#endif    

static int http_rest_get_testconfig(http_request_t *request){
    return http_rest_error(request, 400, "unsupported");
    return 0;
}

static int http_rest_get_flash_vars_test(http_request_t *request){
//#if PLATFORM_XR809
//    return http_rest_error(request, 400, "flash vars unsupported");
//#elif PLATFORM_BL602
//    return http_rest_error(request, 400, "flash vars unsupported");
//#else
//#ifndef DISABLE_FLASH_VARS_VARS
//    char *params = request->url + 17;
//    int increment = 0; 
//    int len = 0;
//    int sres;
//    int i;
//    char tmp[128];
//    FLASH_VARS_STRUCTURE data, *p;
//
//    p = &flash_vars;
//
//    sres = sscanf(params, "%x-%x", &increment, &len);
//
//    ADDLOG_DEBUG(LOG_FEATURE_API, "http_rest_get_flash_vars_test %d %d returned %d", increment, len, sres);
//
//    if (increment == 10){
//        flash_vars_read(&data);
//        p = &data;
//    } else {
//        for (i = 0; i < increment; i++){
//            HAL_FlashVars_IncreaseBootCount();
//        }
//        for (i = 0; i < len; i++){
//            HAL_FlashVars_SaveBootComplete();
//        }
//    }
//
//    sprintf(tmp, "offset %d, boot count %d, boot success %d, bootfailures %d", 
//        flash_vars_offset, 
//        p->boot_count, 
//        p->boot_success_count,
//        p->boot_count - p->boot_success_count );
//
//    return http_rest_error(request, 200, tmp);
//#else 
    return http_rest_error(request, 400, "flash test unsupported");
}


static int http_rest_get_channels(http_request_t *request){
    int i;
    int addcomma = 0;
    /*typedef struct pinsState_s {
    	byte roles[32];
	    byte channels[32];
    } pinsState_t;

    extern pinsState_t g_pins;
    */
    http_setup(request, httpMimeTypeJson);
    poststr(request, "{");

    for (i = 0; i < 32; i++){
        int ch = PIN_GetPinChannelForPinIndex(i);
        int role = PIN_GetPinRoleForPinIndex(i);
        if (role){
            if (addcomma){
                hprintf128(request, ",");
            }
            hprintf128(request, "\"%d\":%d", ch, CHANNEL_Get(ch));
            addcomma = 1;
        }
    }
    poststr(request, "}");
    poststr(request, NULL);
    return 0;
}

// currently crashes the MCU - maybe stack overflow?
static int http_rest_post_channels(http_request_t *request){
    int i;
    int r;
    char tmp[64];

    //https://github.com/zserge/jsmn/blob/master/example/simple.c
    //jsmn_parser p;
    jsmn_parser *p = os_malloc(sizeof(jsmn_parser));
    //jsmntok_t t[128]; /* We expect no more than 128 tokens */
#define TOKEN_COUNT 128
    jsmntok_t *t = os_malloc(sizeof(jsmntok_t)*TOKEN_COUNT);
    char *json_str = request->bodystart;
    int json_len = strlen(json_str);

	memset(p, 0, sizeof(jsmn_parser));
	memset(t, 0, sizeof(jsmntok_t)*128);

    jsmn_init(p);
    r = jsmn_parse(p, json_str, json_len, t, TOKEN_COUNT);
    if (r < 0) {
        ADDLOG_ERROR(LOG_FEATURE_API, "Failed to parse JSON: %d", r);
        sprintf(tmp,"Failed to parse JSON: %d\n", r);
        os_free(p);
        os_free(t);
        return http_rest_error(request, 400, tmp);
    }

    /* Assume the top-level element is an object */
    if (r < 1 || t[0].type != JSMN_ARRAY) {
        ADDLOG_ERROR(LOG_FEATURE_API, "Array expected", r);
        sprintf(tmp,"Object expected\n");
        os_free(p);
        os_free(t);
        return http_rest_error(request, 400, tmp);
    }

    /* Loop over all keys of the root object */
    for (i = 1; i < r; i++) {
        int chanval;
        jsmntok_t *g = &t[i];
        chanval = atoi(json_str + g->start);
        CHANNEL_Set(i-1, chanval, 0);
        ADDLOG_DEBUG(LOG_FEATURE_API, "Set of chan %d to %d", i,
                chanval);
    }

    os_free(p);
    os_free(t);
    return http_rest_error(request, 200, "OK");
    return 0;
}


static int http_rest_post_cmd(http_request_t *request){
    char *cmd = request->bodystart;
    CMD_ExecuteCommand(cmd);
    return http_rest_error(request, 200, "OK");
}

