#!/opt/local//bin/php
<?php
	$nrofintfs = 64;
	$output = "";

	for($i=1;$i<=$nrofintfs;$i++){
		$output .= "update streaminterfaceconfigs set autostart='t', mode='receiver',nosessionid='f' where id=$i;\n";
	}

	for($i=1;$i<=$nrofintfs;$i++){
		$output .= "update streaminterfacesources set sourcetype='rudp', host='192.168.15.7', port=" . (10000+$i) . ", talkbackhost='192.168.15.10', talkbackport=" .  (10000 + $i) . " where id=$i;\n";
	}

	for($i=1;$i<=$nrofintfs;$i++){
		$output .= "update streaminterfacesinks set sinktype='udp', host='127.0.0.1', port=" . (10000+$i) . " where id=$i;\n";
	}

	file_put_contents("testconfig_recievers.sql",$output);
?>

