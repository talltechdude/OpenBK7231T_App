
#include "new_common.h"
#include "logging/logging.h"
#include "httpserver/new_http.h"
#include "new_pins.h"
#include "new_cfg.h"
#include "hal/hal_wifi.h"
#include "hal/hal_flashConfig.h"
#include "cmnds/cmd_public.h"


mainConfig_t g_cfg;

int g_cfg_pendingChanges = 0;

#define CFG_IDENT_0 'C'
#define CFG_IDENT_1 'F'
#define CFG_IDENT_2 'G'

#define MAIN_CFG_VERSION 1

static byte CFG_CalcChecksum(mainConfig_t *inf) {
	byte crc = 0;
	crc ^= Tiny_CRC8((const char*)&inf->version,sizeof(inf->version));
	crc ^= Tiny_CRC8((const char*)&inf->changeCounter,sizeof(inf->changeCounter));
	crc ^= Tiny_CRC8((const char*)&inf->otaCounter,sizeof(inf->otaCounter));
	crc ^= Tiny_CRC8((const char*)&inf->genericFlags,sizeof(inf->genericFlags));
	crc ^= Tiny_CRC8((const char*)&inf->genericFlags2,sizeof(inf->genericFlags2));
	crc ^= Tiny_CRC8((const char*)&inf->wifi_ssid,sizeof(inf->wifi_ssid));
	crc ^= Tiny_CRC8((const char*)&inf->wifi_pass,sizeof(inf->wifi_pass));
	crc ^= Tiny_CRC8((const char*)&inf->mqtt_host,sizeof(inf->mqtt_host));
	crc ^= Tiny_CRC8((const char*)&inf->mqtt_brokerName,sizeof(inf->mqtt_brokerName));
	crc ^= Tiny_CRC8((const char*)&inf->mqtt_userName,sizeof(inf->mqtt_userName));
	crc ^= Tiny_CRC8((const char*)&inf->mqtt_pass,sizeof(inf->mqtt_pass));
	crc ^= Tiny_CRC8((const char*)&inf->mqtt_port,sizeof(inf->mqtt_port));
	crc ^= Tiny_CRC8((const char*)&inf->webappRoot,sizeof(inf->webappRoot));
	crc ^= Tiny_CRC8((const char*)&inf->mac,sizeof(inf->mac));
	crc ^= Tiny_CRC8((const char*)&inf->shortDeviceName,sizeof(inf->shortDeviceName));
	crc ^= Tiny_CRC8((const char*)&inf->longDeviceName,sizeof(inf->longDeviceName));
	crc ^= Tiny_CRC8((const char*)&inf->pins,sizeof(inf->pins));
	crc ^= Tiny_CRC8((const char*)&inf->unusedSectorA,sizeof(inf->unusedSectorA));
	crc ^= Tiny_CRC8((const char*)&inf->unusedSectorB,sizeof(inf->unusedSectorB));
	crc ^= Tiny_CRC8((const char*)&inf->unusedSectorC,sizeof(inf->unusedSectorC));
	crc ^= Tiny_CRC8((const char*)&inf->initCommandLine,sizeof(inf->initCommandLine));

	return crc;
}


static void CFG_SetDefaultConfig() {
	// must be unsigned, else print below prints negatives as e.g. FFFFFFFe
	unsigned char mac[6] = { 0 };

#if PLATFORM_XR809
	HAL_Configuration_GenerateMACForThisModule(mac);
#else
	WiFI_GetMacAddress((char *)mac);
#endif

	memset(&g_cfg,0,sizeof(mainConfig_t));
	g_cfg.mqtt_port = 1883;
	g_cfg.ident0 = CFG_IDENT_0;
	g_cfg.ident1 = CFG_IDENT_1;
	g_cfg.ident2 = CFG_IDENT_2;
	strcpy(g_cfg.mqtt_host, "192.168.0.113");
	strcpy(g_cfg.mqtt_brokerName, "test");
	strcpy(g_cfg.mqtt_userName, "homeassistant");
	strcpy(g_cfg.mqtt_pass, "qqqqqqqqqq");
	// already zeroed but just to remember, open AP by default
	g_cfg.wifi_ssid[0] = 0;
	g_cfg.wifi_pass[0] = 0;
	// i am not sure about this, because some platforms might have no way to store mac outside our cfg?
	memcpy(g_cfg.mac,mac,6);
	strcpy(g_cfg.webappRoot, "https://openbekeniot.github.io/webapp/");
	// Long unique device name, like OpenBK7231T_AABBCCDD
	sprintf(g_cfg.longDeviceName,DEVICENAME_PREFIX_FULL"_%02X%02X%02X%02X",mac[2],mac[3],mac[4],mac[5]);
	sprintf(g_cfg.shortDeviceName,DEVICENAME_PREFIX_SHORT"%02X%02X%02X%02X",mac[2],mac[3],mac[4],mac[5]);

}

const char *CFG_GetWebappRoot(){
	return g_cfg.webappRoot;
}
const char *CFG_GetShortStartupCommand() {
	return g_cfg.initCommandLine;
}


