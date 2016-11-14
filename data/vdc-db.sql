begin transaction;
drop table if exists "device";
create table "device"(id primary key,gtin,name,version);
insert into device values(12 , "7640156791914" , "V-Zug MSLQ - aktiv" , "1");
insert into device values(18 , "7640156791945" , "vDC smarter iKettle 2.0" , "1");
drop table if exists "name_master";
create table "name_master"(id primary key,reference_id,scope,name,lang_code,lang,country);
insert into name_master values(3 , 3 , "device_actions" , "Ausschalten" , "de_DE" , "de" , "DE");
insert into name_master values(4 , 1 , "device_actions_parameter" , "Temperatur" , "de_DE" , "de" , "DE");
insert into name_master values(5 , 2 , "device_actions_parameter" , "Zeit" , "de_DE" , "de" , "DE");
insert into name_master values(6 , 3 , "device_actions_parameter" , "Temperatur" , "de_DE" , "de" , "DE");
insert into name_master values(7 , 4 , "device_actions_parameter" , "Zeit" , "de_DE" , "de" , "DE");
insert into name_master values(8 , 1 , "device_actions_predefined" , "Kuchen" , "de_DE" , "de" , "DE");
insert into name_master values(9 , 2 , "device_actions_predefined" , "Pizza" , "de_DE" , "de" , "DE");
insert into name_master values(10 , 3 , "device_actions_predefined" , "Spargel" , "de_DE" , "de" , "DE");
insert into name_master values(11 , 4 , "device_actions_predefined" , "Stop" , "de_DE" , "de" , "DE");
insert into name_master values(12 , 1 , "device_properties" , "Temperatur" , "de_DE" , "de" , "DE");
insert into name_master values(13 , 2 , "device_properties" , "Endzeit" , "de_DE" , "de" , "DE");
insert into name_master values(14 , 3 , "device_properties" , "Gargut Temperatur" , "de_DE" , "de" , "DE");
insert into name_master values(15 , 2 , "device_events" , "Wasserstand zu niedrig" , "de_DE" , "de" , "DE");
insert into name_master values(16 , 5 , "device_status" , "Betriebszustand" , "de_DE" , "de" , "DE");
insert into name_master values(17 , 6 , "device_status" , "Ventilator" , "de_DE" , "de" , "DE");
insert into name_master values(18 , 7 , "device_status" , "Wecker" , "de_DE" , "de" , "DE");
insert into name_master values(19 , 1 , "device_status_enum" , "heizt" , "de_DE" , "de" , "DE");
insert into name_master values(20 , 2 , "device_status_enum" , "dampft" , "de_DE" , "de" , "DE");
insert into name_master values(21 , 3 , "device_status_enum" , "ausgeschaltet" , "de_DE" , "de" , "DE");
insert into name_master values(22 , 4 , "device_status_enum" , "an" , "de_DE" , "de" , "DE");
insert into name_master values(23 , 5 , "device_status_enum" , "aus" , "de_DE" , "de" , "DE");
insert into name_master values(24 , 6 , "device_status_enum" , "inaktiv" , "de_DE" , "de" , "DE");
insert into name_master values(25 , 7 , "device_status_enum" , "läuft" , "de_DE" , "de" , "DE");
insert into name_master values(43 , 1 , "device_actions" , "bake" , "base" , "" , "");
insert into name_master values(44 , 2 , "device_actions" , "steam" , "base" , "" , "");
insert into name_master values(45 , 3 , "device_actions" , "turn off" , "base" , "" , "");
insert into name_master values(46 , 1 , "device_actions_parameter" , "temperature" , "base" , "" , "");
insert into name_master values(47 , 2 , "device_actions_parameter" , "time" , "base" , "" , "");
insert into name_master values(48 , 3 , "device_actions_parameter" , "temperature" , "base" , "" , "");
insert into name_master values(49 , 4 , "device_actions_parameter" , "time" , "base" , "" , "");
insert into name_master values(50 , 1 , "device_actions_predefined" , "cake" , "base" , "" , "");
insert into name_master values(51 , 2 , "device_actions_predefined" , "pizza" , "base" , "" , "");
insert into name_master values(52 , 3 , "device_actions_predefined" , "asparagus" , "base" , "" , "");
insert into name_master values(53 , 4 , "device_actions_predefined" , "stop" , "base" , "" , "");
insert into name_master values(54 , 1 , "device_properties" , "temperature" , "base" , "" , "");
insert into name_master values(55 , 2 , "device_properties" , "finish time" , "base" , "" , "");
insert into name_master values(56 , 3 , "device_properties" , "core temperature" , "base" , "" , "");
insert into name_master values(57 , 2 , "device_events" , "need water" , "base" , "" , "");
insert into name_master values(58 , 5 , "device_status" , "operation mode" , "base" , "" , "");
insert into name_master values(59 , 6 , "device_status" , "ventilator" , "base" , "" , "");
insert into name_master values(60 , 7 , "device_status" , "alarm clock" , "base" , "" , "");
insert into name_master values(61 , 1 , "device_status_enum" , "heating" , "base" , "" , "");
insert into name_master values(62 , 2 , "device_status_enum" , "steaming" , "base" , "" , "");
insert into name_master values(63 , 3 , "device_status_enum" , "turned of" , "base" , "" , "");
insert into name_master values(64 , 4 , "device_status_enum" , "on" , "base" , "" , "");
insert into name_master values(65 , 5 , "device_status_enum" , "off" , "base" , "" , "");
insert into name_master values(66 , 6 , "device_status_enum" , "inactive" , "base" , "" , "");
insert into name_master values(67 , 7 , "device_status_enum" , "running" , "base" , "" , "");
insert into name_master values(68 , 8 , "device_status" , "operation mode" , "base" , "" , "");
insert into name_master values(69 , 8 , "device_status_enum" , "cooling" , "base" , "" , "");
insert into name_master values(70 , 9 , "device_status_enum" , "heat up" , "base" , "" , "");
insert into name_master values(71 , 10 , "device_status_enum" , "keep warm" , "base" , "" , "");
insert into name_master values(72 , 11 , "device_status_enum" , "ready" , "base" , "" , "");
insert into name_master values(73 , 12 , "device_status_enum" , "released" , "base" , "" , "");
insert into name_master values(74 , 4 , "device_actions" , "stopping" , "base" , "" , "");
insert into name_master values(75 , 5 , "device_actions" , "boil" , "base" , "" , "");
insert into name_master values(76 , 6 , "device_actions" , "boilandcooldown" , "base" , "" , "");
insert into name_master values(77 , 5 , "device_actions_parameter" , "keep warm duration" , "base" , "" , "");
insert into name_master values(78 , 7 , "device_actions_parameter" , "keep warm duration" , "base" , "" , "");
insert into name_master values(79 , 6 , "device_actions_parameter" , "targettemperature" , "base" , "" , "");
insert into name_master values(80 , 8 , "device_actions_parameter" , "targettemperature" , "base" , "" , "");
insert into name_master values(81 , 5 , "device_actions_predefined" , "babyboiling" , "base" , "" , "");
insert into name_master values(82 , 6 , "device_actions_predefined" , "boil" , "base" , "" , "");
insert into name_master values(83 , 7 , "device_actions_predefined" , "stopping" , "base" , "" , "");
insert into name_master values(84 , 3 , "device_events" , "Kettle attached" , "base" , "" , "");
insert into name_master values(88 , 4 , "device_events" , "Boiling started" , "base" , "" , "");
insert into name_master values(89 , 5 , "device_events" , "Keeping warm started" , "base" , "" , "");
insert into name_master values(90 , 6 , "device_events" , "Boil and cool down (Babyboil) started" , "base" , "" , "");
insert into name_master values(91 , 7 , "device_events" , "Boiling ended without keeping warm" , "base" , "" , "");
insert into name_master values(92 , 8 , "device_events" , "Kettle was taken away" , "base" , "" , "");
insert into name_master values(93 , 9 , "device_events" , "Cooled down to specified end temperature without keeping warm" , "base" , "" , "");
insert into name_master values(94 , 10 , "device_events" , "Keeping warm finished (timeout)" , "base" , "" , "");
insert into name_master values(95 , 11 , "device_events" , "Boiling aborted by button press" , "base" , "" , "");
insert into name_master values(96 , 12 , "device_events" , "Cooling down aborted by button press" , "base" , "" , "");
insert into name_master values(97 , 13 , "device_events" , "Keeping warm aborted by button press" , "base" , "" , "");
insert into name_master values(98 , 14 , "device_events" , "Keep warm after boiling started" , "base" , "" , "");
insert into name_master values(99 , 15 , "device_events" , "Keep temperature after cooling down (BabyBoil) started" , "base" , "" , "");
insert into name_master values(100 , 16 , "device_events" , "Boiling aborted because kettle was removed" , "base" , "" , "");
insert into name_master values(101 , 17 , "device_events" , "Cooling down aborted because kettle was removed" , "base" , "" , "");
insert into name_master values(102 , 18 , "device_events" , "Keeping warm aborted because kettle was removed" , "base" , "" , "");
insert into name_master values(135 , 5 , "device_status" , "Operation Mode" , "en_US" , "en" , "US");
insert into name_master values(136 , 6 , "device_status" , "Ventilator" , "en_US" , "en" , "US");
insert into name_master values(137 , 7 , "device_status" , "Alarm Timer" , "en_US" , "en" , "US");
insert into name_master values(139 , 1 , "device_actions" , "Bake" , "en_US" , "en" , "US");
insert into name_master values(140 , 2 , "device_actions" , "Steaming" , "en_US" , "en" , "US");
insert into name_master values(141 , 3 , "device_actions" , "Deactivate" , "en_US" , "en" , "US");
insert into name_master values(145 , 1 , "device_actions_parameter" , "Temperature" , "en_US" , "en" , "US");
insert into name_master values(146 , 2 , "device_actions_parameter" , "Time" , "en_US" , "en" , "US");
insert into name_master values(147 , 3 , "device_actions_parameter" , "Temperature" , "en_US" , "en" , "US");
insert into name_master values(148 , 4 , "device_actions_parameter" , "Time" , "en_US" , "en" , "US");
insert into name_master values(153 , 1 , "device_actions_predefined" , "Cake" , "en_US" , "en" , "US");
insert into name_master values(154 , 2 , "device_actions_predefined" , "Pizza" , "en_US" , "en" , "US");
insert into name_master values(155 , 3 , "device_actions_predefined" , "Asparagus" , "en_US" , "en" , "US");
insert into name_master values(156 , 4 , "device_actions_predefined" , "Stop" , "en_US" , "en" , "US");
insert into name_master values(160 , 1 , "device_status_enum" , "Heating" , "en_US" , "en" , "US");
insert into name_master values(161 , 2 , "device_status_enum" , "Steaming" , "en_US" , "en" , "US");
insert into name_master values(162 , 3 , "device_status_enum" , "Turned Off" , "en_US" , "en" , "US");
insert into name_master values(163 , 4 , "device_status_enum" , "On" , "en_US" , "en" , "US");
insert into name_master values(164 , 5 , "device_status_enum" , "Off" , "en_US" , "en" , "US");
insert into name_master values(165 , 6 , "device_status_enum" , "Inactive" , "en_US" , "en" , "US");
insert into name_master values(166 , 7 , "device_status_enum" , "Running" , "en_US" , "en" , "US");
insert into name_master values(182 , 1 , "common_labels" , "Name" , "de_DE" , "de" , "DE");
insert into name_master values(183 , 1 , "common_labels" , "Name" , "en_US" , "en" , "US");
insert into name_master values(184 , 2 , "common_labels" , "dS Device GTIN" , "de_DE" , "de" , "DE");
insert into name_master values(185 , 2 , "common_labels" , "dS Device GTIN" , "en_US" , "en" , "US");
insert into name_master values(186 , 3 , "common_labels" , "Modellbezeichnung" , "de_DE" , "de" , "DE");
insert into name_master values(187 , 3 , "common_labels" , "Model Name" , "en_US" , "en" , "US");
insert into name_master values(188 , 4 , "common_labels" , "Modellvariante" , "de_DE" , "de" , "DE");
insert into name_master values(189 , 4 , "common_labels" , "Model Version" , "en_US" , "en" , "US");
insert into name_master values(190 , 5 , "common_labels" , "Artikel Kennzeichnung" , "en_US" , "en" , "US");
insert into name_master values(191 , 5 , "common_labels" , "Article Identifier" , "en_US" , "en" , "US");
insert into name_master values(192 , 6 , "common_labels" , "Produkt Kennzeichnung" , "de_DE" , "de" , "DE");
insert into name_master values(193 , 6 , "common_labels" , "Product Id" , "en_US" , "en" , "US");
insert into name_master values(194 , 7 , "common_labels" , "Hersteller" , "de_DE" , "de" , "DE");
insert into name_master values(195 , 7 , "common_labels" , "Vendor" , "en_US" , "en" , "US");
insert into name_master values(196 , 8 , "common_labels" , "Hersteller Kennung" , "de_DE" , "de" , "DE");
insert into name_master values(197 , 8 , "common_labels" , "Vendor Id" , "en_US" , "en" , "US");
insert into name_master values(198 , 9 , "common_labels" , "Geräteklasse" , "de_DE" , "de" , "DE");
insert into name_master values(199 , 9 , "common_labels" , "Device Class" , "en_US" , "en" , "US");
insert into name_master values(200 , 10 , "common_labels" , "Geräteklassen version" , "de_DE" , "de" , "DE");
insert into name_master values(201 , 1 , "common_labels" , "Name" , "base" , "" , "");
insert into name_master values(202 , 2 , "common_labels" , "dS Device GTIN" , "base" , "" , "");
insert into name_master values(203 , 3 , "common_labels" , "Model Name" , "base" , "" , "");
insert into name_master values(204 , 4 , "common_labels" , "Model Version" , "base" , "" , "");
insert into name_master values(205 , 5 , "common_labels" , "Article Identifier" , "base" , "" , "");
insert into name_master values(206 , 6 , "common_labels" , "Product Id" , "base" , "" , "");
insert into name_master values(207 , 7 , "common_labels" , "Vendor" , "base" , "" , "");
insert into name_master values(208 , 8 , "common_labels" , "Vendor Id" , "base" , "" , "");
insert into name_master values(209 , 9 , "common_labels" , "Device Class" , "base" , "" , "");
insert into name_master values(210 , 10 , "common_labels" , "deviceclasses version" , "base" , "" , "");
insert into name_master values(1311 , 1 , "device_actions" , "Back!" , "nl_NL" , "nl" , "NL");
insert into name_master values(1312 , 2 , "device_actions" , "Dampfen" , "nl_NL" , "nl" , "NL");
insert into name_master values(1313 , 3 , "device_actions" , "auuus" , "nl_NL" , "nl" , "NL");
insert into name_master values(1314 , 1 , "device_actions" , "Backen" , "de_DE" , "de" , "DE");
insert into name_master values(1315 , 2 , "device_actions" , "Dampfen" , "de_DE" , "de" , "DE");
insert into name_master values(1316 , 4 , "device_actions" , "stoppen" , "nl_NL" , "nl" , "NL");
insert into name_master values(1317 , 5 , "device_actions" , "koken" , "nl_NL" , "nl" , "NL");
insert into name_master values(1318 , 6 , "device_actions" , "kook en afkoelen" , "nl_NL" , "nl" , "NL");
insert into name_master values(1319 , 3 , "device_events" , "ketel bevestigd" , "nl_NL" , "nl" , "NL");
insert into name_master values(1320 , 4 , "device_events" , "koken begonnen" , "nl_NL" , "nl" , "NL");
insert into name_master values(1321 , 5 , "device_events" , "warm houden begonnen" , "nl_NL" , "nl" , "NL");
insert into name_master values(1322 , 4 , "device_actions" , "Stop" , "en_US" , "en" , "US");
insert into name_master values(1323 , 5 , "device_actions" , "Boil" , "en_US" , "en" , "US");
insert into name_master values(1324 , 6 , "device_actions" , "Boil and cool down" , "en_US" , "en" , "US");
insert into name_master values(1325 , 3 , "device_events" , "Kettle attached" , "en_US" , "en" , "US");
insert into name_master values(1326 , 4 , "device_events" , "Boiling started" , "en_US" , "en" , "US");
insert into name_master values(1327 , 5 , "device_events" , "Keeping warm started" , "en_US" , "en" , "US");
insert into name_master values(1328 , 6 , "device_events" , "Boiling finished, cooling down to target temperature" , "en_US" , "en" , "US");
insert into name_master values(1329 , 7 , "device_events" , "Boiling finished" , "en_US" , "en" , "US");
insert into name_master values(1330 , 8 , "device_events" , "Kettle released" , "en_US" , "en" , "US");
insert into name_master values(1331 , 9 , "device_events" , "Cooling down finished, target temperature reached" , "en_US" , "en" , "US");
insert into name_master values(1332 , 10 , "device_events" , "Keeping warm ended" , "en_US" , "en" , "US");
insert into name_master values(1333 , 11 , "device_events" , "Boiling aborted, button pressed" , "en_US" , "en" , "US");
insert into name_master values(1334 , 12 , "device_events" , "Cooling down aborted, button pressed" , "en_US" , "en" , "US");
insert into name_master values(1335 , 13 , "device_events" , "Keeping warm aborted, button pressed" , "en_US" , "en" , "US");
insert into name_master values(1336 , 14 , "device_events" , "Boiling finished, keeping warm now" , "en_US" , "en" , "US");
insert into name_master values(1337 , 15 , "device_events" , "Cooling down finished, keeping warm now" , "en_US" , "en" , "US");
insert into name_master values(1338 , 16 , "device_events" , "Boiling aborted, kettle released" , "en_US" , "en" , "US");
insert into name_master values(1339 , 17 , "device_events" , "Cooling down aborted, kettle released" , "en_US" , "en" , "US");
insert into name_master values(1340 , 18 , "device_events" , "Keeping warm aborted, button pressed" , "en_US" , "en" , "US");
insert into name_master values(1341 , 8 , "device_status" , "Operation Mode" , "en_US" , "en" , "US");
insert into name_master values(1342 , 5 , "device_actions_predefined" , "Baby Bottle Boiling" , "en_US" , "en" , "US");
insert into name_master values(1343 , 6 , "device_actions_predefined" , "Boil" , "en_US" , "en" , "US");
insert into name_master values(1344 , 7 , "device_actions_predefined" , "Stopping" , "en_US" , "en" , "US");
insert into name_master values(1345 , 5 , "device_actions_parameter" , "Keep-warm Duration" , "en_US" , "en" , "US");
insert into name_master values(1346 , 7 , "device_actions_parameter" , "Keep-Warm Duration" , "en_US" , "en" , "US");
insert into name_master values(1347 , 6 , "device_actions_parameter" , "Target Temperature" , "en_US" , "en" , "US");
insert into name_master values(1348 , 8 , "device_actions_parameter" , "Target Temperature" , "en_US" , "en" , "US");
insert into name_master values(1349 , 8 , "device_status_enum" , "Cooling down" , "en_US" , "en" , "US");
insert into name_master values(1350 , 9 , "device_status_enum" , "Heating up" , "en_US" , "en" , "US");
insert into name_master values(1351 , 10 , "device_status_enum" , "Keeping warm" , "en_US" , "en" , "US");
insert into name_master values(1352 , 11 , "device_status_enum" , "Ready" , "en_US" , "en" , "US");
insert into name_master values(1353 , 12 , "device_status_enum" , "Kettle released" , "en_US" , "en" , "US");
insert into name_master values(1354 , 4 , "device_actions" , "Abschalten" , "de_DE" , "de" , "DE");
insert into name_master values(1355 , 5 , "device_actions" , "Aufheizen" , "de_DE" , "de" , "DE");
insert into name_master values(1356 , 6 , "device_actions" , "Abkochen und abkühlen" , "de_DE" , "de" , "DE");
insert into name_master values(1357 , 3 , "device_events" , "Kocher aufgesetzt" , "de_DE" , "de" , "DE");
insert into name_master values(1358 , 4 , "device_events" , "Aufheizen gestartet" , "de_DE" , "de" , "DE");
insert into name_master values(1359 , 5 , "device_events" , "Warmhalten gestartet" , "de_DE" , "de" , "DE");
insert into name_master values(1360 , 6 , "device_events" , "Aufheizen beendet,  auf Zieltemperatur abkühlen" , "de_DE" , "de" , "DE");
insert into name_master values(1361 , 7 , "device_events" , "Aufheizen beendet" , "de_DE" , "de" , "DE");
insert into name_master values(1362 , 8 , "device_events" , "Kocher abgehoben" , "de_DE" , "de" , "DE");
insert into name_master values(1363 , 9 , "device_events" , "Abkühlen beendet, Zieltemperatur erreichet" , "de_DE" , "de" , "DE");
insert into name_master values(1364 , 10 , "device_events" , "Warmhalten beendet" , "de_DE" , "de" , "DE");
insert into name_master values(1365 , 11 , "device_events" , "Aufheizen abgebrochen, Taste betätigt" , "de_DE" , "de" , "DE");
insert into name_master values(1366 , 12 , "device_events" , "Abkühlen abgebrochen, Taste betätigt" , "de_DE" , "de" , "DE");
insert into name_master values(1367 , 13 , "device_events" , "Warmhalten abgebrochen, Taste betätigt" , "de_DE" , "de" , "DE");
insert into name_master values(1368 , 14 , "device_events" , "Aufheizen beendet, warmhalten" , "de_DE" , "de" , "DE");
insert into name_master values(1369 , 15 , "device_events" , "Abkühlen beendet, warmhalten" , "de_DE" , "de" , "DE");
insert into name_master values(1370 , 16 , "device_events" , "Aufheizen abgebrochen, Kocher abgehoben" , "de_DE" , "de" , "DE");
insert into name_master values(1371 , 17 , "device_events" , "Abkühlen abgebrochen, Kocher abgehoben" , "de_DE" , "de" , "DE");
insert into name_master values(1372 , 18 , "device_events" , "Warmhalten abgebrochen, Kocher abgehoben" , "de_DE" , "de" , "DE");
insert into name_master values(1373 , 8 , "device_status" , "Betriebsmodus" , "de_DE" , "de" , "DE");
insert into name_master values(1374 , 5 , "device_actions_predefined" , "Abkochen und abkühlen" , "de_DE" , "de" , "DE");
insert into name_master values(1375 , 6 , "device_actions_predefined" , "Aufheizen" , "de_DE" , "de" , "DE");
insert into name_master values(1376 , 7 , "device_actions_predefined" , "Abschalten" , "de_DE" , "de" , "DE");
insert into name_master values(1377 , 5 , "device_actions_parameter" , "Warmhaltedauer" , "de_DE" , "de" , "DE");
insert into name_master values(1378 , 7 , "device_actions_parameter" , "Warmhaltedauer" , "de_DE" , "de" , "DE");
insert into name_master values(1379 , 6 , "device_actions_parameter" , "Warmhaltetemperatur" , "de_DE" , "de" , "DE");
insert into name_master values(1380 , 8 , "device_actions_parameter" , "Zieltemperatur" , "de_DE" , "de" , "DE");
insert into name_master values(1381 , 8 , "device_status_enum" , "Abkühlen" , "de_DE" , "de" , "DE");
insert into name_master values(1382 , 9 , "device_status_enum" , "Aufheizen" , "de_DE" , "de" , "DE");
insert into name_master values(1383 , 10 , "device_status_enum" , "Warmhalten" , "de_DE" , "de" , "DE");
insert into name_master values(1384 , 11 , "device_status_enum" , "Bereit" , "de_DE" , "de" , "DE");
insert into name_master values(1385 , 12 , "device_status_enum" , "Abgehoben" , "de_DE" , "de" , "DE");
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
insert into device_labels values(1 , 18 , "notes" , "overview:4");
drop table if exists "common_labels";
create table "common_labels" (id primary key,name,tags);
insert into common_labels values(1 , "name" , "overview:1;settings:1");
insert into common_labels values(2 , "dsDeviceGTIN" , "settings:5");
insert into common_labels values(3 , "model" , "overview:2;settings:2");
insert into common_labels values(4 , "modelVersion" , "invisible");
insert into common_labels values(5 , "hardwareGuid" , "settings:4");
insert into common_labels values(6 , "hardwareModelGuid" , "invisible");
insert into common_labels values(7 , "vendorName" , "overview:3;settings:3");
insert into common_labels values(8 , "vendorId" , "invisible");
insert into common_labels values(9 , "class" , "invisible");
insert into common_labels values(10 , "classVersion" , "invisible");
drop table if exists "device_properties";
create table "device_properties" (id primary key,device_id,name,type_id,min_value,max_value,default_value,resolution,si_unit,tags);
insert into device_properties values(1 , 12 , "temperature" , 1 , "0" , "250" , "0" , "1" , "celsius" , "" );
insert into device_properties values(2 , 12 , "duration" , 2 , "0" , "1800" , "0" , "1" , "second" , "" );
insert into device_properties values(3 , 12 , "temperature.sensor" , 1 , "0" , "250" , "0" , "1" , "celsius" , "readonly" );
insert into device_properties values(4 , 18 , "currentTemperature" , 1 , "0" , "100" , "0" , "1" , "celsius" , "readonly;overview" );
insert into device_properties values(5 , 18 , "waterLevel" , 3 , "0" , "2.0" , "0" , "0.2" , "liter" , "readonly;overview" );
insert into device_properties values(6 , 18 , "defaulttemperature" , 1 , "0" , "100" , "100" , "1" , "celsius" , "settings" );
insert into device_properties values(7 , 18 , "defaultcooldowntemperature" , 1 , "0" , "100" , "80" , "1" , "celsius" , "settings" );
insert into device_properties values(8 , 18 , "defaultkeepwarmtime" , 2 , "0" , "120" , "15" , "1" , "min" , "settings" );
drop table if exists "name_device_properties";
create table "name_device_properties" (id primary key,reference_id,lang_code,displayMask,alt_label,alt_postfix,alt_precision_float,lang,country);
insert into name_device_properties values(1 , 1 , "de_DE" , "Temperatur {value} &deg;" , "Temperatur" , "&deg;" , "0" , "de" , "DE");
insert into name_device_properties values(2 , 2 , "de_DE" , "Endzeit in {value} Sekunden" , "Endzeit" , "Sekunden" , "0" , "de" , "DE");
insert into name_device_properties values(3 , 3 , "de_DE" , "Garguttemperatur {value} &deg;" , "Garguttemperatur" , "&deg;" , "0" , "de" , "DE");
insert into name_device_properties values(4 , 4 , "de_DE" , "Temperatur {value} Grad Celsius" , "Wassertemperatur" , "&deg; C" , "0" , "de" , "DE");
insert into name_device_properties values(5 , 1 , "base" , "temperature {value} &deg; " , "temperature" , "&deg;" , "0" , "" , "");
insert into name_device_properties values(6 , 2 , "base" , "finished in {value} seconds" , "finishtime" , "seconds" , "0" , "" , "");
insert into name_device_properties values(7 , 3 , "base" , "coretemperature {value} &deg;" , "coretemperature" , "&deg;" , "0" , "" , "");
insert into name_device_properties values(8 , 4 , "base" , "temperature {value} &deg;" , "temperature" , "&deg;" , "0" , "" , "");
insert into name_device_properties values(9 , 1 , "en_US" , "temperature {value} &deg; " , "Temperature" , "&deg;" , "0" , "en" , "US");
insert into name_device_properties values(10 , 2 , "en_US" , "finished in {value} seconds" , "Finishtime" , "seconds" , "0" , "en" , "US");
insert into name_device_properties values(11 , 3 , "en_US" , "coretemperature {value} &deg;" , "Core Temperature" , "&deg;" , "0" , "en" , "US");
insert into name_device_properties values(12 , 4 , "en_US" , "Temperature {value} &deg; C" , "Temperature" , "&deg; C" , "0" , "en" , "US");
insert into name_device_properties values(19 , 1 , "nl_NL" , "NL Temperatur: {value} &deg;" , "NL Temperatur" , "NL &deg;" , "0" , "" , "");
insert into name_device_properties values(20 , 3 , "nl_NL" , "coretemperature {value} &deg;" , "coretemperature" , "NL &deg;" , "0" , "" , "");
insert into name_device_properties values(21 , 5 , "base" , "water level {value} &#37;" , "water level" , "&#37;" , "0" , "" , "");
insert into name_device_properties values(22 , 5 , "de_DE" , "Füllstand {value} &#37;" , "Füllstand" , "&#37:" , "0" , "de" , "DE");
insert into name_device_properties values(23 , 5 , "en_US" , "Water level {value} &#37;" , "Water-level" , "&#37;" , "0" , "en" , "US");
insert into name_device_properties values(28 , 6 , "base" , "boil temperature {value}" , "boil temperature" , "" , "0" , "" , "");
insert into name_device_properties values(30 , 7 , "base" , "keepwarm temperature {value}" , "keepwarm temperature" , "" , "0" , "" , "");
insert into name_device_properties values(31 , 8 , "base" , "keepwarm time {value}" , "keepwarm time" , "" , "0" , "" , "");
insert into name_device_properties values(32 , 6 , "de_DE" , "Temperatur Aufheizen {value}" , "Temperatur Aufheizen" , "" , "0" , "de" , "DE");
insert into name_device_properties values(33 , 7 , "de_DE" , "Temperatur Abkochen und Abkühlen {value}" , "Temperatur Abkochen und Abkühlen" , "" , "0" , "de" , "DE");
insert into name_device_properties values(34 , 8 , "de_DE" , "Warmhaltezeit {value}" , "Warmhaltezeit" , "" , "0" , "de" , "DE");
insert into name_device_properties values(35 , 6 , "en_US" , "Temperature Boiling {value}" , "Temperature Boiling" , "" , "0" , "en" , "US");
insert into name_device_properties values(36 , 7 , "en_US" , "Temperature Babyboiling {value}" , "Temperature Babyboiling" , "" , "0" , "en" , "US");
insert into name_device_properties values(37 , 8 , "en_US" , "Keepwarm Time {value}" , "Keepwarm Time" , "" , "0" , "en" , "US");
drop table if exists "name_device_labels";
create table "name_device_labels" (id primary key,reference_id,lang_code,title,value,lang,country);
insert into name_device_labels values(1 , 1 , "de_DE" , "Bemerkungen" , "Bitte prüfen Sie mit der Smartphone App, ob die iKettle Firmware auf dem aktuellsten Stand ist!" , "de" , "DE");
insert into name_device_labels values(2 , 1 , "en_US" , "Notes" , "Please check with the Smartphone App if the iKettle Firmware is up-to-date!" , "en" , "US");
insert into name_device_labels values(3 , 1 , "base" , "notes" , "updatechecknotice" , "" , "");
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
insert into device_actions_parameter values(5 , 6 , "keepwarmtime" , 2 , "0" , "120" , "30" , "1" , "min" , "");
insert into device_actions_parameter values(6 , 6 , "temperature" , 1 , "20" , "100" , "50" , "1" , "celsius" , "");
insert into device_actions_parameter values(7 , 5 , "keepwarmtime" , 2 , "0" , "120" , "30" , "1" , "min" , "");
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
insert into device_actions_predefined_parameter values(7 , "37" , 8 , 6);
insert into device_actions_predefined_parameter values(8 , "30" , 7 , 6);
insert into device_actions_predefined_parameter values(9 , "30" , 5 , 5);
insert into device_actions_predefined_parameter values(10 , "100" , 6 , 5);

