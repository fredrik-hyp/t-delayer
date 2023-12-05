#!/opt/local//bin/php
<?php
	$nrofintfs = 64;
	$output = "";

	for($i=1;$i<=$nrofintfs;$i++){
		$output .= "update streaminterfaceconfigs set autostart='t', mode='transmitter',nosessionid='f' where id=$i;\n";
	}

	for($i=1;$i<=$nrofintfs;$i++){
		$output .= "update streaminterfacesources set sourcetype='udp', host='239.1.1.7', port=17009 where id=$i;\n";
	}

	for($i=1;$i<=$nrofintfs;$i++){
		$output .= "update streaminterfacesinks set sinktype='rudp', host='192.168.15.7', port=" . (10000+$i) . ", talkbackhost='192.168.15.10', talkbackport=" .  (10000 + $i) . " where id=$i;\n";
	}

	file_put_contents("testconfig_transmitters.sql",$output);
?>

