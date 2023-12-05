
insert into streaminterfaces (interfacetype, name, usertext) 
	values 
	('streamtransporter', 'st1', 'unused');

insert into streaminterfaceconfigs 
	(streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
       values 
	('1', 'stored', 'f', 3600000, 'normal' ,'delayer');

insert into streaminterfacesources 
	(streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	values 
	('1', 'st1_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0);

insert into streaminterfacesinks 
	(streaminterfaceconfig_id, name, sinktype, host, port) 
	values 
	('1','st1_out1_3mbps', 'udp', '239.42.223.1', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('1', 'st1_out2_3mbps', 'udp', '239.42.223.2', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('1', 'st1_out3_3mbps', 'udp', '239.42.223.3', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('1', 'st1_out4_3mbps', 'udp', '239.42.223.4', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('1', 'st1_out5_3mbps', 'udp', '239.42.223.5', 1234);




insert into streaminterfaces (interfacetype, name, usertext) 
	values 
	('streamtransporter', 'st2', 'unused');

insert into streaminterfaceconfigs 
	(streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
       values 
	('2', 'stored', 'f', 3600000, 'normal' ,'delayer');

insert into streaminterfacesources 
	(streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	values 
	('2', 'st2_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0);

insert into streaminterfacesinks 
	(streaminterfaceconfig_id, name, sinktype, host, port) 
	values 
	('2', 'st2_out1_30mbps', 'udp', '239.42.223.6', 1234 );

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('2', 'st2_out2_30mbps', 'udp', '239.42.223.7', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('2', 'st2_out3_30mbps', 'udp', '239.42.223.8', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('2', 'st2_out4_30mbps', 'udp', '239.42.223.9', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('2', 'st2_out5_30mbps', 'udp', '239.42.223.10', 1234);




insert into streaminterfaces (interfacetype, name, usertext) 
	values 
	('streamtransporter', 'st3', 'unused');

insert into streaminterfaceconfigs 
	(streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
       values 
	('3', 'stored', 'f', 3600000, 'normal' ,'delayer');

insert into streaminterfacesources 
	(streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	values 
	('3', 'st3_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0);

insert into streaminterfacesinks 
	(streaminterfaceconfig_id, name, sinktype, host, port) 
	values 
	('3', 'st3_out1_30mbps', 'udp', '239.42.223.11', 1234 );

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('3', 'st3_out2_30mbps', 'udp', '239.42.223.12', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('3', 'st3_out3_30mbps', 'udp', '239.42.223.13', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('3', 'st3_out4_30mbps', 'udp', '239.42.223.14', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('3', 'st3_out5_30mbps', 'udp', '239.42.223.15', 1234);

	



insert into streaminterfaces (interfacetype, name, usertext) 
	values 
	('streamtransporter', 'st4', 'unused');

insert into streaminterfaceconfigs 
	(streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
       values 
	('4', 'stored', 'f', 3600000, 'normal' ,'delayer');

insert into streaminterfacesources 
	(streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	values 
	('4', 'st4_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0);

insert into streaminterfacesinks 
	(streaminterfaceconfig_id, name, sinktype, host, port) 
	values 
	('4', 'st4_out1_30mbps', 'udp', '239.42.223.16', 1234 );

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('4', 'st4_out2_30mbps', 'udp', '239.42.223.17', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('4', 'st4_out3_30mbps', 'udp', '239.42.223.18', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('4', 'st4_out4_30mbps', 'udp', '239.42.223.19', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('4', 'st4_out5_30mbps', 'udp', '239.42.223.20', 1234);



insert into streaminterfaces (interfacetype, name, usertext) 
	values 
	('streamtransporter', 'st5', 'unused');

insert into streaminterfaceconfigs 
	(streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
       values 
	('5', 'stored', 'f', 3600000, 'normal' ,'delayer');

insert into streaminterfacesources 
	(streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	values 
	('5', 'st4_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0);

insert into streaminterfacesinks 
	(streaminterfaceconfig_id, name, sinktype, host, port) 
	values 
	('5', 'st5_out1_30mbps', 'udp', '239.42.223.21', 1234 );

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('5', 'st5_out2_30mbps', 'udp', '239.42.223.22', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('5', 'st5_out3_30mbps', 'udp', '239.42.223.23', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('5', 'st5_out4_30mbps', 'udp', '239.42.223.24', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('5', 'st5_out5_30mbps', 'udp', '239.42.223.25', 1234);




insert into streaminterfaces (interfacetype, name, usertext) 
	values 
	('streamtransporter', 'st6', 'unused');

insert into streaminterfaceconfigs 
	(streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
       values 
	('6', 'stored', 'f', 3600000, 'normal' ,'delayer');

insert into streaminterfacesources 
	(streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	values 
	('6', 'st6_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0);

insert into streaminterfacesinks 
	(streaminterfaceconfig_id, name, sinktype, host, port) 
	values 
	('6', 'st6_out1_30mbps', 'udp', '239.42.223.26', 1234 );

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('6', 'st6_out2_30mbps', 'udp', '239.42.223.27', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('6', 'st6_out3_30mbps', 'udp', '239.42.223.28', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('6', 'st6_out4_30mbps', 'udp', '239.42.223.29', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('6', 'st6_out5_30mbps', 'udp', '239.42.223.30', 1234);





insert into streaminterfaces (interfacetype, name, usertext) 
	values 
	('streamtransporter', 'st7', 'unused');

insert into streaminterfaceconfigs 
	(streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
       values 
	('7', 'stored', 'f', 3600000, 'normal' ,'delayer');

insert into streaminterfacesources 
	(streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	values 
	('7', 'st7_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0);

insert into streaminterfacesinks 
	(streaminterfaceconfig_id, name, sinktype, host, port) 
	values 
	('7', 'st7_out1_30mbps', 'udp', '239.42.223.31', 1234 );

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('7', 'st7_out2_30mbps', 'udp', '239.42.223.32', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('7', 'st7_out3_30mbps', 'udp', '239.42.223.33', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('7', 'st7_out4_30mbps', 'udp', '239.42.223.34', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('7', 'st7_out5_30mbps', 'udp', '239.42.223.35', 1234);




insert into streaminterfaces (interfacetype, name, usertext) 
	values 
	('streamtransporter', 'st8', 'unused');

insert into streaminterfaceconfigs 
	(streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
       values 
	('8', 'stored', 'f', 3600000, 'normal' ,'delayer');

insert into streaminterfacesources 
	(streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	values 
	('8', 'st8_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0);

insert into streaminterfacesinks 
	(streaminterfaceconfig_id, name, sinktype, host, port) 
	values 
	('8', 'st8_out1_30mbps', 'udp', '239.42.223.36', 1234 );

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('8', 'st8_out2_30mbps', 'udp', '239.42.223.37', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('8', 'st8_out3_30mbps', 'udp', '239.42.223.38', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('8', 'st8_out4_30mbps', 'udp', '239.42.223.39', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('8', 'st8_out5_30mbps', 'udp', '239.42.223.40', 1234);





insert into streaminterfaces (interfacetype, name, usertext) 
	values 
	('streamtransporter', 'st9', 'unused');

insert into streaminterfaceconfigs 
	(streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
       values 
	('9', 'stored', 'f', 3600000, 'normal' ,'delayer');

insert into streaminterfacesources 
	(streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	values 
	('9', 'st9_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0);

insert into streaminterfacesinks 
	(streaminterfaceconfig_id, name, sinktype, host, port) 
	values 
	('9', 'st9_out1_30mbps', 'udp', '239.42.223.41', 1234 );

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('9', 'st9_out2_30mbps', 'udp', '239.42.223.42', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('9', 'st9_out3_30mbps', 'udp', '239.42.223.43', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('9', 'st9_out4_30mbps', 'udp', '239.42.223.44', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('9', 'st9_out5_30mbps', 'udp', '239.42.223.45', 1234);







insert into streaminterfaces (interfacetype, name, usertext) 
	values 
	('streamtransporter', 'st10', 'unused');

insert into streaminterfaceconfigs 
	(streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) 
       values 
	('10', 'stored', 'f', 3600000, 'normal' ,'delayer');

insert into streaminterfacesources 
	(streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) 
	values 
	('10', 'st9_in_30mbps', 'udp', '234.41.221.51', 12351, '', 0);

insert into streaminterfacesinks 
	(streaminterfaceconfig_id, name, sinktype, host, port) 
	values 
	('10', 'st9_out1_30mbps', 'udp', '239.42.223.46', 1234 );

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('10', 'st9_out2_30mbps', 'udp', '239.42.223.47', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('10', 'st9_out3_30mbps', 'udp', '239.42.223.48', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('10', 'st9_out4_30mbps', 'udp', '239.42.223.49', 1234);

insert into streaminterfacebackupsinks 
	(streaminterfacesink_id, name, sinktype, host, port) 
	values 
	('10', 'st10_out5_30mbps', 'udp', '239.42.223.50', 1234);