drop view if exists "name_master_2";
create view if not exists "name_master_2"  as
select scope, name, reference_id, lang, country,lang_code, 1 as langorder
from name_master as n1
where not(lang_code="base")
union
select scope, name, reference_id, lang, "ZZ" as country,lang_code, 2 as langorder
from name_master as n1 
where not(lang_code="base") 
union
select scope, name, reference_id, "ZZ" as lang,"ZZ" as country,lang_code, 3 as langorder
from name_master as n1
where (lang_code="en_US")
order by langorder;
		
		
drop view if exists "name_device_properties_2";
create view if not exists "name_device_properties_2" as 
select reference_id,lang_code,displayMask,alt_label,alt_postfix,alt_precision_float,lang,country, 1 as langorder
from name_device_properties where not(lang_code="base")
union
select reference_id,lang_code,displayMask,alt_label,alt_postfix,alt_precision_float,lang, "ZZ" as country, 2 as langorder
from name_device_properties where not(lang_code="base")
union
select reference_id,lang_code,displayMask,alt_label,alt_postfix,alt_precision_float,"ZZ" as lang,"ZZ" as country, 3 as langorder
from name_device_properties where (lang_code="en_US")		
order by langorder;
		
drop view if exists "name_device_labels_2";
create view if not exists "name_device_labels_2" as 
select reference_id,lang_code,title,value,lang,country, 1 as langorder
from name_device_labels where not(lang_code="base")
union
select reference_id,lang_code,title,value,lang, "ZZ" as country, 2 as langorder
from name_device_labels where not(lang_code="base")
union
select reference_id,lang_code,title,value, "ZZ" as lang,"ZZ" as country, 3 as langorder
from name_device_labels where (lang_code="en_US")		
order by langorder;
				
		
drop view if exists "name_device_status";
create view if not exists "name_device_status"  as select reference_id,name,lang_code,lang,country,langorder from name_master_2 where scope="device_status";	
drop view if exists "name_device_status_enum";
create view if not exists "name_device_status_enum"  as select reference_id,name,lang_code,lang,country,langorder from name_master_2 where scope="device_status_enum";	
drop view if exists "name_device_actions";
create view if not exists "name_device_actions"  as select reference_id,name,lang_code,lang,country,langorder from name_master_2 where scope="device_actions";
drop view if exists "name_device_events";
create view if not exists "name_device_events"  as select reference_id,name,lang_code,lang,country,langorder from name_master_2 where scope="device_events";
drop view if exists "name_device_actions";
create view if not exists "name_device_actions"  as select reference_id,name,lang_code,lang,country,langorder from name_master_2 where scope="device_actions";
drop view if exists "name_device_actions_parameter_type";
create view if not exists "name_device_actions_parameter_type" as select reference_id,name,lang_code,lang,country,langorder from name_master_2 where scope="device_actions_parameter_type";
drop view if exists "name_device_actions_parameter_type_enum";
create view if not exists "name_device_actions_parameter_type_enum"  as select reference_id,name,lang_code,lang,country,langorder from name_master_2 where scope="device_actions_parameter_type_enum";
drop view if exists "name_device_actions_predefined";
create view if not exists "name_device_actions_predefined"  as select reference_id,name,lang_code,lang,country,langorder from name_master_2 where scope="device_actions_predefined";
drop view if exists "name_device_actions_parameter";
create view if not exists "name_device_actions_parameter"  as select reference_id,name,lang_code,lang,country,langorder from name_master_2 where scope="device_actions_parameter";
drop view if exists "name_device_actions";
create view if not exists "name_device_actions"  as select reference_id,name,lang_code,lang,country,langorder from name_master_2 where scope="device_actions";
drop view if exists "name_common_labels";
create view if not exists "name_common_labels"  as select reference_id,name,lang_code,lang,country,langorder from name_master_2 where scope="common_labels";

