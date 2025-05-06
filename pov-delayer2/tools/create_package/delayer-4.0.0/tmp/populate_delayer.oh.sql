-- interface 1
insert into streaminterfaces (id, interfacetype, name, usertext) 
	select  
	'1', 'streamtransporter', 'st1', 'unused'
	WHERE NOT EXISTS ( SELECT 1 FROM streaminterfaces WHERE id = 1);

insert into streaminterfaceconfigs 
	(id, streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
    select 
	'1', '1', 'stored', 'f', 3600000, 'normal' ,'delayer'
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfaceconfigs WHERE streaminterface_id = 1);

insert into streaminterfacesources 
	(id, streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	select 
	'1', '1', 'st1_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesources WHERE streaminterfaceconfig_id = 1);

insert into streaminterfacesinks 
	(id, streaminterfaceconfig_id, name, sinktype, host, port) 
	select 
	'1', '1','st1_out1_3mbps', 'udp', '239.42.223.1', 1234
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesinks WHERE streaminterfaceconfig_id = 1);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'1', '1', 'st1_out2_3mbps', 'udp', '239.42.223.2', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 1) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select 
	'1', '2', 'st1_out3_3mbps', 'udp', '239.42.223.3', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 1) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select
	'1', '3', 'st1_out4_3mbps', 'udp', '239.42.223.4', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 1) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'1', '4', 'st1_out5_3mbps', 'udp', '239.42.223.5', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 1) < 4;


-- interface 2
insert into streaminterfaces (id, interfacetype, name, usertext) 
	select  
	'2', 'streamtransporter', 'st2', 'unused'
	WHERE NOT EXISTS ( SELECT 1 FROM streaminterfaces WHERE id = 2);

insert into streaminterfaceconfigs 
	(id, streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
    select 
	'2', '2', 'stored', 'f', 3600000, 'normal' ,'delayer'
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfaceconfigs WHERE streaminterface_id = 2);

insert into streaminterfacesources 
	(id, streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	select 
	'2', '2', 'st1_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesources WHERE streaminterfaceconfig_id = 2);

insert into streaminterfacesinks 
	(id, streaminterfaceconfig_id, name, sinktype, host, port) 
	select 
	'2', '2','st1_out1_3mbps', 'udp', '239.42.223.11', 1234
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesinks WHERE streaminterfaceconfig_id = 2);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'2', '1', 'st1_out2_3mbps', 'udp', '239.42.223.12', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 2) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select 
	'2', '2', 'st1_out3_3mbps', 'udp', '239.42.223.13', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 2) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select
	'2', '3', 'st1_out4_3mbps', 'udp', '239.42.223.14', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 2) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'2', '4', 'st1_out5_3mbps', 'udp', '239.42.223.15', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 2) < 4;


-- interface 3
insert into streaminterfaces (id, interfacetype, name, usertext) 
	select  
	'3', 'streamtransporter', 'st3', 'unused'
	WHERE NOT EXISTS ( SELECT 1 FROM streaminterfaces WHERE id = 3);

insert into streaminterfaceconfigs 
	(id, streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
    select 
	'3', '3', 'stored', 'f', 3600000, 'normal' ,'delayer'
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfaceconfigs WHERE streaminterface_id = 3);

insert into streaminterfacesources 
	(id, streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	select 
	'3', '3', 'st1_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesources WHERE streaminterfaceconfig_id = 3);

insert into streaminterfacesinks 
	(id, streaminterfaceconfig_id, name, sinktype, host, port) 
	select 
	'3', '3','st1_out1_3mbps', 'udp', '239.42.223.21', 1234
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesinks WHERE streaminterfaceconfig_id = 3);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'3', '1', 'st1_out2_3mbps', 'udp', '239.42.223.22', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 3) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select 
	'3', '2', 'st1_out3_3mbps', 'udp', '239.42.223.23', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 3) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select
	'3', '3', 'st1_out4_3mbps', 'udp', '239.42.223.24', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 3) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'3', '4', 'st1_out5_3mbps', 'udp', '239.42.223.25', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 3) < 4;



-- interface 4
insert into streaminterfaces (id, interfacetype, name, usertext) 
	select  
	'4', 'streamtransporter', 'st4', 'unused'
	WHERE NOT EXISTS ( SELECT 1 FROM streaminterfaces WHERE id = 4);

insert into streaminterfaceconfigs 
	(id, streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
    select 
	'4', '4', 'stored', 'f', 3600000, 'normal' ,'delayer'
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfaceconfigs WHERE streaminterface_id = 4);

