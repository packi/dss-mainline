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

#include <iosfwd>
#include <bitset>
#include <utility>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "src/ds485types.h"
#include "src/datetools.h"
#include "addressablemodelitem.h"
#include "businterface.h"

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

namespace dss {

  class Group;
  class State;
  class DSMeter;

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
    bool pairing;       // device supports pairing
    bool syncButtonID;  // sync button ID setting of slave with master
    bool hasOutputAngle;
  } DeviceFeatures_t;

  typedef struct {
    int m_inputIndex;        // input line index
    int m_inputType;         // type of input signal
    int m_inputId;           // target Id, like ButtonId
    int m_targetGroupType;   // type of target group: standard, user, apartment
    int m_targetGroupId;     // index of target group, 0..63
  } DeviceBinaryInput_t;

  typedef struct {
    int m_sensorIndex;       // sensor index
    int m_sensorType;        // type of sensor
    int m_sensorPollInterval;
    bool m_sensorBroadcastFlag;
    bool m_sensorPushConversionFlag;
    bool m_sensorValueValidity;
    unsigned int m_sensorValue;
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
    DEVICE_TYPE_IST     = 7,
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
    DEVICE_OEM_UNKOWN,
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
    std::bitset<63> m_GroupBitmask;
    std::vector<int> m_Groups;
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

    bool m_IsConfigLocked;

    uint8_t m_binaryInputCount;
    std::vector<boost::shared_ptr<DeviceBinaryInput_t> > m_binaryInputs;
    std::vector<boost::shared_ptr<State> > m_binaryInputStates;

    uint8_t m_sensorInputCount;
    std::vector<boost::shared_ptr<DeviceSensor_t> > m_sensorInputs;
    uint8_t m_outputChannelCount;
    std::vector<int> m_outputChannels;

    std::string m_AKMInputProperty;

    mutable boost::mutex m_deviceMutex;

  protected:
    /** Sends the application a note that something has changed.
     * This will cause the \c apartment.xml to be updated. */
    void dirty();

    void fillSensorTable(std::vector<DeviceSensorSpec_t>& _slist);
    bool hasExtendendSceneTable();
    void calculateHWInfo();
    void updateIconPath();
    std::string getAKMButtonInputString(const int _mode);

  public:
    /** Creates and initializes a device. */
    Device(const dsuid_t _dsid, Apartment* _pApartment);
    virtual ~Device();

    /** @copydoc DeviceReference::isOn() */
    virtual bool isOn() const;

    /** Set device configuration values */
    void setDeviceConfig(uint8_t _configClass, uint8_t _configIndex, uint8_t _value);
    void setDeviceConfig16(uint8_t _configClass, uint8_t _configIndex, uint16_t _value);
    void setDeviceButtonID(uint8_t _buttonId);
    void setDeviceButtonActiveGroup(uint8_t _buttonActiveGroup);
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

    /** Binary input devices */
    void setDeviceBinaryInputId(uint8_t _inputIndex, uint8_t _targetId);
    void setDeviceBinaryInputTarget(uint8_t _inputIndex, uint8_t _targetType, uint8_t _targetGroup);
    void setDeviceBinaryInputType(uint8_t _inputIndex, uint8_t _inputType);
    uint8_t getDeviceBinaryInputType(uint8_t _inputIndex);
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
    uint8_t getDeviceSensorType(const int _sensorIndex);

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

    /** Returns the group bitmask (1 based) of the device */
    std::bitset<63>& getGroupBitmask();
    /** @copydoc getGroupBitmask() */
    const std::bitset<63>& getGroupBitmask() const;

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

    /** Retuturns group to which the joker is configured or -1 if device is not
        a joker */
    int getJokerGroup() const;

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
    void setOutputMode(const uint8_t _value) { m_OutputMode = _value; }
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
    bool isSceneDevice (void) const { return false; } //TODO: other devices not defined yet

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

    static const std::string getDeviceTypeString(const DeviceTypes_t _type);
    static const std::string getDeviceClassString(const DeviceClasses_t _class);
    static const std::string getColorString(const int _class);

    void setOemProductInfo(const std::string& _productName, const std::string& _iconPath, const std::string& _productURL);
    void setOemProductInfoState(const DeviceOEMState_t _state);
    std::string getOemProductInfoStateAsString() const { return oemStateToString(m_OemProductInfoState); }
    const DeviceOEMState_t getOemProductInfoState() const { return m_OemProductInfoState; }
    const std::string& getOemProductName() const { return m_OemProductName; }
    const std::string& getOemProductIcon() const { return m_OemProductIcon; }
    const std::string& getOemProductURL() const { return m_OemProductURL; }
    bool isOemCoupledWith(boost::shared_ptr<Device> _otherDev);
    void setConfigLock(bool _lockConfig);
    bool isConfigLocked() const { return m_IsConfigLocked; }
    void setBinaryInputs(boost::shared_ptr<Device> me, const std::vector<DeviceBinaryInputSpec_t>& _binaryInput);
    const uint8_t getBinaryInputCount() const;
    const std::vector<boost::shared_ptr<DeviceBinaryInput_t> >& getBinaryInputs() const;
    const boost::shared_ptr<DeviceBinaryInput_t> getBinaryInput(uint8_t _inputIndex) const;
    void setBinaryInputTarget(uint8_t _index, uint8_t targetGroupType, uint8_t targetGroup);
    void setBinaryInputId(uint8_t _index, uint8_t _inputId);
    void setBinaryInputType(uint8_t _index, uint8_t _inputType);

    boost::shared_ptr<State> getBinaryInputState(uint8_t _inputIndex) const;
    void clearBinaryInputStates();

    void setSensors(boost::shared_ptr<Device> me, const std::vector<DeviceSensorSpec_t>& _binaryInput);
    const uint8_t getSensorCount() const;
    const std::vector<boost::shared_ptr<DeviceSensor_t> >& getSensors() const;
    const boost::shared_ptr<DeviceSensor_t> getSensor(uint8_t _inputIndex) const;
    const void setSensorValue(int _sensorIndex, unsigned int _sensorValue) const;
    const void setSensorDataValidity(int _sensorIndex, bool _valid) const;
    bool isSensorDataValid(int _sensorIndex) const;

    void setOutputChannels(boost::shared_ptr<Device> me, const std::vector<int>& _outputChannels);
    const int getOutputChannelCount() const;
    const int getOutputChannelIndex(int _channelId) const;
    const int getOutputChannel(int _index) const;

  }; // Device

  std::ostream& operator<<(std::ostream& out, const Device& _dt);

} // namespace dss

#endif // DEVICE_H