commit;
begin transaction;
		
drop view if exists "callGetStates";
create view if not exists "callGetStates" as
SELECT device_status.name, name_device_status.name as displayName, device_status_enum.value, name_device_status_enum.name as enumName, device_status.tags, device.gtin,name_device_status.lang,name_device_status.country,name_device_status.langorder
FROM device INNER JOIN
((device_status INNER JOIN name_device_status ON device_status.id=name_device_status.reference_id)
		INNER JOIN (device_status_enum INNER JOIN name_device_status_enum ON device_status_enum.id=name_device_status_enum.reference_id) ON device_status.id=device_status_enum.device_id) ON device.id=device_status.device_id
		WHERE name_device_status.lang_code = name_device_status_enum.lang_code
		ORDER BY name_device_status.langorder,device_status.name;
		
drop view if exists "callGetStatesBase";
create view if not exists "callGetStatesBase" as 
	SELECT device_status.name, device_status_enum.value, device_status.tags, device.gtin
	FROM device INNER JOIN (device_status INNER JOIN device_status_enum ON device_status.id=device_status_enum.device_id) ON device.id=device_status.device_id
	ORDER BY device_status.name;

drop view if exists "callGetEvents";
create view if not exists "callGetEvents" as 
	SELECT device_events.name, name_device_events.name as displayName, device.gtin, name_device_events.lang,name_device_events.country,name_device_events.langorder
	FROM (device INNER JOIN 
		(device_events INNER JOIN name_device_events ON device_events.id=name_device_events.reference_id) ON device.id=device_events.device_id)
	ORDER BY name_device_events.langorder;
		

