/**
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * Utilities for converting Nordic datastructures to Espruino and vice versa
 * ----------------------------------------------------------------------------
 */
#include <stdio.h>

#include "bt.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "jswrap_bluetooth.h"
#include "bluetooth.h"
#include "jsutils.h"
#include "jsparse.h"

#include "BLE/esp32_gap_func.h"
#include "BLE/esp32_gatts_func.h"
#include "BLE/esp32_gattc_func.h"
#include "BLE/esp32_bluetooth_utils.h"
#include "jshardwareESP32.h"

#define UNUSED(x) (void)(x)
 
volatile BLEStatus bleStatus;
uint16_t bleAdvertisingInterval;           /**< The advertising interval (in units of 0.625 ms). */
volatile uint16_t m_peripheral_conn_handle;    /**< Handle of the current connection. */
volatile uint16_t m_central_conn_handles[1]; /**< Handle of central mode connection */

/** Initialise the BLE stack */
void jsble_init(){
	esp_err_t ret;
	if(ESP32_Get_NVS_Status(ESP_NETWORK_BLE)){
		ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
		if(ret) {
			jsExceptionHere(JSET_ERROR,"mem release failed:%x\n",ret);
			return;
		}
	
		if(initController()) return;
		if(initBluedroid()) return;
		if(registerCallbacks()) return;
		setMtu();
		gap_init_security();
	}
	else{
		ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT); 
		jsWarn("Bluetooth is disabled per ESP32.enableBLE(false)\n");
	}
}
/** Completely deinitialise the BLE stack. Return true on success */
bool jsble_kill(){
	jsWarn("kill not implemented yet\n");
  return true;
}

void jsble_queue_pending_buf(BLEPending blep, uint16_t data, char *ptr, size_t len){
	jsWarn("queue_pending_buf not implemented yet");
	UNUSED(blep);	
	UNUSED(data);
	UNUSED(ptr);
	UNUSED(len);
}

void jsble_queue_pending(BLEPending blep, uint16_t data){
	jsWarn("queue_pending not implemented yet");
	UNUSED(blep);
	UNUSED(data);
}

int jsble_exec_pending(IOEvent *event){
	jsWarn("exec_pending not implemented yet");
	UNUSED(event);
	return 0;
}

void jsble_restart_softdevice(JsVar *jsFunction){
	bleStatus &= ~(BLE_NEEDS_SOFTDEVICE_RESTART | BLE_SERVICES_WERE_SET);
	if (bleStatus & BLE_IS_SCANNING) {
		bluetooth_gap_setScan(false);
	}
	if (jsvIsFunction(jsFunction))
	  jspExecuteFunction(jsFunction,NULL,0,NULL);
	jswrap_ble_reconfigure_softdevice();
}

uint32_t jsble_advertising_start(){
	esp_err_t status;
	if (bleStatus & BLE_IS_ADVERTISING) return;
	status = bluetooth_gap_startAdvertizing(true);
	return status;
}
void jsble_advertising_stop(){
	esp_err_t status;
	status = bluetooth_gap_startAdvertizing(false);
	if(status){
	   jsExceptionHere(JSET_ERROR,"error in stop advertising:0X%x",status);
	}
}
/** Is BLE connected to any device at all? */
bool jsble_has_connection(){
#if CENTRAL_LINK_COUNT>0
  return (m_central_conn_handles[0] != BLE_GATT_HANDLE_INVALID) ||
         (m_peripheral_conn_handle != BLE_GATT_HANDLE_INVALID);
#else
  return m_peripheral_conn_handle != BLE_GATT_HANDLE_INVALID;
#endif
}

/** Is BLE connected to a central device at all? */
bool jsble_has_central_connection(){
#if CENTRAL_LINK_COUNT>0
  return (m_central_conn_handles[0] != BLE_GATT_HANDLE_INVALID);
#else
  return false;
#endif
}

/** Is BLE connected to a server device at all (eg, the simple, 'slave' mode)? */
bool jsble_has_peripheral_connection(){
  return (m_peripheral_conn_handle != BLE_GATT_HANDLE_INVALID);
}

/// Checks for error and reports an exception if there was one. Return true on error
bool jsble_check_error_line(uint32_t err_code, int lineNumber) {
	jsWarn("check error not implemented yet:%x\n", err_code);
	UNUSED(err_code);
	UNUSED(lineNumber);
	return false;
}
/// Scanning for advertisign packets
uint32_t jsble_set_scanning(bool enabled, bool activeScan){
  if (activeScan) {
  	jsWarn("active scan not implemented\n");
  }
	bluetooth_gap_setScan(enabled);
	return 0;
}

