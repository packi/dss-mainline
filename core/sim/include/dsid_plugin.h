/*
 * dsid_plugin.h
 *
 *  Created on: Aug 20, 2008
 *      Author: patrick
 */

#ifndef DSID_PLUGIN_H_
#define DSID_PLUGIN_H_

#define DSID_PLUGIN_API_VERSION 2

#ifdef __cplusplus
extern "C" {
#endif

  struct dsid_interface {
      /** Returns the value for _parameterNumber of the device specified by _handle. */
    double (*get_value)(int _handle, int _parameterNumber);
    void (*set_value)(int _handle, int _parameterNumber, double _value);
    void (*increase_value)(int _handle, int _parameterNumber);
    void (*decrease_value)(int _handle, int _parameterNumber);
    void (*call_scene)(int _handle, int _sceneNr);
    void (*save_scene)(int _handle, int _sceneNr);
    void (*undo_scene)(int _handle, int _sceneNr);
    void (*enable)(int _handle);
    void (*disable)(int _handle);
    void (*get_group)(int _handle);

    void (*start_dim)(int _handle, int _directionUp, int _parameterNumber);
    void (*end_dim)(int _handle, int _parameterNumber);
    int (*get_group_id)(int _handle);
    int (*get_function_id)(int _handle);

    const char* (*get_parameter_name)(int _handle, int _parameterNumber);

    void (*set_configuration_parameter)(int _handle, const char* _name, const char* _value);
    int (*get_configuration_parameter)(int _handle, const char* _name, char* _value, int _maxLen);
  };

  int dsid_getversion();
  const char* dsid_get_plugin_name();
  int dsid_create_instance();
  void dsid_register_callback(void (*callback)(int _handle, int _eventID));
  void dsid_destroy_instance(int _handle);
  struct dsid_interface* dsid_get_interface();


#ifdef __cplusplus
}
#endif

#endif /* DSID_PLUGIN_H_ */
