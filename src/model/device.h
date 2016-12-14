/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

    This file is part of digitalSTROM Server.

    digitalSTROM Server is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    digitalSTROM Server is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef DEVICE_H
#define DEVICE_H

#include <cassert>
#include <iosfwd>
#include <bitset>
#include <utility>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "src/ds485types.h"
#include "src/datetools.h"
#include "src/model/data_types.h"
#include "addressablemodelitem.h"
#include "businterface.h"
#include "modelevent.h"
#include "modelconst.h"

#define DEV_PARAM_BUTTONINPUT_STANDARD              0
#define DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT1   5
#define DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT2   6
#define DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT3   7
#define DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT4   8
#define DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT1   9
#define DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT2   10
#define DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT3   11
#define DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT4   12
#define DEV_PARAM_BUTTONINPUT_2WAY                  13
#define DEV_PARAM_BUTTONINPUT_1WAY                  14
#define DEV_PARAM_BUTTONINPUT_AKM_STANDARD          16
#define DEV_PARAM_BUTTONINPUT_AKM_INVERTED          17
#define DEV_PARAM_BUTTONINPUT_AKM_ON_RISING_EDGE    18
#define DEV_PARAM_BUTTONINPUT_AKM_ON_FALLING_EDGE   19
#define DEV_PARAM_BUTTONINPUT_AKM_OFF_RISING_EDGE   20
#define DEV_PARAM_BUTTONINPUT_AKM_OFF_FALLING_EDGE  21
#define DEV_PARAM_BUTTONINPUT_AKM_RISING_EDGE       22
#define DEV_PARAM_BUTTONINPUT_AKM_FALLING_EDGE      23

#define DEV_PARAM_BUTTONINPUT_SDS_SLAVE_M1_M2       0xff

#define BUTTON_ACTIVE_GROUP_RESET    0xff

// pairing JSON strings
#define BUTTONINPUT_1WAY            "1way"
#define BUTTONINPUT_2WAY_DOWN       "2way_down"
#define BUTTONINPUT_2WAY_UP         "2way_up"
#define BUTTONINPUT_2WAY            "2way"
#define BUTTONINPUT_1WAY_COMBINED   "1way_combined"

// AKM JSON strings
#define BUTTONINPUT_AKM_STANDARD            "standard"
#define BUTTONINPUT_AKM_INVERTED            "inverted"
#define BUTTONINPUT_AKM_ON_RISING_EDGE      "on_rising_edge"
#define BUTTONINPUT_AKM_ON_FALLING_EDGE     "on_falling_edge"
#define BUTTONINPUT_AKM_OFF_RISING_EDGE     "off_rising_edge"
#define BUTTONINPUT_AKM_OFF_FALLING_EDGE    "off_falling_edge"
#define BUTTONINPUT_AKM_RISING_EDGE         "rising_edge"
#define BUTTONINPUT_AKM_FALLING_EDGE        "falling_edge"

// UMR output modes for pairing
#define OUTPUT_MODE_TWO_STAGE_SWITCH        34
#define OUTPUT_MODE_BIPOLAR_SWITCH          43
#define OUTPUT_MODE_THREE_STAGE_SWITCH      38
namespace vdcapi {
  class PropertyElement;
}
namespace dss {

  class Group;
  class State;
  class DSMeter;
  class VdcElementReader;
  struct VdsdSpec_t;

  typedef struct {
    bool dontcare;
    bool localprio;
    bool specialmode;
    bool flashmode;
    uint8_t ledconIndex;
    uint8_t dimtimeIndex;
  } DeviceSceneSpec_t;

  typedef struct {
    uint8_t colorSelect;
    uint8_t modeSelect;
    bool dimMode;
    bool rgbMode;
    bool groupColorMode;
  } DeviceLedSpec_t;

  typedef struct {
    std::string name;
    uint8_t sensorIndex;
    uint8_t test;
    uint8_t action;
    uint16_t value;
    uint16_t hysteresis;
    uint8_t sceneDeviceMode;
    uint8_t validity;
    uint8_t buttonNumber;
    uint8_t clickType;
    uint8_t sceneID;
  } DeviceSensorEventSpec_t;

  typedef struct {
    uint16_t protectionTimer;
    uint16_t emergencyTimer;
    uint8_t emergencyControlValue;
  } DeviceValveTimerSpec_t;

  typedef struct {
    uint16_t pwmPeriod;
    uint8_t pwmMinX;
    uint8_t pwmMaxX;
    uint8_t pwmMinY;
    uint8_t pwmMaxY;
    int8_t pwmOffset;
  } DeviceValvePwmSpec_t;

