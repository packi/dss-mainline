/*
 * pluginbase.cpp
 *
 *  Created on: Apr 27, 2009
 *      Author: patrick
 */

#include "../include/dsid_plugin.h"
#include "pluginbase.h"

DSIDFactory* DSIDFactory::m_Instance = 0;


int dsid_getversion() {
  return DSID_PLUGIN_API_VERSION;
} // dsid_getversion

const char* dsid_get_plugin_name() {
  return DSIDFactory::getInstance().getPluginName().c_str();
} // dsid_get_plugin_name

int dsid_create_instance() {
  return DSIDFactory::getInstance().createDSID();
}

void dsid_register_callback(void (*callback)(int _handle, int _eventID)) {
}

void dsid_destroy_instance(int _handle) {
  DSIDFactory::getInstance().destroyDSID(_handle);
}

double get_value(int _handle, int _parameterNumber) {
  return DSIDFactory::getInstance().getDSID(_handle)->getValue(_parameterNumber);
} // get_value

void set_value(int _handle, int _parameterNumber, double _value) {
  DSIDFactory::getInstance().getDSID(_handle)->setValue(_value, _parameterNumber);
} // set_value

void increase_value(int _handle, int _parameterNumber) {
  DSIDFactory::getInstance().getDSID(_handle)->increaseValue(_parameterNumber);
} // increase_value

void decrease_value(int _handle, int _parameterNumber) {
  DSIDFactory::getInstance().getDSID(_handle)->decreaseValue(_parameterNumber);
} // decrease_value

void call_scene(int _handle, int _sceneNr) {
  DSIDFactory::getInstance().getDSID(_handle)->callScene(_sceneNr);
} // call_scene

void save_scene(int _handle, int _sceneNr) {
  DSIDFactory::getInstance().getDSID(_handle)->saveScene(_sceneNr);
} // save_scene

void undo_scene(int _handle, int _sceneNr) {
  DSIDFactory::getInstance().getDSID(_handle)->undoScene(_sceneNr);
} // undo_scene

void enable(int _handle) {
  DSIDFactory::getInstance().getDSID(_handle)->enable();
} // enable

void disable(int _handle) {
  DSIDFactory::getInstance().getDSID(_handle)->disable();
} // disable

void get_group(int _handle) {
} // get_group

void start_dim(int _handle, int _directionUp, int _parameterNumber) {
} // start_dim

void end_dim(int _handle, int _parameterNumber) {
} // end_dim

int get_group_id(int _handle) {
  return -1;
} // get_group_id

int get_function_id(int _handle) {
  return -1;
} // get_function_id

const char* get_parameter_name(int _handle, int _parameterNumber) {
  return NULL;
} // get_parameter_name

void set_configuration_parameter(int _handle, const char* _name, const char* _value) {
  DSIDFactory::getInstance().getDSID(_handle)->setConfigurationParameter(_name, _value);
}

static struct dsid_interface intf_description = {
  &get_value,
  &set_value,
  &increase_value,
  &decrease_value,
  &call_scene,
  &save_scene,
  &undo_scene,
  &enable,
  &disable,
  &get_group,

  &start_dim,
  &end_dim,
  &get_group_id,
  &get_function_id,

  &get_parameter_name,

  &set_configuration_parameter,
};


struct dsid_interface* dsid_get_interface() {
  return &intf_description;
}