drop view if exists "callGetEventsBase";
create view if not exists "callGetEventsBase" as 
	SELECT device_events.name, device.gtin
		FROM (device INNER JOIN device_events ON device.id=device_events.device_id);

drop view if exists "callGetPropertiesBase";
create view if not exists "callGetPropertiesBase" as 
	SELECT device_properties.name, device_properties.type_id, device_properties.default_value, device_properties.min_value, device_properties.max_value, device_properties.resolution, device_properties.si_unit, device_properties.tags, device.gtin 
		FROM (device INNER JOIN device_properties ON device.id=device_properties.device_id);

		
drop view if exists "callGetActions";
create view if not exists "callGetActions" as 
	SELECT device_actions.command, name_device_actions.name as actionName, device_actions_parameter.name as parameterName, name_device_actions_parameter.name as parameterDisplayName, device_actions_parameter.type_id, device_actions_parameter.default_value, 
		device_actions_parameter.min_value, device_actions_parameter.max_value, device_actions_parameter.resolution, device_actions_parameter.si_unit, device_actions_parameter.tags, 
		CASE WHEN device_actions_parameter.type_id is NULL THEN "true" ELSE "false" END AS paramexists, device.gtin, name_device_actions.lang,name_device_actions.country,name_device_actions.langorder 
    FROM (device
    	INNER JOIN (device_actions INNER JOIN name_device_actions ON device_actions.id=name_device_actions.reference_id) ON device.id=device_actions.device_id) 
        LEFT JOIN (device_actions_parameter INNER JOIN name_device_actions_parameter ON device_actions_parameter.id=name_device_actions_parameter.reference_id) ON device_actions.id=device_actions_parameter.device_actions_id 
    WHERE ((name_device_actions.lang_code=name_device_actions_parameter.lang_code) OR paramexists="true")
    ORDER BY name_device_actions.langorder,device_actions.command;
		