  typedef struct {
    bool ctrlClipMinZero;
    bool ctrlClipMinLower;
    bool ctrlClipMaxHigher;
    bool ctrlNONC;
    uint8_t ctrlRawValue;
  } DeviceValveControlSpec_t;

  typedef struct {
    bool pairing;       // device supports pairing
    bool syncButtonID;  // sync button ID setting of slave with master
    bool hasOutputAngle;
    bool posTimeMax; // device supports maximum positioning time
  } DeviceFeatures_t;

  class Device;
  struct DeviceBinaryInput : boost::noncopyable {
    int m_inputIndex;        // input line index
    BinaryInputType m_inputType; // type of input signal
    BinaryInputId m_inputId;           // target Id, like ButtonId
    GroupType m_targetGroupType;   // type of target group: standard, user, apartment
    int m_targetGroupId;     // index of target group, 0..63

    DeviceBinaryInput(Device& device, const DeviceBinaryInputSpec_t& spec, int index);
    ~DeviceBinaryInput();

    const State& getState() const { return *m_state; }
    void setTarget(GroupType type, uint8_t group);
    void setInputId(BinaryInputId inputId);
    void setInputType(BinaryInputType inputType);
    void handleEvent(BinaryInputState inputState);
  private:
    __DECL_LOG_CHANNEL__;
    Device& m_device;
    std::string m_name;
    boost::shared_ptr<State> m_state;
    class GroupStateHandle;
    friend class GroupStateHandle;
    std::unique_ptr<GroupStateHandle> m_groupState;

    void updateGroupState();
  };

  typedef struct {
    int m_sensorIndex;       // sensor index
    SensorType m_sensorType;
    uint32_t m_sensorPollInterval;
    bool m_sensorBroadcastFlag;
    bool m_sensorPushConversionFlag;
    bool m_sensorValueValidity;
    unsigned int m_sensorValue;
    double m_sensorValueFloat;
    DateTime m_sensorValueTS;
  } DeviceSensor_t;

  typedef enum {
    DEVICE_TYPE_INVALID = -1,
    DEVICE_TYPE_KM      = 0,
    DEVICE_TYPE_TKM     = 1,
    DEVICE_TYPE_SDM     = 2,
    DEVICE_TYPE_KL      = 3,
    DEVICE_TYPE_TUP     = 4,
    DEVICE_TYPE_ZWS     = 5,
    DEVICE_TYPE_SDS     = 6,
    DEVICE_TYPE_SK      = 7,
    DEVICE_TYPE_AKM     = 8,
    DEVICE_TYPE_TNY     = 10,
    DEVICE_TYPE_UMV     = 11,
    DEVICE_TYPE_UMR     = 12,
  } DeviceTypes_t;

  typedef enum {
    DEVICE_CLASS_INVALID    = -1,
    DEVICE_CLASS_GE         = 1,
    DEVICE_CLASS_GR         = 2,
    DEVICE_CLASS_BL         = 3,
    DEVICE_CLASS_TK         = 4,
    DEVICE_CLASS_MG         = 5,
    DEVICE_CLASS_RT         = 6,
    DEVICE_CLASS_GN         = 7,
    DEVICE_CLASS_SW         = 8,
    DEVICE_CLASS_WE         = 9
  } DeviceClasses_t;

  typedef enum {
    DEVICE_OEM_UNKNOWN,
    DEVICE_OEM_NONE,
    DEVICE_OEM_LOADING,
    DEVICE_OEM_VALID,
  } DeviceOEMState_t;

  typedef enum {
    DEVICE_OEM_EAN_NO_EAN_CONFIGURED = 0,
    DEVICE_OEM_EAN_NO_INTERNET_ACCESS = 1,
    DEVICE_OEM_EAN_INTERNET_ACCESS_OPTIONAL = 2,
    DEVICE_OEM_EAN_INTERNET_ACCESS_MANDATORY = 3,
  } DeviceOEMInetState_t;

  typedef enum {
    DEVICE_VALVE_UNKNOWN = 0,
    DEVICE_VALVE_FLOOR = 1,
    DEVICE_VALVE_RADIATOR = 2,
    DEVICE_VALVE_WALL = 3,
  } DeviceValveType_t;

