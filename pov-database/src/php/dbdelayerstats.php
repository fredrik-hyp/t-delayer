<?php
	require_once("dblib.php");
	
	function DBFetchDelayerstatsByStreaminterfaceId($streaminterfaceid)
    {
		global $DB;
		$sth = $DB->prepare("SELECT * FROM delayerstats WHERE streaminterface_id=:streaminterfaceid ORDER BY pathnumber");
		$sth->execute(array(":streaminterfaceid" => $streaminterfaceid));
		$result = $sth->fetchAll();
		if($result === false)
        {
			return array();
		}
		return $result;
	}
?>