drop view if exists "callGetActionsBase";
create view if not exists "callGetActionsBase" as 
	SELECT device_actions.command, device_actions_parameter.name as parameterName, device_actions_parameter.type_id, device_actions_parameter.default_value, 
		device_actions_parameter.min_value, device_actions_parameter.max_value, device_actions_parameter.resolution, device_actions_parameter.si_unit, device_actions_parameter.tags, device.gtin
	FROM (device
		INNER JOIN device_actions ON device.id=device_actions.device_id) 
		LEFT JOIN device_actions_parameter ON device_actions.id=device_actions_parameter.device_actions_id 
	ORDER BY device_actions.command;
		
		
drop view if exists "callGetStandardActions";
create view if not exists "callGetStandardActions" as 
	SELECT device_actions_predefined.name as predefName, name_device_actions_predefined.name as displayPredefName, device_actions.command, device_actions_parameter.name as paramName, device_actions_predefined_parameter.value, CASE WHEN device_actions_parameter.type_id is NULL THEN "true" ELSE "false" END AS paramexists,
		device.gtin,name_device_actions_predefined.lang,name_device_actions_predefined.country,name_device_actions_predefined.langorder
    FROM device INNER JOIN ((device_actions 
    	INNER JOIN (device_actions_predefined INNER JOIN name_device_actions_predefined ON device_actions_predefined.id=name_device_actions_predefined.reference_id) ON device_actions.id=device_actions_predefined.device_actions_id) 
        LEFT JOIN (device_actions_parameter INNER JOIN device_actions_predefined_parameter ON device_actions_predefined_parameter.device_actions_parameter_id=device_actions_parameter.id) 
        ON device_actions.id=device_actions_predefined.device_actions_id AND device_actions_predefined.id=device_actions_predefined_parameter.device_actions_predefined_id) 
        ON device.id=device_actions.device_id 
    ORDER BY name_device_actions_predefined.langorder,device_actions.command;
		