/// returning RSSI values for current connection
uint32_t jsble_set_rssi_scan(bool enabled){
	jsWarn("set rssi scan not implemeted yet\n");
	UNUSED(enabled);
	return 0;
}

/** Actually set the services defined in the 'data' object. Note: we can
 * only do this *once* - so to change it we must reset the softdevice and
 * then call this again */
void jsble_set_services(JsVar *data){
	gatts_set_services(data);
}

/// Disconnect from the given connection
uint32_t jsble_disconnect(uint16_t conn_handle){
	return gattc_disconnect(conn_handle);
	return 0;
}

/// For BLE HID, send an input report to the receiver. Must be <= HID_KEYS_MAX_LEN
void jsble_send_hid_input_report(uint8_t *data, int length){
	jsWarn("send hid input report not implemented yet\n");
	UNUSED(data);
	UNUSED(length);
}

/// Connect to the given peer address. When done call bleCompleteTask
void jsble_central_connect(ble_gap_addr_t peer_addr, JsVar *options){
  // Ignore options for now
	gattc_connect(peer_addr.addr);
}
/// Get primary services. Filter by UUID unless UUID is invalid, in which case return all. When done call bleCompleteTask
void jsble_central_getPrimaryServices(uint16_t central_conn_handle, ble_uuid_t uuid){
	gattc_searchService(uuid);
}
/// Get characteristics. Filter by UUID unless UUID is invalid, in which case return all. When done call bleCompleteTask
void jsble_central_getCharacteristics(uint16_t central_conn_handle, JsVar *service, ble_uuid_t uuid){
	gattc_getCharacteristic(uuid);
	UNUSED(service);
}
// Write data to the given characteristic. When done call bleCompleteTask
void jsble_central_characteristicWrite(uint16_t central_conn_handle, JsVar *characteristic, char *dataPtr, size_t dataLen){
	uint16_t handle = jsvGetIntegerAndUnLock(jsvObjectGetChild(characteristic, "handle_value", 0));
	gattc_writeValue(handle,dataPtr,dataLen);
}
// Read data from the given characteristic. When done call bleCompleteTask
void jsble_central_characteristicRead(uint16_t central_conn_handle, JsVar *characteristic){
	uint16_t handle = jsvGetIntegerAndUnLock(jsvObjectGetChild(characteristic, "handle_value", 0));
	gattc_readValue(handle);
}
// Discover descriptors of characteristic
void jsble_central_characteristicDescDiscover(uint16_t central_conn_handle, JsVar *characteristic){
	jsWarn("Central characteristicDescDiscover not implemented yet\n");
	UNUSED(characteristic);
}
// Set whether to notify on the given characteristic. When done call bleCompleteTask
void jsble_central_characteristicNotify(uint16_t central_conn_handle, JsVar *characteristic, bool enable){
	jsWarn("central characteristic notify not implemented yet\n");
	UNUSED(characteristic);
	UNUSED(enable);
}
/// Start bonding on the current central connection
void jsble_central_startBonding(uint16_t central_conn_handle, bool forceRePair){
	jsWarn("central start bonding not implemented yet\n");
	UNUSED(forceRePair);
}
/// RSSI monitoring in central mode
uint32_t jsble_set_central_rssi_scan(uint16_t central_conn_handle, bool enabled){
	jsWarn("central set rssi scan not implemented yet\n");
	return 0;
}
// Set whether or not the whitelist is enabled
void jsble_central_setWhitelist(uint16_t central_conn_handle, bool whitelist){
	jsWarn("central set Whitelist not implemented yet\n");
	return 0;
}

void jsble_update_security() {
}

/// Return an object showing the security status of the given connection
JsVar *jsble_get_security_status(uint16_t conn_handle) {
  return 0;
}

/// Set the transmit power of the current (and future) connections
void jsble_set_tx_power(int8_t pwr) {
  jsWarn("jsble_set_tx_power not implemented yet\n");
}

uint32_t jsble_central_send_passkey(uint16_t central_conn_handle, char *passkey) {
  jsWarn("central set Whitelist not implemented yet\n");
  return 0;
}