void CFG_SetShortStartupCommand_AndExecuteNow(const char *s) {
	CFG_SetShortStartupCommand(s);
	CMD_ExecuteCommand(s);
}
void CFG_SetShortStartupCommand(const char *s) {
	// this will return non-zero if there were any changes 
	if(strcpy_safe_checkForChanges(g_cfg.initCommandLine, s,sizeof(g_cfg.initCommandLine))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
int CFG_SetWebappRoot(const char *s) {
	// this will return non-zero if there were any changes 
	if(strcpy_safe_checkForChanges(g_cfg.webappRoot, s,sizeof(g_cfg.webappRoot))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
	return 1;
}

const char *CFG_GetDeviceName(){
	return g_cfg.longDeviceName;
}
const char *CFG_GetShortDeviceName(){
	return g_cfg.shortDeviceName;
}

int CFG_GetMQTTPort() {
	return g_cfg.mqtt_port;
}
void CFG_SetMQTTPort(int p) {
	// is there a change?
	if(g_cfg.mqtt_port != p) {
		g_cfg.mqtt_port = p;
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_SetOpenAccessPoint() {
	// is there a change?
	if(g_cfg.wifi_ssid[0] == 0 && g_cfg.wifi_pass[0] == 0) {
		return;
	}
	g_cfg.wifi_ssid[0] = 0;
	g_cfg.wifi_pass[0] = 0;
	// mark as dirty (value has changed)
	g_cfg_pendingChanges++;
}
const char *CFG_GetWiFiSSID(){
	return g_cfg.wifi_ssid;
}
const char *CFG_GetWiFiPass(){
	return g_cfg.wifi_pass;
}
void CFG_SetWiFiSSID(const char *s) {
	// this will return non-zero if there were any changes 
	if(strcpy_safe_checkForChanges(g_cfg.wifi_ssid, s,sizeof(g_cfg.wifi_ssid))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_SetWiFiPass(const char *s) {
	// this will return non-zero if there were any changes 
	if(strcpy_safe_checkForChanges(g_cfg.wifi_pass, s,sizeof(g_cfg.wifi_pass))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
const char *CFG_GetMQTTHost() {
	return g_cfg.mqtt_host;
}
const char *CFG_GetMQTTBrokerName() {
	return g_cfg.mqtt_brokerName;
}
const char *CFG_GetMQTTUserName() {
	return g_cfg.mqtt_userName;
}
const char *CFG_GetMQTTPass() {
	return g_cfg.mqtt_pass;
}
void CFG_SetMQTTHost(const char *s) {
	// this will return non-zero if there were any changes 
	if(strcpy_safe_checkForChanges(g_cfg.mqtt_host, s,sizeof(g_cfg.mqtt_host))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_SetMQTTBrokerName(const char *s) {
	// this will return non-zero if there were any changes 
	if(strcpy_safe_checkForChanges(g_cfg.mqtt_brokerName, s,sizeof(g_cfg.mqtt_brokerName))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_SetMQTTUserName(const char *s) {
	// this will return non-zero if there were any changes 
	if(strcpy_safe_checkForChanges(g_cfg.mqtt_userName, s,sizeof(g_cfg.mqtt_userName))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_SetMQTTPass(const char *s) {
	// this will return non-zero if there were any changes 
	if(strcpy_safe_checkForChanges(g_cfg.mqtt_pass, s,sizeof(g_cfg.mqtt_pass))) {
		// mark as dirty (value has changed)
		g_cfg_pendingChanges++;
	}
}
void CFG_ClearPins() {
	memset(&g_cfg.pins,0,sizeof(g_cfg.pins));
	g_cfg_pendingChanges++;
}
void CFG_IncrementOTACount() {
	g_cfg.otaCounter++;
	g_cfg_pendingChanges++;
}
void CFG_Save_IfThereArePendingChanges() {
	if(g_cfg_pendingChanges > 0) {
		g_cfg.changeCounter++;
		g_cfg.crc = CFG_CalcChecksum(&g_cfg);
		HAL_Configuration_SaveConfigMemory(&g_cfg,sizeof(g_cfg));
		g_cfg_pendingChanges = 0;
	}
}
void PIN_SetPinChannelForPinIndex(int index, int ch) {
	if(index < 0 || index >= PLATFORM_GPIO_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "PIN_SetPinChannelForPinIndex: Pin index %i out of range <0,%i).",index,PLATFORM_GPIO_MAX);
		return;
	}
	if(ch < 0 || ch >= CHANNEL_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "PIN_SetPinChannelForPinIndex: Channel index %i out of range <0,%i).",ch,CHANNEL_MAX);
		return;
	}
	if(g_cfg.pins.channels[index] != ch) {
		g_cfg_pendingChanges++;
		g_cfg.pins.channels[index] = ch;
	}
}
void PIN_SetPinChannel2ForPinIndex(int index, int ch) {
	if(index < 0 || index >= PLATFORM_GPIO_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "PIN_SetPinChannel2ForPinIndex: Pin index %i out of range <0,%i).",index,PLATFORM_GPIO_MAX);
		return;
	}
	if(g_cfg.pins.channels2[index] != ch) {
		g_cfg_pendingChanges++;
		g_cfg.pins.channels2[index] = ch;
	}
}

void CFG_InitAndLoad() {
	byte chkSum;

	HAL_Configuration_ReadConfigMemory(&g_cfg,sizeof(g_cfg));
	chkSum = CFG_CalcChecksum(&g_cfg);
	if(g_cfg.ident0 != CFG_IDENT_0 || g_cfg.ident1 != CFG_IDENT_1 || g_cfg.ident2 != CFG_IDENT_2 
		|| chkSum != g_cfg.crc) {
			addLogAdv(LOG_WARN, LOG_FEATURE_CFG, "CFG_InitAndLoad: Config crc or ident mismatch. Default config will be loaded.");
		CFG_SetDefaultConfig();
		// mark as changed
		g_cfg_pendingChanges ++;
	} else {
		addLogAdv(LOG_WARN, LOG_FEATURE_CFG, "CFG_InitAndLoad: Correct config has been loaded with %i changes count.",g_cfg.changeCounter);
	}
}
