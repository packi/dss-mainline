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
insert into name_master values(85 , 3 , "device_events" , "Kocher abgesetzt" , "de_DE");
insert into name_master values(86 , 3 , "device_events" , "Kettle attached" , "en_EN");
insert into name_master values(105 , 4 , "device_events" , "Boiling started" , "en_EN");
insert into name_master values(106 , 4 , "device_events" , "Aufheizen gestartet" , "de_DE");
insert into name_master values(107 , 5 , "device_events" , "Keeping warm started" , "en_EN");
insert into name_master values(108 , 5 , "device_events" , "Warmhalten gestartet" , "de_DE");
insert into name_master values(109 , 6 , "device_events" , "Boil and cool down started (Babyboil)" , "en_EN");
insert into name_master values(110 , 6 , "device_events" , "Aufheizen mit anschliessendem Warmhalten gestartet" , "de_DE");
insert into name_master values(111 , 7 , "device_events" , "Boiling ended without keeping warm" , "en_EN");
insert into name_master values(112 , 7 , "device_events" , "Aufheizen beendet ohne Warmhalten" , "de_DE");
insert into name_master values(113 , 8 , "device_events" , "Kettle was taken away" , "en_EN");
insert into name_master values(114 , 8 , "device_events" , "Wasserkocher abgenommen" , "de_DE");
insert into name_master values(115 , 9 , "device_events" , "Cooled down to specified end temperature without keeping warm" , "en_EN");
insert into name_master values(116 , 9 , "device_events" , "Abgekühlt auf die gewünschte Temperatur ohne weiteres warmhalten" , "de_DE");
insert into name_master values(117 , 10 , "device_events" , "Keeping warm finished" , "en_EN");
insert into name_master values(118 , 10 , "device_events" , "Warmhalten nach Ablauf der Zeit beendet" , "de_DE");
insert into name_master values(119 , 11 , "device_events" , "Aufheizen durch Tastendruck beendet" , "de_DE");
insert into name_master values(120 , 11 , "device_events" , "Boiling aborted by button press" , "en_EN");
insert into name_master values(121 , 12 , "device_events" , "Cooling down aborted by button press" , "en_EN");
insert into name_master values(122 , 12 , "device_events" , "Abkühlen durch Tastendruck beendet" , "de_DE");
insert into name_master values(123 , 13 , "device_events" , "Keeping warm aborted by button press" , "en_EN");
insert into name_master values(124 , 13 , "device_events" , "Warmhalten durch Tastendruck abgebrochen" , "de_DE");
insert into name_master values(125 , 14 , "device_events" , "Keep warm after boiling started" , "en_EN");
insert into name_master values(126 , 14 , "device_events" , "Warmhalten nach Aufheizen gestartet" , "de_DE");
insert into name_master values(127 , 15 , "device_events" , "Keep temperature after cooling down (BabyBoil) started" , "en_EN");
insert into name_master values(128 , 15 , "device_events" , "Warmhalten nach Erreichen der gewünschten Temperatur gestartet" , "de_DE");
insert into name_master values(129 , 16 , "device_events" , "Boiling aborted because kettle was removed" , "en_EN");
insert into name_master values(130 , 16 , "device_events" , "Aufheizen abgebrochen weil der Wasserkocher abgehoben wurde" , "de_DE");
insert into name_master values(131 , 17 , "device_events" , "Cooling down aborted because kettle was removed" , "en_EN");
insert into name_master values(132 , 17 , "device_events" , "Abkühlen abgebrochen weil der Wasserkocher abgehoben wurde" , "de_DE");
insert into name_master values(133 , 18 , "device_events" , "Keeping warm aborted because kettle was removed" , "en_EN");
insert into name_master values(134 , 18 , "device_events" , "Warmhalten abgebrochen weil der Wasserkocher abgehoben wurde" , "de_DE");
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
drop table if exists "device_properties";
create table "device_properties"(id primary key,device_id,name,type_id,min_value,max_value,default_value,resolution,si_unit,tags);
insert into device_properties values(1 , 12 , "temperature" , 1 , "0" , "250" , "0" , "1" , "celsius" , "" );
insert into device_properties values(2 , 12 , "duration" , 2 , "0" , "1800" , "0" , "1" , "second" , "" );
insert into device_properties values(3 , 12 , "temperature.sensor" , 1 , "0" , "250" , "0" , "1" , "celsius" , "readonly" );
insert into device_properties values(4 , 18 , "currentTemperature" , 1 , "0" , "250" , "0" , "1" , "celsius" , "readonly;overview" );
drop table if exists "name_device_properties";
create table "name_device_properties"(id primary key,reference_id,lang_code,displayMask,alt_label,alt_postfix,alt_precision_float);
insert into name_device_properties values(1 , 1 , "de_DE" , "Temperatur {value} &deg; " , "Temperatur" , "&deg;" , "0");
insert into name_device_properties values(2 , 2 , "de_DE" , "Endzeit in {value} Sekunden" , "Endzeit" , "Sekunden" , "0");
insert into name_device_properties values(3 , 3 , "de_DE" , "Garguttemperatur {value} &deg;" , "Garguttemperatur" , "&deg;" , "0");
insert into name_device_properties values(4 , 4 , "de_DE" , "Temperatur {value} &deg; " , "Temperatur [&deg;C]" , " " , "0");
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
commit;