  /** Represents a dsID */
  class Device : public AddressableModelItem,
                 public boost::noncopyable {
  private:
    std::string m_Name;
    dsuid_t m_DSID;
    devid_t m_ShortAddress;
    devid_t m_LastKnownShortAddress;
    int m_ZoneID;
    int m_LastKnownZoneID;
    dsuid_t m_DSMeterDSID;
    dsuid_t m_LastKnownMeterDSID;
    std::string m_DSMeterDSIDstr; // for proptree publishing
    std::string m_DSMeterDSUIDstr; // for proptree publishing
    std::string m_LastKnownMeterDSIDstr; // for proptree publishing
    std::string m_LastKnownMeterDSUIDstr; // for proptree publishing
    std::vector<int> m_Groups;
    int m_ActiveGroup;
    int m_DefaultGroup;
    int m_FunctionID;
    int m_ProductID;
    int m_VendorID;
    int m_RevisionID;
    std::string m_hardwareInfo;
    std::string m_GTIN;
    int m_LastCalledScene;
    int m_LastButOneCalledScene;
    unsigned long m_Consumption;
    unsigned long m_Energymeter;
    DateTime m_LastDiscovered;
    DateTime m_FirstSeen;
    bool m_IsValid;
    bool m_IsLockedInDSM;

    uint8_t m_OutputMode;
    uint8_t m_ButtonInputMode;
    uint8_t m_ButtonInputIndex;
    uint8_t m_ButtonInputCount;
    bool m_ButtonSetsLocalPriority;
    bool m_ButtonCallsPresent;
    int m_ButtonGroupMembership;
    int m_ButtonActiveGroup;
    int m_ButtonID;
    unsigned long long m_OemEanNumber;
    uint16_t m_OemSerialNumber;
    uint8_t m_OemPartNumber;
    DeviceOEMInetState_t m_OemInetState;
    DeviceOEMState_t m_OemState;
    bool m_OemIsIndependent;

    PropertyNodePtr m_pAliasNode;
    PropertyNodePtr m_TagsNode;
    std::string m_HWInfo;
    std::string m_iconPath;
    DeviceOEMState_t m_OemProductInfoState;
    std::string m_OemProductName;
    std::string m_OemProductIcon;
    std::string m_OemProductURL;
    std::string m_OemConfigLink;

    bool m_isVdcDevice;
    std::unique_ptr<VdsdSpec_t> m_vdcSpec;
    std::string m_VdcHardwareModelGuid;
    std::string m_VdcModelUID;
    std::string m_VdcModelVersion;
    std::string m_VdcVendorGuid;
    std::string m_VdcOemGuid;
    std::string m_VdcOemModelGuid;
    std::string m_VdcConfigURL;
    std::string m_VdcHardwareGuid;
    std::string m_VdcHardwareInfo;
    std::string m_VdcHardwareVersion;
    std::string m_VdcIconPath;
    boost::shared_ptr<std::vector<int> > m_VdcModelFeatures;
    bool m_hasActions;

    DeviceValveType_t m_ValveType;

    bool m_IsConfigLocked;

    std::vector<boost::shared_ptr<DeviceBinaryInput> > m_binaryInputs;

    typedef std::map<std::string, boost::shared_ptr<State> > States;
    States m_states;

    uint8_t m_sensorInputCount;
    std::vector<boost::shared_ptr<DeviceSensor_t> > m_sensorInputs;
    uint8_t m_outputChannelCount;
    std::vector<int> m_outputChannels;

    std::string m_AKMInputProperty;

    mutable boost::recursive_mutex m_deviceMutex;

    CardinalDirection_t m_cardinalDirection; //<  wind protection of blinds
    WindProtectionClass_t m_windProtectionClass;
    int m_floor;
    int m_pairedDevices; // currently relevant for TNY only
    bool m_visible; // currently relevant for TNY only

  protected:
    /** Sends the application a note that something has changed.
     * This will cause the \c apartment.xml to be updated. */
    void dirty();

    /** Sends the application a note that the cluster for the given device
     *  has to be checked and reassigned. */
    void checkAutoCluster();

    void fillSensorTable(std::vector<DeviceSensorSpec_t>& _slist);
    bool hasExtendendSceneTable();
    void calculateHWInfo();
    void updateIconPath();
    std::string getAKMButtonInputString(const int _mode);
    bool hasBlinkSettings();

  public:
    /** Creates and initializes a device. */
    Device(const dsuid_t _dsid, Apartment* _pApartment);
    virtual ~Device();

    boost::shared_ptr<Device> sharedFromThis() { return boost::static_pointer_cast<Device>(shared_from_this()); }
    /** @copydoc DeviceReference::isOn() */
    virtual bool isOn() const;

    /** Set device configuration values */
    void setDeviceConfig(uint8_t _configClass, uint8_t _configIndex, uint8_t _value);
    void setDeviceConfig16(uint8_t _configClass, uint8_t _configIndex, uint16_t _value);
    void setDeviceButtonID(uint8_t _buttonId);
    void setDeviceButtonActiveGroup(uint8_t _buttonActiveGroup);
    void setDeviceButtonConfig();
    void setDeviceJokerGroup(uint8_t _groupId);
    void setDeviceOutputMode(uint8_t _modeId);
    void setDeviceButtonInputMode(uint8_t _modeId);
    void setProgMode(uint8_t _modeId);

    void increaseDeviceOutputChannelValue(uint8_t _channel);
    void decreaseDeviceOutputChannelValue(uint8_t _channel);
    void stopDeviceOutputChannelValue(uint8_t _channel);
    uint16_t getDeviceOutputChannelValue(uint8_t _channel);
    void setDeviceOutputChannelValue(uint8_t _channel, uint8_t _size,
                                     uint16_t _value, bool _applyNow = true);
    uint16_t getDeviceOutputChannelSceneValue(uint8_t _channel, uint8_t _scene);
    void setDeviceOutputChannelSceneValue(uint8_t _channel, uint8_t _size,
                                          uint8_t _scene, uint16_t _value);
    void getDeviceOutputChannelSceneConfig(uint8_t _scene,
                                           DeviceSceneSpec_t& _config);
    void setDeviceOutputChannelSceneConfig(uint8_t _scene,
                                           DeviceSceneSpec_t _config);
    void setDeviceOutputChannelDontCareFlags(uint8_t _scene, uint16_t _value);
    uint16_t getDeviceOutputChannelDontCareFlags(uint8_t _scene);

    std::string getDisplayID() const;

    bool is2WayMaster() const;
    bool is2WaySlave() const;
    bool hasMultibuttons() const;
    bool hasInput() const;
    bool hasOutput() const;
    DeviceTypes_t getDeviceType() const;
    int getDeviceNumber() const;
    DeviceClasses_t getDeviceClass() const;
    const DeviceFeatures_t getFeatures() const;
    std::string getAKMInputProperty() const;
    int multiDeviceIndex() const;

    /** Configure scene configuration */
    void setDeviceSceneMode(uint8_t _sceneId, DeviceSceneSpec_t _config);
    void getDeviceSceneMode(uint8_t _sceneId, DeviceSceneSpec_t& _config);
    void setDeviceLedMode(uint8_t _ledconIndex, DeviceLedSpec_t _config);
    void getDeviceLedMode(uint8_t _ledconIndex, DeviceLedSpec_t& _config);
    void setDeviceTransitionTime(uint8_t _dimtimeIndex, int up, int down);
    void getDeviceTransitionTime(uint8_t _dimtimeIndex, int& up, int& down);
    int transitionVal2Time(uint8_t _value);
    uint8_t transitionTimeEval(int timems);
    void setSceneValue(const int _scene, const int _value);
    int getSceneValue(const int _scene);
    void setSceneAngle(const int _scene, const int _angle);
    int getSceneAngle(const int _scene);
    void configureAreaMembership(const int _areaScene, const bool _addToArea);

    /** Configure climate actuator */
    void setDeviceValveTimer(DeviceValveTimerSpec_t _config);
    void getDeviceValveTimer(DeviceValveTimerSpec_t& _config);
    void setDeviceValvePwm(DeviceValvePwmSpec_t _config);
    void getDeviceValvePwm(DeviceValvePwmSpec_t& _config);
    void setDeviceValveControl(DeviceValveControlSpec_t _config);
    void getDeviceValveControl(DeviceValveControlSpec_t& _config);

    /** Binary input devices */
    void setDeviceBinaryInputId(uint8_t _inputIndex, BinaryInputId _targetId);
    void setDeviceBinaryInputTarget(uint8_t _inputIndex, GroupType targetType, uint8_t _targetGroup);
    void setDeviceBinaryInputType(uint8_t _inputIndex, BinaryInputType _inputType);
    BinaryInputType getDeviceBinaryInputType(uint8_t _inputIndex);
    /** AKM2xx timeout settings */
    void setDeviceAKMInputTimeouts(int _onDelay, int _offDelay);
    void getDeviceAKMInputTimeouts(int& _onDelay, int& _offDelay);

    /** Returns device configuration value */
    uint8_t getDeviceConfig(uint8_t _configIndex, uint8_t _value);
    uint16_t getDeviceConfigWord(uint8_t _configIndex, uint8_t _value);

    /** Verify communication path */
    std::pair<uint8_t, uint16_t> getDeviceTransmissionQuality();

    /** Set device output value */
    void setDeviceValue(uint8_t _value);
    void setDeviceOutputValue(uint8_t _offset, uint16_t _value);
    uint16_t getDeviceOutputValue(uint8_t _offset);

    /** query device sensor values */
    uint32_t getDeviceSensorValue(const int _sensorIndex);

    /** query cached last known values */
    virtual unsigned long getPowerConsumption();
    virtual unsigned long getEnergymeterValue();

    /** convenience methods for simulated devices */
    virtual void nextScene(const callOrigin_t _origin, const SceneAccessCategory _category);
    virtual void previousScene(const callOrigin_t _origin, const SceneAccessCategory _category);

    /** Returns the function ID of the device.
     * A function ID specifies a certain subset of functionality that
     * a device implements.
     */
    int getFunctionID() const;
    /** Sets the functionID to \a _value */
    void setFunctionID(const int _value);

    /** Returns the Product ID of the device.
     * The Product ID identifies the manufacturer and type of device.
     */
    int getProductID() const;
    /** Sets the ProductID to \a _value */
    void setProductID(const int _value);

    /** Returns the Revision ID of the device.
     * The revision identifies the device hardware revision.
     */
    int getRevisionID() const;
    /** Sets the ProductID to \a _value */
    void setRevisionID(const int _value);

    /** Returns the Vendor ID of the device.
     * The ID identifies the device hardware vendor.
     */
    int getVendorID() const;
    /** Sets the VendorID to \a _value */
    void setVendorID(const int _value);

    /** Returns the name of the device. */
    const std::string& getName() const;
    /** Sets the name of the device.
     * @note This will cause the apartment to store
     * \c apartment.xml
     */
    void setName(const std::string& _name);

    const std::string& getHWInfo() const { return m_HWInfo; }
    const std::string& getGTIN() const { return m_GTIN; }
    const std::string& getIconPath() const { return m_iconPath; }

    static const std::string getDeviceTypeString(const DeviceTypes_t _type);
    static const std::string getDeviceClassString(const DeviceClasses_t _class);
    static const std::string getColorString(const int _class);

    /** Returns wheter the device is in group \a _groupID or not. */
    bool isInGroup(const int _groupID) const;
    /** Adds the device to group \a _groupID. */
    void addToGroup(const int _groupID);
    /** Removes the device from group \a _groupID */
    void removeFromGroup(const int _groupID);

    /** Returns the group id of the \a _index'th group */
    int getGroupIdByIndex(const int _index) const;
    /** Returns \a _index'th group of the device */
    boost::shared_ptr<Group> getGroupByIndex(const int _index);
    /** Returns the number of groups the device is a member of */
    int getGroupsCount() const;
    /** Returns the vector of groups the device is in */
    const std::vector<int>& getGroups() const;
    /** Retuturns group to which the joker is configured or -1 if device is not
        a joker */
    int getJokerGroup() const;
    /** Returns the zoneID that this device group is in. */
    int getGroupZoneID(int groupID);

    /** Removes the device from all group.
     * The device will remain in the broadcastgroup though.
     */
    void resetGroups();

    /** Returns the last called scene.
     * At the moment this information is used to determine whether a device is
     * turned on or not. */
    int getLastCalledScene() const { return m_LastCalledScene; }
    /** Sets the last called scene. */
    void setLastCalledScene(const int _value) {
      if (_value != m_LastCalledScene) {
        m_LastButOneCalledScene = m_LastCalledScene;
        m_LastCalledScene = _value;
      }
    }
    /** If state hasn't changed undo the last called scene command and restore state. */
    void setLastButOneCalledScene(const int _value) {
      if (_value == m_LastCalledScene) {
        m_LastCalledScene = m_LastButOneCalledScene;
      }
    }
    /** Undo the last called scene command and restore state. */
    void setLastButOneCalledScene() {
      m_LastCalledScene = m_LastButOneCalledScene;
    }

    /** Returns the short address of the device. This is the address
     * the device got from the dSM. */
    devid_t getShortAddress() const;
    /** Sets the short-address of the device. */
    void setShortAddress(const devid_t _shortAddress);

    devid_t getLastKnownShortAddress() const;
    void setLastKnownShortAddress(const devid_t _shortAddress);
    /** Returns the DSID of the device */
    dsuid_t getDSID() const;
    /** Returns the DSID of the dsMeter the device is connected to */
    dsuid_t getDSMeterDSID() const;

    const dsuid_t& getLastKnownDSMeterDSID() const;
    void setLastKnownDSMeterDSID(const dsuid_t& _value);
    void setDSMeter(boost::shared_ptr<DSMeter> _dsMeter);

    /** Returns the zone ID the device resides in. */
    int getZoneID() const;
    /** Sets the zone ID of the device. */
    void setZoneID(const int _value);
    int getLastKnownZoneID() const;
    void setLastKnownZoneID(const int _value);
    /** Returns the apartment the device resides in. */
    Apartment& getApartment() const;

    const DateTime& getLastDiscovered() const { return m_LastDiscovered; }
    const DateTime& getFirstSeen() const { return m_FirstSeen; }
    void setFirstSeen(const DateTime& _value) { m_FirstSeen = _value; }

    const PropertyNodePtr& getPropertyNode() const { return m_pPropertyNode; }
    PropertyNodePtr getPropertyNode() { return m_pPropertyNode; }

    void updateAKMNode();
    /** Publishes the device to the property tree.
     * @see DSS::getPropertySystem */
    void publishToPropertyTree();
    /** Removes the device from the propertytree. */
    void removeFromPropertyTree();

    /** Returns wheter the dSM has been told to never forget
        this device (cached value) */
    bool getIsLockedInDSM() const { return m_IsLockedInDSM; }
    void setIsLockedInDSM(const bool _value);

    /** Returns whether the device data is completely synchronized or
        the synchronization is pending. */
    bool isValid() const { return m_IsValid; }
    void setIsValid(const bool _value) { m_IsValid = _value; }

    /** Tells the dSM to never forget a device. */
    void lock();
    /** Tells the dSM that it may forget a device if it's not present. */
    void unlock();

    /** Device level active and default group used for Global Applications. */
    void setActiveGroup(const int _value) { m_ActiveGroup = _value; }
    int getActiveGroup() const { return m_ActiveGroup; }
    void setDefaultGroup(const int _value) { m_DefaultGroup = _value; }
    int getDefaultGroup() const { return m_DefaultGroup; }

    void setButtonSetsLocalPriority(const bool _value) { m_ButtonSetsLocalPriority = _value; }
    bool getButtonSetsLocalPriority() const { return m_ButtonSetsLocalPriority; }
    void setButtonCallsPresent(const bool _value) { m_ButtonCallsPresent = _value; }
    bool getButtonCallsPresent() const { return m_ButtonCallsPresent; }
    void setButtonGroupMembership(const int _value) { m_ButtonGroupMembership = _value; }
    int getButtonGroupMembership() const { return m_ButtonGroupMembership; }
    void setButtonActiveGroup(const int _value) { m_ButtonActiveGroup = _value; }
    int getButtonActiveGroup() const { return m_ButtonActiveGroup; }
    void setButtonID(const int _value) { m_ButtonID = _value; }
    int getButtonID() const { return m_ButtonID; }
    uint8_t getOutputMode() const { return m_OutputMode; }
    void setOutputMode(const uint8_t _value);
    void setButtonInputMode(const uint8_t _value);
    uint8_t getButtonInputMode() const { return m_ButtonInputMode; }
    void setButtonInputIndex(const uint8_t _value) { m_ButtonInputIndex = _value; }
    uint8_t getButtonInputIndex() const { return m_ButtonInputIndex; }
    void setButtonInputCount(const uint8_t _value) { m_ButtonInputCount = _value; }
    uint8_t getButtonInputCount() const { return m_ButtonInputCount; }

    bool hasTag(const std::string& _tagName) const;
    void addTag(const std::string& _tagName);
    void removeTag(const std::string& _tagName);
    std::vector<std::string> getTags();

    /** Returns wheter two devices are equal.
     * Devices are considered equal if their DSID are a match.*/
    bool operator==(const Device& _other) const;

    std::string getSensorEventName(const int _eventIndex) const;
    void setSensorEventName(const int _eventIndex, std::string& _name);
    void getSensorEventEntry(const int _eventIndex, DeviceSensorEventSpec_t& _entry);
    void setSensorEventEntry(const int _eventIndex, DeviceSensorEventSpec_t _entry);

    // valve configuration
    bool isValveDevice() const;
    const DeviceValveType_t getValveType() const;
    void setValveType(DeviceValveType_t _type);

    std::string getValveTypeAsString() const;
    bool setValveTypeAsString(const std::string &_value, bool _ignoreValveCheck = false);

    static std::string valveTypeToString(const DeviceValveType_t _type);
    static bool getValveTypeFromString(const char* _string, DeviceValveType_t &deviceType);

    void publishValveTypeToPropertyTree();

    void setOemInfo(const unsigned long long _eanNumber,
        const uint16_t _serialNumber, const uint8_t _partNumber,
        const DeviceOEMInetState_t _iNetState,
        const bool _isIndependent);
    void setOemInfoState(const DeviceOEMState_t _state);

    unsigned long long getOemEan() const { return m_OemEanNumber; }
    std::string getOemEanAsString() const { return unsignedLongIntToString(m_OemEanNumber); }
    DeviceOEMState_t getOemInfoState() const { return m_OemState; }
    std::string getOemStateAsString() const { return oemStateToString(m_OemState); }
    static std::string oemStateToString(const DeviceOEMState_t _state);
    uint16_t getOemSerialNumber() const { return m_OemSerialNumber; }
    uint8_t getOemPartNumber() const { return m_OemPartNumber; }
    bool getOemIsIndependent() const { return m_OemIsIndependent; }
    DeviceOEMInetState_t getOemInetState() const { return m_OemInetState; }
    std::string getOemInetStateAsString() const { return oemInetStateToString(m_OemInetState); }
    static std::string oemInetStateToString(const DeviceOEMInetState_t _state);

    DeviceOEMState_t getOemStateFromString(const char* _string) const;
    DeviceOEMInetState_t getOemInetStateFromString(const char* _string) const;

    void setOemProductInfo(const std::string& _productName, const std::string& _iconPath, const std::string& _productURL, const std::string& _configLink);
    void setOemProductInfoState(const DeviceOEMState_t _state);
    std::string getOemProductInfoStateAsString() const { return oemStateToString(m_OemProductInfoState); }
    const DeviceOEMState_t getOemProductInfoState() const { return m_OemProductInfoState; }
    const std::string& getOemProductName() const { return m_OemProductName; }
    const std::string& getOemProductIcon() const { return m_OemProductIcon; }
    const std::string& getOemProductURL() const { return m_OemProductURL; }
    const std::string& getOemConfigLink() const { return m_OemConfigLink; }
    bool isOemCoupledWith(boost::shared_ptr<Device> _otherDev);
    void setConfigLock(bool _lockConfig);
    bool isConfigLocked() const { return m_IsConfigLocked; }

    void publishVdcToPropertyTree();
    void setVdcDevice(bool _isVdcDevice);
    bool isVdcDevice() const { return m_isVdcDevice; }
    const VdsdSpec_t& getVdcSpec() const { return *m_vdcSpec; }
    void setVdcSpec(VdsdSpec_t &&x);
    void setVdcHardwareModelGuid(const std::string& _value) { m_VdcHardwareModelGuid = _value; }
    const std::string& getVdcHardwareModelGuid() const { return m_VdcHardwareModelGuid; }
    void setVdcModelUID(const std::string& _value) { m_VdcModelUID = _value; }
    const std::string& getVdcModelUID() const { return m_VdcModelUID; }
    void setVdcModelVersion(const std::string& _value) { m_VdcModelVersion = _value; }
    const std::string& getVdcModelVersion() const { return m_VdcModelVersion; }
    void setVdcVendorGuid(const std::string& _value) { m_VdcVendorGuid = _value; }
    const std::string& getVdcVendorGuid() const { return m_VdcVendorGuid; }
    void setVdcOemGuid(const std::string& _value) { m_VdcOemGuid = _value; }
    const std::string& getVdcOemGuid() const { return m_VdcOemGuid; }
    void setVdcOemModelGuid(const std::string& _value) { m_VdcOemModelGuid = _value; }
    const std::string& getVdcOemModelGuid() const { return m_VdcOemModelGuid; }
    void setVdcConfigURL(const std::string& _value) { m_VdcConfigURL = _value; }
    const std::string& getVdcConfigURL() const { return m_VdcConfigURL; }
    void setVdcHardwareGuid(const std::string& _value) { m_VdcHardwareGuid = _value; }
    const std::string& getVdcHardwareGuid() const { return m_VdcHardwareGuid; }
    void setVdcHardwareInfo(const std::string& _value) {
      m_VdcHardwareInfo = _value; calculateHWInfo();
    }
    const std::string& getVdcHardwareInfo() const { return m_VdcHardwareInfo; }
    void setVdcHardwareVersion(const std::string& _value) { m_VdcHardwareVersion = _value; }
    const std::string& getVdcHardwareVersion() const { return m_VdcHardwareVersion; }
    void setVdcIconPath(const std::string& _value) {
      m_VdcIconPath = _value; updateIconPath();
    }
    const std::string& getVdcIconPath() const { return m_VdcIconPath; }
    void setVdcModelFeatures(const boost::shared_ptr<std::vector<int> >& _value) { m_VdcModelFeatures = _value; }
    const boost::shared_ptr<std::vector<int> >& getVdcModelFeatures() const { return m_VdcModelFeatures; }

    void setHasActions(bool x) { m_hasActions = x; }
    bool getHasActions() const { return m_hasActions; }

    void setBinaryInputs(const std::vector<DeviceBinaryInputSpec_t>& _binaryInput);
    const uint8_t getBinaryInputCount() const;
    const std::vector<boost::shared_ptr<DeviceBinaryInput> >& getBinaryInputs() const;
    const boost::shared_ptr<DeviceBinaryInput> getBinaryInput(uint8_t _inputIndex) const;
    void setBinaryInputTarget(uint8_t _index, GroupType targetGroupType, uint8_t targetGroup);
    void setBinaryInputId(uint8_t _index, BinaryInputId _inputId);
    void setBinaryInputType(uint8_t _index, BinaryInputType _inputType);
    void clearBinaryInputs();

    void initStates(const std::vector<DeviceStateSpec_t>& specs);
    const std::map<std::string, boost::shared_ptr<State> >& getStates() const { return m_states; }
    void clearStates();
    void setStateValue(const std::string& name, const std::string& value);
    void setStateValues(const std::vector<std::pair<std::string, std::string> >& values);

    void setSensors(const std::vector<DeviceSensorSpec_t>& _binaryInput);
    const uint8_t getSensorCount() const;
    const std::vector<boost::shared_ptr<DeviceSensor_t> >& getSensors() const;
    const boost::shared_ptr<DeviceSensor_t> getSensor(uint8_t _inputIndex) const;
    const boost::shared_ptr<DeviceSensor_t> getSensorByType(SensorType _sensorType) const;
    const void setSensorValue(int _sensorIndex, unsigned int _sensorValue) const;
    const void setSensorValue(int _sensorIndex, double _sensorValue) const;
    const void setSensorDataValidity(int _sensorIndex, bool _valid) const;
    bool isSensorDataValid(int _sensorIndex) const;

    void setOutputChannels(const std::vector<int>& _outputChannels);
    const int getOutputChannelCount() const;
    const int getOutputChannelIndex(int _channelId) const;
    const int getOutputChannel(int _index) const;

    uint8_t getDeviceUMVRelayValue();
    void setDeviceUMVRelayValue(uint8_t _value);

    void setDeviceUMRBlinkRepetitions(uint8_t _count);
    void setDeviceUMROnDelay(double delay);
    void setDeviceUMROffDelay(double delay);
    void getDeviceUMRDelaySettings(double *_ondelay, double *_offdelay, uint8_t *_count);

    void setCardinalDirection(CardinalDirection_t _direction, bool _initial = false);
    CardinalDirection_t getCardinalDirection() const {
      return m_cardinalDirection;
    }

    void setWindProtectionClass(WindProtectionClass_t _klass, bool _initial = false);
    WindProtectionClass_t getWindProtectionClass() const {
      return m_windProtectionClass;
    }


    uint8_t getSWThresholdAddress() const;
    void setSwitchThreshold(uint8_t _threshold);
    uint8_t getSwitchThreshold();

    void setTemperatureOffset(int8_t _offset);
    int8_t getTemperatureOffset();

    void setFloor(int _floor) {
      if (m_floor != _floor) {
        m_floor = _floor;
        dirty();
      }
    }
    int getFloor() const {
      return m_floor;
    }

    std::vector<int> getLockedScenes();
    bool isInLockedCluster();

    uint16_t getDeviceMaxMotionTime();
    void setDeviceMaxMotionTime(uint16_t seconds);

    // below functions are currently only supported by TNYs
    void setPairedDevices(int _num);
    int getPairedDevices() const;
    void setVisibility(bool _isVisible);
    void setDeviceVisibility(bool _isVisible);
    bool isVisible() const;
    bool isMainDevice() const;
    dsuid_t getMainDeviceDSUID() const;

    void handleBinaryInputEvent(const int index, BinaryInputState state);

    /// Calls (invokes) device specific action.
    void callAction(const std::string& actionId, const vdcapi::PropertyElement& params);

    void setProperty(const vdcapi::PropertyElement& element); // element.name() is property id
    void setCustomAction(const std::string& id, const std::string& title, const std::string& action,
                         const vdcapi::PropertyElement& params);
    vdcapi::Message getVdcProperty(const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& query);
    void setVdcProperty(const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& query);
  }; // Device

  std::ostream& operator<<(std::ostream& out, const Device& _dt);

  class DeviceBank3_BL {
  public:
    DeviceBank3_BL(boost::shared_ptr<Device> device);

    void setValveProtectionTimer(uint16_t timeout);
    uint16_t getValveProtectionTimer();
    void setEmergencySetPoint(int8_t set_point);
    int8_t getEmergencySetPoint();
    void setEmergencyTimer(int16_t timeout);
    uint16_t getEmergencyTimer();

    void setPwmPeriod(uint16_t pwmPeriod);
    uint16_t getPwmPeriod();
    void setPwmMinX(int8_t set_point);
    int8_t getPwmMinX();
    void setPwmMaxX(int8_t set_point);
    int8_t getPwmMaxX();
    void setPwmMinY(int8_t set_point);
    int8_t getPwmMinY();
    void setPwmMaxY(int8_t set_point);
    int8_t getPwmMaxY();
    void setPwmConfig(uint8_t config);
    uint8_t getPwmConfig();
    void setPwmOffset(int8_t config);
    int8_t getPwmOffset();

  private:
    boost::shared_ptr<Device> m_device;
  };
} // namespace dss

#endif // DEVICE_H
