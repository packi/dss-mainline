begin transaction;
drop table if exists "device";
create table "device"(id primary key,gtin,name,version);
insert into device values(12 , "7640156791914" , "V-Zug MSLQ - aktiv" , "1");
insert into device values(18 , "7640156791945" , "vDC smarter iKettle 2.0" , "1");
drop table if exists "name_master";
create table "name_master"(id primary key,reference_id,scope,name,lang_code);
insert into name_master values(1 , 1 , "device_actions" , "Backen" , "de_DE");
insert into name_master values(2 , 2 , "device_actions" , "Dampfen" , "de_DE");
insert into name_master values(3 , 3 , "device_actions" , "Ausschalten" , "de_DE");
insert into name_master values(4 , 1 , "device_actions_parameter" , "Temperatur" , "de_DE");
insert into name_master values(5 , 2 , "device_actions_parameter" , "Zeit" , "de_DE");
insert into name_master values(6 , 3 , "device_actions_parameter" , "Temperatur" , "de_DE");
insert into name_master values(7 , 4 , "device_actions_parameter" , "Zeit" , "de_DE");
insert into name_master values(8 , 1 , "device_actions_predefined" , "Kuchen" , "de_DE");
insert into name_master values(9 , 2 , "device_actions_predefined" , "Pizza" , "de_DE");
insert into name_master values(10 , 3 , "device_actions_predefined" , "Spargel" , "de_DE");
insert into name_master values(11 , 4 , "device_actions_predefined" , "Stop" , "de_DE");
insert into name_master values(12 , 1 , "device_properties" , "Temperatur" , "de_DE");
insert into name_master values(13 , 2 , "device_properties" , "Endzeit" , "de_DE");
insert into name_master values(14 , 3 , "device_properties" , "Gargut Temperatur" , "de_DE");
insert into name_master values(15 , 2 , "device_events" , "Wasserstand zu niedrig" , "de_DE");
insert into name_master values(16 , 5 , "device_status" , "Betriebszustand" , "de_DE");
insert into name_master values(17 , 6 , "device_status" , "Ventilator" , "de_DE");
insert into name_master values(18 , 7 , "device_status" , "Wecker" , "de_DE");
insert into name_master values(19 , 1 , "device_status_enum" , "heizt" , "de_DE");
insert into name_master values(20 , 2 , "device_status_enum" , "dampft" , "de_DE");
insert into name_master values(21 , 3 , "device_status_enum" , "ausgeschaltet" , "de_DE");
insert into name_master values(22 , 4 , "device_status_enum" , "an" , "de_DE");
insert into name_master values(23 , 5 , "device_status_enum" , "aus" , "de_DE");
insert into name_master values(24 , 6 , "device_status_enum" , "inaktiv" , "de_DE");
insert into name_master values(25 , 7 , "device_status_enum" , "läuft" , "de_DE");
insert into name_master values(26 , 8 , "device_status" , "Betriebsmodus" , "de_DE");
insert into name_master values(27 , 8 , "device_status_enum" , "Abkühlen" , "de_DE");
insert into name_master values(28 , 9 , "device_status_enum" , "Aufheizen" , "de_DE");
insert into name_master values(29 , 10 , "device_status_enum" , "Warmhalten" , "de_DE");
insert into name_master values(30 , 11 , "device_status_enum" , "Bereit" , "de_DE");
insert into name_master values(31 , 12 , "device_status_enum" , "Abgehoben" , "de_DE");
insert into name_master values(33 , 4 , "device_actions" , "Stoppen" , "de_DE");
insert into name_master values(34 , 5 , "device_actions" , "Aufheizen" , "de_DE");
insert into name_master values(35 , 6 , "device_actions" , "Abkochen und abkühlen" , "de_DE");
insert into name_master values(36 , 5 , "device_actions_parameter" , "Warmhaltedauer" , "de_DE");
insert into name_master values(37 , 7 , "device_actions_parameter" , "Warmhaltedauer" , "de_DE");
insert into name_master values(38 , 6 , "device_actions_parameter" , "Zieltemperatur" , "de_DE");
insert into name_master values(39 , 8 , "device_actions_parameter" , "Zieltemperatur" , "de_DE");
insert into name_master values(40 , 5 , "device_actions_predefined" , "Abkochen" , "de_DE");
insert into name_master values(41 , 6 , "device_actions_predefined" , "Normales Kochen" , "de_DE");
insert into name_master values(42 , 7 , "device_actions_predefined" , "Beenden" , "de_DE");
insert into name_master values(43 , 1 , "device_actions" , "bake" , "base");
insert into name_master values(44 , 2 , "device_actions" , "steam" , "base");
insert into name_master values(45 , 3 , "device_actions" , "turn off" , "base");
insert into name_master values(46 , 1 , "device_actions_parameter" , "temperature" , "base");
insert into name_master values(47 , 2 , "device_actions_parameter" , "time" , "base");
insert into name_master values(48 , 3 , "device_actions_parameter" , "temperature" , "base");
insert into name_master values(49 , 4 , "device_actions_parameter" , "time" , "base");
insert into name_master values(50 , 1 , "device_actions_predefined" , "cake" , "base");
insert into name_master values(51 , 2 , "device_actions_predefined" , "pizza" , "base");
insert into name_master values(52 , 3 , "device_actions_predefined" , "asparagus" , "base");
insert into name_master values(53 , 4 , "device_actions_predefined" , "stop" , "base");
insert into name_master values(54 , 1 , "device_properties" , "temperature" , "base");
insert into name_master values(55 , 2 , "device_properties" , "finish time" , "base");
insert into name_master values(56 , 3 , "device_properties" , "core temperature" , "base");
insert into name_master values(57 , 2 , "device_events" , "need water" , "base");
insert into name_master values(58 , 5 , "device_status" , "operation mode" , "base");
insert into name_master values(59 , 6 , "device_status" , "ventilator" , "base");
insert into name_master values(60 , 7 , "device_status" , "alarm clock" , "base");
insert into name_master values(61 , 1 , "device_status_enum" , "heating" , "base");
insert into name_master values(62 , 2 , "device_status_enum" , "steaming" , "base");
insert into name_master values(63 , 3 , "device_status_enum" , "turned of" , "base");
insert into name_master values(64 , 4 , "device_status_enum" , "on" , "base");
insert into name_master values(65 , 5 , "device_status_enum" , "off" , "base");
insert into name_master values(66 , 6 , "device_status_enum" , "inactive" , "base");
insert into name_master values(67 , 7 , "device_status_enum" , "running" , "base");
insert into name_master values(68 , 8 , "device_status" , "operation mode" , "base");
insert into name_master values(69 , 8 , "device_status_enum" , "cooling" , "base");
insert into name_master values(70 , 9 , "device_status_enum" , "heat up" , "base");
insert into name_master values(71 , 10 , "device_status_enum" , "keep warm" , "base");
insert into name_master values(72 , 11 , "device_status_enum" , "ready" , "base");
insert into name_master values(73 , 12 , "device_status_enum" , "released" , "base");
insert into name_master values(74 , 4 , "device_actions" , "stopping" , "base");
insert into name_master values(75 , 5 , "device_actions" , "boil" , "base");
insert into name_master values(76 , 6 , "device_actions" , "boilandcooldown" , "base");
insert into name_master values(77 , 5 , "device_actions_parameter" , "keep warm duration" , "base");
insert into name_master values(78 , 7 , "device_actions_parameter" , "keep warm duration" , "base");
insert into name_master values(79 , 6 , "device_actions_parameter" , "targettemperature" , "base");
insert into name_master values(80 , 8 , "device_actions_parameter" , "targettemperature" , "base");
insert into name_master values(81 , 5 , "device_actions_predefined" , "babyboiling" , "base");
insert into name_master values(82 , 6 , "device_actions_predefined" , "boil" , "base");
insert into name_master values(83 , 7 , "device_actions_predefined" , "stopping" , "base");
insert into name_master values(84 , 3 , "device_events" , "Kettle attached" , "base");
insert into name_master values(85 , 3 , "device_events" , "Kocher abgesetzt" , "de_DE");
insert into name_master values(86 , 3 , "device_events" , "Kettle attached" , "en_US");
insert into name_master values(88 , 4 , "device_events" , "Boiling started" , "base");
insert into name_master values(89 , 5 , "device_events" , "Keeping warm started" , "base");
insert into name_master values(90 , 6 , "device_events" , "Boil and cool down (Babyboil) started" , "base");
insert into name_master values(91 , 7 , "device_events" , "Boiling ended without keeping warm" , "base");
insert into name_master values(92 , 8 , "device_events" , "Kettle was taken away" , "base");
insert into name_master values(93 , 9 , "device_events" , "Cooled down to specified end temperature without keeping warm" , "base");
insert into name_master values(94 , 10 , "device_events" , "Keeping warm finished (timeout)" , "base");
insert into name_master values(95 , 11 , "device_events" , "Boiling aborted by button press" , "base");
insert into name_master values(96 , 12 , "device_events" , "Cooling down aborted by button press" , "base");
insert into name_master values(97 , 13 , "device_events" , "Keeping warm aborted by button press" , "base");
insert into name_master values(98 , 14 , "device_events" , "Keep warm after boiling started" , "base");
insert into name_master values(99 , 15 , "device_events" , "Keep temperature after cooling down (BabyBoil) started" , "base");
insert into name_master values(100 , 16 , "device_events" , "Boiling aborted because kettle was removed" , "base");
insert into name_master values(101 , 17 , "device_events" , "Cooling down aborted because kettle was removed" , "base");
insert into name_master values(102 , 18 , "device_events" , "Keeping warm aborted because kettle was removed" , "base");
insert into name_master values(105 , 4 , "device_events" , "Boiling started" , "en_US");
insert into name_master values(106 , 4 , "device_events" , "Aufheizen gestartet" , "de_DE");
insert into name_master values(107 , 5 , "device_events" , "Keeping warm started" , "en_US");
insert into name_master values(108 , 5 , "device_events" , "Warmhalten gestartet" , "de_DE");
insert into name_master values(109 , 6 , "device_events" , "Boil and cool down started (Babyboil)" , "en_US");
insert into name_master values(110 , 6 , "device_events" , "Aufheizen mit anschliessendem Warmhalten gestartet" , "de_DE");
insert into name_master values(111 , 7 , "device_events" , "Boiling ended without keeping warm" , "en_US");
insert into name_master values(112 , 7 , "device_events" , "Aufheizen beendet ohne Warmhalten" , "de_DE");
insert into name_master values(113 , 8 , "device_events" , "Kettle was taken away" , "en_US");
insert into name_master values(114 , 8 , "device_events" , "Wasserkocher abgenommen" , "de_DE");
insert into name_master values(115 , 9 , "device_events" , "Cooled down to specified end temperature without keeping warm" , "en_US");
insert into name_master values(116 , 9 , "device_events" , "Abgekühlt auf die gewünschte Temperatur ohne weiteres warmhalten" , "de_DE");
insert into name_master values(117 , 10 , "device_events" , "Keeping warm finished" , "en_US");
insert into name_master values(118 , 10 , "device_events" , "Warmhalten nach Ablauf der Zeit beendet" , "de_DE");
insert into name_master values(119 , 11 , "device_events" , "Aufheizen durch Tastendruck beendet" , "de_DE");
insert into name_master values(120 , 11 , "device_events" , "Boiling aborted by button press" , "en_US");
insert into name_master values(121 , 12 , "device_events" , "Cooling down aborted by button press" , "en_US");
insert into name_master values(122 , 12 , "device_events" , "Abkühlen durch Tastendruck beendet" , "de_DE");
insert into name_master values(123 , 13 , "device_events" , "Keeping warm aborted by button press" , "en_US");
insert into name_master values(124 , 13 , "device_events" , "Warmhalten durch Tastendruck abgebrochen" , "de_DE");
insert into name_master values(125 , 14 , "device_events" , "Keep warm after boiling started" , "en_US");
insert into name_master values(126 , 14 , "device_events" , "Warmhalten nach Aufheizen gestartet" , "de_DE");
insert into name_master values(127 , 15 , "device_events" , "Keep temperature after cooling down (BabyBoil) started" , "en_US");
insert into name_master values(128 , 15 , "device_events" , "Warmhalten nach Erreichen der gewünschten Temperatur gestartet" , "de_DE");
insert into name_master values(129 , 16 , "device_events" , "Boiling aborted because kettle was removed" , "en_US");
insert into name_master values(130 , 16 , "device_events" , "Aufheizen abgebrochen weil der Wasserkocher abgehoben wurde" , "de_DE");
insert into name_master values(131 , 17 , "device_events" , "Cooling down aborted because kettle was removed" , "en_US");
insert into name_master values(132 , 17 , "device_events" , "Abkühlen abgebrochen weil der Wasserkocher abgehoben wurde" , "de_DE");
insert into name_master values(133 , 18 , "device_events" , "Keeping warm aborted because kettle was removed" , "en_US");
insert into name_master values(134 , 18 , "device_events" , "Warmhalten abgebrochen weil der Wasserkocher abgehoben wurde" , "de_DE");
insert into name_master values(135 , 5 , "device_status" , "Operation Mode" , "en_US");
insert into name_master values(136 , 6 , "device_status" , "Ventilator" , "en_US");
insert into name_master values(137 , 7 , "device_status" , "Alarm Timer" , "en_US");
insert into name_master values(138 , 8 , "device_status" , "Operation Mode" , "en_US");
insert into name_master values(139 , 1 , "device_actions" , "Bake" , "en_US");
insert into name_master values(140 , 2 , "device_actions" , "Steaming" , "en_US");
insert into name_master values(141 , 3 , "device_actions" , "Deactivate" , "en_US");
insert into name_master values(142 , 4 , "device_actions" , "Stopping" , "en_US");
insert into name_master values(143 , 5 , "device_actions" , "Boil" , "en_US");
insert into name_master values(144 , 6 , "device_actions" , "Boil and cool down" , "en_US");
insert into name_master values(145 , 1 , "device_actions_parameter" , "Temperature" , "en_US");
insert into name_master values(146 , 2 , "device_actions_parameter" , "Time" , "en_US");
insert into name_master values(147 , 3 , "device_actions_parameter" , "Temperature" , "en_US");
insert into name_master values(148 , 4 , "device_actions_parameter" , "Time" , "en_US");
insert into name_master values(149 , 5 , "device_actions_parameter" , "Keep warm duration" , "en_US");
insert into name_master values(150 , 7 , "device_actions_parameter" , "Keep warm duration" , "en_US");
insert into name_master values(151 , 6 , "device_actions_parameter" , "Target Temperature" , "en_US");
insert into name_master values(152 , 8 , "device_actions_parameter" , "Target Temperature" , "en_US");
insert into name_master values(153 , 1 , "device_actions_predefined" , "Cake" , "en_US");
insert into name_master values(154 , 2 , "device_actions_predefined" , "Pizza" , "en_US");
insert into name_master values(155 , 3 , "device_actions_predefined" , "Asparagus" , "en_US");
insert into name_master values(156 , 4 , "device_actions_predefined" , "Stop" , "en_US");
insert into name_master values(157 , 5 , "device_actions_predefined" , "Baby Bottle Boil" , "en_US");
insert into name_master values(158 , 6 , "device_actions_predefined" , "Normal Boil" , "en_US");
insert into name_master values(159 , 7 , "device_actions_predefined" , "Stopping" , "en_US");
insert into name_master values(160 , 1 , "device_status_enum" , "Heating" , "en_US");
insert into name_master values(161 , 2 , "device_status_enum" , "Steaming" , "en_US");
insert into name_master values(162 , 3 , "device_status_enum" , "Turned Off" , "en_US");
insert into name_master values(163 , 4 , "device_status_enum" , "On" , "en_US");
insert into name_master values(164 , 5 , "device_status_enum" , "Off" , "en_US");
insert into name_master values(165 , 6 , "device_status_enum" , "Inactive" , "en_US");
insert into name_master values(166 , 7 , "device_status_enum" , "Running" , "en_US");
insert into name_master values(167 , 8 , "device_status_enum" , "Cooling" , "en_US");
insert into name_master values(168 , 9 , "device_status_enum" , "Heat up" , "en_US");
insert into name_master values(169 , 10 , "device_status_enum" , "Keep warm" , "en_US");
insert into name_master values(170 , 11 , "device_status_enum" , "Ready" , "en_US");
insert into name_master values(171 , 12 , "device_status_enum" , "Released" , "en_US");
insert into name_master values(179 , 1 , "device_actions" , "Back!" , "nl_NL");
insert into name_master values(180 , 2 , "device_actions" , "Dampfen" , "nl_NL");
insert into name_master values(181 , 3 , "device_actions" , "auuus" , "nl_NL");
insert into name_master values(182 , 1 , "common_labels" , "Name" , "de_DE");
insert into name_master values(183 , 1 , "common_labels" , "Name" , "en_US");
insert into name_master values(184 , 2 , "common_labels" , "dS Device GTIN" , "de_DE");
insert into name_master values(185 , 2 , "common_labels" , "dS Device GTIN" , "en_US");
insert into name_master values(186 , 3 , "common_labels" , "Modellbezeichnung" , "de_DE");
insert into name_master values(187 , 3 , "common_labels" , "Model Name" , "en_US");
insert into name_master values(188 , 4 , "common_labels" , "Modellvariante" , "de_DE");
insert into name_master values(189 , 4 , "common_labels" , "Model Version" , "en_US");
insert into name_master values(190 , 5 , "common_labels" , "Artikel Kennzeichnung" , "en_US");
insert into name_master values(191 , 5 , "common_labels" , "Article Identifier" , "en_US");
insert into name_master values(192 , 6 , "common_labels" , "Produkt Kennzeichnung" , "de_DE");
insert into name_master values(193 , 6 , "common_labels" , "Product Id" , "en_US");
insert into name_master values(194 , 7 , "common_labels" , "Hersteller" , "de_DE");
insert into name_master values(195 , 7 , "common_labels" , "Vendor" , "en_US");
insert into name_master values(196 , 8 , "common_labels" , "Hersteller Kennung" , "de_DE");
insert into name_master values(197 , 8 , "common_labels" , "Vendor Id" , "en_US");
insert into name_master values(198 , 9 , "common_labels" , "Geräteklasse" , "de_DE");
insert into name_master values(199 , 9 , "common_labels" , "Device Class" , "en_US");
insert into name_master values(200 , 10 , "common_labels" , "Geräteklassen version" , "de_DE");
insert into name_master values(201 , 1 , "common_labels" , "Name" , "base");
insert into name_master values(202 , 2 , "common_labels" , "dS Device GTIN" , "base");
insert into name_master values(203 , 3 , "common_labels" , "Model Name" , "base");
insert into name_master values(204 , 4 , "common_labels" , "Model Version" , "base");
insert into name_master values(205 , 5 , "common_labels" , "Article Identifier" , "base");
insert into name_master values(206 , 6 , "common_labels" , "Product Id" , "base");
insert into name_master values(207 , 7 , "common_labels" , "Vendor" , "base");
insert into name_master values(208 , 8 , "common_labels" , "Vendor Id" , "base");
insert into name_master values(209 , 9 , "common_labels" , "Device Class" , "base");
insert into name_master values(210 , 10 , "common_labels" , "deviceclasses version" , "base");
drop table if exists "device_status";
create table "device_status"(id primary key,device_id,name,tags);
insert into device_status values(5 , 12 , "operationMode" , "overview");
insert into device_status values(6 , 12 , "fan" , "");
insert into device_status values(7 , 12 , "timer" , "");
insert into device_status values(8 , 18 , "operation" , "overview");
drop table if exists "device_status_enum";
create table "device_status_enum"(id primary key,device_id,value);
insert into device_status_enum values(1 , 5 , "heating");
insert into device_status_enum values(2 , 5 , "steaming");
insert into device_status_enum values(3 , 5 , "off");
insert into device_status_enum values(4 , 6 , "on");
insert into device_status_enum values(5 , 6 , "off");
insert into device_status_enum values(6 , 7 , "inactive");
insert into device_status_enum values(7 , 7 , "running");
insert into device_status_enum values(8 , 8 , "cooldown");
insert into device_status_enum values(9 , 8 , "heating");
insert into device_status_enum values(10 , 8 , "keepwarm");
insert into device_status_enum values(11 , 8 , "ready");
insert into device_status_enum values(12 , 8 , "removed");
drop table if exists "device_labels";
create table "device_labels" (id primary key,device_id,name,tags);
insert into device_labels values(1 , 18 , "SpecialField" , "overview:100");
insert into device_labels values(2 , 18 , "hardwareModelGuid" , "overview:101");
drop table if exists "common_labels";
create table "common_labels" (id primary key,name,tags);
insert into common_labels values(1 , "name" , "overview:1");
insert into common_labels values(2 , "dsDeviceGTIN" , "overview:2");
insert into common_labels values(3 , "model" , "overview:3");
insert into common_labels values(4 , "modelVersion" , "overview:4");
insert into common_labels values(5 , "hardwareGuid" , "overview:5");
insert into common_labels values(6 , "hardwareModelGuid" , "invisible");
insert into common_labels values(7 , "vendorName" , "overview:7");
insert into common_labels values(8 , "vendorId" , "");
insert into common_labels values(9 , "class" , "invisible");
insert into common_labels values(10 , "classVersion" , "invisible");
drop table if exists "device_properties";
create table "device_properties" (id primary key,device_id,name,type_id,min_value,max_value,default_value,resolution,si_unit,tags);
insert into device_properties values(1 , 12 , "temperature" , 1 , "0" , "250" , "0" , "1" , "celsius" , "" );
insert into device_properties values(2 , 12 , "duration" , 2 , "0" , "1800" , "0" , "1" , "second" , "" );
insert into device_properties values(3 , 12 , "temperature.sensor" , 1 , "0" , "250" , "0" , "1" , "celsius" , "readonly" );
insert into device_properties values(4 , 18 , "currentTemperature" , 1 , "0" , "250" , "0" , "1" , "celsius" , "readonly;overview" );
drop table if exists "name_device_properties";
create table "name_device_properties" (id primary key,reference_id,lang_code,displayMask,alt_label,alt_postfix,alt_precision_float);
insert into name_device_properties values(1 , 1 , "de_DE" , "Temperatur {value} &deg;" , "Temperatur" , "&deg;" , "0");
insert into name_device_properties values(2 , 2 , "de_DE" , "Endzeit in {value} Sekunden" , "Endzeit" , "Sekunden" , "0");
insert into name_device_properties values(3 , 3 , "de_DE" , "Garguttemperatur {value} &deg;" , "Garguttemperatur" , "&deg;" , "0");
insert into name_device_properties values(4 , 4 , "de_DE" , "Temperatur {value} &deg; " , "Temperatur" , " " , "0");
insert into name_device_properties values(5 , 1 , "base" , "temperature {value} &deg; " , "temperature" , "&deg;" , "0");
insert into name_device_properties values(6 , 2 , "base" , "finished in {value} seconds" , "finishtime" , "seconds" , "0");
insert into name_device_properties values(7 , 3 , "base" , "coretemperature {value} &deg;" , "coretemperature" , "&deg;" , "0");
insert into name_device_properties values(8 , 4 , "base" , "temperature {value} &deg;" , "temperature" , "&deg;" , "0");
insert into name_device_properties values(9 , 1 , "en_US" , "temperature {value} &deg; " , "Temperature" , "&deg;" , "0");
insert into name_device_properties values(10 , 2 , "en_US" , "finished in {value} seconds" , "Finishtime" , "seconds" , "0");
insert into name_device_properties values(11 , 3 , "en_US" , "coretemperature {value} &deg;" , "Core Temperature" , "&deg;" , "0");
insert into name_device_properties values(12 , 4 , "en_US" , "temperature {value} &deg;" , "Temperature" , "&deg;" , "0");
insert into name_device_properties values(19 , 1 , "nl_NL" , "NL Temperatur: {value} &deg;" , "NL Temperatur" , "NL &deg;" , "0");
insert into name_device_properties values(20 , 3 , "nl_NL" , "coretemperature {value} &deg;" , "coretemperature" , "NL &deg;" , "0");
drop table if exists "name_device_labels";
create table "name_device_labels" (id primary key,reference_id,lang_code,title,value);
insert into name_device_labels values(1 , 1 , "de_DE" , "Spezialfeld" , "Sondereintrag fÃ¼r den iKettle v2");
insert into name_device_labels values(2 , 1 , "en_US" , "Special Field" , "Extra data for the iKettle v2");
insert into name_device_labels values(3 , 1 , "base" , "Special Field" , "Extra data for the iKettle v2");
drop table if exists "device_events";
create table "device_events"(id primary key,device_id,name);
insert into device_events values(2 , 12 , "needsWater");
insert into device_events values(3 , 18 , "KettleAttached");
insert into device_events values(4 , 18 , "BoilingStarted");
insert into device_events values(5 , 18 , "KeepWarm");
insert into device_events values(6 , 18 , "BabycoolingStarted");
insert into device_events values(7 , 18 , "BoilingFinished");
insert into device_events values(8 , 18 , "KettleReleased");
insert into device_events values(9 , 18 , "BabycoolingFinished");
insert into device_events values(10 , 18 , "KeepWarmFinished");
insert into device_events values(11 , 18 , "BoilingAborted");
insert into device_events values(12 , 18 , "BabycoolingAborted");
insert into device_events values(13 , 18 , "KeepwarmAborted");
insert into device_events values(14 , 18 , "KeepWarmAfterBoiling");
insert into device_events values(15 , 18 , "KeepWarmAfterBabycooling");
insert into device_events values(16 , 18 , "BoilingAbortedAndKettleReleased");
insert into device_events values(17 , 18 , "BabyCoolingAbortedAndKettleReleased");
insert into device_events values(18 , 18 , "KeepWarmAbortedAndKettleReleased");
drop table if exists "device_actions";
create table "device_actions"(id primary key,device_id,command);
insert into device_actions values(1 , 12 , "bake");
insert into device_actions values(2 , 12 , "steam");
insert into device_actions values(3 , 12 , "stop");
insert into device_actions values(4 , 18 , "stop");
insert into device_actions values(5 , 18 , "heat");
insert into device_actions values(6 , 18 , "boilandcooldown");
drop table if exists "device_actions_parameter";
create table "device_actions_parameter"(id primary key,device_actions_id,name,type_id,min_value,max_value,default_value,resolution,si_unit,tags);
insert into device_actions_parameter values(1 , 1 , "temperature" , 1 , "50" , "240" , "180" , "1" , "celsius" , "");
insert into device_actions_parameter values(2 , 1 , "duration" , 2 , "60" , "7200" , "30" , "10" , "second" , "");
insert into device_actions_parameter values(3 , 2 , "temperature" , 1 , "50" , "240" , "180" , "1" , "celsius" , "");
insert into device_actions_parameter values(4 , 2 , "duration" , 2 , "60" , "7200" , "30" , "10" , "second" , "");
insert into device_actions_parameter values(5 , 6 , "keepwarmtime" , 2 , "0" , "1800" , "600" , "1" , "second" , "");
insert into device_actions_parameter values(6 , 6 , "temperature" , 1 , "20" , "100" , "37" , "1" , "celsius" , "");
insert into device_actions_parameter values(7 , 5 , "keepwarmtime" , 2 , "0" , "1800" , "600" , "1" , "second" , "");
insert into device_actions_parameter values(8 , 5 , "temperature" , 1 , "20" , "100" , "100" , "1" , "celsius" , "");
drop table if exists "device_actions_parameter_type";
create table "device_actions_parameter_type"(id primary key,name);
insert into device_actions_parameter_type values(1 , "int.temperature");
insert into device_actions_parameter_type values(2 , "int.timespan");
insert into device_actions_parameter_type values(3 , "numeric");
insert into device_actions_parameter_type values(4 , "string");
insert into device_actions_parameter_type values(5 , "enumeration");
drop table if exists "device_actions_parameter_type_enum";
create table "device_actions_parameter_type_enum"(id primary key,device_action_parameter_type_id,value);
drop table if exists "device_actions_predefined";
create table "device_actions_predefined"(id primary key,name,device_actions_id);
insert into device_actions_predefined values(1 , "std.cake" , 1);
insert into device_actions_predefined values(2 , "std.pizza" , 1);
insert into device_actions_predefined values(3 , "std.asparagus" , 2);
insert into device_actions_predefined values(4 , "std.stop" , 3);
insert into device_actions_predefined values(5 , "std.boilandcooldown" , 6);
insert into device_actions_predefined values(6 , "std.heat" , 5);
insert into device_actions_predefined values(7 , "std.stop" , 4);
drop table if exists "device_actions_predefined_parameter";
create table "device_actions_predefined_parameter"(id primary key,value,device_actions_parameter_id,device_actions_predefined_id);
insert into device_actions_predefined_parameter values(1 , "160" , 1 , 1);
insert into device_actions_predefined_parameter values(2 , "3000" , 2 , 1);
insert into device_actions_predefined_parameter values(3 , "180" , 1 , 2);
insert into device_actions_predefined_parameter values(4 , "1200" , 2 , 2);
insert into device_actions_predefined_parameter values(5 , "180" , 3 , 3);
insert into device_actions_predefined_parameter values(6 , "2520" , 4 , 3);
insert into device_actions_predefined_parameter values(7 , "80" , 8 , 6);
insert into device_actions_predefined_parameter values(8 , "0" , 7 , 6);
insert into device_actions_predefined_parameter values(9 , "0" , 5 , 5);
insert into device_actions_predefined_parameter values(10 , "100" , 6 , 5);
create view if not exists "name_device_actions"  as select id,reference_id,name,lang_code from name_master where scope="device_actions";
create view if not exists "name_device_status"  as select id,reference_id,name,lang_code from name_master where scope="device_status";
create view if not exists "name_device_status_enum"  as select id,reference_id,name,lang_code from name_master where scope="device_status_enum";
create view if not exists "name_device_events"  as select id,reference_id,name,lang_code from name_master where scope="device_events";
create view if not exists "name_device_actions"  as select id,reference_id,name,lang_code from name_master where scope="device_actions";
create view if not exists "name_device_actions_parameter_type"  as select id,reference_id,name,lang_code from name_master where scope="device_actions_parameter_type";
create view if not exists "name_device_actions_parameter_type_enum"  as select id,reference_id,name,lang_code from name_master where scope="device_actions_parameter_type_enum";
create view if not exists "name_device_actions_predefined"  as select id,reference_id,name,lang_code from name_master where scope="device_actions_predefined";
create view if not exists "name_device_actions_parameter"  as select id,reference_id,name,lang_code from name_master where scope="device_actions_parameter";
create view if not exists "name_device_actions"  as select id,reference_id,name,lang_code from name_master where scope="device_actions";
create view if not exists "name_common_labels"  as select id,reference_id,name,lang_code from name_master where scope="common_labels";
		