drop view if exists "callGetStandardActionsBase";
create view if not exists "callGetStandardActionsBase" as 
	SELECT device_actions_predefined.name as predefName, device_actions.command, device_actions_parameter.name, device_actions_predefined_parameter.value,device.gtin
	FROM device INNER JOIN ((device_actions 
		INNER JOIN device_actions_predefined ON device_actions.id=device_actions_predefined.device_actions_id) 
		LEFT JOIN (device_actions_parameter INNER JOIN device_actions_predefined_parameter ON device_actions_predefined_parameter.device_actions_parameter_id=device_actions_parameter.id) 
			ON device_actions.id=device_actions_predefined.device_actions_id AND device_actions_predefined.id=device_actions_predefined_parameter.device_actions_predefined_id) 
			ON device.id=device_actions.device_id 
	ORDER BY device_actions.command;

drop view if exists "callGetSpec";
create view if not exists "callGetSpec" as 
	SELECT common_labels.name,name_common_labels.name as title,tags,"" as value,"" as gtin,
		name_common_labels.lang,name_common_labels.country,name_common_labels.langorder
		FROM common_labels LEFT JOIN name_common_labels ON common_labels.id=name_common_labels.reference_id 
	UNION	
	SELECT device_labels.name,name_device_labels_2.title,device_labels.tags,name_device_labels_2.value,device.gtin,
		name_device_labels_2.lang,name_device_labels_2.country,name_device_labels_2.langorder
		FROM (device_labels INNER JOIN name_device_labels_2 ON device_labels.id=name_device_labels_2.reference_id) INNER JOIN device on device.id=device_labels.device_id 
	ORDER BY langorder;
		
drop view if exists "callGetSpecBase";
create view if not exists "callGetSpecBase" as 
	SELECT common_labels.name,tags,"" as gtin
		FROM common_labels
	UNION	
	SELECT device_labels.name,device_labels.tags,device.gtin
		FROM device_labels INNER JOIN device on device.id=device_labels.device_id; 
		
		
drop view if exists "callGetProperties";
create view if not exists "callGetProperties" as 
	SELECT device_properties.name, name_device_properties_2.alt_label, device_properties.type_id, device_properties.default_value, device_properties.min_value, device_properties.max_value, device_properties.resolution, device_properties.si_unit, device_properties.tags, device.gtin,
		name_device_properties_2.lang,name_device_properties_2.country,name_device_properties_2.langorder
    FROM (device
		INNER JOIN (device_properties
        INNER JOIN name_device_properties_2 ON device_properties.id=name_device_properties_2.reference_id) 
        	ON device.id=device_properties.device_id)
	ORDER BY name_device_properties_2.langorder;
		
commit;
