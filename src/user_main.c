/*

 */
//

#include "hal/hal_wifi.h"
#include "hal/hal_generic.h"
#include "hal/hal_flashVars.h"
#include "new_common.h"

#include "driver/drv_public.h"

// Commands register, execution API and cmd tokenizer
#include "cmnds/cmd_public.h"

// overall config variables for app - like BK_LITTLEFS
#include "obk_config.h"

#include "httpserver/new_http.h"
#include "new_pins.h"
#include "new_cfg.h"
#include "logging/logging.h"
#include "httpserver/http_tcp_server.h"
#include "httpserver/rest_interface.h"
#include "mqtt/new_mqtt.h"

#ifdef BK_LITTLEFS
#include "littlefs/our_lfs.h"
#endif


#include "driver/drv_ntp.h"

static int g_secondsElapsed = 0;
// open access point after this number of seconds
static int g_openAP = 0;
// connect to wifi after this number of seconds
static int g_connectToWiFi = 0;
// many boots failed? do not run pins or anything risky
static int bSafeMode = 0;
// reset after this number of seconds
static int g_reset = 0;
// is connected to WiFi?
int g_bHasWiFiConnected = 0;

static int g_bootFailures = 0;

static int g_saveCfgAfter = 0;

#define LOG_FEATURE LOG_FEATURE_MAIN


#if PLATFORM_XR809
size_t xPortGetFreeHeapSize() {
	return 0;
}
#endif
#if PLATFORM_BL602



OSStatus rtos_create_thread( beken_thread_t* thread, 
							uint8_t priority, const char* name, 
							beken_thread_function_t function,
							uint32_t stack_size, beken_thread_arg_t arg ) {
    OSStatus err = kNoErr;


    err = xTaskCreate(function, name, stack_size, arg, 1, thread);
/*
 BaseType_t xTaskCreate(
							  TaskFunction_t pvTaskCode,
							  const char * const pcName,
							  configSTACK_DEPTH_TYPE usStackDepth,
							  void *pvParameters,
							  UBaseType_t uxPriority,
							  TaskHandle_t *pvCreatedTask
						  );
*/
	if(err == pdPASS){ 
		printf("Thread create %s - pdPASS\n",name);
		return 0;
	} else if(err == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY ) {
		printf("Thread create %s - errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY\n",name);
	} else {
		printf("Thread create %s - err %i\n",name,err);
	}
	return 1;
}

OSStatus rtos_delete_thread( beken_thread_t* thread ) {
	vTaskDelete(thread);
	return kNoErr;
}
#endif
void RESET_ScheduleModuleReset(int delSeconds) {
	g_reset = delSeconds;
}

int Time_getUpTimeSeconds() {
	return g_secondsElapsed;
}



void Main_OnWiFiStatusChange(int code){

    ADDLOGF_INFO("Main_OnWiFiStatusChange %d\r\n", code);

    switch(code){
        case WIFI_STA_CONNECTING:
            PIN_set_wifi_led(0);
			g_bHasWiFiConnected = 0;
            break;
        case WIFI_STA_DISCONNECTED:
            // try to connect again in few seconds
            g_connectToWiFi = 15;
            PIN_set_wifi_led(0);
			g_bHasWiFiConnected = 0;
            break;
        case WIFI_STA_AUTH_FAILED:
            PIN_set_wifi_led(0);
            // try to connect again in few seconds
            g_connectToWiFi = 60;
			g_bHasWiFiConnected = 0;
            break;
        case WIFI_STA_CONNECTED:  
            PIN_set_wifi_led(1);
			g_bHasWiFiConnected = 1;
            break;
        /* for softap mode */
        case WIFI_AP_CONNECTED: 
            PIN_set_wifi_led(1);
			g_bHasWiFiConnected = 1;
            break;
        case WIFI_AP_FAILED:      
            PIN_set_wifi_led(0);
			g_bHasWiFiConnected = 0;
            break;
        default:
            break;
    }

}


void CFG_Save_SetupTimer() {
	g_saveCfgAfter = 3;
}