create view if not exists "callGetStates" as 
	SELECT device_status.name, name_device_status.name, device_status_enum.value, name_device_status_enum.name, device_status.tags, device.gtin,name_device_status.lang_code
	FROM device INNER JOIN 
		((device_status INNER JOIN name_device_status ON device_status.id=name_device_status.reference_id) 
    	INNER JOIN (device_status_enum INNER JOIN name_device_status_enum ON device_status_enum.id=name_device_status_enum.reference_id) ON device_status.id=device_status_enum.device_id) ON device.id=device_status.device_id
	WHERE name_device_status.lang_code = name_device_status_enum.lang_code 
	ORDER BY device_status.name;

create view if not exists "callGetEvents" as 
	SELECT device_events.name, name_device_events.name as displayName, device.gtin, name_device_events.lang_code
	FROM (device INNER JOIN 
		(device_events INNER JOIN name_device_events ON device_events.id=name_device_events.reference_id) ON device.id=device_events.device_id);

create view if not exists "callGetProperties" as 
	SELECT device_properties.name, name_device_properties.alt_label, device_properties.type_id, device_properties.default_value, device_properties.min_value, device_properties.max_value, device_properties.resolution, device_properties.si_unit, device_properties.tags, device.gtin,name_device_properties.lang_code 
    FROM (device
		INNER JOIN (device_properties
        INNER JOIN name_device_properties ON device_properties.id=name_device_properties.reference_id) 
        	ON device.id=device_properties.device_id);
		
