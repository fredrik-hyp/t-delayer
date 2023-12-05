#!/usr/bin/php
<?php
	$nrofintfs = 128;
	$output = "";
	for($i=1;$i<=$nrofintfs;$i++){
		$output .= "insert into streaminterfaces (interfacetype, name, usertext) select 'streamtransporter', 'st$i', 'unused' where not exists  (select id from streaminterfaces where id=$i);\n";
	}

	for($i=1;$i<=$nrofintfs;$i++){
		$output .= "insert into streaminterfaceconfigs (streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) select $i, 'stored', 'f',400, 'normal' ,'transmitter' where not exists  (select id from streaminterfaceconfigs where id=$i);\n";
	}

	for($i=1;$i<=$nrofintfs;$i++){
		$output .= "insert into streaminterfacesources (streaminterfaceconfig_id, name, sourcetype, host, port, talkbackhost, talkbackport) select $i,'sourcename','udp', '224.1.1.11', 10000, '',0 where not exists  (select id from streaminterfacesources where id=$i);\n";
	}

	for($i=1;$i<=$nrofintfs;$i++){
		$output .= "insert into streaminterfacesinks (streaminterfaceconfig_id, name, sinktype, host, port, talkbackhost, talkbackport) select $i,'sinkname', 'rudp', '192.168.1.57',2039, '127.0.0.1',3434 where not exists  (select id from streaminterfacesinks where id=$i);\n";
	}

	file_put_contents("population_out.sql",$output);
?>