insert into streaminterfacesources 
	(id, streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	select 
	'4', '4', 'st1_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesources WHERE streaminterfaceconfig_id = 4);

insert into streaminterfacesinks 
	(id, streaminterfaceconfig_id, name, sinktype, host, port) 
	select 
	'4', '4','st1_out1_3mbps', 'udp', '239.42.223.31', 1234
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesinks WHERE streaminterfaceconfig_id = 4);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'4', '1', 'st1_out2_3mbps', 'udp', '239.42.223.32', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 4) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select 
	'4', '2', 'st1_out3_3mbps', 'udp', '239.42.223.33', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 4) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select
	'4', '3', 'st1_out4_3mbps', 'udp', '239.42.223.34', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 4) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'4', '4', 'st1_out5_3mbps', 'udp', '239.42.223.35', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 4) < 4;


-- interface 5
insert into streaminterfaces (id, interfacetype, name, usertext) 
	select  
	'5', 'streamtransporter', 'st5', 'unused'
	WHERE NOT EXISTS ( SELECT 1 FROM streaminterfaces WHERE id = 5);

insert into streaminterfaceconfigs 
	(id, streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
    select 
	'5', '5', 'stored', 'f', 3600000, 'normal' ,'delayer'
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfaceconfigs WHERE streaminterface_id = 5);

insert into streaminterfacesources 
	(id, streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	select 
	'5', '5', 'st1_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesources WHERE streaminterfaceconfig_id = 5);

insert into streaminterfacesinks 
	(id, streaminterfaceconfig_id, name, sinktype, host, port) 
	select 
	'5', '5','st1_out1_3mbps', 'udp', '239.42.223.41', 1234
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesinks WHERE streaminterfaceconfig_id = 5);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'5', '1', 'st1_out2_3mbps', 'udp', '239.42.223.42', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 5) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select 
	'5', '2', 'st1_out3_3mbps', 'udp', '239.42.223.43', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 5) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select
	'5', '3', 'st1_out4_3mbps', 'udp', '239.42.223.44', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 5) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'5', '4', 'st1_out5_3mbps', 'udp', '239.42.223.45', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 5) < 4;


-- interface 6
insert into streaminterfaces (id, interfacetype, name, usertext) 
	select  
	'6', 'streamtransporter', 'st6', 'unused'
	WHERE NOT EXISTS ( SELECT 1 FROM streaminterfaces WHERE id = 6);

insert into streaminterfaceconfigs 
	(id, streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
    select 
	'6', '6', 'stored', 'f', 3600000, 'normal' ,'delayer'
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfaceconfigs WHERE streaminterface_id = 6);

insert into streaminterfacesources 
	(id, streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	select 
	'6', '6', 'st1_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesources WHERE streaminterfaceconfig_id = 6);

insert into streaminterfacesinks 
	(id, streaminterfaceconfig_id, name, sinktype, host, port) 
	select 
	'6', '6','st1_out1_3mbps', 'udp', '239.42.223.51', 1234
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesinks WHERE streaminterfaceconfig_id = 6);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'6', '1', 'st1_out2_3mbps', 'udp', '239.42.223.52', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 6) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select 
	'6', '2', 'st1_out3_3mbps', 'udp', '239.42.223.53', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 6) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select
	'6', '3', 'st1_out4_3mbps', 'udp', '239.42.223.54', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 6) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'6', '4', 'st1_out5_3mbps', 'udp', '239.42.223.55', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 6) < 4;


-- interface 7
insert into streaminterfaces (id, interfacetype, name, usertext) 
	select  
	'7', 'streamtransporter', 'st7', 'unused'
	WHERE NOT EXISTS ( SELECT 1 FROM streaminterfaces WHERE id = 7);

insert into streaminterfaceconfigs 
	(id, streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
    select 
	'7', '7', 'stored', 'f', 3600000, 'normal' ,'delayer'
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfaceconfigs WHERE streaminterface_id = 7);

insert into streaminterfacesources 
	(id, streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	select 
	'7', '7', 'st1_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesources WHERE streaminterfaceconfig_id = 7);

insert into streaminterfacesinks 
	(id, streaminterfaceconfig_id, name, sinktype, host, port) 
	select 
	'7', '7','st1_out1_3mbps', 'udp', '239.42.223.61', 1234
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesinks WHERE streaminterfaceconfig_id = 7);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'7', '1', 'st1_out2_3mbps', 'udp', '239.42.223.62', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 7) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select 
	'7', '2', 'st1_out3_3mbps', 'udp', '239.42.223.63', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 7) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select
	'7', '3', 'st1_out4_3mbps', 'udp', '239.42.223.64', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 7) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'7', '4', 'st1_out5_3mbps', 'udp', '239.42.223.65', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 7) < 4;