create view if not exists "callGetActions" as 
	SELECT device_actions.command, name_device_actions.name as actionName, device_actions_parameter.name as parameterName, name_device_actions_parameter.name as parameterDisplayName, device_actions_parameter.type_id, device_actions_parameter.default_value, 
		device_actions_parameter.min_value, device_actions_parameter.max_value, device_actions_parameter.resolution, device_actions_parameter.si_unit, device_actions_parameter.tags, 
		CASE WHEN device_actions_parameter.type_id is NULL THEN "true" ELSE "false" END AS paramexists, device.gtin,name_device_actions.lang_code
    FROM (device
    	INNER JOIN (device_actions INNER JOIN name_device_actions ON device_actions.id=name_device_actions.reference_id) ON device.id=device_actions.device_id) 
        LEFT JOIN (device_actions_parameter INNER JOIN name_device_actions_parameter ON device_actions_parameter.id=name_device_actions_parameter.reference_id) ON device_actions.id=device_actions_parameter.device_actions_id 
    WHERE ((name_device_actions.lang_code=name_device_actions_parameter.lang_code) OR paramexists="true")
    ORDER BY device_actions.command;

create view if not exists "callGetStandardActions" as 
	SELECT device_actions_predefined.name as predefName, name_device_actions_predefined.name as displayPredefName, device_actions.command, device_actions_parameter.name, device_actions_predefined_parameter.value, CASE WHEN device_actions_parameter.type_id is NULL THEN "true" ELSE "false" END AS paramexists,
		device.gtin,name_device_actions_predefined.lang_code
    FROM device INNER JOIN ((device_actions 
    	INNER JOIN (device_actions_predefined INNER JOIN name_device_actions_predefined ON device_actions_predefined.id=name_device_actions_predefined.reference_id) ON device_actions.id=device_actions_predefined.device_actions_id) 
        LEFT JOIN (device_actions_parameter INNER JOIN device_actions_predefined_parameter ON device_actions_predefined_parameter.device_actions_parameter_id=device_actions_parameter.id) 
        ON device_actions.id=device_actions_predefined.device_actions_id AND device_actions_predefined.id=device_actions_predefined_parameter.device_actions_predefined_id) 
        ON device.id=device_actions.device_id 
    ORDER BY device_actions.command;

create view if not exists "callGetSpec" as 
	SELECT common_labels.name,name_common_labels.name as title,tags,"" as value,name_common_labels.lang_code,"" as gtin
	FROM common_labels INNER JOIN name_common_labels ON common_labels.id=name_common_labels.reference_id 
	UNION	
	SELECT device_labels.name,name_device_labels.title,device_labels.tags,name_device_labels.value,name_device_labels.lang_code,device.gtin
	FROM (device_labels INNER JOIN name_device_labels ON device_labels.id=name_device_labels.reference_id) INNER JOIN device on device.id=device_labels.device_id; 
		
commit;
