begin transaction;
drop table if exists "device";
create table "device"(id primary key,gtin,name,version);
insert into device values(11 , "1234567890123" , "DevelopmentDevice" , "1");
insert into device values(12 , "7640156791914" , "V-Zug MSLQ - aktiv" , "1");
insert into device values(18 , "7640156791945" , "vDC smarter iKettle 2.0" , "1");
insert into device values(24 , "7640156792072" , "Logitech Harmony" , "");
insert into device values(25 , "7640156792089" , "Dornbracht shower sensory sky" , "");
insert into device values(26 , "7640156792096" , "Siemens Coffee Maker CT636LES6" , "");
insert into device values(27 , "7640156792102" , "Siemens Coffee Maker EQ.9 connect s900" , "");
insert into device values(28 , "7640156792119" , "Dornbracht eUnit Kitchen" , "");
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
insert into name_master values(1944 , 1 , "device_actions" , "Backen" , "de_DE" , "de" , "DE");
insert into name_master values(1945 , 2 , "device_actions" , "Dampfen" , "de_DE" , "de" , "DE");
insert into name_master values(2036 , 9 , "device_actions_parameter" , "dummy action parameter number one" , "base" , "" , "");
insert into name_master values(2037 , 10 , "device_actions_parameter" , "dummy action parameter number two" , "base" , "" , "");
insert into name_master values(2038 , 7 , "device_actions" , "dummy action one" , "base" , "" , "");
insert into name_master values(2039 , 8 , "device_actions" , "dummy action two" , "base" , "" , "");
insert into name_master values(2040 , 19 , "device_events" , "Dummy Event Alpha" , "base" , "" , "");
insert into name_master values(2041 , 20 , "device_events" , "Dummy Event Other" , "base" , "" , "");
insert into name_master values(2042 , 9 , "device_state" , "Dummy Device State" , "base" , "" , "");
insert into name_master values(2043 , 8 , "device_actions_predefined" , "Standard Dummy 1" , "base" , "" , "");
insert into name_master values(2044 , 9 , "device_actions_predefined" , "Standard More Dummy" , "base" , "" , "");
insert into name_master values(3688 , 4 , "device_actions" , "Abschalten" , "de_DE" , "de" , "DE");
insert into name_master values(3689 , 5 , "device_actions" , "Aufheizen" , "de_DE" , "de" , "DE");
insert into name_master values(3690 , 6 , "device_actions" , "Abkochen und abkühlen" , "de_DE" , "de" , "DE");
insert into name_master values(3691 , 3 , "device_events" , "Kocher aufgesetzt" , "de_DE" , "de" , "DE");
insert into name_master values(3692 , 4 , "device_events" , "Aufheizen gestartet" , "de_DE" , "de" , "DE");
insert into name_master values(3693 , 5 , "device_events" , "Warmhalten gestartet" , "de_DE" , "de" , "DE");
insert into name_master values(3694 , 6 , "device_events" , "Aufheizen beendet,  auf Zieltemperatur abkühlen" , "de_DE" , "de" , "DE");
insert into name_master values(3695 , 7 , "device_events" , "Aufheizen beendet" , "de_DE" , "de" , "DE");
insert into name_master values(3696 , 8 , "device_events" , "Kocher abgehoben" , "de_DE" , "de" , "DE");
insert into name_master values(3697 , 9 , "device_events" , "Abkühlen beendet, Zieltemperatur erreichet" , "de_DE" , "de" , "DE");
insert into name_master values(3698 , 10 , "device_events" , "Warmhalten beendet" , "de_DE" , "de" , "DE");
insert into name_master values(3699 , 11 , "device_events" , "Aufheizen abgebrochen, Taste betätigt" , "de_DE" , "de" , "DE");
insert into name_master values(3700 , 12 , "device_events" , "Abkühlen abgebrochen, Taste betätigt" , "de_DE" , "de" , "DE");
insert into name_master values(3701 , 13 , "device_events" , "Warmhalten abgebrochen, Taste betätigt" , "de_DE" , "de" , "DE");
insert into name_master values(3702 , 14 , "device_events" , "Aufheizen beendet, warmhalten" , "de_DE" , "de" , "DE");
insert into name_master values(3703 , 15 , "device_events" , "Abkühlen beendet, warmhalten" , "de_DE" , "de" , "DE");
insert into name_master values(3704 , 16 , "device_events" , "Aufheizen abgebrochen, Kocher abgehoben" , "de_DE" , "de" , "DE");
insert into name_master values(3705 , 17 , "device_events" , "Abkühlen abgebrochen, Kocher abgehoben" , "de_DE" , "de" , "DE");
insert into name_master values(3706 , 18 , "device_events" , "Warmhalten abgebrochen, Kocher abgehoben" , "de_DE" , "de" , "DE");
insert into name_master values(3707 , 8 , "device_status" , "Betriebsmodus" , "de_DE" , "de" , "DE");
insert into name_master values(3708 , 5 , "device_actions_predefined" , "Abkochen und abkühlen" , "de_DE" , "de" , "DE");
insert into name_master values(3709 , 6 , "device_actions_predefined" , "Aufheizen" , "de_DE" , "de" , "DE");
insert into name_master values(3710 , 7 , "device_actions_predefined" , "Abschalten" , "de_DE" , "de" , "DE");
insert into name_master values(3711 , 5 , "device_actions_parameter" , "Warmhaltedauer" , "de_DE" , "de" , "DE");
insert into name_master values(3712 , 7 , "device_actions_parameter" , "Warmhaltedauer" , "de_DE" , "de" , "DE");
insert into name_master values(3713 , 6 , "device_actions_parameter" , "Warmhaltetemperatur" , "de_DE" , "de" , "DE");
insert into name_master values(3714 , 8 , "device_actions_parameter" , "Zieltemperatur" , "de_DE" , "de" , "DE");
insert into name_master values(3715 , 8 , "device_status_enum" , "Abkühlen" , "de_DE" , "de" , "DE");
insert into name_master values(3716 , 9 , "device_status_enum" , "Aufheizen" , "de_DE" , "de" , "DE");
insert into name_master values(3717 , 10 , "device_status_enum" , "Warmhalten" , "de_DE" , "de" , "DE");
insert into name_master values(3718 , 11 , "device_status_enum" , "Bereit" , "de_DE" , "de" , "DE");
insert into name_master values(3719 , 12 , "device_status_enum" , "Abgehoben" , "de_DE" , "de" , "DE");
insert into name_master values(3720 , 1 , "common_labels" , "Name" , "de_DE" , "de" , "DE");
insert into name_master values(3721 , 2 , "common_labels" , "dS Device GTIN" , "de_DE" , "de" , "DE");
insert into name_master values(3722 , 3 , "common_labels" , "Modell" , "de_DE" , "de" , "DE");
insert into name_master values(3723 , 4 , "common_labels" , "Modellvariante" , "de_DE" , "de" , "DE");
insert into name_master values(3724 , 5 , "common_labels" , "Artikel Kennzeichnung" , "de_DE" , "de" , "DE");
insert into name_master values(3725 , 6 , "common_labels" , "Produkt Kennzeichnung" , "de_DE" , "de" , "DE");
insert into name_master values(3726 , 7 , "common_labels" , "Hersteller" , "de_DE" , "de" , "DE");
insert into name_master values(3727 , 8 , "common_labels" , "Hersteller Kennung" , "de_DE" , "de" , "DE");
insert into name_master values(3728 , 9 , "common_labels" , "Geräteklasse" , "de_DE" , "de" , "DE");
insert into name_master values(3729 , 10 , "common_labels" , "Geräteklassen Version" , "de_DE" , "de" , "DE");
insert into name_master values(3730 , 4 , "device_actions" , "Stopper" , "fr_CH" , "fr" , "CH");
insert into name_master values(3731 , 5 , "device_actions" , "Chauffer" , "fr_CH" , "fr" , "CH");
insert into name_master values(3732 , 6 , "device_actions" , "Bouillir et refroidir" , "fr_CH" , "fr" , "CH");
insert into name_master values(3733 , 3 , "device_events" , "Bouilloire mise en place" , "fr_CH" , "fr" , "CH");
insert into name_master values(3734 , 4 , "device_events" , "Chauffe démarrée" , "fr_CH" , "fr" , "CH");
insert into name_master values(3735 , 5 , "device_events" , "Maintien chaleur démarré" , "fr_CH" , "fr" , "CH");
insert into name_master values(3736 , 6 , "device_events" , "Chauffe terminée, refroidir à la température souhaitée" , "fr_CH" , "fr" , "CH");
insert into name_master values(3737 , 7 , "device_events" , "Chauffe terminée" , "fr_CH" , "fr" , "CH");
insert into name_master values(3738 , 8 , "device_events" , "Bouilloire retirée" , "fr_CH" , "fr" , "CH");
insert into name_master values(3739 , 9 , "device_events" , "Refroidissement terminé, température souhaitée atteinte" , "fr_CH" , "fr" , "CH");
insert into name_master values(3740 , 10 , "device_events" , "Maintien chaleur terminé" , "fr_CH" , "fr" , "CH");
insert into name_master values(3741 , 11 , "device_events" , "Chauffe interrompue, appui sur bouton" , "fr_CH" , "fr" , "CH");
insert into name_master values(3742 , 12 , "device_events" , "Refroidissement interrompu, appui sur bouton" , "fr_CH" , "fr" , "CH");
insert into name_master values(3743 , 13 , "device_events" , "Maintien chaleur interrompu, appui sur bouton" , "fr_CH" , "fr" , "CH");
insert into name_master values(3744 , 14 , "device_events" , "Chauffe terminée, maintien chaleur" , "fr_CH" , "fr" , "CH");
insert into name_master values(3745 , 15 , "device_events" , "Refroidissement terminé, maintien chaleur" , "fr_CH" , "fr" , "CH");
insert into name_master values(3746 , 16 , "device_events" , "Chauffe interrompue, bouilloire retirée" , "fr_CH" , "fr" , "CH");
insert into name_master values(3747 , 17 , "device_events" , "Refroidissement interrompu, bouilloire retirée" , "fr_CH" , "fr" , "CH");
insert into name_master values(3748 , 18 , "device_events" , "Maintien chaleur interrompu, bouiloire retirée" , "fr_CH" , "fr" , "CH");
insert into name_master values(3749 , 8 , "device_status" , "Mode de fonctionnement" , "fr_CH" , "fr" , "CH");
insert into name_master values(3750 , 5 , "device_actions_predefined" , "Bouillir et refroidir" , "fr_CH" , "fr" , "CH");
insert into name_master values(3751 , 6 , "device_actions_predefined" , "Chauffer" , "fr_CH" , "fr" , "CH");
insert into name_master values(3752 , 7 , "device_actions_predefined" , "Stopper" , "fr_CH" , "fr" , "CH");
insert into name_master values(3753 , 5 , "device_actions_parameter" , "Durée maintien chaleur" , "fr_CH" , "fr" , "CH");
insert into name_master values(3754 , 7 , "device_actions_parameter" , "Durée maintien chaleur" , "fr_CH" , "fr" , "CH");
insert into name_master values(3755 , 6 , "device_actions_parameter" , "Température de maintien en chaleur" , "fr_CH" , "fr" , "CH");
insert into name_master values(3756 , 8 , "device_actions_parameter" , "Température souhaitée" , "fr_CH" , "fr" , "CH");
insert into name_master values(3757 , 8 , "device_status_enum" , "Refroidir" , "fr_CH" , "fr" , "CH");
insert into name_master values(3758 , 9 , "device_status_enum" , "Chauffer" , "fr_CH" , "fr" , "CH");
insert into name_master values(3759 , 10 , "device_status_enum" , "Maintien chaleur" , "fr_CH" , "fr" , "CH");
insert into name_master values(3760 , 11 , "device_status_enum" , "Prêt" , "fr_CH" , "fr" , "CH");
insert into name_master values(3761 , 12 , "device_status_enum" , "Retiré" , "fr_CH" , "fr" , "CH");
insert into name_master values(3762 , 1 , "common_labels" , "Nom" , "fr_CH" , "fr" , "CH");
insert into name_master values(3763 , 2 , "common_labels" , "dS Device GTIN" , "fr_CH" , "fr" , "CH");
insert into name_master values(3764 , 3 , "common_labels" , "Nom du type d'appareil" , "fr_CH" , "fr" , "CH");
insert into name_master values(3765 , 4 , "common_labels" , "Variante de type d'appareil" , "fr_CH" , "fr" , "CH");
insert into name_master values(3766 , 5 , "common_labels" , "Dénomination de l'article" , "fr_CH" , "fr" , "CH");
insert into name_master values(3767 , 6 , "common_labels" , "Dénomination du produit" , "fr_CH" , "fr" , "CH");
insert into name_master values(3768 , 7 , "common_labels" , "Fabricant" , "fr_CH" , "fr" , "CH");
insert into name_master values(3769 , 8 , "common_labels" , "Identification du fabricant" , "fr_CH" , "fr" , "CH");
insert into name_master values(3770 , 9 , "common_labels" , "Classe d'appareil" , "fr_CH" , "fr" , "CH");
insert into name_master values(3771 , 10 , "common_labels" , "Version de la classe d'appareil" , "fr_CH" , "fr" , "CH");
insert into name_master values(3772 , 4 , "device_actions" , "Stop" , "en_US" , "en" , "US");
insert into name_master values(3773 , 5 , "device_actions" , "Boil" , "en_US" , "en" , "US");
insert into name_master values(3774 , 6 , "device_actions" , "Boil and cool down" , "en_US" , "en" , "US");
insert into name_master values(3775 , 3 , "device_events" , "Kettle attached" , "en_US" , "en" , "US");
insert into name_master values(3776 , 4 , "device_events" , "Boiling started" , "en_US" , "en" , "US");
insert into name_master values(3777 , 5 , "device_events" , "Keeping warm started" , "en_US" , "en" , "US");
insert into name_master values(3778 , 6 , "device_events" , "Boiling finished, cooling down to target temperature" , "en_US" , "en" , "US");
insert into name_master values(3779 , 7 , "device_events" , "Boiling finished" , "en_US" , "en" , "US");
insert into name_master values(3780 , 8 , "device_events" , "Kettle released" , "en_US" , "en" , "US");
insert into name_master values(3781 , 9 , "device_events" , "Cooling down finished, target temperature reached" , "en_US" , "en" , "US");
insert into name_master values(3782 , 10 , "device_events" , "Keeping warm ended" , "en_US" , "en" , "US");
insert into name_master values(3783 , 11 , "device_events" , "Boiling aborted, button pressed" , "en_US" , "en" , "US");
insert into name_master values(3784 , 12 , "device_events" , "Cooling down aborted, button pressed" , "en_US" , "en" , "US");
insert into name_master values(3785 , 13 , "device_events" , "Keeping warm aborted, button pressed" , "en_US" , "en" , "US");
insert into name_master values(3786 , 14 , "device_events" , "Boiling finished, keeping warm now" , "en_US" , "en" , "US");
insert into name_master values(3787 , 15 , "device_events" , "Cooling down finished, keeping warm now" , "en_US" , "en" , "US");
insert into name_master values(3788 , 16 , "device_events" , "Boiling aborted, kettle released" , "en_US" , "en" , "US");
insert into name_master values(3789 , 17 , "device_events" , "Cooling down aborted, kettle released" , "en_US" , "en" , "US");
insert into name_master values(3790 , 18 , "device_events" , "Keeping warm aborted, button pressed" , "en_US" , "en" , "US");
insert into name_master values(3791 , 8 , "device_status" , "Operation Mode" , "en_US" , "en" , "US");
insert into name_master values(3792 , 5 , "device_actions_predefined" , "Boil and cool down" , "en_US" , "en" , "US");
insert into name_master values(3793 , 6 , "device_actions_predefined" , "Boil" , "en_US" , "en" , "US");
insert into name_master values(3794 , 7 , "device_actions_predefined" , "Stopping" , "en_US" , "en" , "US");
insert into name_master values(3795 , 5 , "device_actions_parameter" , "Keep warm Duration" , "en_US" , "en" , "US");
insert into name_master values(3796 , 7 , "device_actions_parameter" , "Keep warm Duration" , "en_US" , "en" , "US");
insert into name_master values(3797 , 6 , "device_actions_parameter" , "Target Temperature" , "en_US" , "en" , "US");
insert into name_master values(3798 , 8 , "device_actions_parameter" , "Target Temperature" , "en_US" , "en" , "US");
insert into name_master values(3799 , 8 , "device_status_enum" , "Cooling down" , "en_US" , "en" , "US");
insert into name_master values(3800 , 9 , "device_status_enum" , "Heating up" , "en_US" , "en" , "US");
insert into name_master values(3801 , 10 , "device_status_enum" , "Keeping warm" , "en_US" , "en" , "US");
insert into name_master values(3802 , 11 , "device_status_enum" , "Ready" , "en_US" , "en" , "US");
insert into name_master values(3803 , 12 , "device_status_enum" , "Kettle released" , "en_US" , "en" , "US");
insert into name_master values(3804 , 1 , "common_labels" , "Name" , "en_US" , "en" , "US");
insert into name_master values(3805 , 2 , "common_labels" , "dS Device GTIN" , "en_US" , "en" , "US");
insert into name_master values(3806 , 3 , "common_labels" , "Model" , "en_US" , "en" , "US");
insert into name_master values(3807 , 4 , "common_labels" , "Model Version" , "en_US" , "en" , "US");
insert into name_master values(3808 , 5 , "common_labels" , "Article Identifier" , "en_US" , "en" , "US");
insert into name_master values(3809 , 6 , "common_labels" , "Product Id" , "en_US" , "en" , "US");
insert into name_master values(3810 , 7 , "common_labels" , "Vendor" , "en_US" , "en" , "US");
insert into name_master values(3811 , 8 , "common_labels" , "Vendor Id" , "en_US" , "en" , "US");
insert into name_master values(3812 , 9 , "common_labels" , "Device Class" , "en_US" , "en" , "US");
insert into name_master values(3813 , 10 , "common_labels" , "Device Class Version" , "en_US" , "en" , "US");
insert into name_master values(3814 , 1 , "device_actions" , "Back!" , "nl_NL" , "nl" , "NL");
insert into name_master values(3815 , 2 , "device_actions" , "Dampfen" , "nl_NL" , "nl" , "NL");
insert into name_master values(3816 , 3 , "device_actions" , "auuus" , "nl_NL" , "nl" , "NL");
insert into name_master values(3817 , 4 , "device_actions" , "Uitschakelen" , "nl_NL" , "nl" , "NL");
insert into name_master values(3818 , 5 , "device_actions" , "Verwarmen" , "nl_NL" , "nl" , "NL");
insert into name_master values(3819 , 6 , "device_actions" , "Koken en afkoelen" , "nl_NL" , "nl" , "NL");
insert into name_master values(3820 , 3 , "device_events" , "Waterkoker neergezet" , "nl_NL" , "nl" , "NL");
insert into name_master values(3821 , 4 , "device_events" , "Verwarmen begonnen" , "nl_NL" , "nl" , "NL");
insert into name_master values(3822 , 5 , "device_events" , "Warm houden begonnen" , "nl_NL" , "nl" , "NL");
insert into name_master values(3823 , 6 , "device_events" , "Verwarmen beëindigd, naar gewenste temperatuur afkoelen" , "nl_NL" , "nl" , "NL");
insert into name_master values(3824 , 7 , "device_events" , "Verwarmen beëindigd" , "nl_NL" , "nl" , "NL");
insert into name_master values(3825 , 8 , "device_events" , "Waterkoker opgetild" , "nl_NL" , "nl" , "NL");
insert into name_master values(3826 , 9 , "device_events" , "Afkoelen beëindigd, gewenste temperatuur bereikt" , "nl_NL" , "nl" , "NL");
insert into name_master values(3827 , 10 , "device_events" , "Warm houden beëindigd" , "nl_NL" , "nl" , "NL");
insert into name_master values(3828 , 11 , "device_events" , "Verwarmen afgebroken, op knop gedrukt" , "nl_NL" , "nl" , "NL");
insert into name_master values(3829 , 12 , "device_events" , "Afkoelen afgebroken, op knop gedrukt" , "nl_NL" , "nl" , "NL");
insert into name_master values(3830 , 13 , "device_events" , "warm houden afgebroken, op knop gedrukt" , "nl_NL" , "nl" , "NL");
insert into name_master values(3831 , 14 , "device_events" , "Verwarmen beëindigd, warm houden" , "nl_NL" , "nl" , "NL");
insert into name_master values(3832 , 15 , "device_events" , "Afkoelen beëindigd, warm houden" , "nl_NL" , "nl" , "NL");
insert into name_master values(3833 , 16 , "device_events" , "Verwarmen afgebroken, waterkoker opgetild" , "nl_NL" , "nl" , "NL");
insert into name_master values(3834 , 17 , "device_events" , "Afkoelen afgebroken, waterkoker opgetild" , "nl_NL" , "nl" , "NL");
insert into name_master values(3835 , 18 , "device_events" , "Warm houden afgebroken, waterkoker opgetild" , "nl_NL" , "nl" , "NL");
insert into name_master values(3836 , 8 , "device_status" , "Modus" , "nl_NL" , "nl" , "NL");
insert into name_master values(3837 , 5 , "device_actions_predefined" , "Koken en afkoelen" , "nl_NL" , "nl" , "NL");
insert into name_master values(3838 , 6 , "device_actions_predefined" , "Verwarmen" , "nl_NL" , "nl" , "NL");
insert into name_master values(3839 , 7 , "device_actions_predefined" , "Uitschakelen" , "nl_NL" , "nl" , "NL");
insert into name_master values(3840 , 5 , "device_actions_parameter" , "Warmhoudtijdsduur" , "nl_NL" , "nl" , "NL");
insert into name_master values(3841 , 7 , "device_actions_parameter" , "Warmhoudtijdsduur" , "nl_NL" , "nl" , "NL");
insert into name_master values(3842 , 6 , "device_actions_parameter" , "Warmhoudtemperatuur" , "nl_NL" , "nl" , "NL");
insert into name_master values(3843 , 8 , "device_actions_parameter" , "Gewenste temperatuur" , "nl_NL" , "nl" , "NL");
insert into name_master values(3844 , 8 , "device_status_enum" , "Afkoelen" , "nl_NL" , "nl" , "NL");
insert into name_master values(3845 , 9 , "device_status_enum" , "Verwarmen" , "nl_NL" , "nl" , "NL");
insert into name_master values(3846 , 10 , "device_status_enum" , "Warm houden" , "nl_NL" , "nl" , "NL");
insert into name_master values(3847 , 11 , "device_status_enum" , "Gereed" , "nl_NL" , "nl" , "NL");
insert into name_master values(3848 , 12 , "device_status_enum" , "Opgetild" , "nl_NL" , "nl" , "NL");
insert into name_master values(3849 , 1 , "common_labels" , "Naam" , "nl_NL" , "nl" , "NL");
insert into name_master values(3850 , 2 , "common_labels" , "dS apparaat GTIN" , "nl_NL" , "nl" , "NL");
insert into name_master values(3851 , 3 , "common_labels" , "Model" , "nl_NL" , "nl" , "NL");
insert into name_master values(3852 , 4 , "common_labels" , "Model variant" , "nl_NL" , "nl" , "NL");
insert into name_master values(3853 , 5 , "common_labels" , "Artikel aanduiding" , "nl_NL" , "nl" , "NL");
insert into name_master values(3854 , 6 , "common_labels" , "Product aanduiding" , "nl_NL" , "nl" , "NL");
insert into name_master values(3855 , 7 , "common_labels" , "Fabrikant" , "nl_NL" , "nl" , "NL");
insert into name_master values(3856 , 8 , "common_labels" , "Fabrikant aanduiding" , "nl_NL" , "nl" , "NL");
insert into name_master values(3857 , 9 , "common_labels" , "Apparaatcategorie" , "nl_NL" , "nl" , "NL");
insert into name_master values(3858 , 10 , "common_labels" , "Apparaatcategorie versie" , "nl_NL" , "nl" , "NL");
insert into name_master values(3859 , 4 , "device_actions" , "Kapat" , "tr_TR" , "tr" , "TR");
insert into name_master values(3860 , 5 , "device_actions" , "Isıt" , "tr_TR" , "tr" , "TR");
insert into name_master values(3861 , 6 , "device_actions" , "Kaynat ve soğut" , "tr_TR" , "tr" , "TR");
insert into name_master values(3862 , 3 , "device_events" , "Su ısıtıcısı yerleştirildi" , "tr_TR" , "tr" , "TR");
insert into name_master values(3863 , 4 , "device_events" , "Isıtma başlatıldı" , "tr_TR" , "tr" , "TR");
insert into name_master values(3864 , 5 , "device_events" , "Sıcak tutma başlatıldı" , "tr_TR" , "tr" , "TR");
insert into name_master values(3865 , 6 , "device_events" , "Isıtma işlemi bitti, istenilen sıcaklık için soğutuluyor" , "tr_TR" , "tr" , "TR");
insert into name_master values(3866 , 7 , "device_events" , "Isıtma işlemi bitti" , "tr_TR" , "tr" , "TR");
insert into name_master values(3867 , 8 , "device_events" , "Su ısıtıcısı kaldırıldı" , "tr_TR" , "tr" , "TR");
insert into name_master values(3868 , 9 , "device_events" , "Soğutma işlemi bitti, istenilen sıcaklığa ulaşıldı" , "tr_TR" , "tr" , "TR");
insert into name_master values(3869 , 10 , "device_events" , "Sıcak tutma işlemi sonlandırıldı" , "tr_TR" , "tr" , "TR");
insert into name_master values(3870 , 11 , "device_events" , "Isıtma işlemi iptal edildi, cihaz üzerindeki butona basıldı" , "tr_TR" , "tr" , "TR");
insert into name_master values(3871 , 12 , "device_events" , "Soğutma işlemi iptal edildi, cihaz üzerindeki butona basıldı" , "tr_TR" , "tr" , "TR");
insert into name_master values(3872 , 13 , "device_events" , "Sıcak tutma işlemi iptal edildi, cihaz üzerindeki butona basıldı" , "tr_TR" , "tr" , "TR");
insert into name_master values(3873 , 14 , "device_events" , "Isıtma işlemi bitti, sıcak tutuluyor" , "tr_TR" , "tr" , "TR");
insert into name_master values(3874 , 15 , "device_events" , "Soğutma işlemi bitti, sıcak tutuluyor" , "tr_TR" , "tr" , "TR");
insert into name_master values(3875 , 16 , "device_events" , "Isıtma işlemi iptal edildi, ısıtıcı kaldırıldı" , "tr_TR" , "tr" , "TR");
insert into name_master values(3876 , 17 , "device_events" , "Soğutma işlemi iptal edildi, ısıtıcı kaldırıldı" , "tr_TR" , "tr" , "TR");
insert into name_master values(3877 , 18 , "device_events" , "Sıcak tutma işlemi iptal edildi, ısıtıcı kaldırıldı" , "tr_TR" , "tr" , "TR");
insert into name_master values(3878 , 8 , "device_status" , "Çalışma modu" , "tr_TR" , "tr" , "TR");
insert into name_master values(3879 , 5 , "device_actions_predefined" , "Kaynat ve soğut" , "tr_TR" , "tr" , "TR");
insert into name_master values(3880 , 6 , "device_actions_predefined" , "Isıt" , "tr_TR" , "tr" , "TR");
insert into name_master values(3881 , 7 , "device_actions_predefined" , "Kapat" , "tr_TR" , "tr" , "TR");
insert into name_master values(3882 , 5 , "device_actions_parameter" , "Sıcak tutma süresi" , "tr_TR" , "tr" , "TR");
insert into name_master values(3883 , 7 , "device_actions_parameter" , "Sıcak tutma süresi" , "tr_TR" , "tr" , "TR");
insert into name_master values(3884 , 6 , "device_actions_parameter" , "Sıcak tutma derecesi" , "tr_TR" , "tr" , "TR");
insert into name_master values(3885 , 8 , "device_actions_parameter" , "İstenilen sıcaklık" , "tr_TR" , "tr" , "TR");
insert into name_master values(3886 , 8 , "device_status_enum" , "Soğut" , "tr_TR" , "tr" , "TR");
insert into name_master values(3887 , 9 , "device_status_enum" , "Isıt" , "tr_TR" , "tr" , "TR");
insert into name_master values(3888 , 10 , "device_status_enum" , "Sıcak tut" , "tr_TR" , "tr" , "TR");
insert into name_master values(3889 , 11 , "device_status_enum" , "Hazır" , "tr_TR" , "tr" , "TR");
insert into name_master values(3890 , 12 , "device_status_enum" , "Kaldırıldı" , "tr_TR" , "tr" , "TR");
insert into name_master values(3891 , 1 , "common_labels" , "İsim" , "tr_TR" , "tr" , "TR");
insert into name_master values(3892 , 2 , "common_labels" , "dS Device GTIN" , "tr_TR" , "tr" , "TR");
insert into name_master values(3893 , 3 , "common_labels" , "Model" , "tr_TR" , "tr" , "TR");
insert into name_master values(3894 , 4 , "common_labels" , "Model sürümü" , "tr_TR" , "tr" , "TR");
insert into name_master values(3895 , 5 , "common_labels" , "Ürün tanımı" , "tr_TR" , "tr" , "TR");
insert into name_master values(3896 , 6 , "common_labels" , "Cihaz tanımı" , "tr_TR" , "tr" , "TR");
insert into name_master values(3897 , 7 , "common_labels" , "Üretici firma" , "tr_TR" , "tr" , "TR");
insert into name_master values(3898 , 8 , "common_labels" , "Üretici tanımı" , "tr_TR" , "tr" , "TR");
insert into name_master values(3899 , 9 , "common_labels" , "Cihaz sınıfı" , "tr_TR" , "tr" , "TR");
insert into name_master values(3900 , 10 , "common_labels" , "Cihaz sınıf sürümü" , "tr_TR" , "tr" , "TR");
insert into name_master values(3901 , 4 , "device_actions" , "Spegnere" , "it_IT" , "it" , "IT");
insert into name_master values(3902 , 5 , "device_actions" , "Riscaldare" , "it_IT" , "it" , "IT");
insert into name_master values(3903 , 6 , "device_actions" , "Bollire e raffreddare" , "it_IT" , "it" , "IT");
insert into name_master values(3904 , 3 , "device_events" , "Bollitore collegato" , "it_IT" , "it" , "IT");
insert into name_master values(3905 , 4 , "device_events" , "Riscaldamento avviato " , "it_IT" , "it" , "IT");
insert into name_master values(3906 , 5 , "device_events" , "Riscaldatore avviato" , "it_IT" , "it" , "IT");
insert into name_master values(3907 , 6 , "device_events" , "Riscaldamento terminato, raffreddamento alla temperatura desiderata." , "it_IT" , "it" , "IT");
insert into name_master values(3908 , 7 , "device_events" , "Riscaldamento terminato" , "it_IT" , "it" , "IT");
insert into name_master values(3909 , 8 , "device_events" , "Bollitore rimosso" , "it_IT" , "it" , "IT");
insert into name_master values(3910 , 9 , "device_events" , "Raffreddamento terminato, temperatura desiderata raggiunta." , "it_IT" , "it" , "IT");
insert into name_master values(3911 , 10 , "device_events" , "Mantenimento in temperatura terminato" , "it_IT" , "it" , "IT");
insert into name_master values(3912 , 11 , "device_events" , "Riscaldamento interrotto tramite tasto manuale" , "it_IT" , "it" , "IT");
insert into name_master values(3913 , 12 , "device_events" , "Raffreddamento interrotto tramite tasto manuale" , "it_IT" , "it" , "IT");
insert into name_master values(3914 , 13 , "device_events" , "Tenuta in temperatura interrotto tramite tasto manuale" , "it_IT" , "it" , "IT");
insert into name_master values(3915 , 14 , "device_events" , "Riscaldamento terminato, mantenimento in temperatura" , "it_IT" , "it" , "IT");
insert into name_master values(3916 , 15 , "device_events" , "Raffreddamento terminato, mantenimento in temperatura" , "it_IT" , "it" , "IT");
insert into name_master values(3917 , 16 , "device_events" , "Riscaldamento interrotto, rimozione bollitore" , "it_IT" , "it" , "IT");
insert into name_master values(3918 , 17 , "device_events" , "Raffreddamento interrotto, rimozione bollitore" , "it_IT" , "it" , "IT");
insert into name_master values(3919 , 18 , "device_events" , "mantenimento in temperatura interrotto, rimozione bollitore" , "it_IT" , "it" , "IT");
insert into name_master values(3920 , 8 , "device_status" , "Modalità operativa" , "it_IT" , "it" , "IT");
insert into name_master values(3921 , 5 , "device_actions_predefined" , "Bollire e raffreddare" , "it_IT" , "it" , "IT");
insert into name_master values(3922 , 6 , "device_actions_predefined" , "Riscaldare" , "it_IT" , "it" , "IT");
insert into name_master values(3923 , 7 , "device_actions_predefined" , "Spegnere" , "it_IT" , "it" , "IT");
insert into name_master values(3924 , 5 , "device_actions_parameter" , "Tempo mantieni caldo" , "it_IT" , "it" , "IT");
insert into name_master values(3925 , 7 , "device_actions_parameter" , "Tempo mantieni caldo" , "it_IT" , "it" , "IT");
insert into name_master values(3926 , 6 , "device_actions_parameter" , "Temperatura mantieni caldo" , "it_IT" , "it" , "IT");
insert into name_master values(3927 , 8 , "device_actions_parameter" , "Temperatura desiderata" , "it_IT" , "it" , "IT");
insert into name_master values(3928 , 8 , "device_status_enum" , "Raffreddamento" , "it_IT" , "it" , "IT");
insert into name_master values(3929 , 9 , "device_status_enum" , "Riscaldamento" , "it_IT" , "it" , "IT");
insert into name_master values(3930 , 10 , "device_status_enum" , "Mantenere in temperatura" , "it_IT" , "it" , "IT");
insert into name_master values(3931 , 11 , "device_status_enum" , "Pronto" , "it_IT" , "it" , "IT");
insert into name_master values(3932 , 12 , "device_status_enum" , "Rimosso" , "it_IT" , "it" , "IT");
insert into name_master values(3933 , 1 , "common_labels" , "Nome" , "it_IT" , "it" , "IT");
insert into name_master values(3934 , 2 , "common_labels" , "dS Device GTIN" , "it_IT" , "it" , "IT");
insert into name_master values(3935 , 3 , "common_labels" , "Modello" , "it_IT" , "it" , "IT");
insert into name_master values(3936 , 4 , "common_labels" , "Versione modello" , "it_IT" , "it" , "IT");
insert into name_master values(3937 , 5 , "common_labels" , "Identificatore articolo" , "it_IT" , "it" , "IT");
insert into name_master values(3938 , 6 , "common_labels" , "Id prodotto" , "it_IT" , "it" , "IT");
insert into name_master values(3939 , 7 , "common_labels" , "Costruttore" , "it_IT" , "it" , "IT");
insert into name_master values(3940 , 8 , "common_labels" , "Id costruttore" , "it_IT" , "it" , "IT");
insert into name_master values(3941 , 9 , "common_labels" , "Classe apparecchio" , "it_IT" , "it" , "IT");
insert into name_master values(3942 , 10 , "common_labels" , "Versione classe prodotto" , "it_IT" , "it" , "IT");
insert into name_master values(3960 , 22 , "device_events" , "Nochnevent" , "base" , "" , "");
insert into name_master values(3961 , 10 , "device_status" , "operation" , "base" , "" , "");
insert into name_master values(3962 , 17 , "device_status_enum" , "Inactive" , "base" , "" , "");
insert into name_master values(3963 , 18 , "device_status_enum" , "Ready" , "base" , "" , "");
insert into name_master values(3964 , 19 , "device_status_enum" , "Run" , "base" , "" , "");
insert into name_master values(3965 , 20 , "device_status_enum" , "ActionRequired" , "base" , "" , "");
insert into name_master values(3966 , 21 , "device_status_enum" , "Finished" , "base" , "" , "");
insert into name_master values(3967 , 22 , "device_status_enum" , "Error" , "base" , "" , "");
insert into name_master values(3968 , 23 , "device_status_enum" , "Aborting" , "base" , "" , "");
insert into name_master values(3969 , 9 , "device_actions" , "espresso" , "base" , "" , "");
insert into name_master values(3970 , 10 , "device_actions_predefined" , "std.espresso" , "base" , "" , "");
insert into name_master values(3976 , 10 , "device_actions" , "stop" , "base" , "" , "");
insert into name_master values(3977 , 16 , "device_actions_predefined" , "std.stop" , "base" , "" , "");
insert into name_master values(3978 , 11 , "device_status" , "operation" , "base" , "" , "");
insert into name_master values(3979 , 24 , "device_status_enum" , "Inactive" , "base" , "" , "");
insert into name_master values(3980 , 25 , "device_status_enum" , "Ready" , "base" , "" , "");
insert into name_master values(3981 , 26 , "device_status_enum" , "Run" , "base" , "" , "");
insert into name_master values(3982 , 27 , "device_status_enum" , "ActionRequired" , "base" , "" , "");
insert into name_master values(3983 , 28 , "device_status_enum" , "Finished" , "base" , "" , "");
insert into name_master values(3984 , 29 , "device_status_enum" , "Error" , "base" , "" , "");
insert into name_master values(3985 , 30 , "device_status_enum" , "Aborting" , "base" , "" , "");
insert into name_master values(3986 , 11 , "device_actions" , "espresso" , "base" , "" , "");
insert into name_master values(3987 , 12 , "device_actions" , "stop" , "base" , "" , "");
insert into name_master values(3988 , 17 , "device_actions_predefined" , "std.stop" , "base" , "" , "");
insert into name_master values(3989 , 18 , "device_actions_predefined" , "std.espresso" , "base" , "" , "");
insert into name_master values(3995 , 13 , "device_actions" , "activity" , "base" , "" , "");
insert into name_master values(3996 , 11 , "device_actions_parameter" , "activity" , "base" , "" , "");
insert into name_master values(3997 , 24 , "device_actions_predefined" , "std.startactivity" , "base" , "" , "");
insert into name_master values(3998 , 25 , "device_actions_predefined" , "std.endactivity" , "base" , "" , "");
insert into name_master values(3999 , 14 , "device_actions" , "stop" , "base" , "" , "");
insert into name_master values(4000 , 26 , "device_actions_predefined" , "std.stop" , "base" , "" , "");
insert into name_master values(4001 , 15 , "device_actions" , "espressoMacchiato" , "base" , "" , "");
insert into name_master values(4002 , 16 , "device_actions" , "coffee" , "base" , "" , "");
insert into name_master values(4003 , 17 , "device_actions" , "cappuccino" , "base" , "" , "");
insert into name_master values(4004 , 18 , "device_actions" , "latteMacchiato" , "base" , "" , "");
insert into name_master values(4005 , 19 , "device_actions" , "caffeLatte" , "base" , "" , "");
insert into name_master values(4006 , 27 , "device_actions_predefined" , "std.espressoMacchiato" , "base" , "" , "");
insert into name_master values(4007 , 28 , "device_actions_predefined" , "std.coffee" , "base" , "" , "");
insert into name_master values(4008 , 29 , "device_actions_predefined" , "std.cappuccino" , "base" , "" , "");
insert into name_master values(4009 , 30 , "device_actions_predefined" , "std.latteMacchiato" , "base" , "" , "");
insert into name_master values(4010 , 31 , "device_actions_predefined" , "std.caffeLatte" , "base" , "" , "");
insert into name_master values(4011 , 12 , "device_actions_parameter" , "temperatureLevel" , "base" , "" , "");
insert into name_master values(4013 , 13 , "device_actions_parameter" , "temperatureLevel" , "base" , "" , "");
insert into name_master values(4014 , 14 , "device_actions_parameter" , "temperatureLevel" , "base" , "" , "");
insert into name_master values(4015 , 15 , "device_actions_parameter" , "temperatureLevel" , "base" , "" , "");
insert into name_master values(4016 , 16 , "device_actions_parameter" , "temperatureLevel" , "base" , "" , "");
insert into name_master values(4017 , 17 , "device_actions_parameter" , "temperatureLevel" , "base" , "" , "");
insert into name_master values(4018 , 18 , "device_actions_parameter" , "beanAmount" , "base" , "" , "");
insert into name_master values(4019 , 19 , "device_actions_parameter" , "beanAmount" , "base" , "" , "");
insert into name_master values(4020 , 20 , "device_actions_parameter" , "beanAmount" , "base" , "" , "");
insert into name_master values(4021 , 21 , "device_actions_parameter" , "beanAmount" , "base" , "" , "");
insert into name_master values(4022 , 22 , "device_actions_parameter" , "beanAmount" , "base" , "" , "");
insert into name_master values(4023 , 23 , "device_actions_parameter" , "beanAmount" , "base" , "" , "");
insert into name_master values(4024 , 24 , "device_actions_parameter" , "fillQuantity" , "base" , "" , "");
insert into name_master values(4025 , 25 , "device_actions_parameter" , "fillQuantity" , "base" , "" , "");
insert into name_master values(4026 , 26 , "device_actions_parameter" , "fillQuantity" , "base" , "" , "");
insert into name_master values(4027 , 27 , "device_actions_parameter" , "fillQuantity" , "base" , "" , "");
insert into name_master values(4028 , 28 , "device_actions_parameter" , "fillQuantity" , "base" , "" , "");
insert into name_master values(4029 , 29 , "device_actions_parameter" , "fillQuantity" , "base" , "" , "");
insert into name_master values(4030 , 30 , "device_actions_parameter" , "temperatureLevel" , "base" , "" , "");
insert into name_master values(4031 , 31 , "device_actions_parameter" , "beanAmount" , "base" , "" , "");
insert into name_master values(4032 , 32 , "device_actions_parameter" , "fillQuantity" , "base" , "" , "");
insert into name_master values(4033 , 20 , "device_actions" , "espressoMacchiato" , "base" , "" , "");
insert into name_master values(4034 , 33 , "device_actions_parameter" , "temperatureLevel" , "base" , "" , "");
insert into name_master values(4035 , 34 , "device_actions_parameter" , "beanAmount" , "base" , "" , "");
insert into name_master values(4036 , 35 , "device_actions_parameter" , "fillQuantity" , "base" , "" , "");
insert into name_master values(4037 , 32 , "device_actions_predefined" , "std.espressoMacchiato" , "base" , "" , "");
insert into name_master values(4038 , 21 , "device_actions" , "coffee" , "base" , "" , "");
insert into name_master values(4039 , 36 , "device_actions_parameter" , "temperatureLevel" , "base" , "" , "");
insert into name_master values(4040 , 37 , "device_actions_parameter" , "beanAmount" , "base" , "" , "");
insert into name_master values(4041 , 38 , "device_actions_parameter" , "fillQuantity" , "base" , "" , "");
insert into name_master values(4042 , 33 , "device_actions_predefined" , "std.coffee" , "base" , "" , "");
insert into name_master values(4043 , 22 , "device_actions" , "cappuccino" , "base" , "" , "");
insert into name_master values(4044 , 39 , "device_actions_parameter" , "temperatureLevel" , "base" , "" , "");
insert into name_master values(4045 , 40 , "device_actions_parameter" , "beanAmount" , "base" , "" , "");
insert into name_master values(4046 , 41 , "device_actions_parameter" , "fillQuantity" , "base" , "" , "");
insert into name_master values(4047 , 34 , "device_actions_predefined" , "std.cappuccino" , "base" , "" , "");
insert into name_master values(4048 , 23 , "device_actions" , "latteMacchiato" , "base" , "" , "");
insert into name_master values(4049 , 42 , "device_actions_parameter" , "temperatureLevel" , "base" , "" , "");
insert into name_master values(4050 , 43 , "device_actions_parameter" , "beanAmount" , "base" , "" , "");
insert into name_master values(4051 , 44 , "device_actions_parameter" , "fillQuantity" , "base" , "" , "");
insert into name_master values(4052 , 35 , "device_actions_predefined" , "std.latteMacchiato" , "base" , "" , "");
insert into name_master values(4053 , 24 , "device_actions" , "caffeLatte" , "base" , "" , "");
insert into name_master values(4054 , 45 , "device_actions_parameter" , "temperatureLevel" , "base" , "" , "");
insert into name_master values(4055 , 46 , "device_actions_parameter" , "beanAmount" , "base" , "" , "");
insert into name_master values(4056 , 47 , "device_actions_parameter" , "fillQuantity" , "base" , "" , "");
insert into name_master values(4057 , 36 , "device_actions_predefined" , "std.caffeLatte" , "base" , "" , "");
insert into name_master values(4058 , 25 , "device_actions" , "temperature" , "base" , "" , "");
insert into name_master values(4059 , 48 , "device_actions_parameter" , "temperature" , "base" , "" , "");
insert into name_master values(4060 , 37 , "device_actions_predefined" , "std.temperature" , "base" , "" , "");
insert into name_master values(4061 , 26 , "device_actions" , "flow" , "base" , "" , "");
insert into name_master values(4062 , 38 , "device_actions_predefined" , "std.flow" , "base" , "" , "");
insert into name_master values(4063 , 49 , "device_actions_parameter" , "flow" , "base" , "" , "");
insert into name_master values(4064 , 27 , "device_actions" , "on" , "base" , "" , "");
insert into name_master values(4065 , 28 , "device_actions" , "off" , "base" , "" , "");
insert into name_master values(4066 , 29 , "device_actions" , "dose" , "base" , "" , "");
insert into name_master values(4067 , 30 , "device_actions" , "fill" , "base" , "" , "");
insert into name_master values(4068 , 31 , "device_actions" , "alloff" , "base" , "" , "");
insert into name_master values(4069 , 32 , "device_actions" , "stop" , "base" , "" , "");
insert into name_master values(4070 , 39 , "device_actions_predefined" , "std.alloff" , "base" , "" , "");
insert into name_master values(4071 , 40 , "device_actions_predefined" , "std.stop" , "base" , "" , "");
insert into name_master values(4072 , 50 , "device_actions_parameter" , "functionid" , "base" , "" , "");
insert into name_master values(4073 , 51 , "device_actions_parameter" , "amount" , "base" , "" , "");
insert into name_master values(4074 , 52 , "device_actions_parameter" , "temperature" , "base" , "" , "");
insert into name_master values(4075 , 41 , "device_actions_predefined" , "std.fill" , "base" , "" , "");
insert into name_master values(4076 , 53 , "device_actions_parameter" , "functionid" , "base" , "" , "");
insert into name_master values(4077 , 54 , "device_actions_parameter" , "amount" , "base" , "" , "");
insert into name_master values(4078 , 42 , "device_actions_predefined" , "std.dose" , "base" , "" , "");
insert into name_master values(4079 , 55 , "device_actions_parameter" , "functionid" , "base" , "" , "");
insert into name_master values(4080 , 43 , "device_actions_predefined" , "std.off" , "base" , "" , "");
insert into name_master values(4081 , 56 , "device_actions_parameter" , "functionid" , "base" , "" , "");
insert into name_master values(4082 , 44 , "device_actions_predefined" , "std.on" , "base" , "" , "");
drop table if exists "device_status";
create table "device_status"(id primary key,device_id,name,tags);
insert into device_status values(5 , 12 , "operationMode" , "overview");
insert into device_status values(6 , 12 , "fan" , "");
insert into device_status values(7 , 12 , "timer" , "");
insert into device_status values(8 , 18 , "operation" , "overview");
insert into device_status values(9 , 11 , "dummyState" , "overview");
insert into device_status values(10 , 26 , "operation" , "");
insert into device_status values(11 , 27 , "operation" , "");
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
insert into device_status_enum values(13 , 9 , "d");
insert into device_status_enum values(14 , 9 , "u");
insert into device_status_enum values(15 , 9 , "mm");
insert into device_status_enum values(16 , 9 , "y");
insert into device_status_enum values(17 , 10 , "Inactive");
insert into device_status_enum values(18 , 10 , "Ready");
insert into device_status_enum values(19 , 10 , "Run");
insert into device_status_enum values(20 , 10 , "ActionRequired");
insert into device_status_enum values(21 , 10 , "Finished");
insert into device_status_enum values(22 , 10 , "Error");
insert into device_status_enum values(23 , 10 , "Aborting");
insert into device_status_enum values(24 , 11 , "Inactive");
insert into device_status_enum values(25 , 11 , "Ready");
insert into device_status_enum values(26 , 11 , "Run");
insert into device_status_enum values(27 , 11 , "ActionRequired");
insert into device_status_enum values(28 , 11 , "Finished");
insert into device_status_enum values(29 , 11 , "Error");
insert into device_status_enum values(30 , 11 , "Aborting");
drop table if exists "device_labels";
create table "device_labels" (id primary key,device_id,name,tags);
insert into device_labels values(1 , 18 , "notes" , "overview:4");
insert into device_labels values(3 , 11 , "dummyNode3" , "overview");
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
insert into device_properties values(7 , 18 , "defaultcooldowntemperature" , 1 , "0" , "100" , "80" , "1" , "celsius" , "invisible" );
insert into device_properties values(8 , 18 , "defaultkeepwarmtime" , 2 , "0" , "30" , "15" , "1" , "min" , "settings" );
insert into device_properties values(9 , 11 , "dummyProperty" , 4 , "" , "" , "" , "" , "" , "settings" );
insert into device_properties values(10 , 25 , "outerRingWaterSession" , 4 , "" , "" , "0" , "1" , "liter" , "" );
insert into device_properties values(11 , 25 , "innerRingWaterSession" , 4 , "" , "" , "0" , "1" , "liter" , "" );
insert into device_properties values(12 , 25 , "outerRingWaterCounter" , 4 , "" , "" , "0" , "1" , "liter" , "" );
insert into device_properties values(13 , 25 , "innerRingWaterCounter" , 4 , "" , "" , "0" , "1" , "liter" , "" );
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
insert into name_device_properties values(35 , 6 , "en_US" , "Temperature Boil {value}" , "Temperature Boil" , "" , "0" , "en" , "US");
insert into name_device_properties values(36 , 7 , "en_US" , "Temperature Boil and Cool Down {value}" , "Temperature Boil and Cool Down" , "" , "0" , "en" , "US");
insert into name_device_properties values(37 , 8 , "en_US" , "Keep warm Time {value}" , "Keep warm Time" , "" , "0" , "en" , "US");
insert into name_device_properties values(38 , 9 , "base" , "dummy asdvalue" , "dummy asdvalueasd" , "asd" , "1" , "" , "");
insert into name_device_properties values(39 , 4 , "nl_NL" , "Temperatuur {value} graden Celsius" , "Watertemperatuur" , "&deg; C" , "0" , "nl" , "NL");
insert into name_device_properties values(40 , 5 , "nl_NL" , "Water niveau {value} &#37;" , "Water niveau" , "&#37;" , "0" , "nl" , "NL");
insert into name_device_properties values(41 , 6 , "nl_NL" , "Temperatuur verwarmen {value}" , "Temperatuur verwarmen" , "" , "0" , "nl" , "NL");
insert into name_device_properties values(42 , 7 , "nl_NL" , "Temperatuur koken en afkoelen {value}" , "Temperatuur koken en afkoelen" , "" , "0" , "nl" , "NL");
insert into name_device_properties values(43 , 8 , "nl_NL" , "Warmhoudtijd {value}" , "Warmhoudtijd" , "" , "0" , "nl" , "NL");
insert into name_device_properties values(44 , 4 , "tr_TR" , "Sıcaklık {value} derece" , "Su sıcaklığı" , "&deg; C" , "0" , "tr" , "TR");
insert into name_device_properties values(45 , 5 , "tr_TR" , "Doluluk oranı {value} &#37;" , "Doluluk oranı" , "&#37;" , "0" , "tr" , "TR");
insert into name_device_properties values(46 , 6 , "tr_TR" , "Sıcaklığı yükselt {value}" , "Sıcaklığı yükselt" , "" , "0" , "tr" , "TR");
insert into name_device_properties values(47 , 7 , "tr_TR" , "Kaynatma ve Soğutma sıcaklığı {value}" , "Kaynatma ve Soğutma sıcaklığı" , "" , "0" , "tr" , "TR");
insert into name_device_properties values(48 , 8 , "tr_TR" , "Sıcak tutma süresi {value}" , "Sıcak tutma süresi" , "" , "0" , "tr" , "TR");
insert into name_device_properties values(49 , 4 , "it_IT" , "Temperatura {value} gradi Celsius" , "Temperatura acqua" , "&deg; C" , "0" , "it" , "IT");
insert into name_device_properties values(50 , 5 , "it_IT" , "Livello acqua {value} &#37;" , "Livello acqua" , "&#37:" , "0" , "it" , "IT");
insert into name_device_properties values(51 , 6 , "it_IT" , "Temperatura riscaldamento  {value}" , "Temperatura riscaldamento" , "" , "0" , "it" , "IT");
insert into name_device_properties values(52 , 7 , "it_IT" , "Temperatura bollitura e raffreddamento {value}" , "Temperatura bollitura e raffreddamento" , "" , "0" , "it" , "IT");
insert into name_device_properties values(53 , 8 , "it_IT" , "Tempo mantenimento in temperatura  {value}" , "Tempo mantenimento in temperatura" , "" , "0" , "it" , "IT");
insert into name_device_properties values(54 , 4 , "fr_CH" , "Température {value} degrés Celsius" , "Température de l'eau" , "&deg; C" , "0" , "fr" , "CH");
insert into name_device_properties values(55 , 5 , "fr_CH" , "Niveau de remplissage {value} &#37;" , "Niveau de remplissage" , "&#37;" , "0" , "fr" , "CH");
insert into name_device_properties values(56 , 6 , "fr_CH" , "Température Chauffe {value}" , "Température Chauffe" , "" , "0" , "fr" , "CH");
insert into name_device_properties values(57 , 7 , "fr_CH" , "keepwarm temperature {value}" , "Température bouillir et refroidir" , "" , "0" , "fr" , "CH");
insert into name_device_properties values(58 , 10 , "base" , "" , "outer ring consumption in this session" , "" , "0" , "" , "");
insert into name_device_properties values(59 , 11 , "base" , "" , "inner ring consumption in this session" , "" , "0" , "" , "");
insert into name_device_properties values(60 , 12 , "base" , "" , "outer ring consumption overall" , "" , "0" , "" , "");
insert into name_device_properties values(61 , 13 , "base" , "" , "inner ring consumption overall" , "" , "0" , "" , "");
drop table if exists "name_device_labels";
create table "name_device_labels" (id primary key,reference_id,lang_code,title,value,lang,country);
insert into name_device_labels values(2 , 1 , "en_US" , " Notes" , "Please check with the 'Smarter' smartphone app if the iKettle firmware is up-to-date!" , "en" , "US");
insert into name_device_labels values(3 , 1 , "base" , "notes" , "updatechecknotice" , "" , "");
insert into name_device_labels values(4 , 1 , "de_DE" , "Bemerkungen" , "Bitte prüfen Sie mit der 'Smarter' Smartphone App, ob die iKettle Firmware auf dem aktuellsten Stand ist!" , "de" , "DE");
insert into name_device_labels values(5 , 1 , "nl_NL" , "Opmerkingen" , "Controleer met de 'Smarter' smartphone app of de iKettle firmware de nieuwste versie is!" , "nl" , "NL");
insert into name_device_labels values(6 , 1 , "tr_TR" , "Açıklamalar" , "Lütfen akıllı telefonunuz üzerindeki 'Smarter' uygulaması üzerinden, IKettle cihazınızın firmware sürümünün güncel olup olmadığını kontrol edin!" , "tr" , "TR");
insert into name_device_labels values(7 , 1 , "it_IT" , "Note" , "Prego controllare con la Smartphone App 'Smarter' che il Firmware  dell'iKettle sia attuale!" , "it" , "IT");
insert into name_device_labels values(8 , 3 , "base" , "dummy title2" , "dummy value223" , "" , "");
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
insert into device_events values(19 , 11 , "dummyEventAlphaer");
insert into device_events values(20 , 11 , "dummyEventOther");
insert into device_events values(21 , 11 , "dummyEventUI2");
insert into device_events values(22 , 11 , "Nochnevent");
drop table if exists "device_actions";
create table "device_actions"(id primary key,device_id,command);
insert into device_actions values(1 , 12 , "bake");
insert into device_actions values(2 , 12 , "steam");
insert into device_actions values(3 , 12 , "stop");
insert into device_actions values(4 , 18 , "stop");
insert into device_actions values(5 , 18 , "heat");
insert into device_actions values(6 , 18 , "boilandcooldown");
insert into device_actions values(7 , 11 , "dummyAction1");
insert into device_actions values(8 , 11 , "dummyAction2");
insert into device_actions values(9 , 26 , "espresso");
insert into device_actions values(10 , 26 , "stop");
insert into device_actions values(11 , 27 , "espresso");
insert into device_actions values(12 , 27 , "stop");
insert into device_actions values(13 , 24 , "activity");
insert into device_actions values(14 , 24 , "stop");
insert into device_actions values(15 , 26 , "espressoMacchiato");
insert into device_actions values(16 , 26 , "coffee");
insert into device_actions values(17 , 26 , "cappuccino");
insert into device_actions values(18 , 26 , "latteMacchiato");
insert into device_actions values(19 , 26 , "caffeLatte");
insert into device_actions values(20 , 27 , "espressoMacchiato");
insert into device_actions values(21 , 27 , "coffee");
insert into device_actions values(22 , 27 , "cappuccino");
insert into device_actions values(23 , 27 , "latteMacchiato");
insert into device_actions values(24 , 27 , "caffeLatte");
insert into device_actions values(25 , 25 , "temperature");
insert into device_actions values(26 , 25 , "flow");
insert into device_actions values(27 , 25 , "on");
insert into device_actions values(28 , 25 , "off");
insert into device_actions values(29 , 25 , "dose");
insert into device_actions values(30 , 25 , "fill");
insert into device_actions values(31 , 25 , "alloff");
insert into device_actions values(32 , 25 , "stop");
drop table if exists "device_actions_parameter";
create table "device_actions_parameter"(id primary key,device_actions_id,name,type_id,min_value,max_value,default_value,resolution,si_unit,tags);
insert into device_actions_parameter values(1 , 1 , "temperature" , 1 , "50" , "240" , "180" , "1" , "celsius" , "");
insert into device_actions_parameter values(2 , 1 , "duration" , 2 , "60" , "7200" , "30" , "10" , "second" , "");
insert into device_actions_parameter values(3 , 2 , "temperature" , 1 , "50" , "240" , "180" , "1" , "celsius" , "");
insert into device_actions_parameter values(4 , 2 , "duration" , 2 , "60" , "7200" , "30" , "10" , "second" , "");
insert into device_actions_parameter values(5 , 6 , "keepwarmtime" , 2 , "0" , "30" , "30" , "1" , "min" , "");
insert into device_actions_parameter values(6 , 6 , "temperature" , 1 , "20" , "100" , "50" , "1" , "celsius" , "");
insert into device_actions_parameter values(7 , 5 , "keepwarmtime" , 2 , "0" , "30" , "30" , "1" , "min" , "");
insert into device_actions_parameter values(8 , 5 , "temperature" , 1 , "20" , "100" , "100" , "1" , "celsius" , "");
insert into device_actions_parameter values(9 , 7 , "dummyActionParam1" , 4 , "" , "" , "" , "" , "milligram" , "");
insert into device_actions_parameter values(10 , 7 , "dummyActionParam2" , 2 , "1" , "12" , "1" , "0,01" , "liter" , "");
insert into device_actions_parameter values(11 , 13 , "activity" , 4 , "" , "" , "TV" , "" , "" , "");
insert into device_actions_parameter values(12 , 9 , "temperatureLevel" , 6 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(13 , 15 , "temperatureLevel" , 6 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(14 , 16 , "temperatureLevel" , 6 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(15 , 17 , "temperatureLevel" , 6 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(16 , 18 , "temperatureLevel" , 6 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(17 , 19 , "temperatureLevel" , 6 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(18 , 9 , "beanAmount" , 7 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(19 , 15 , "beanAmount" , 7 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(20 , 16 , "beanAmount" , 7 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(21 , 17 , "beanAmount" , 7 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(22 , 18 , "beanAmount" , 7 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(23 , 19 , "beanAmount" , 7 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(24 , 9 , "fillQuantity" , 3 , "35" , "60" , "" , "5" , "milliliter" , "");
insert into device_actions_parameter values(25 , 15 , "fillQuantity" , 3 , "40" , "60" , "" , "10" , "milliliter" , "");
insert into device_actions_parameter values(26 , 16 , "fillQuantity" , 3 , "60" , "250" , "" , "10" , "milliliter" , "");
insert into device_actions_parameter values(27 , 17 , "fillQuantity" , 3 , "100" , "300" , "" , "20" , "milliliter" , "");
insert into device_actions_parameter values(28 , 18 , "fillQuantity" , 3 , "200" , "400" , "" , "20" , "milliliter" , "");
insert into device_actions_parameter values(29 , 19 , "fillQuantity" , 3 , "100" , "400" , "" , "20" , "milliliter" , "");
insert into device_actions_parameter values(30 , 11 , "temperatureLevel" , 6 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(31 , 11 , "beanAmount" , 7 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(32 , 11 , "fillQuantity" , 3 , "35" , "60" , "" , "5" , "milliliter" , "");
insert into device_actions_parameter values(33 , 20 , "temperatureLevel" , 6 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(34 , 20 , "beanAmount" , 7 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(35 , 20 , "fillQuantity" , 3 , "40" , "60" , "" , "10" , "milliliter" , "");
insert into device_actions_parameter values(36 , 21 , "temperatureLevel" , 6 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(37 , 21 , "beanAmount" , 7 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(38 , 21 , "fillQuantity" , 3 , "60" , "250" , "" , "10" , "milliliter" , "");
insert into device_actions_parameter values(39 , 22 , "temperatureLevel" , 6 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(40 , 22 , "beanAmount" , 7 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(41 , 22 , "fillQuantity" , 3 , "100" , "300" , "" , "20" , "milliliter" , "");
insert into device_actions_parameter values(42 , 23 , "temperatureLevel" , 6 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(43 , 23 , "beanAmount" , 7 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(44 , 23 , "fillQuantity" , 3 , "200" , "400" , "" , "20" , "milliliter" , "");
insert into device_actions_parameter values(45 , 24 , "temperatureLevel" , 6 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(46 , 24 , "beanAmount" , 7 , "" , "" , "" , "" , "" , "");
insert into device_actions_parameter values(47 , 24 , "fillQuantity" , 3 , "100" , "400" , "" , "20" , "milliliter" , "");
insert into device_actions_parameter values(48 , 25 , "temperature" , 1 , "1" , "43" , "38.5" , "1" , "celsius" , "");
insert into device_actions_parameter values(49 , 26 , "flow" , 3 , "0" , "100" , "50" , "0" , "percent" , "");
insert into device_actions_parameter values(50 , 30 , "functionid" , 3 , "1" , "1" , "1" , "0" , "" , "");
insert into device_actions_parameter values(51 , 30 , "amount" , 3 , "1" , "200" , "1" , "0" , "liter" , "");
insert into device_actions_parameter values(52 , 30 , "temperature" , 1 , "1" , "43" , "38.5" , "1" , "celsius" , "");
insert into device_actions_parameter values(53 , 29 , "functionid" , 3 , "1" , "1" , "1" , "0" , "" , "");
insert into device_actions_parameter values(54 , 29 , "amount" , 3 , "0.1" , "3" , "1" , "2" , "liter" , "");
insert into device_actions_parameter values(55 , 28 , "functionid" , 3 , "1" , "1" , "1" , "0" , "" , "");
insert into device_actions_parameter values(56 , 27 , "functionid" , 3 , "1" , "1" , "1" , "0" , "" , "");
drop table if exists "device_actions_parameter_type";
create table "device_actions_parameter_type"(id primary key,name);
insert into device_actions_parameter_type values(1 , "int.temperature");
insert into device_actions_parameter_type values(2 , "int.timespan");
insert into device_actions_parameter_type values(3 , "numeric");
insert into device_actions_parameter_type values(4 , "string");
insert into device_actions_parameter_type values(5 , "enumeration");
insert into device_actions_parameter_type values(6 , "enum.coffee.temperatureLevel");
insert into device_actions_parameter_type values(7 , "enum.coffee.beanAmount");
drop table if exists "device_actions_parameter_type_enum";
create table "device_actions_parameter_type_enum"(id primary key,device_action_parameter_type_id,value);
insert into device_actions_parameter_type_enum values(1 , 6 , "Normal");
insert into device_actions_parameter_type_enum values(2 , 6 , "High");
insert into device_actions_parameter_type_enum values(3 , 6 , "VeryHigh");
insert into device_actions_parameter_type_enum values(4 , 7 , "VeryMild");
insert into device_actions_parameter_type_enum values(5 , 7 , "Mild");
insert into device_actions_parameter_type_enum values(6 , 7 , "Normal");
insert into device_actions_parameter_type_enum values(7 , 7 , "Strong");
insert into device_actions_parameter_type_enum values(8 , 7 , "VeryStrong");
insert into device_actions_parameter_type_enum values(9 , 7 , "DoubleShot");
insert into device_actions_parameter_type_enum values(10 , 7 , "DoubleShotPlus");
insert into device_actions_parameter_type_enum values(11 , 7 , "DoubleShotPlusPlus");
drop table if exists "device_actions_predefined";
create table "device_actions_predefined"(id primary key,name,device_actions_id);
insert into device_actions_predefined values(1 , "std.cake" , 1);
insert into device_actions_predefined values(2 , "std.pizza" , 1);
insert into device_actions_predefined values(3 , "std.asparagus" , 2);
insert into device_actions_predefined values(4 , "std.stop" , 3);
insert into device_actions_predefined values(5 , "std.boilandcooldown" , 6);
insert into device_actions_predefined values(6 , "std.heat" , 5);
insert into device_actions_predefined values(7 , "std.stop" , 4);
insert into device_actions_predefined values(8 , "std.dummy" , 7);
insert into device_actions_predefined values(9 , "std.moreDummy" , 8);
insert into device_actions_predefined values(10 , "std.espresso" , 9);
insert into device_actions_predefined values(16 , "std.stop" , 10);
insert into device_actions_predefined values(17 , "std.stop" , 12);
insert into device_actions_predefined values(18 , "std.espresso" , 11);
insert into device_actions_predefined values(24 , "std.startactivity" , 13);
insert into device_actions_predefined values(25 , "std.endactivity" , 13);
insert into device_actions_predefined values(26 , "std.stop" , 14);
insert into device_actions_predefined values(27 , "std.espressoMacchiato" , 15);
insert into device_actions_predefined values(28 , "std.coffee" , 16);
insert into device_actions_predefined values(29 , "std.cappuccino" , 17);
insert into device_actions_predefined values(30 , "std.latteMacchiato" , 18);
insert into device_actions_predefined values(31 , "std.caffeLatte" , 19);
insert into device_actions_predefined values(32 , "std.espressoMacchiato" , 20);
insert into device_actions_predefined values(33 , "std.coffee" , 21);
insert into device_actions_predefined values(34 , "std.cappuccino" , 22);
insert into device_actions_predefined values(35 , "std.latteMacchiato" , 23);
insert into device_actions_predefined values(36 , "std.caffeLatte" , 24);
insert into device_actions_predefined values(37 , "std.temperature" , 25);
insert into device_actions_predefined values(38 , "std.flow" , 26);
insert into device_actions_predefined values(39 , "std.alloff" , 31);
insert into device_actions_predefined values(40 , "std.stop" , 32);
insert into device_actions_predefined values(41 , "std.fill" , 30);
insert into device_actions_predefined values(42 , "std.dose" , 29);
insert into device_actions_predefined values(43 , "std.off" , 28);
insert into device_actions_predefined values(44 , "std.on" , 27);
drop table if exists "device_actions_predefined_parameter";
create table "device_actions_predefined_parameter"(id primary key,value,device_actions_parameter_id,device_actions_predefined_id);
insert into device_actions_predefined_parameter values(1 , "160" , 1 , 1);
insert into device_actions_predefined_parameter values(2 , "3000" , 2 , 1);
insert into device_actions_predefined_parameter values(3 , "180" , 1 , 2);
insert into device_actions_predefined_parameter values(4 , "1200" , 2 , 2);
insert into device_actions_predefined_parameter values(5 , "180" , 3 , 3);
insert into device_actions_predefined_parameter values(6 , "2520" , 4 , 3);
insert into device_actions_predefined_parameter values(9 , "40" , 6 , 5);
insert into device_actions_predefined_parameter values(17 , "TV" , 11 , 24);
insert into device_actions_predefined_parameter values(18 , "TV" , 11 , 25);
insert into device_actions_predefined_parameter values(19 , "" , 33 , 32);
insert into device_actions_predefined_parameter values(20 , "" , 34 , 32);
insert into device_actions_predefined_parameter values(21 , "" , 35 , 32);
insert into device_actions_predefined_parameter values(22 , "" , 36 , 33);
insert into device_actions_predefined_parameter values(23 , "" , 37 , 33);
insert into device_actions_predefined_parameter values(24 , "" , 38 , 33);
insert into device_actions_predefined_parameter values(25 , "" , 39 , 34);
insert into device_actions_predefined_parameter values(26 , "" , 40 , 34);
insert into device_actions_predefined_parameter values(27 , "" , 41 , 34);
insert into device_actions_predefined_parameter values(28 , "" , 42 , 35);
insert into device_actions_predefined_parameter values(29 , "" , 43 , 35);
insert into device_actions_predefined_parameter values(30 , "" , 44 , 35);
insert into device_actions_predefined_parameter values(31 , "" , 45 , 36);
insert into device_actions_predefined_parameter values(32 , "" , 46 , 36);
insert into device_actions_predefined_parameter values(33 , "" , 47 , 36);
insert into device_actions_predefined_parameter values(34 , "" , 48 , 37);
insert into device_actions_predefined_parameter values(35 , "" , 50 , 41);
insert into device_actions_predefined_parameter values(36 , "" , 51 , 41);
insert into device_actions_predefined_parameter values(37 , "" , 52 , 41);
insert into device_actions_predefined_parameter values(38 , "" , 53 , 42);
insert into device_actions_predefined_parameter values(39 , "" , 54 , 42);
insert into device_actions_predefined_parameter values(40 , "" , 55 , 43);
insert into device_actions_predefined_parameter values(41 , "" , 56 , 44);

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