-- interface 8
insert into streaminterfaces (id, interfacetype, name, usertext) 
	select  
	'8', 'streamtransporter', 'st8', 'unused'
	WHERE NOT EXISTS ( SELECT 1 FROM streaminterfaces WHERE id = 8);

insert into streaminterfaceconfigs 
	(id, streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
    select 
	'8', '8', 'stored', 'f', 3600000, 'normal' ,'delayer'
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfaceconfigs WHERE streaminterface_id = 8);

insert into streaminterfacesources 
	(id, streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	select 
	'8', '8', 'st1_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesources WHERE streaminterfaceconfig_id = 8);

insert into streaminterfacesinks 
	(id, streaminterfaceconfig_id, name, sinktype, host, port) 
	select 
	'8', '8','st1_out1_3mbps', 'udp', '239.42.223.71', 1234
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesinks WHERE streaminterfaceconfig_id = 8);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'8', '1', 'st1_out2_3mbps', 'udp', '239.42.223.72', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 8) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select 
	'8', '2', 'st1_out3_3mbps', 'udp', '239.42.223.73', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 8) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select
	'8', '3', 'st1_out4_3mbps', 'udp', '239.42.223.74', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 8) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'8', '4', 'st1_out5_3mbps', 'udp', '239.42.223.75', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 8) < 4;


-- interface 9
insert into streaminterfaces (id, interfacetype, name, usertext) 
	select  
	'9', 'streamtransporter', 'st9', 'unused'
	WHERE NOT EXISTS ( SELECT 1 FROM streaminterfaces WHERE id = 9);

insert into streaminterfaceconfigs 
	(id, streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
    select 
	'9', '9', 'stored', 'f', 3600000, 'normal' ,'delayer'
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfaceconfigs WHERE streaminterface_id = 9);

insert into streaminterfacesources 
	(id, streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	select 
	'9', '9', 'st1_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesources WHERE streaminterfaceconfig_id = 9);

insert into streaminterfacesinks 
	(id, streaminterfaceconfig_id, name, sinktype, host, port) 
	select 
	'9', '9','st1_out1_3mbps', 'udp', '239.42.223.81', 1234
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesinks WHERE streaminterfaceconfig_id = 9);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'9', '1', 'st1_out2_3mbps', 'udp', '239.42.223.82', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 9) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select 
	'9', '2', 'st1_out3_3mbps', 'udp', '239.42.223.83', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 9) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select
	'9', '3', 'st1_out4_3mbps', 'udp', '239.42.223.84', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 9) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'9', '4', 'st1_out5_3mbps', 'udp', '239.42.223.85', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 9) < 4;

-- interface 10
insert into streaminterfaces (id, interfacetype, name, usertext) 
	select  
	'10', 'streamtransporter', 'st10', 'unused'
	WHERE NOT EXISTS ( SELECT 1 FROM streaminterfaces WHERE id = 10);

insert into streaminterfaceconfigs 
	(id, streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
    select 
	'10', '10', 'stored', 'f', 3600000, 'normal' ,'delayer'
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfaceconfigs WHERE streaminterface_id = 10);

insert into streaminterfacesources 
	(id, streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	select 
	'10', '10', 'st1_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesources WHERE streaminterfaceconfig_id = 10);

insert into streaminterfacesinks 
	(id, streaminterfaceconfig_id, name, sinktype, host, port) 
	select 
	'10', '10','st1_out1_3mbps', 'udp', '239.42.223.91', 1234
	WHERE NOT EXISTS (SELECT 1 FROM streaminterfacesinks WHERE streaminterfaceconfig_id = 10);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'10', '1', 'st1_out2_3mbps', 'udp', '239.42.223.92', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 10) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select 
	'10', '2', 'st1_out3_3mbps', 'udp', '239.42.223.93', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 10) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id,  priority, name, sinktype, host, port) 
	select
	'10', '3', 'st1_out4_3mbps', 'udp', '239.42.223.94', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 10) < 4;

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, priority, name, sinktype, host, port) 
	select
	'10', '4', 'st1_out5_3mbps', 'udp', '239.42.223.95', 1234
	WHERE (SELECT COUNT(*) FROM streaminterfacebackupsinks WHERE streaminterfacesink_id = 10) < 4;