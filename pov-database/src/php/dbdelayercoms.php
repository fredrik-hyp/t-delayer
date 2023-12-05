<?php
	require_once("dblib.php");
	
	function DBAddDelayercom($streaminterfaceid, $action, $jumptopos, $startplayat, $playto)
    {
		global $DB;
		$sth = $DB->prepare("INSERT INTO delayercoms (streaminterface_id, action, jumptopos, startplayat, playto) VALUES (:streaminterfaceid, :action, :jumptopos, :startplayat, :playto)");
		$result = $sth->execute(array( ":streaminterfaceid" => $streaminterfaceid,
									   ":action" => $action,
								   		":jumptopos" => $jumptopos,
										":startplayat" => $startplayat,
										":playto" => $playto));

		if($result)
		{
			return $DB->lastInsertId("delayercoms_id_seq");
		}
		return $result;
	}
?>