void Main_OnEverySecond()
{
	int bMQTTconnected;

	// run_adc_test();
	bMQTTconnected = MQTT_RunEverySecondUpdate();
	RepeatingEvents_OnEverySecond();
#ifndef OBK_DISABLE_ALL_DRIVERS
	DRV_OnEverySecond();
#endif

	g_secondsElapsed ++;
	if(bSafeMode) { 
		ADDLOGF_INFO("[SAFE MODE] Timer %i, free mem %d, MQTT state %i\n", g_secondsElapsed, xPortGetFreeHeapSize(),bMQTTconnected);
	} else {
		ADDLOGF_INFO("Timer %i, free mem %d, MQTT state %i\n", g_secondsElapsed, xPortGetFreeHeapSize(),bMQTTconnected);
	}

	// print network info
	if (!(g_secondsElapsed % 10)){
		HAL_PrintNetworkInfo();
	}

	// when we hit 30s, mark as boot complete.
	if (g_secondsElapsed == BOOT_COMPLETE_SECONDS){
		HAL_FlashVars_SaveBootComplete();
		g_bootFailures = HAL_FlashVars_GetBootFailures();
	}

	if (g_openAP){
		g_openAP--;
		if (0 == g_openAP){
			HAL_SetupWiFiOpenAccessPoint(CFG_GetDeviceName());
		}
	}
	if(g_connectToWiFi){
		g_connectToWiFi --;
		if(0 == g_connectToWiFi && g_bHasWiFiConnected == 0) {
			const char *wifi_ssid, *wifi_pass;
			wifi_ssid = CFG_GetWiFiSSID();
			wifi_pass = CFG_GetWiFiPass();
			HAL_ConnectToWiFi(wifi_ssid,wifi_pass);
			// register function to get callbacks about wifi changes.
			HAL_WiFi_SetupStatusCallback(Main_OnWiFiStatusChange);
			ADDLOGF_DEBUG("Registered for wifi changes\r\n");
			// reconnect after 10 minutes?
			//g_connectToWiFi = 60 * 10; 
		}
	}

	// config save moved here because of stack size problems
	if (g_saveCfgAfter){
		g_saveCfgAfter--;
		if (!g_saveCfgAfter){
			CFG_Save_IfThereArePendingChanges(); 
		}
	}
	if (g_reset){
		g_reset--;
		if (!g_reset){
			// ensure any config changes are saved before reboot.
			CFG_Save_IfThereArePendingChanges(); 
			ADDLOGF_INFO("Going to call HAL_RebootModule\r\n");
			HAL_RebootModule(); 
		} else {

			ADDLOGF_INFO("Module reboot in %i...\r\n",g_reset);
		}
	}



}

void app_on_generic_dbl_click(int btnIndex)
{
	if(g_secondsElapsed < 5) {
		CFG_SetOpenAccessPoint();
	}
}


int Main_IsConnectedToWiFi() {
	return g_bHasWiFiConnected;
}
int Main_GetLastRebootBootFailures() {
	return g_bootFailures;
}

void Main_Init()
{
	int bForceOpenAP = 0;
	const char *wifi_ssid, *wifi_pass;

	// read or initialise the boot count flash area
	HAL_FlashVars_IncreaseBootCount();

	g_bootFailures = HAL_FlashVars_GetBootFailures();
	if (g_bootFailures > 3){
		bForceOpenAP = 1;
		ADDLOGF_INFO("###### force AP mode - boot failures %d", g_bootFailures);
	}
	if (g_bootFailures > 4){
		bSafeMode = 1;
		ADDLOGF_INFO("###### safe mode activated - boot failures %d", g_bootFailures);
	}

#define AUTO_ON
#ifdef AUTO_ON  
  CHANNEL_Set(1, 100, 0);
  CHANNEL_Set(2, 100, 0);
#endif

	CFG_InitAndLoad();
	wifi_ssid = CFG_GetWiFiSSID();
	wifi_pass = CFG_GetWiFiPass();

#if 0
	// you can use this if you bricked your module by setting wrong access point data
	wifi_ssid = "qqqqqqqqqq";
	wifi_pass = "Fqqqqqqqqqqqqqqqqqqqqqqqqqqq"
#endif
#ifdef SPECIAL_UNBRICK_ALWAYS_OPEN_AP
	// you can use this if you bricked your module by setting wrong access point data
	bForceOpenAP = 1;
#endif

	if(*wifi_ssid == 0 || *wifi_pass == 0 || bForceOpenAP) {
		// start AP mode in 5 seconds
		g_openAP = 5;
		//HAL_SetupWiFiOpenAccessPoint();
	} else {
		g_connectToWiFi = 5;
	}

	ADDLOGF_INFO("Using SSID [%s]\r\n",wifi_ssid);
	ADDLOGF_INFO("Using Pass [%s]\r\n",wifi_pass);

	// NOT WORKING, I done it other way, see ethernetif.c
	//net_dhcp_hostname_set(g_shortDeviceName);

	HTTPServer_Start();
	ADDLOGF_DEBUG("Started http tcp server\r\n");

	// only initialise certain things if we are not in AP mode
	if (!bSafeMode){
		g_enable_pins = 1;
		// this actually sets the pins, moved out so we could avoid if necessary
		PIN_SetupPins();

#ifndef OBK_DISABLE_ALL_DRIVERS
		DRV_Generic_Init();
#endif
		RepeatingEvents_Init();

		PIN_Init();
		ADDLOGF_DEBUG("Initialised pins\r\n");

        PIN_set_wifi_led(0);

		// initialise MQTT - just sets up variables.
		// all MQTT happens in timer thread?
		MQTT_init();

		PIN_SetGenericDoubleClickCallback(app_on_generic_dbl_click);
		ADDLOGF_DEBUG("Initialised other callbacks\r\n");

#ifdef BK_LITTLEFS
		// initialise the filesystem, only if present.
		// don't create if it does not mount
		// do this for ST mode only, as it may be something in FS which is killing us,
		// and we may add a command to empty fs just be writing first sector?
		init_lfs(0);
#endif

		// initialise rest interface
		init_rest();

		// add some commands...
		taslike_commands_init();
		fortest_commands_init();
		NewLED_InitCommands();

		// NOTE: this will try to read autoexec.bat,
		// so ALL commands expected in autoexec.bat should have been registered by now...
		// but DON't run autoexec if we have had 2+ boot failures
		CMD_Init();

		if (g_bootFailures < 2){
			CMD_ExecuteCommand(CFG_GetShortStartupCommand());
			CMD_ExecuteCommand("exec autoexec.bat");
		}
	}

}